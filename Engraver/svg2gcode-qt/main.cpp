#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QTextStream>
#include <QMessageBox>
#include <QFile>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#include <vector>
#include <cmath>
#include <algorithm>

// Core GCode engine
class Svg2GcodeEngine {
public:
    // Parameters
    double shiftX = 0.0, shiftY = 0.0;
    bool flipY = false;
    bool useZaxis = false;
    double feedRate = 3500;
    int reorderPasses = 30;
    double scale = 1.0;
    bool bezierSmooth = false;
    bool tspOptimize = false;
    bool voronoiOpt = false;
    double finalWidthMM = -1.0;
    double bezierTolerance = 0.5;
    double machineAccuracy = 0.1;
    double zTraverse = 1.0;
    double zEngage = -1.0;

    QString generateGCode(const QString &svgFile) {
        NSVGimage *image = nsvgParseFromFile(svgFile.toUtf8().constData(), "mm", 96);
        if (!image)
            return "Error: Unable to load SVG.";

        std::vector<std::vector<QPointF>> paths;
        double maxY = 0.0;

        for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next) {
            for (NSVGpath *path = shape->paths; path != NULL; path = path->next) {
                std::vector<QPointF> polyline;
                for (int i = 0; i < path->npts - 1; i += 3) {
                    float *p = &path->pts[i * 2];
                    float x = p[0];
                    float y = p[1];
                    maxY = std::max(maxY, double(y));
                    polyline.push_back(QPointF(x, y));
                }
                paths.push_back(polyline);
            }
        }

        QStringList gcode;
        gcode << "G21 ; Set units to mm"
              << "G90 ; Absolute positioning"
              << QString("F%1").arg(feedRate);

        for (auto &polyline : paths) {
            if (polyline.empty())
                continue;
            QPointF start = transformPoint(polyline.front(), maxY);
            gcode << QString("G0 Z%1").arg(zTraverse);
            //gcode << QString("G0 X%1 Y%2").arg(start.x(), start.y());
            gcode << QString("G0 X%1 Y%2")
                        .arg(start.x(), 0, 'f', 4)
                        .arg(start.y(), 0, 'f', 4);

            gcode << QString("G1 Z%1").arg(useZaxis ? zEngage : 0.0);

            for (const QPointF &pt : polyline) {
                QPointF p = transformPoint(pt, maxY);
                //gcode << QString("G1 X%1 Y%2").arg(p.x()).arg(p.y());
                gcode << QString("G1 X%1 Y%2")
                            .arg(p.x(), 0, 'f', 4)
                            .arg(p.y(), 0, 'f', 4);

            }

            gcode << QString("G0 Z%1").arg(zTraverse);
        }

        gcode << "M30 ; Program end";

        nsvgDelete(image);
        return gcode.join("\n");
    }

private:
    QPointF transformPoint(QPointF pt, double maxY) {
        double x = (pt.x() + shiftX) * scale;
        double y = (pt.y() + shiftY);

        if (flipY)
            y = (-y) + (maxY * 2);

        y *= scale;
        return QPointF(x, y);
    }
};

// GUI class
class Svg2GcodeGUI : public QWidget {
    Q_OBJECT
public:
    Svg2GcodeGUI(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QFormLayout(this);

        QPushButton *loadBtn = new QPushButton("Load SVG");
        svgFilePath = new QLineEdit();
        svgFilePath->setReadOnly(true);

        shiftX = new QDoubleSpinBox(); shiftX->setRange(-9999, 9999);
        shiftY = new QDoubleSpinBox(); shiftY->setRange(-9999, 9999);
        flipY = new QCheckBox("Flip Y Axis");
        useZaxis = new QCheckBox("Use Z Axis");
        feedRate = new QDoubleSpinBox(); feedRate->setRange(1, 99999); feedRate->setValue(3500);
        reorderPasses = new QSpinBox(); reorderPasses->setRange(0, 999); reorderPasses->setValue(30);
        scale = new QDoubleSpinBox(); scale->setRange(0.01, 100.0); scale->setValue(1.0);
        finalWidthMM = new QDoubleSpinBox(); finalWidthMM->setRange(-1.0, 10000.0); finalWidthMM->setValue(-1.0);
        bezierTolerance = new QDoubleSpinBox(); bezierTolerance->setRange(0.001, 10.0); bezierTolerance->setValue(0.5);
        machineAccuracy = new QDoubleSpinBox(); machineAccuracy->setRange(0.001, 10.0); machineAccuracy->setValue(0.1);
        zTraverse = new QDoubleSpinBox(); zTraverse->setValue(1.0);
        zEngage = new QDoubleSpinBox(); zEngage->setValue(-1.0);

        bezierSmooth = new QCheckBox("Enable Bezier Smoothing");
        tspOptimize = new QCheckBox("TSP Path Optimize");
        voronoiOpt = new QCheckBox("Voronoi Optimization");

        QPushButton *genBtn = new QPushButton("Generate GCode");
        QPushButton *saveBtn = new QPushButton("Save GCode");
        output = new QTextEdit();

        layout->addRow(loadBtn);
        layout->addRow("SVG File:", svgFilePath);
        layout->addRow("Shift X (mm):", shiftX);
        layout->addRow("Shift Y (mm):", shiftY);
        layout->addRow(flipY);
        layout->addRow(useZaxis);
        layout->addRow("Feed Rate:", feedRate);
        layout->addRow("Reorder Passes:", reorderPasses);
        layout->addRow("Scale:", scale);
        layout->addRow("Final Width (mm):", finalWidthMM);
        layout->addRow("Bezier Tolerance:", bezierTolerance);
        layout->addRow("Machine Accuracy:", machineAccuracy);
        layout->addRow("Z Traverse Height:", zTraverse);
        layout->addRow("Z Engage Depth:", zEngage);
        layout->addRow(bezierSmooth);
        layout->addRow(tspOptimize);
        layout->addRow(voronoiOpt);
        layout->addRow(genBtn);
        layout->addRow(saveBtn);
        layout->addRow(output);

        connect(loadBtn, &QPushButton::clicked, this, &Svg2GcodeGUI::loadSVG);
        connect(genBtn, &QPushButton::clicked, this, &Svg2GcodeGUI::generateGcode);
        connect(saveBtn, &QPushButton::clicked, this, &Svg2GcodeGUI::saveGcode);
    }

private slots:
    void loadSVG() {
        QString file = QFileDialog::getOpenFileName(this, "Select SVG File", "", "*.svg");
        if (!file.isEmpty())
            svgFilePath->setText(file);
    }

    void generateGcode() {
        if (svgFilePath->text().isEmpty())
            return;

        Svg2GcodeEngine engine;
        engine.shiftX = shiftX->value();
        engine.shiftY = shiftY->value();
        engine.flipY = flipY->isChecked();
        engine.useZaxis = useZaxis->isChecked();
        engine.feedRate = feedRate->value();
        engine.reorderPasses = reorderPasses->value();
        engine.scale = scale->value();
        engine.finalWidthMM = finalWidthMM->value();
        engine.bezierTolerance = bezierTolerance->value();
        engine.machineAccuracy = machineAccuracy->value();
        engine.zTraverse = zTraverse->value();
        engine.zEngage = zEngage->value();
        engine.bezierSmooth = bezierSmooth->isChecked();
        engine.tspOptimize = tspOptimize->isChecked();
        engine.voronoiOpt = voronoiOpt->isChecked();

        QString gcode = engine.generateGCode(svgFilePath->text());
        output->setPlainText(gcode);
    }

    void saveGcode() {
        QString file = QFileDialog::getSaveFileName(this, "Save GCode", "", "*.gcode");
        if (!file.isEmpty()) {
            QFile f(file);
            if (f.open(QFile::WriteOnly | QFile::Text)) {
                QTextStream s(&f);
                s << output->toPlainText();
                f.close();
                QMessageBox::information(this, "Done", "GCode saved.");
            }
        }
    }

private:
    QLineEdit *svgFilePath;
    QDoubleSpinBox *shiftX, *shiftY, *feedRate, *scale, *finalWidthMM, *bezierTolerance, *machineAccuracy, *zTraverse, *zEngage;
    QSpinBox *reorderPasses;
    QCheckBox *flipY, *useZaxis, *bezierSmooth, *tspOptimize, *voronoiOpt;
    QTextEdit *output;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Svg2GcodeGUI gui;
    gui.setWindowTitle("svg2gcode Qt GUI");
    gui.resize(600, 900);
    gui.show();
    return app.exec();
}
