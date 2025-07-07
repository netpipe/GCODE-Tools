// TSlotSlicer.cpp
// Qt + OpenGL + STL loader + slicer with T-slot + V-bit support + ClipperLib for pocketing/contours

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
#include "clipper2/clipper.h"

using namespace Clipper2Lib;

struct Triangle {
    QVector3D v0, v1, v2;
};

enum ToolType { TSlot, VBit };

struct Tool {
    ToolType type = TSlot;
    double shaft_diameter = 6.0;
    double head_diameter = 10.0;
    double v_angle_deg = 60.0; // used for V-bit
    double cut_depth = -5.0;
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
        if (pts.size() == 2) {
            lines.push_back(pts[0]);
            lines.push_back(pts[1]);
        }
    }
    qDebug() << "test";
    return lines;
}

QString generateGCodeWithClipper(const std::vector<std::vector<QVector3D>>& allLayers, const Tool& tool) {
    QString code = "G21\nG90\nG0 Z5\nT1 M6\n";

    for (const auto& layer : allLayers) {
        if (layer.empty()) continue;

        // Convert line segments to Clipper Path
        PathsD paths;
        for (size_t i = 0; i + 1 < layer.size(); i += 2) {
            PathD path;
            path.push_back(PointD(layer[i].x(), layer[i].y()));
            path.push_back(PointD(layer[i + 1].x(), layer[i + 1].y()));
            paths.push_back(path);
        }

        // Optional: unify/join paths, offset for tool radius if needed
        auto offsetPaths = InflatePaths(paths, -tool.shaft_diameter / 2.0, JoinType::Round, EndType::Round);

        code += QString("G1 Z%1 F200\n").arg(tool.cut_depth);
        for (const auto& path : offsetPaths) {
            if (path.empty()) continue;
            code += QString("G0 X%1 Y%2\n").arg(path[0].x).arg(path[0].y);
            code += "G1 F500\n";
            for (size_t i = 1; i < path.size(); ++i) {
                code += QString("G1 X%1 Y%2\n").arg(path[i].x).arg(path[i].y);
            }
            code += "G0 Z5\n";
        }
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

        QString gcode = generateGCodeWithClipper(layers, tool);
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
    }
    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_LINES);
        glColor3f(0, 1, 0);
        if (!layers.empty()) {
            for (const auto& v : layers.front()) {
                glVertex2f(v.x() / 100.0f, v.y() / 100.0f);
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
