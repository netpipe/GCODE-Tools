// main.cpp
// Grayscale Heightmap to Pocketing G-code (Standalone Qt App)

#include <QApplication>
#include <QImage>
#include <QFileDialog>
#include <QFile>
#include <QPainterPath>
#include <QDebug>
#include "clipper2/clipper.h"
#include <QBitmap>

using namespace Clipper2Lib;

const double pixel_size_mm = 0.2; // scale: 1 pixel = 0.2 mm
const double layer_height_mm = 1.0;
const double max_depth_mm = 5.0;
const double tool_radius_mm = 1.0;

// Convert QPainterPath to Clipper Path
PathD pathFromQPainterPath(const QPainterPath& qpath) {
    PathD path;
    for (int i = 0; i < qpath.elementCount(); ++i) {
        auto el = qpath.elementAt(i);
        path.emplace_back(el.x * pixel_size_mm, el.y * pixel_size_mm);
    }
    return path;
}

QString generateGCode(const PathsD& layers, double depth) {
    QString code;
    code += QString("G1 Z%1 F300\n").arg(-depth);
    for (const auto& path : layers) {
        if (path.empty()) continue;
        code += QString("G0 X%1 Y%2\n").arg(path[0].x).arg(path[0].y);
        code += "G1 F500\n";
        for (size_t i = 1; i < path.size(); ++i) {
            code += QString("G1 X%1 Y%2\n").arg(path[i].x).arg(path[i].y);
        }
        code += "G0 Z5\n";
    }
    return code;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QString file = QFileDialog::getOpenFileName(nullptr, "Open Heightmap PNG", "", "Images (*.png *.bmp *.jpg)");
    if (file.isEmpty()) return 0;

    QImage img(file);
    if (img.isNull()) return 1;

    img = img.convertToFormat(QImage::Format_Grayscale8);
    int w = img.width();
    int h = img.height();

    QString gcode = "G21\nG90\nG0 Z5\n";

    for (double depth = 0; depth <= max_depth_mm; depth += layer_height_mm) {
        int threshold = static_cast<int>(255.0 * (depth / max_depth_mm));

        QImage binary = img;
        for (int y = 0; y < h; ++y) {
            uchar* scan = binary.scanLine(y);
            for (int x = 0; x < w; ++x) {
                scan[x] = (scan[x] < threshold) ? 0 : 255;
            }
        }

        QPainterPath path;
        path.addRegion(QRegion(QBitmap::fromImage(binary)));
        PathD base = pathFromQPainterPath(path);

        PathsD offset = InflatePaths(PathsD{base}, -tool_radius_mm, JoinType::Round, EndType::Polygon);
        gcode += generateGCode(offset, depth);
    }

    gcode += "M30\n";

    QFile out("pocket.gcode");
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        out.write(gcode.toUtf8());
        out.close();
        qDebug() << "G-code saved to pocket.gcode";
    }

    return 0;
}
