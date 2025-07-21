#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

struct Point {
    double x, y;
};

Point cubicBezier(Point p0, Point p1, Point p2, Point p3, double t) {
    double u = 1 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    return {
        uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x,
        uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y
    };
}

vector<Point> flattenCubicBezier(Point p0, Point p1, Point p2, Point p3, int steps) {
    vector<Point> result;
    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        result.push_back(cubicBezier(p0, p1, p2, p3, t));
    }
    return result;
}

void writeGCode(const vector<Point>& points, const string& outputFile) {
    ofstream out(outputFile);
    if (!out) {
        cerr << "Failed to open output file.\n";
        return;
    }

    out << "G21 ; Set units to millimeters\n";
    out << "G90 ; Absolute positioning\n";
    out << "G1 F1000 ; Set feed rate\n";

    bool first = true;
    for (const Point& p : points) {
        if (first) {
            out << "G0 X" << p.x << " Y" << p.y << "\n";
            out << "M3 ; Laser ON\n";
            first = false;
        }
        out << "G1 X" << p.x << " Y" << p.y << "\n";
    }

    out << "M5 ; Laser OFF\n";
    out.close();
}

vector<Point> parsePathData(const string& d) {
    vector<Point> points;
    stringstream ss(d);
    char cmd;
    double x, y;
    Point currentPoint = {0, 0};
    while (ss >> cmd) {
        if (cmd == 'M' || cmd == 'L') {
            ss >> x;
            if (ss.peek() == ',') ss.ignore();
            ss >> y;
            currentPoint = {x, y};
            points.push_back(currentPoint);
        } else if (cmd == 'C') {
            Point cp1, cp2, end;
            ss >> cp1.x;
            if (ss.peek() == ',') ss.ignore();
            ss >> cp1.y;

            ss >> cp2.x;
            if (ss.peek() == ',') ss.ignore();
            ss >> cp2.y;

            ss >> end.x;
            if (ss.peek() == ',') ss.ignore();
            ss >> end.y;

            vector<Point> bezierPoints = flattenCubicBezier(currentPoint, cp1, cp2, end, 20);
            // Avoid duplicating starting point
            bezierPoints.erase(bezierPoints.begin());
            points.insert(points.end(), bezierPoints.begin(), bezierPoints.end());
            currentPoint = end;
        }
        // TODO: handle other commands like Q, Z, etc.
    }
    return points;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " input.svg output.gcode\n";
        return 1;
    }

    string inputFile = argv[1];
    string outputFile = argv[2];

    XMLDocument doc;
    if (doc.LoadFile(inputFile.c_str()) != XML_SUCCESS) {
        cerr << "Failed to load SVG file.\n";
        return 1;
    }

    XMLElement* svg = doc.FirstChildElement("svg");
    if (!svg) {
        cerr << "No <svg> element found.\n";
        return 1;
    }

    vector<Point> allPoints;

    for (XMLElement* path = svg->FirstChildElement("path"); path; path = path->NextSiblingElement("path")) {
        const char* d = path->Attribute("d");
        if (d) {
            vector<Point> points = parsePathData(d);
            allPoints.insert(allPoints.end(), points.begin(), points.end());
        }
    }

    if (allPoints.empty()) {
        cerr << "No path data found.\n";
        return 1;
    }

    writeGCode(allPoints, outputFile);

    cout << "G-code written to " << outputFile << endl;
    return 0;
}
