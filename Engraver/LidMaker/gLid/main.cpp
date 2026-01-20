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
    double depth,
    double toolDiameter,
    int passes
) {
    QString gcode;
    double radius = (innerDiameter - toolDiameter) / 2.0;
    double stepDown = depth / passes;

    gcode += "(Wooden Lid Thread Milling)\n";
    gcode += "G21  (mm units)\n";
    gcode += "G90  (absolute positioning)\n";
    gcode += "G17  (XY plane)\n";
    gcode += "G0 Z5\n";
    gcode += "G0 X" + QString::number(radius, 'f', 3) + " Y0\n";

    double currentZ = 0.0;

    for (int i = 1; i <= passes; ++i) {
        currentZ = -stepDown * i;

        gcode += "(Pass " + QString::number(i) + ")\n";
        gcode += "G1 Z" + QString::number(currentZ, 'f', 3) + " F200\n";

        double revolutions = depth / pitch;
        int fullTurns = std::ceil(revolutions);

        for (int t = 0; t < fullTurns; ++t) {
            double zMove = pitch;
            gcode += "G2 X" + QString::number(radius, 'f', 3)
                   + " Y0"
                   + " I" + QString::number(-radius, 'f', 3)
                   + " J0"
                   + " Z" + QString::number(currentZ - zMove, 'f', 3)
                   + " F300\n";
            currentZ -= zMove;
        }
    }

    gcode += "G0 Z5\n";
    gcode += "M30\n";

    return gcode;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Wood Thread G-Code Generator");

    QVBoxLayout *layout = new QVBoxLayout(&window);
    QFormLayout *form = new QFormLayout();

    QLineEdit *diameterEdit = new QLineEdit("86.0");
    QLineEdit *pitchEdit = new QLineEdit("4.233"); // approx wide-mouth style
    QLineEdit *depthEdit = new QLineEdit("6.0");
    QLineEdit *toolEdit = new QLineEdit("3.175");
    QLineEdit *passesEdit = new QLineEdit("4");

    form->addRow("Inner Diameter (mm)", diameterEdit);
    form->addRow("Thread Pitch (mm)", pitchEdit);
    form->addRow("Thread Depth (mm)", depthEdit);
    form->addRow("Tool Diameter (mm)", toolEdit);
    form->addRow("Passes", passesEdit);

    layout->addLayout(form);

    QPushButton *generateBtn = new QPushButton("Generate G-Code");
    QTextEdit *output = new QTextEdit();
    output->setFontFamily("monospace");

    layout->addWidget(generateBtn);
    layout->addWidget(output);

    QObject::connect(generateBtn, &QPushButton::clicked, [&]() {
        double diameter = diameterEdit->text().toDouble();
        double pitch = pitchEdit->text().toDouble();
        double depth = depthEdit->text().toDouble();
        double tool = toolEdit->text().toDouble();
        int passes = passesEdit->text().toInt();

        QString gcode = generateGCode(diameter, pitch, depth, tool, passes);
        output->setPlainText(gcode);
    });

    window.resize(600, 500);
    window.show();

    return app.exec();
}
