// TSlotSlicer.cpp
// Qt + OpenGL + STL loader + slicer with T-slot + V-bit support

#include <QApplication>
#include <QOpenGLWidget>
#include <QFileDialog>
#include <QVector3D>
#include <QSurfaceFormat>
#include <QTimer>
#include <QDebug>
#include <vector>
#include <fstream>
#include <cmath>
#include <QFile>
#include <QPainterPath>
#include <QPolygonF>

struct Triangle {
    QVector3D v0, v1, v2;
};

enum ToolType { TSlot, VBit };

struct Tool {
    ToolType type;
    double shaft_diameter;
    double head_diameter;
    double v_angle_deg; // used for V-bit
    double cut_depth;

    Tool(ToolType t = TSlot,
         double shaft = 6.0,
         double head = 10.0,
         double v_angle = 60.0,
         double depth = -5.0)
        : type(t), shaft_diameter(shaft), head_diameter(head), v_angle_deg(v_angle), cut_depth(depth) {}
};


std::vector<Triangle> loadSTL(const QString& path) {
    std::vector<Triangle> tris;
    std::ifstream file(path.toStdString(), std::ios::binary);
    if (!file) return tris;

    char header[80];
    file.read(header, 80);
    uint32_t count;
    file.read(reinterpret_cast<char*>(&count), 4);

    for (uint32_t i = 0; i < count; ++i) {
        float n[3], v[9];
        uint16_t attr;
        file.read(reinterpret_cast<char*>(n), 12);
        file.read(reinterpret_cast<char*>(v), 36);
        file.read(reinterpret_cast<char*>(&attr), 2);
        tris.push_back({
            QVector3D(v[0], v[1], v[2]),
            QVector3D(v[3], v[4], v[5]),
            QVector3D(v[6], v[7], v[8])
        });
    }
    return tris;
}

std::vector<QVector3D> sliceLayer(const std::vector<Triangle>& tris, float z) {
    std::vector<QVector3D> lines;
    for (const auto& tri : tris) {
        std::vector<QVector3D> pts;
        auto test = [&](QVector3D a, QVector3D b) {
            if ((a.z() < z && b.z() > z) || (a.z() > z && b.z() < z)) {
                float t = (z - a.z()) / (b.z() - a.z());
                pts.push_back(a + t * (b - a));
            }
        };
        test(tri.v0, tri.v1);
        test(tri.v1, tri.v2);
        test(tri.v2, tri.v0);
        // qDebug() << tri.v0;
        if (pts.size() == 2) {
            lines.push_back(pts[0]);
            lines.push_back(pts[1]);
        }
    }
    qDebug() << "layer sliced";
    return lines;
}

QString generateGCode(const std::vector<std::vector<QVector3D>>& allLayers, const Tool& tool) {
    QString code = "G21\nG90\nG0 Z5\nT1 M6\n";

    for (const auto& layer : allLayers) {
        code += QString("G1 Z%1 F200\n").arg(tool.cut_depth);
        for (size_t i = 0; i + 1 < layer.size(); i += 2) {
            code += QString("G0 X%1 Y%2\n").arg(layer[i].x()).arg(layer[i].y());
            code += QString("G1 X%1 Y%2 F500\n").arg(layer[i + 1].x()).arg(layer[i + 1].y());
        }
        code += "G0 Z5\n";
    }
    code += "M30\n";
    return code;
}

class SlicerWidget : public QOpenGLWidget {
public:
    std::vector<Triangle> model;
    std::vector<std::vector<QVector3D>> layers;
    Tool tool;
    float layerHeight = 1.0f;
    float maxZ = 0.0f;

    void loadModel(const QString& path) {
        model = loadSTL(path);

        maxZ = 0.0f;
        for (const auto& tri : model) {
            maxZ = std::max({ maxZ, tri.v0.z(), tri.v1.z(), tri.v2.z() });
        }

        layers.clear();
        for (float z = 0.0f; z <= maxZ; z += layerHeight) {
            layers.push_back(sliceLayer(model, z));
        }

        QString gcode = generateGCode(layers, tool);
        QFile file("output.gcode");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(gcode.toUtf8());
            file.close();
        }
        update();
    }

protected:
    void initializeGL() override {
        glClearColor(0.1f, 0.1f, 0.1f, 1);
    }
    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = float(w) / float(h ? h : 1);
        glOrtho(-100 * aspect, 100 * aspect, -100, 100, -1, 1);
        glMatrixMode(GL_MODELVIEW);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();
        glColor3f(0, 1, 0);

        glBegin(GL_LINES);
        if (!layers.empty()) {
            const auto& layer = layers.front();
            for (size_t i = 0; i + 1 < layer.size(); i += 2) {
                glVertex2f(layer[i].x(), layer[i].y());
                glVertex2f(layer[i + 1].x(), layer[i + 1].y());
            }
        }
        glEnd();
    }

};

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    SlicerWidget* slicer = new SlicerWidget;
    slicer->resize(800, 600);
    slicer->tool = { TSlot, 6.0, 10.0, 60.0, -5.0 }; // Default tool is T-slot
    slicer->show();

    QTimer::singleShot(500, [&] {
        QString file = QFileDialog::getOpenFileName(nullptr, "Load STL", "", "STL Files (*.stl)");
        if (!file.isEmpty()) slicer->loadModel(file);
    });

    return app.exec();
}
