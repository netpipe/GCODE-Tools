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
    bool flipY = true;
    bool useZaxis = true;
    double feedRate = 300;
    int reorderPasses = 30;
    double scale = 1.0;
    bool bezierSmooth = false;
    bool tspOptimize = false;
    bool voronoiOpt = false;
    double finalWidthMM = -1.0;
    double bezierTolerance = 0.5;
    double machineAccuracy = 0.1;
    double zTraverse = 1.0;
    double zEngage = -0.20;

    QString generateGCode(const QString &svgFile) {
        NSVGimage *image = nsvgParseFromFile(svgFile.toUtf8().constData(), "mm", 96);
        if (!image)
            return "Error: Unable to load SVG.";

        std::vector<std::vector<QPointF>> paths;
        double maxY = 0.0;

        for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next) {
            for (NSVGpath *path = shape->paths; path != NULL; path = path->next) {
                std::vector<QPointF> polyline;
                const int segments = bezierSmooth ? 10 : 1;

                for (int i = 0; i < path->npts - 1; i += 3) {
                    float *p = &path->pts[i * 2];

                    float x1 = p[0], y1 = p[1];
                    float x2 = p[2], y2 = p[3];
                    float x3 = p[4], y3 = p[5];
                    float x4 = p[6], y4 = p[7];

                    for (int j = 0; j <= segments; ++j) {
                        float t = (float)j / segments;
                        float it = 1.0f - t;

                        // Cubic Bezier formula
                        float x = it*it*it*x1 + 3*it*it*t*x2 + 3*it*t*t*x3 + t*t*t*x4;
                        float y = it*it*it*y1 + 3*it*it*t*y2 + 3*it*t*t*y3 + t*t*t*y4;

                        polyline.push_back(QPointF(x, y));
                        maxY = std::max(maxY, double(y));
                    }
                }

                paths.push_back(polyline);
            }
        }

        QStringList gcode;
        gcode << "G21 ; Set units to mm"
              << "G90 ; Absolute positioning"
              << QString("F%1").arg(feedRate);
        gcode << QString("M03 S1000");
        for (auto &polyline : paths) {
            if (polyline.empty())
                continue;
            QPointF start = transformPoint(polyline.front(), maxY);


            gcode << QString("G0 Z%1").arg(zTraverse);
            //gcode << QString("G0 X%1 Y%2").arg(start.x(), start.y());
            gcode << QString("G0 X%1 Y%2")
                        .arg(start.x(), 0, 'f', 4)
                        .arg(start.y(), 0, 'f', 4);
            //gcode << QString("M400");
          //        gcode << QString("M03 S1000");
            gcode << QString("G1 Z%1F200").arg(useZaxis ? zEngage : 0.0);
            for (const QPointF &pt : polyline) {
                QPointF p = transformPoint(pt, maxY);
                gcode << QString("G1 X%1 Y%2")
                            .arg(p.x(), 0, 'f', 4)
                            .arg(p.y(), 0, 'f', 4);

            }

            gcode << QString("G0 Z%1").arg(zTraverse);
           // gcode << "M05";
        }

        gcode << "M05";
        gcode << "M02";
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
        shiftX->setValue(-200);
        shiftY = new QDoubleSpinBox(); shiftY->setRange(-9999, 9999);
        shiftY->setValue(500);
        flipY = new QCheckBox("Flip Y Axis");flipY->setChecked(1);
        useZaxis = new QCheckBox("Use Z Axis");useZaxis->setChecked(1);
        feedRate = new QDoubleSpinBox(); feedRate->setRange(1, 99999); feedRate->setValue(300);
        reorderPasses = new QSpinBox(); reorderPasses->setRange(0, 999); reorderPasses->setValue(30);
        scale = new QDoubleSpinBox(); scale->setRange(-100.0, 100.0); scale->setValue(1.0); scale->setSingleStep(0.5);
        finalWidthMM = new QDoubleSpinBox(); finalWidthMM->setRange(-1.0, 10000.0); finalWidthMM->setValue(-1.0);
        bezierTolerance = new QDoubleSpinBox(); bezierTolerance->setRange(0.001, 10.0); bezierTolerance->setValue(0.5);
        machineAccuracy = new QDoubleSpinBox(); machineAccuracy->setRange(0.001, 10.0); machineAccuracy->setValue(0.1);
        zTraverse = new QDoubleSpinBox(); zTraverse->setValue(5.0);
        zEngage = new QDoubleSpinBox(); zEngage->setValue(-1.0);
        zEngage->setMinimum(-100);
        zEngage->setValue(-0.2);

        bezierSmooth = new QCheckBox("Enable Bezier Smoothing"); bezierSmooth->setChecked(1);
        tspOptimize = new QCheckBox("TSP Path Optimize"); tspOptimize->setChecked(1);
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
