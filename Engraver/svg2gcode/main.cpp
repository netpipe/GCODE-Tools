#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QMainWindow>
#include <QDomDocument>
#include <QFile>
#include <QMessageBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGroupBox>
#include <QPen>
#include <cmath>
#include <QTextStream>

struct Point {
    double x, y;
};

QPointF cubicBezier(QPointF p0, QPointF p1, QPointF p2, QPointF p3, double t) {
    double u = 1 - t;
    return QPointF(
        pow(u, 3) * p0.x() + 3 * pow(u, 2) * t * p1.x() + 3 * u * pow(t, 2) * p2.x() + pow(t, 3) * p3.x(),
        pow(u, 3) * p0.y() + 3 * pow(u, 2) * t * p1.y() + 3 * u * pow(t, 2) * p2.y() + pow(t, 3) * p3.y()
    );
}

QPointF quadraticBezier(QPointF p0, QPointF p1, QPointF p2, double t) {
    double u = 1 - t;
    return QPointF(
        u * u * p0.x() + 2 * u * t * p1.x() + t * t * p2.x(),
        u * u * p0.y() + 2 * u * t * p1.y() + t * t * p2.y()
    );
}

QVector<QPointF> flattenCubicBezier(QPointF p0, QPointF p1, QPointF p2, QPointF p3, int steps) {
    QVector<QPointF> result;
    for (int i = 0; i <= steps; ++i)
        result.append(cubicBezier(p0, p1, p2, p3, static_cast<double>(i) / steps));
    return result;
}

QVector<QPointF> flattenQuadraticBezier(QPointF p0, QPointF p1, QPointF p2, int steps) {
    QVector<QPointF> result;
    for (int i = 0; i <= steps; ++i)
        result.append(quadraticBezier(p0, p1, p2, static_cast<double>(i) / steps));
    return result;
}

QVector<QPointF> parsePathData(const QString& d) {
    QVector<QPointF> points;
    QTextStream stream(const_cast<QString*>(&d), QIODevice::ReadOnly);

    QPointF current(0, 0), start(0, 0);
    QChar cmd;
    double x, y;
    while (!stream.atEnd()) {
        stream >> cmd;
        if (cmd == 'M' || cmd == 'm') {
            stream >> x >> y;
            if (cmd == 'm') { x += current.x(); y += current.y(); }
            current = QPointF(x, y);
            start = current;
            points.append(current);
        } else if (cmd == 'L' || cmd == 'l') {
            stream >> x >> y;
            if (cmd == 'l') { x += current.x(); y += current.y(); }
            current = QPointF(x, y);
            points.append(current);
        } else if (cmd == 'C' || cmd == 'c') {
            QPointF cp1, cp2, end;
            stream >> cp1.rx() >> cp1.ry();
            stream >> cp2.rx() >> cp2.ry();
            stream >> end.rx() >> end.ry();
            if (cmd == 'c') {
                cp1 += current;
                cp2 += current;
                end += current;
            }
            QVector<QPointF> bezier = flattenCubicBezier(current, cp1, cp2, end, 20);
            bezier.removeFirst();
            points += bezier;
            current = end;
        } else if (cmd == 'Q' || cmd == 'q') {
            QPointF cp, end;
            stream >> cp.rx() >> cp.ry();
            stream >> end.rx() >> end.ry();
            if (cmd == 'q') {
                cp += current;
                end += current;
            }
            QVector<QPointF> bezier = flattenQuadraticBezier(current, cp, end, 20);
            bezier.removeFirst();
            points += bezier;
            current = end;
        } else if (cmd == 'Z' || cmd == 'z') {
            points.append(start);
            current = start;
        }
    }
    return points;
}

QString generateGCode(const QVector<QPointF>& points, double scale, double offsetX, double offsetY) {
    QString gcode;
    gcode += "G21 ; Set units to mm\n";
    gcode += "G90 ; Absolute positioning\n";
    gcode += "G1 F1000\n";

    bool first = true;
    for (const QPointF& p : points) {
        double px = p.x() * scale + offsetX;
        double py = p.y() * scale + offsetY;
        if (first) {
            gcode += QString("G0 X%1 Y%2\nM3\n").arg(px).arg(py);
            first = false;
        }
        gcode += QString("G1 X%1 Y%2\n").arg(px).arg(py);
    }
    gcode += "M5\n";
    return gcode;
}

QString generateGCMC(const QVector<QPointF>& points, double scale, double offsetX, double offsetY) {
    QString gcmc;
    gcmc += "unit(mm);\n";
    gcmc += "spindle(true);\n";

    bool first = true;
    for (const QPointF& p : points) {
        double px = p.x() * scale + offsetX;
        double py = p.y() * scale + offsetY;
        if (first) {
            gcmc += QString("goto([%1, %2]);\n").arg(px).arg(py);
            first = false;
        } else {
            gcmc += QString("linear([%1, %2]);\n").arg(px).arg(py);
        }
    }

    gcmc += "spindle(false);\n";
    return gcmc;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() {
        QWidget* widget = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(widget);

        QPushButton* loadButton = new QPushButton("Load SVG");
        QPushButton* saveButton = new QPushButton("Save Output");
        generateButton = new QPushButton("Generate Output");
        generateButton->setEnabled(false);

        formatSelector = new QComboBox;
        formatSelector->addItem("G-Code");
        formatSelector->addItem("GCMC");

        QGroupBox* optionsBox = new QGroupBox("Options");
        QVBoxLayout* optionsLayout = new QVBoxLayout(optionsBox);

        scaleInput = new QDoubleSpinBox;
        scaleInput->setRange(0.01, 1000.0);
        scaleInput->setValue(1.0);

        offsetXInput = new QDoubleSpinBox;
        offsetYInput = new QDoubleSpinBox;
        offsetXInput->setRange(-10000, 10000);
        offsetYInput->setRange(-10000, 10000);

        optionsLayout->addWidget(new QLabel("Scale Factor:"));
        optionsLayout->addWidget(scaleInput);
        optionsLayout->addWidget(new QLabel("Offset X (mm):"));
        optionsLayout->addWidget(offsetXInput);
        optionsLayout->addWidget(new QLabel("Offset Y (mm):"));
        optionsLayout->addWidget(offsetYInput);
        optionsLayout->addWidget(new QLabel("Output Format:"));
        optionsLayout->addWidget(formatSelector);

        gcodeOutput = new QTextEdit;
        gcodeOutput->setReadOnly(true);

        previewScene = new QGraphicsScene;
        previewView = new QGraphicsView(previewScene);
        previewView->setMinimumHeight(300);

        layout->addWidget(loadButton);
        layout->addWidget(optionsBox);
        layout->addWidget(generateButton);
        layout->addWidget(previewView);
        layout->addWidget(saveButton);
        layout->addWidget(gcodeOutput);

        setCentralWidget(widget);
        setWindowTitle("SVG to G-Code/GCMC Converter");

        connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadSVG);
        connect(generateButton, &QPushButton::clicked, this, &MainWindow::generateOutputFromSVG);
        connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveOutput);
    }

private slots:
    void loadSVG() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open SVG File", "", "SVG Files (*.svg)");
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Error", "Cannot open SVG file.");
            return;
        }

        QDomDocument doc;
        if (!doc.setContent(&file)) {
            QMessageBox::warning(this, "Error", "Failed to parse SVG file.");
            return;
        }

        svgPaths.clear();
        QDomNodeList pathList = doc.elementsByTagName("path");
        for (int i = 0; i < pathList.size(); ++i) {
            QDomElement pathElem = pathList.at(i).toElement();
            QString d = pathElem.attribute("d");
            if (!d.isEmpty()) {
                QVector<QPointF> pathPoints = parsePathData(d);
                svgPaths += pathPoints;
            }
        }

        file.close();

        if (svgPaths.isEmpty()) {
            QMessageBox::information(this, "Info", "No paths found in SVG.");
        } else {
            generateButton->setEnabled(true);
            updatePreview();
            QMessageBox::information(this, "Info", "SVG loaded successfully.");
        }
    }

    void generateOutputFromSVG() {
        if (svgPaths.isEmpty()) return;

        double scale = scaleInput->value();
        double offsetX = offsetXInput->value();
        double offsetY = offsetYInput->value();

        if (formatSelector->currentText() == "GCMC") {
            generatedOutput = generateGCMC(svgPaths, scale, offsetX, offsetY);
        } else {
            generatedOutput = generateGCode(svgPaths, scale, offsetX, offsetY);
        }

        gcodeOutput->setPlainText(generatedOutput);
    }

    void saveOutput() {
        if (generatedOutput.isEmpty()) return;
        QString fileName = QFileDialog::getSaveFileName(this, "Save Output", "", "Text Files (*.txt *.gcode *.gcmc)");
        if (fileName.isEmpty()) return;

        QFile outFile(fileName);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Cannot save output file.");
            return;
        }
        outFile.write(generatedOutput.toUtf8());
        outFile.close();
        QMessageBox::information(this, "Saved", "File saved successfully.");
    }

    void updatePreview() {
        previewScene->clear();
        if (svgPaths.isEmpty()) return;

        double scale = 1.0;  // Display in SVG units
        QPointF lastPoint;
        bool first = true;
        QPen pen(Qt::blue, 0);

        for (const QPointF& p : svgPaths) {
            QPointF current = p * scale;
            if (!first) {
                previewScene->addLine(lastPoint.x(), -lastPoint.y(), current.x(), -current.y(), pen);
            }
            lastPoint = current;
            first = false;
        }

        previewScene->setSceneRect(previewScene->itemsBoundingRect());
    }

private:
    QTextEdit* gcodeOutput;
    QPushButton* generateButton;
    QDoubleSpinBox* scaleInput;
    QDoubleSpinBox* offsetXInput;
    QDoubleSpinBox* offsetYInput;
    QComboBox* formatSelector;
    QGraphicsScene* previewScene;
    QGraphicsView* previewView;
    QVector<QPointF> svgPaths;
    QString generatedOutput;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(800, 800);
    window.show();
    return app.exec();
}
