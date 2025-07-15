// main.cpp
// DXF to G-code generator for lids: pocketing, contours, tool library (T-slot, V-bit, flat), multipass

#include <QApplication>
#include <QFileDialog>
#include <QPainter>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>
#include "clipper2/clipper.h"
#include "drw_interface.h"
#include "libdxfrw.h"

#include "drw_interface.h"  // contains DRW_Interface
#include "drw_header.h"     // for DRW_Header
#include "drw_entities.h"   // entity definitions

using namespace Clipper2Lib;

enum ToolType { FlatEnd, TSlot, VBit };

struct Tool {
    ToolType type;
    double diameter;
    double depth_per_pass;
    double total_depth;
    double v_angle_deg; // Only for VBit
};

class DXFViewer : public QWidget, public DRW_Interface {
public:
    std::vector<QPointF> polyline;
    std::map<std::string, std::vector<PathD>> layerPaths;
    Tool currentTool = { TSlot, 6.0, 1.5, 4.5, 60.0 }; // Default tool

    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}

    DXFViewer() {
        auto* layout = new QVBoxLayout(this);
        auto* btnLoad = new QPushButton("Load DXF", this);
        auto* btnGcode = new QPushButton("Export G-code", this);
        layout->addWidget(btnLoad);
        layout->addWidget(btnGcode);
        connect(btnLoad, &QPushButton::clicked, this, &DXFViewer::loadDXF);
        connect(btnGcode, &QPushButton::clicked, this, &DXFViewer::exportGCode);
        setMinimumSize(800, 600);
    }

    void loadDXF() {
        QString path = QFileDialog::getOpenFileName(this, "Open DXF", "", "*.dxf");
        if (path.isEmpty()) return;
        polyline.clear();
        layerPaths.clear();

        DRW_Interface* iface = this;
        DRW_Header header;
     //   dxfRW::read( path.toStdString().c_str(), iface, false);
        dxfRW* dxf = new dxfRW(path.toStdString().c_str());
        bool success = dxf->read(this, false);

        //if (!dxfReader.read(&header)) {
       //     qWarning() << "Failed to read DXF.";
       // }
        update();
    }

    void exportGCode() {
        QString code = "G21\nG90\nG0 Z5\nT1 M6\n";

        for (const auto& [layer, paths] : layerPaths) {
            bool isPocket = layer.find("pocket") != std::string::npos;
            bool isContour = layer.find("cut") != std::string::npos;

            auto toolRadius = currentTool.diameter / 2.0;
            auto offsetPaths = InflatePaths(paths, isPocket ? -toolRadius : toolRadius, JoinType::Round, EndType::Polygon);

            for (double depth = -currentTool.depth_per_pass; depth >= -currentTool.total_depth; depth -= currentTool.depth_per_pass) {
                for (const auto& path : offsetPaths) {
                    if (path.empty()) continue;
                    code += QString("G0 X%1 Y%2\n").arg(path[0].x).arg(path[0].y);
                    code += QString("G1 Z%1 F200\n").arg(depth);
                    code += "G1 F300\n";
                    for (size_t i = 1; i < path.size(); ++i) {
                        code += QString("G1 X%1 Y%2\n").arg(path[i].x).arg(path[i].y);
                    }
                    code += "G0 Z5\n";
                }
            }
        }

        code += "M30\n";
        QFile f("lid_output.gcode");
        if (f.open(QIODevice::WriteOnly)) {
            f.write(code.toUtf8());
            f.close();
        }
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(width() / 2, height() / 2);
        p.scale(1, -1);
        p.setPen(Qt::green);
        for (const auto& pt : polyline) {
            p.drawEllipse(pt, 1, 1);
        }
        for (size_t i = 1; i < polyline.size(); ++i) {
            p.drawLine(polyline[i - 1], polyline[i]);
        }
    }

    std::string currentLayer;

    void setLayer(const std::string& name) {
        currentLayer = name;
    }

    void addLine(const DRW_Line& data) override {
        polyline.push_back(QPointF(data.basePoint.x, data.basePoint.y));
        polyline.push_back(QPointF(data.secPoint.x, data.secPoint.y));
        PathD path;
        path.push_back({data.basePoint.x, data.basePoint.y});
        path.push_back({data.secPoint.x, data.secPoint.y});
        layerPaths[currentLayer].push_back(path);
    }

    void addLWPolyline(const DRW_LWPolyline& data) override {
        PathD path;
        for (const auto& v : data.vertlist) {
            polyline.push_back(QPointF(v->x, v->y));
            path.push_back({v->x, v->y});
        }
        if (data.flags & 1 && !path.empty()) {
            path.push_back(path.front()); // close if needed
        }
        layerPaths[currentLayer].push_back(path);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    DXFViewer viewer;
    viewer.show();
    return app.exec();
}
