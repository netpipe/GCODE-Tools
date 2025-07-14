#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>

//testing still
//https://github.com/bomeara/STLtoGCODE loosly based on this one

struct Vec3 {
    double x, y, z;
};

struct Triangle {
    Vec3 normal;
    Vec3 v1, v2, v3;
};

std::vector<Triangle> readSTL(const std::string& filename) {
    std::vector<Triangle> triangles;
    std::ifstream file(filename);
    std::string line;

    if (!file) {
        std::cerr << "Cannot open STL file.\n";
        return triangles;
    }

    Triangle tri;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string word;
        iss >> word;
        if (word == "facet") {
            iss >> word; // "normal"
            iss >> tri.normal.x >> tri.normal.y >> tri.normal.z;
        } else if (word == "vertex") {
            double x, y, z;
            iss >> x >> y >> z;
            if (tri.v1.x == 0 && tri.v1.y == 0 && tri.v1.z == 0)
                tri.v1 = {x, y, z};
            else if (tri.v2.x == 0 && tri.v2.y == 0 && tri.v2.z == 0)
                tri.v2 = {x, y, z};
            else {
                tri.v3 = {x, y, z};
                triangles.push_back(tri);
                tri = Triangle(); // reset
            }
        }
    }
    return triangles;
}

// Linear interpolation helper
Vec3 interpolate(const Vec3& a, const Vec3& b, double z) {
    double t = (z - a.z) / (b.z - a.z);
    return {
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        z
    };
}

// Simple slicing: intersect triangles with Z-plane, get line segments
std::vector<std::pair<Vec3, Vec3>> sliceAtZ(const std::vector<Triangle>& tris, double z) {
    std::vector<std::pair<Vec3, Vec3>> segments;

    for (const auto& tri : tris) {
        std::vector<Vec3> points;

        Vec3 verts[3] = {tri.v1, tri.v2, tri.v3};

        for (int i = 0; i < 3; ++i) {
            Vec3 a = verts[i];
            Vec3 b = verts[(i + 1) % 3];

            if ((a.z < z && b.z > z) || (a.z > z && b.z < z)) {
                points.push_back(interpolate(a, b, z));
            }
        }

        if (points.size() == 2) {
            segments.emplace_back(points[0], points[1]);
        }
    }
    return segments;
}

void writeGCode(const std::string& filename, const std::vector<std::vector<std::pair<Vec3, Vec3>>>& layers) {
    std::ofstream out(filename);

    out << "G21 ; set units to millimeters\n";
    out << "G90 ; absolute positioning\n";
    out << "G28 ; home\n";
    out << "G1 F1200\n";

    for (size_t i = 0; i < layers.size(); ++i) {
        out << "; Layer " << i << "\n";
        for (const auto& seg : layers[i]) {
            out << "G0 X" << seg.first.x << " Y" << seg.first.y << " Z" << seg.first.z << "\n";
            out << "G1 X" << seg.second.x << " Y" << seg.second.y << "\n";
        }
    }

    out << "M84 ; disable motors\n";
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cout << "Usage: ./stl2gcode input.stl output.gcode layer_height\n";
        return 1;
    }

    std::string input_stl = argv[1];
    std::string output_gcode = argv[2];
    double layer_height = std::stod(argv[3]);

    auto triangles = readSTL(input_stl);
    std::cout << "Read " << triangles.size() << " triangles.\n";

    double minZ = 0.0, maxZ = 50.0; // Adjust as needed or compute from mesh
    std::vector<std::vector<std::pair<Vec3, Vec3>>> all_layers;

    for (double z = minZ; z <= maxZ; z += layer_height) {
        auto segments = sliceAtZ(triangles, z);
        std::cout << "Layer Z=" << z << " : " << segments.size() << " segments\n";
        all_layers.push_back(segments);
    }

    writeGCode(output_gcode, all_layers);

    std::cout << "G-code written to " << output_gcode << "\n";

    return 0;
}
