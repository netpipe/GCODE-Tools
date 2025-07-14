#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>
#include <limits>

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 normal;
    Vec3 v1, v2, v3;
};

// Detect file format
bool isBinarySTL(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    char header[80];
    file.read(header, 80);

    uint32_t num_triangles = 0;
    file.read(reinterpret_cast<char*>(&num_triangles), sizeof(num_triangles));

    std::streampos expected_size = 80 + 4 + (std::streampos)num_triangles * 50;
    file.seekg(0, std::ios::end);

    return file.tellg() == expected_size;
}

std::vector<Triangle> readBinarySTL(const std::string& filename) {
    std::vector<Triangle> triangles;
    std::ifstream file(filename, std::ios::binary);

    file.ignore(80);
    uint32_t num_triangles = 0;
    file.read(reinterpret_cast<char*>(&num_triangles), sizeof(num_triangles));

    for (uint32_t i = 0; i < num_triangles; ++i) {
        Triangle tri;
        file.read(reinterpret_cast<char*>(&tri.normal), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v1), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v2), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v3), sizeof(Vec3));
        file.ignore(2);
        triangles.push_back(tri);
    }

    return triangles;
}

std::vector<Triangle> readASCIISTL(const std::string& filename) {
    std::vector<Triangle> triangles;
    std::ifstream file(filename);
    std::string line;
    Triangle tri;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string word;
        iss >> word;
        if (word == "facet") {
            iss >> word;
            iss >> tri.normal.x >> tri.normal.y >> tri.normal.z;
        } else if (word == "vertex") {
            Vec3 v;
            iss >> v.x >> v.y >> v.z;
            if (tri.v1.x == 0 && tri.v1.y == 0 && tri.v1.z == 0)
                tri.v1 = v;
            else if (tri.v2.x == 0 && tri.v2.y == 0 && tri.v2.z == 0)
                tri.v2 = v;
            else {
                tri.v3 = v;
                triangles.push_back(tri);
                tri = Triangle();
            }
        }
    }
    return triangles;
}

std::vector<Triangle> readSTL(const std::string& filename) {
    if (isBinarySTL(filename)) {
        std::cout << "Detected Binary STL\n";
        return readBinarySTL(filename);
    } else {
        std::cout << "Detected ASCII STL\n";
        return readASCIISTL(filename);
    }
}

// Slicing helpers
Vec3 interpolate(const Vec3& a, const Vec3& b, float z) {
    float t = (z - a.z) / (b.z - a.z);
    return {
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        z
    };
}

std::vector<std::pair<Vec3, Vec3>> sliceAtZ(const std::vector<Triangle>& tris, float z) {
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
    out << "G28 ; home all axes\n";
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
    float layer_height = std::stof(argv[3]);

    auto triangles = readSTL(input_stl);
    std::cout << "Read " << triangles.size() << " triangles.\n";

    // Compute Z bounds
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& tri : triangles) {
        Vec3 verts[3] = {tri.v1, tri.v2, tri.v3};
        for (const auto& v : verts) {
            if (v.z < minZ) minZ = v.z;
            if (v.z > maxZ) maxZ = v.z;
        }
    }

    std::cout << "Z range: " << minZ << " to " << maxZ << "\n";

    std::vector<std::vector<std::pair<Vec3, Vec3>>> all_layers;

    for (float z = minZ; z <= maxZ; z += layer_height) {
        auto segments = sliceAtZ(triangles, z);
        std::cout << "Layer Z=" << z << " : " << segments.size() << " segments\n";
        all_layers.push_back(segments);
    }

    writeGCode(output_gcode, all_layers);

    std::cout << "G-code written to " << output_gcode << "\n";

    return 0;
}
