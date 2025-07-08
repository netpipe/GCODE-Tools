#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QTimer>
#include <QVector3D>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <glu.h>


struct GCodeCommand {
    enum Type { Rapid, Linear, ArcCW, ArcCCW } type;
    QVector3D target;
    float i = 0, j = 0; // for arcs (center offset)
};

class GCodeViewer3D : public QOpenGLWidget, protected QOpenGLFunctions {
    QVector<GCodeCommand> cmds;
    QVector<QVector3D> toolpath;
    QVector<QVector3D> cutVolume;

    QVector3D position = {0, 0, 0};
    int index = 0;

    float rotX = 20, rotY = -45, dist = 300;
    QPoint lastMousePos;

    QTimer timer;

public:
    GCodeViewer3D(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
        setFixedSize(800, 800);
        parseGCode("demo3d.gcode");
        connect(&timer, &QTimer::timeout, this, &GCodeViewer3D::step);
        timer.start(20);
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, w / float(h), 1.0, 1000.0);
        glMatrixMode(GL_MODELVIEW);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        glTranslatef(0, 0, -dist);
        glRotatef(rotX, 1, 0, 0);
        glRotatef(rotY, 0, 1, 0);

        drawGrid();
        drawToolpath();
        drawCutVolume();
    }

    void drawGrid() {
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINES);
        for (int i = -100; i <= 100; i += 10) {
            glVertex3f(i, -100, 0);
            glVertex3f(i, 100, 0);
            glVertex3f(-100, i, 0);
            glVertex3f(100, i, 0);
        }
        glEnd();
    }

    void drawToolpath() {
        glColor3f(0.0f, 1.0f, 0.0f);
        glBegin(GL_LINE_STRIP);
        for (auto& pt : toolpath)
            glVertex3f(pt.x(), pt.y(), pt.z());
        glEnd();

        glPointSize(6);
        glBegin(GL_POINTS);
        glVertex3f(position.x(), position.y(), position.z());
        glEnd();
    }

    void drawCutVolume() {
        glColor3f(0.8f, 0.3f, 0.1f);
        for (auto& pt : cutVolume) {
            glPushMatrix();
            glTranslatef(pt.x(), pt.y(), pt.z());
            //glutSolidCube(1.0);  // for simplicity, voxelized cubes
            glPopMatrix();
        }
    }

    void mousePressEvent(QMouseEvent* e) override {
        lastMousePos = e->pos();
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        QPoint delta = e->pos() - lastMousePos;
        rotX += delta.y();
        rotY += delta.x();
        lastMousePos = e->pos();
        update();
    }
    bool playing = true;
    int substep = 0;
    int maxSubsteps = 10; // Smooth interpolation steps
QVector3D currentStart;
GCodeCommand currentCmd;
QVector3D arcCenter;
float startAngle = 0, endAngle = 0;
bool isArc = false;

void step() {
    if (!playing || index >= cmds.size()) return;

    if (substep == 0) {
        currentCmd = cmds[index];
        currentStart = position;

        if (currentCmd.type == GCodeCommand::ArcCW || currentCmd.type == GCodeCommand::ArcCCW) {
            isArc = true;
            arcCenter = position + QVector3D(currentCmd.i, currentCmd.j, 0);

            QVector2D v0(position.x() - arcCenter.x(), position.y() - arcCenter.y());
            QVector2D v1(currentCmd.target.x() - arcCenter.x(), currentCmd.target.y() - arcCenter.y());

            startAngle = atan2f(v0.y(), v0.x());
            endAngle = atan2f(v1.y(), v1.x());

            if (currentCmd.type == GCodeCommand::ArcCCW && endAngle < startAngle)
                endAngle += 2 * M_PI;
            if (currentCmd.type == GCodeCommand::ArcCW && endAngle > startAngle)
                endAngle -= 2 * M_PI;
        } else {
            isArc = false;
        }
    }

    float t = float(substep) / maxSubsteps;
    QVector3D interpPos;

    if (isArc) {
        float angle = startAngle + t * (endAngle - startAngle);
        float radius = (position - arcCenter).length();
        float z = currentStart.z() + t * (currentCmd.target.z() - currentStart.z());

        float x = arcCenter.x() + radius * cos(angle);
        float y = arcCenter.y() + radius * sin(angle);
        interpPos = QVector3D(x, y, z);
    } else {
        interpPos = currentStart * (1 - t) + currentCmd.target * t;
    }

    toolpath.append(interpPos);
    if (currentCmd.type == GCodeCommand::Linear ||
        currentCmd.type == GCodeCommand::ArcCW ||
        currentCmd.type == GCodeCommand::ArcCCW) {
        cutVolume.append(interpPos);
    }

    ++substep;
    if (substep > maxSubsteps) {
        position = currentCmd.target;
        ++index;
        substep = 0;
    }

    update();
}



    void parseGCode(const QString& file) {
        QFile f(file);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning("G-code file not found");
            return;
        }

        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed().toUpper();
            if (line.isEmpty() || line.startsWith('(')) continue;

            GCodeCommand cmd;
            if (line.startsWith("G0")) cmd.type = GCodeCommand::Rapid;
            else if (line.startsWith("G1")) cmd.type = GCodeCommand::Linear;
            else if (line.startsWith("G2")) cmd.type = GCodeCommand::ArcCW;
            else if (line.startsWith("G3")) cmd.type = GCodeCommand::ArcCCW;
            else continue;

            if (line.contains("X"))
                cmd.target.setX(extractFloat(line, 'X'));
            else cmd.target.setX(position.x());

            if (line.contains("Y"))
                cmd.target.setY(extractFloat(line, 'Y'));
            else cmd.target.setY(position.y());

            if (line.contains("Z"))
                cmd.target.setZ(extractFloat(line, 'Z'));
            else cmd.target.setZ(position.z());

            if (line.contains("I"))
                cmd.i = extractFloat(line, 'I');

            if (line.contains("J"))
                cmd.j = extractFloat(line, 'J');

            cmds.append(cmd);
        }

        f.close();
    }

    float extractFloat(const QString& line, QChar key) {
        int idx = line.indexOf(key);
        if (idx < 0) return 0;
        QString substr = line.mid(idx + 1).split(QRegExp("[ A-Z]")).first();
        return substr.toFloat();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Optional: Create demo G-code if not already present
    QFile file("demo3d.gcode");
    if (!file.exists()) {
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        out << "G0 X0 Y0 Z0\n";
        out << "G1 X50 Y0 Z-1\n";
        out << "G1 X50 Y50 Z-1\n";
        out << "G2 X0 Y50 I-25 J0\n";
        out << "G3 X0 Y0 I0 J-25\n";
        out << "G0 Z10\n";
        file.close();
    }

    GCodeViewer3D viewer;
    viewer.show();

    return app.exec();
}
