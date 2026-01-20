#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QString>
#include <cmath>

QString generateGCode(
    double innerDiameter,
    double pitch,
    double totalDepth,
    double toolDiameter,
    int passes
) {
    QString g;

    double radius = (innerDiameter - toolDiameter) / 2.0;
    double passDepth = totalDepth / passes;
    int segmentsPerRev = 8; // safer for GRBL
    double angleStep = (2 * M_PI) / segmentsPerRev;

    g += "(GRBL Wooden Lid Thread Milling)\n";
    g += "G21\n";
    g += "G90\n";
    g += "G17\n";
    g += "G94\n";
    g += "G0 Z5\n";
    g += "G0 X" + QString::number(radius, 'f', 3) + " Y0\n";

    for (int p = 1; p <= passes; ++p) {
        double currentZ = -passDepth * p;
        g += "(Pass " + QString::number(p) + ")\n";
        g += "G1 Z" + QString::number(currentZ, 'f', 3) + " F150\n";

        double revolutions = totalDepth / pitch;
        int totalSegments = std::ceil(revolutions * segmentsPerRev);

        double angle = 0.0;
        double z = currentZ;

        for (int s = 0; s < totalSegments; ++s) {
            double nextAngle = angle + angleStep;

            double x = radius * cos(nextAngle);
            double y = radius * sin(nextAngle);

            z -= pitch / segmentsPerRev;

            double i = -radius * cos(angle);
            double j = -radius * sin(angle);

            g += "G2 X" + QString::number(x, 'f', 3)
              + " Y" + QString::number(y, 'f', 3)
              + " I" + QString::number(i, 'f', 3)
              + " J" + QString::number(j, 'f', 3)
              + " Z" + QString::number(z, 'f', 3)
              + " F300\n";

            angle = nextAngle;
        }

        g += "G0 Z5\n";
        g += "G0 X" + QString::number(radius, 'f', 3) + " Y0\n";
    }

    g += "M30\n";
    return g;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("GRBL Wood Thread Generator");

    QVBoxLayout *layout = new QVBoxLayout(&window);
    QFormLayout *form = new QFormLayout();

    QLineEdit *diameterEdit = new QLineEdit("86.0");
    QLineEdit *pitchEdit = new QLineEdit("4.233");
    QLineEdit *depthEdit = new QLineEdit("6.0");
    QLineEdit *toolEdit = new QLineEdit("3.175");
    QLineEdit *passesEdit = new QLineEdit("4");

    form->addRow("Inner Diameter (mm)", diameterEdit);
    form->addRow("Thread Pitch (mm)", pitchEdit);
    form->addRow("Thread Depth (mm)", depthEdit);
    form->addRow("Tool Diameter (mm)", toolEdit);
    form->addRow("Passes", passesEdit);

    layout->addLayout(form);

    QPushButton *generateBtn = new QPushButton("Generate GRBL G-Code");
    QTextEdit *output = new QTextEdit();
    output->setFontFamily("monospace");

    layout->addWidget(generateBtn);
    layout->addWidget(output);

    QObject::connect(generateBtn, &QPushButton::clicked, [&]() {
        QString gcode = generateGCode(
            diameterEdit->text().toDouble(),
            pitchEdit->text().toDouble(),
            depthEdit->text().toDouble(),
            toolEdit->text().toDouble(),
            passesEdit->text().toInt()
        );
        output->setPlainText(gcode);
    });

    window.resize(640, 520);
    window.show();

    return app.exec();
}
