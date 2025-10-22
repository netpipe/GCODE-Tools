#include <math.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
#include <cstdint>

using namespace std;

struct Vec3 { double x,y,z; };
struct Tri { Vec3 v0, v1, v2; };

static inline bool read_file_to_string(const string &path, string &out) {
    ifstream ifs(path, ios::binary);
    if(!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

bool is_likely_ascii_stl(const string &buf) {
    if (buf.size() < 6) return false;
    string head = buf.substr(0,5);
    if (head == "solid") {
        // if "facet" exists later we'll assume ascii
        if (buf.find("facet") != string::npos) return true;
    }
    // Heuristic: binary stl often has non-text bytes early
    for(size_t i=0;i<min<size_t>(200, buf.size());++i){
        unsigned char c = buf[i];
        if ((c < 9 || (c>13 && c<32)) && c != 0) return false; // weird binary
    }
    return false;
}

bool parse_ascii_stl(const string &s, vector<Tri> &out) {
    istringstream ss(s);
    string token;
    Vec3 last_normal{0,0,0};
    while (ss >> token) {
        if (token == "facet") {
            ss >> token; // should be "normal"
            if (token != "normal") {
                // skip line
                string rest; getline(ss, rest);
                continue;
            }
            double nx,ny,nz;
            ss >> nx >> ny >> nz;
            last_normal = {nx,ny,nz};
            // expect "outer loop"
            ss >> token; // outer
            ss >> token; // loop
            Vec3 v[3];
            for(int i=0;i<3;i++){
                ss >> token; // "vertex"
                if (token != "vertex") {
                    // try to recover
                    ss >> token;
                }
                double vx,vy,vz;
                ss >> vx >> vy >> vz;
                v[i] = {vx,vy,vz};
            }

            Tri t; t.v0 = v[0]; t.v1 = v[1]; t.v2 = v[2];
            out.push_back(t);
        } else {
            // skip token
        }
    }
    return !out.empty();
}

static float read_float_le(const unsigned char* p) {
    uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
    float f;
    memcpy(&f, &v, sizeof(f));
    return f;
}

bool parse_binary_stl(const string &buf, vector<Tri> &out) {
    if (buf.size() < 84) return false;
    const unsigned char* data = (const unsigned char*)buf.data();
    uint32_t tri_count = (uint32_t)data[80] | ((uint32_t)data[81]<<8) | ((uint32_t)data[82]<<16) | ((uint32_t)data[83]<<24);
    size_t expected = 84 + size_t(tri_count) * 50;
    if (buf.size() < expected) {
        // maybe truncated but attempt to parse what we can
    }
    size_t offset = 84;
    for (uint32_t i=0; i<tri_count && offset + 50 <= buf.size(); ++i) {
        // normal floats
        float nx = read_float_le(data + offset + 0);
        float ny = read_float_le(data + offset + 4);
        float nz = read_float_le(data + offset + 8);
        float v0x = read_float_le(data + offset + 12);
        float v0y = read_float_le(data + offset + 16);
        float v0z = read_float_le(data + offset + 20);
        float v1x = read_float_le(data + offset + 24);
        float v1y = read_float_le(data + offset + 28);
        float v1z = read_float_le(data + offset + 32);
        float v2x = read_float_le(data + offset + 36);
        float v2y = read_float_le(data + offset + 40);
        float v2z = read_float_le(data + offset + 44);
        Tri t;
        t.v0 = {double(v0x), double(v0y), double(v0z)};
        t.v1 = {double(v1x), double(v1y), double(v1z)};
        t.v2 = {double(v2x), double(v2y), double(v2z)};
        out.push_back(t);
        offset += 50;
    }
    return !out.empty();
}

bool load_stl(const string &path, vector<Tri> &out) {
    string buf;
    if (!read_file_to_string(path, buf)) return false;
    if (is_likely_ascii_stl(buf)) {
        if (parse_ascii_stl(buf, out)) return true;
    }
    // fallback to binary
    if (parse_binary_stl(buf, out)) return true;
    // final try ASCII (some binary detection failed)
    if (parse_ascii_stl(buf, out)) return true;
    return false;
}

static inline Vec3 operator-(const Vec3 &a, const Vec3 &b){ return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline Vec3 operator+(const Vec3 &a, const Vec3 &b){ return {a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline Vec3 operator*(const Vec3 &a, double s){ return {a.x*s, a.y*s, a.z*s}; }
static inline double dot(const Vec3 &a, const Vec3 &b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3 cross(const Vec3 &a, const Vec3 &b){ return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }


bool point_in_triangle_barycentric(double px, double py, const Tri &t, double &out_z) {
    // project to XY
    double x0 = t.v0.x, y0 = t.v0.y;
    double x1 = t.v1.x, y1 = t.v1.y;
    double x2 = t.v2.x, y2 = t.v2.y;
    // Compute barycentric coordinates in 2D
    double denom = (y1 - y2)*(x0 - x2) + (x2 - x1)*(y0 - y2);
    if (fabs(denom) < 1e-12) return false; // degenerate/projected-degenerate
    double a = ((y1 - y2)*(px - x2) + (x2 - x1)*(py - y2)) / denom;
    double b = ((y2 - y0)*(px - x2) + (x0 - x2)*(py - y2)) / denom;
    double c = 1.0 - a - b;
    // inside when 0<=a,b,c<=1 (allow small epsilon)
    const double eps = -1e-9;
    if (a+eps < 0 || b+eps < 0 || c+eps < 0) return false;
    // compute z with barycentric interpolation
    out_z = a * t.v0.z + b * t.v1.z + c * t.v2.z;
    return true;
}

// ----------------------------- Heightmap creation -----------------------------
struct Bounds {
    double minx, miny, minz;
    double maxx, maxy, maxz;
    void reset(){ minx = miny = minz = 1e30; maxx = maxy = maxz = -1e30; }
    void include(const Vec3 &v){ minx=min(minx,v.x); miny=min(miny,v.y); minz=min(minz,v.z); maxx=max(maxx,v.x); maxy=max(maxy,v.y); maxz=max(maxz,v.z); }
};

bool compute_bounds(const vector<Tri> &tris, Bounds &b) {
    b.reset();
    if (tris.empty()) return false;
    for (auto &t : tris) {
        b.include(t.v0);
        b.include(t.v1);
        b.include(t.v2);
    }
    return true;
}

// Make heightmap: width x height.
bool make_heightmap(const vector<Tri> &tris, int width, int height, vector<uint16_t> &out, Bounds &usedBounds, double pad_ratio = 0.02) {
    if (tris.empty() || width<=0 || height<=0) return false;
    Bounds b;
    compute_bounds(tris, b);

    double padx = (b.maxx - b.minx) * pad_ratio;
    double pady = (b.maxy - b.miny) * pad_ratio;
    b.minx -= padx; b.maxx += padx;
    b.miny -= pady; b.maxy += pady;

    double grid_w = b.maxx - b.minx;
    double grid_h = b.maxy - b.miny;
    if (grid_w <= 0 || grid_h <= 0) return false;

    // We'll compute max Z per pixel (topmost surface). Initialize with very low
    vector<double> zbuf((size_t)width * height, numeric_limits<double>::lowest());

    // For speed: for each triangle compute its projected pixel bbox and iterate pixels
    for (const Tri &t : tris) {
        // triangle bbox in XY
        double tri_minx = min({t.v0.x, t.v1.x, t.v2.x});
        double tri_maxx = max({t.v0.x, t.v1.x, t.v2.x});
        double tri_miny = min({t.v0.y, t.v1.y, t.v2.y});
        double tri_maxy = max({t.v0.y, t.v1.y, t.v2.y});
        // convert to pixel coordinates [0,width-1], [0,height-1]
        int x0 = (int)floor((tri_minx - b.minx) / grid_w * (width-1));
        int x1 = (int)ceil ((tri_maxx - b.minx) / grid_w * (width-1));
        int y0 = (int)floor((tri_miny - b.miny) / grid_h * (height-1));
        int y1 = (int)ceil ((tri_maxy - b.miny) / grid_h * (height-1));
        x0 = max(0, min(width-1, x0));
        x1 = max(0, min(width-1, x1));
        y0 = max(0, min(height-1, y0));
        y1 = max(0, min(height-1, y1));
        if (x1 < x0 || y1 < y0) continue;

        // For each pixel in bbox test barycentric inside and compute z
        for (int py = y0; py <= y1; ++py) {
            // compute world y for pixel center
            double yw = b.miny + (double)py / (double)(height-1) * grid_h;
            for (int px = x0; px <= x1; ++px) {
                double xw = b.minx + (double)px / (double)(width-1) * grid_w;
                double zval;
                if (point_in_triangle_barycentric(xw, yw, t, zval)) {
                    size_t idx = (size_t)py * width + px;
                    if (zval > zbuf[idx]) zbuf[idx] = zval;
                }
            }
        }
    }

    // After filling, find valid min/max z (ignore cells still at lowest)
    double zmin = 1e30, zmax = -1e30;
    for (double vz : zbuf) {
        if (vz == numeric_limits<double>::lowest()) continue;
        zmin = min(zmin, vz);
        zmax = max(zmax, vz);
    }
    // If no triangles hit any pixels, bail
    if (zmax < zmin) return false;

    // Normalize to 16-bit (0..65535). We will map zmin->65535 (white/high) and zmax->0 (dark)
    // because many lasers interpret higher grayscale as more power; invert if you prefer.
    out.assign((size_t)width*height, 0);
    for (int y=0;y<height;++y){
        for (int x=0;x<width;++x){
            size_t idx = (size_t)y*width + x;
            double vz = zbuf[idx];
            uint16_t val = 0;
            if (vz == numeric_limits<double>::lowest()) {
                // no geometry -> set to background (max or min?) choose white (no engraving)
                val = 65535; // white (no depth)
            } else {
                // normalized inverted: zmin -> 65535, zmax -> 0
                double tnorm = (vz - zmin) / (zmax - zmin);
                if (tnorm < 0) tnorm = 0;
                if (tnorm > 1) tnorm = 1;
                double inv = 1.0 - tnorm;
                uint32_t q = (uint32_t)round(inv * 65535.0);
                if (q > 65535) q = 65535;
                val = (uint16_t)q;
            }
            out[idx] = val;
        }
    }
    usedBounds = b;
    return true;
}

bool write_pgm_16be(const string &path, int width, int height, const vector<uint16_t> &data) {
    ofstream ofs(path, ios::binary);
    if(!ofs) return false;
    // PGM header: P5\n<width> <height>\n<maxval>\n (maxval up to 65535)
    ofs << "P5\n" << width << " " << height << "\n" << 65535 << "\n";
    // PGM requires big-endian 2 bytes per sample
    for (size_t i=0;i<data.size();++i) {
        uint16_t v = data[i];
        unsigned char hi = (unsigned char)((v >> 8) & 0xFF);
        unsigned char lo = (unsigned char)(v & 0xFF);
        ofs.put(hi);
        ofs.put(lo);
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " input.stl output.pgm [width] [height]\n";
        cerr << "Example: " << argv[0] << " model.stl heightmap.pgm 2048 2048\n";
        return 1;
    }
    string inpath = argv[1];
    string outpath = argv[2];
    int width = 1024, height = 1024;
    if (argc >= 4) width = atoi(argv[3]);
    if (argc >= 5) height = atoi(argv[4]);
    if (width <= 0) width = 1024;
    if (height <= 0) height = 1024;

    vector<Tri> tris;
    cerr << "Loading STL '" << inpath << "' ...\n";
    if (!load_stl(inpath, tris)) {
        cerr << "Failed to load STL or no triangles found.\n";
        return 2;
    }
    cerr << "Loaded " << tris.size() << " triangles.\n";

    vector<uint16_t> heightmap;
    Bounds usedB;
    cerr << "Rasterizing to " << width << "x" << height << " ... (may take a while for very large resolutions)\n";
    bool ok = make_heightmap(tris, width, height, heightmap, usedB, 0.01);
    if (!ok) {
        cerr << "Failed to rasterize heightmap\n";
        return 3;
    }
    cerr << "Bounds used: X[" << usedB.minx << ", " << usedB.maxx << "] Y[" << usedB.miny << ", " << usedB.maxy << "] Z[" << usedB.minz << ", " << usedB.maxz << "]\n";

    cerr << "Writing PGM '" << outpath << "' ...\n";
    if (!write_pgm_16be(outpath, width, height, heightmap)) {
        cerr << "Failed to write PGM\n";
        return 4;
    }
    cerr << "Done. Output is 16-bit PGM. Convert to PNG with ImageMagick if needed:\n";
    cerr << "  magick " << outpath << " output-16bit.png\n";
    cerr << "Or to 8-bit: magick " << outpath << " -depth 8 output-8bit.png\n";
    return 0;
}
