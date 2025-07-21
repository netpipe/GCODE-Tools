#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QTextEdit>
#include <QProcess>

//https://github.com/holgafreak/svg2gcode frontend

class Svg2GcodeFrontend : public QWidget {
    Q_OBJECT

public:
    Svg2GcodeFrontend(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QFormLayout(this);

        QPushButton *loadButton = new QPushButton("Load SVG");
        layout->addRow(loadButton);

        filePath = new QLineEdit();
        filePath->setReadOnly(true);
        layout->addRow("SVG File:", filePath);

        shiftY = new QDoubleSpinBox(); shiftY->setRange(-10000, 10000);
        shiftX = new QDoubleSpinBox(); shiftX->setRange(-10000, 10000);
        useZAxis = new QCheckBox("Use Z-axis instead of Laser");
        feedRate = new QSpinBox(); feedRate->setRange(1, 100000); feedRate->setValue(3500);
        reorders = new QSpinBox(); reorders->setRange(0, 1000); reorders->setValue(30);
        scale = new QDoubleSpinBox(); scale->setRange(0.01, 100.0); scale->setValue(1.0);scale->setSingleStep(0.5);
        flipYAxis = new QCheckBox("Flip Y-axis");
        width = new QDoubleSpinBox(); width->setRange(0.1, 10000.0);
        bezierTolerance = new QDoubleSpinBox(); bezierTolerance->setRange(0.01, 10.0); bezierTolerance->setValue(0.5);
        machineAccuracy = new QDoubleSpinBox(); machineAccuracy->setRange(0.001, 10.0); machineAccuracy->setValue(0.1);
        zTraverse = new QDoubleSpinBox(); zTraverse->setValue(1.0);
        zEngage = new QDoubleSpinBox(); zEngage->setValue(-1.0);        zEngage->setMinimum(-100);
        bezierSmoothing = new QCheckBox("Bezier Curve Smoothing");
        tspPath = new QCheckBox("Engrave only TSP-path");
        voronoiOptimize = new QCheckBox("Optimize for Voronoi Stipples");


        layout->addRow("Shift Y:", shiftY);
        layout->addRow("Shift X:", shiftX);
        layout->addRow(useZAxis);
        layout->addRow("Feed Rate:", feedRate);
        layout->addRow("Reorders:", reorders);
        layout->addRow("Scale:", scale);
        layout->addRow(flipYAxis);
        layout->addRow("Final Width (mm):", width);
        layout->addRow("Bezier Tolerance:", bezierTolerance);
        layout->addRow("Machine Accuracy:", machineAccuracy);
        layout->addRow("Z-Traverse:", zTraverse);
        layout->addRow("Z-Engage:", zEngage);
        layout->addRow(bezierSmoothing);
        layout->addRow(tspPath);
        layout->addRow(voronoiOptimize);

        QPushButton *generateButton = new QPushButton("Generate GCode");
        layout->addRow(generateButton);

        outputGCode = new QTextEdit();
        layout->addRow(outputGCode);

        connect(loadButton, &QPushButton::clicked, this, &Svg2GcodeFrontend::loadFile);
        connect(generateButton, &QPushButton::clicked, this, &Svg2GcodeFrontend::generateGCode);
    }

private slots:
    void loadFile() {
        QString file = QFileDialog::getOpenFileName(this, "Select SVG File", "", "SVG Files (*.svg)");
        if (!file.isEmpty()) {
            filePath->setText(file);
        }
    }

    void generateGCode() {
        if (filePath->text().isEmpty()) return;

        QStringList args;
        args << "-Y" << QString::number(shiftY->value())
             << "-X" << QString::number(shiftX->value());
        if (useZAxis->isChecked()) args << "-c";
        args << "-f" << QString::number(feedRate->value())
             << "-n" << QString::number(reorders->value())
             << "-s" << QString::number(scale->value());
        if (flipYAxis->isChecked()) args << "-F";
        if (width->value() > 0.1) args << "-w" << QString::number(width->value());
        args << "-t" << QString::number(bezierTolerance->value())
             << "-m" << QString::number(machineAccuracy->value())
             << "-z" << QString::number(zTraverse->value())
             << "-Z" << QString::number(zEngage->value());
        if (bezierSmoothing->isChecked()) args << "-B";
        if (tspPath->isChecked()) args << "-T";
        if (voronoiOptimize->isChecked()) args << "-V";
        args << filePath->text();

        QString outputFilePath = filePath->text() + ".gcode";
        args << outputFilePath;



        QProcess process;
        process.start(QApplication::applicationDirPath() + "/svg2gcode", args);  // assumes svg2gcode binary is in PATH
        process.waitForFinished();

        QString output = process.readAllStandardOutput();
        QString errors = process.readAllStandardError();

        outputGCode->setPlainText(output + "\n" + errors);
    }

private:
    QLineEdit *filePath;
    QDoubleSpinBox *shiftY;
    QDoubleSpinBox *shiftX;
    QCheckBox *useZAxis;
    QSpinBox *feedRate;
    QSpinBox *reorders;
    QDoubleSpinBox *scale;
    QCheckBox *flipYAxis;
    QDoubleSpinBox *width;
    QDoubleSpinBox *bezierTolerance;
    QDoubleSpinBox *machineAccuracy;
    QDoubleSpinBox *zTraverse;
    QDoubleSpinBox *zEngage;
    QCheckBox *bezierSmoothing;
    QCheckBox *tspPath;
    QCheckBox *voronoiOptimize;
    QTextEdit *outputGCode;
};


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Svg2GcodeFrontend window;
    window.setWindowTitle("SVG2GCODE Qt Frontend");
    window.resize(600, 800);
    window.show();
    return app.exec();
}
#include "main.moc"
