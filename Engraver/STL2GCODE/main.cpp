#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 normal;
    Vec3 v1, v2, v3;
};

// Detect file format: return true if binary, false if ASCII
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
    if (!file) {
        std::cerr << "Cannot open binary STL file.\n";
        return triangles;
    }

    file.ignore(80); // skip header

    uint32_t num_triangles = 0;
    file.read(reinterpret_cast<char*>(&num_triangles), sizeof(num_triangles));

    for (uint32_t i = 0; i < num_triangles; ++i) {
        Triangle tri;
        file.read(reinterpret_cast<char*>(&tri.normal), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v1), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v2), sizeof(Vec3));
        file.read(reinterpret_cast<char*>(&tri.v3), sizeof(Vec3));
        file.ignore(2); // attribute byte count
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

// Slicing & G-code functions omitted here for brevity (same as before)
// You can reuse sliceAtZ() and writeGCode() from the earlier version.

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

    // Call slicing & G-code writing functions here
    // Use sliceAtZ() and writeGCode() as from the previous response

    return 0;
}
