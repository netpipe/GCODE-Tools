#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

struct Vec3 { float x, y, z; };
struct Triangle { Vec3 v0, v1, v2; };

std::vector<std::vector<Vec3>> loadDXF(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::vector<Vec3>> polylines;
    std::vector<Vec3> current;
    std::string line;
    while (std::getline(file, line)) {
        if (line == "POLYLINE") {
            current.clear();
        } else if (line == "VERTEX") {
            float x = 0, y = 0, z = 0;
            while (std::getline(file, line)) {
                if (line == "10") { std::getline(file, line); x = std::stof(line); }
                else if (line == "20") { std::getline(file, line); y = std::stof(line); }
                else if (line == "30") { std::getline(file, line); z = std::stof(line); }
                else if (line == "SEQEND") break;
            }
            current.push_back({x, y, z});
        } else if (line == "SEQEND") {
            if (!current.empty()) polylines.push_back(current);
        }
    }
    return polylines;
}

void saveSTL(const std::string& filename, const std::vector<std::vector<Vec3>>& polylines) {
    std::ofstream file(filename);
    file << "solid dxf_export\n";
    for (const auto& poly : polylines) {
        for (size_t i = 1; i + 1 < poly.size(); ++i) {
            file << "facet normal 0 0 1\nouter loop\n";
            file << "vertex " << poly[0].x << " " << poly[0].y << " " << poly[0].z << "\n";
            file << "vertex " << poly[i].x << " " << poly[i].y << " " << poly[i].z << "\n";
            file << "vertex " << poly[i + 1].x << " " << poly[i + 1].y << " " << poly[i + 1].z << "\n";
            file << "endloop\nendfacet\n";
        }
    }
    file << "endsolid\n";
}

std::vector<Triangle> loadSTL(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<Triangle> tris;
    std::string line;
    Vec3 v[3];
    int vi = 0;
    while (std::getline(file, line)) {
        if (line.find("vertex") != std::string::npos) {
            std::istringstream iss(line);
            std::string dummy;
            iss >> dummy >> v[vi].x >> v[vi].y >> v[vi].z;
            if (++vi == 3) {
                tris.push_back({v[0], v[1], v[2]});
                vi = 0;
            }
        }
    }
    return tris;
}

void saveDXF(const std::string& filename, const std::vector<Triangle>& tris) {
    std::ofstream file(filename);
    file << "0\nSECTION\n2\nENTITIES\n";
    for (const auto& tri : tris) {
        file << "0\nPOLYLINE\n8\nSTL_EXPORT\n";
        auto write_vertex = [&](const Vec3& v) {
            file << "0\nVERTEX\n10\n" << v.x << "\n20\n" << v.y << "\n30\n" << v.z << "\n";
        };
        write_vertex(tri.v0);
        write_vertex(tri.v1);
        write_vertex(tri.v2);
        write_vertex(tri.v0);  // close the triangle
        file << "0\nSEQEND\n";
    }
    file << "0\nENDSEC\n0\nEOF\n";
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cout << "Usage:\n";
        std::cout << "  dxf_stl_converter dxf2stl input.dxf output.stl\n";
        std::cout << "  dxf_stl_converter stl2dxf input.stl output.dxf\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string input = argv[2];
    std::string output = argv[3];

    if (mode == "dxf2stl") {
        auto polylines = loadDXF(input);
        saveSTL(output, polylines);
        std::cout << "DXF to STL conversion complete.\n";
    } else if (mode == "stl2dxf") {
        auto tris = loadSTL(input);
        saveDXF(output, tris);
        std::cout << "STL to DXF conversion complete.\n";
    } else {
        std::cerr << "Unknown mode: " << mode << "\n";
        return 2;
    }

    return 0;
}
