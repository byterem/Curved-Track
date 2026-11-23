// ICS 3204: Formula 1 race animation
// Optional demo supplied from the pasted Claude chat; exceeds the course 2D scope.
// Track geometry follows Person 1's anchors/handles. F1 models replace the team's
// side-view body/wheels; midpoint circles here draw scenery and helmet slices.
// Person 4 integration: clipping.h and local build/output verification.
//
// Pure C++17, no OpenGL, no external libraries. Writes f1_race.gif (the animation)
// and f1_frame.bmp (one still frame).
//
// Ideas used:
//   * Perspective projection: a pinhole camera turns world (X, Y, height) into pixels.
//   * Each F1 car is a 3D model built from stacked top-view slices (like a loaf of
//     bread). Every slice is drawn on the ground plane at its own height, so the
//     car looks solid from any direction as it drives round the bends.
//   * Painter's algorithm: far objects are drawn first, near ones last.
//   * Supersampling anti-aliasing: render at 3x size, then average down.
//   * Animation: every car moves along the Bezier track by arc length, frame by frame.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "clipping.h"

// ---------------------------------------------------------------------------
// Basic types and helpers
// ---------------------------------------------------------------------------
struct Color { unsigned char r, g, b; };
struct Point { int x, y; };
struct Vec2 { double x, y; };

const double PI = 3.14159265358979323846;

inline double clamp01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

Color mix(Color a, Color b, double t) {
    t = clamp01(t);
    return {static_cast<unsigned char>(std::lround(a.r + (b.r - a.r) * t)),
            static_cast<unsigned char>(std::lround(a.g + (b.g - a.g) * t)),
            static_cast<unsigned char>(std::lround(a.b + (b.b - a.b) * t))};
}

Color shade(Color c, double f) {
    auto cl = [](double v) { return static_cast<unsigned char>(std::min(255.0, std::max(0.0, v))); };
    return {cl(c.r * f), cl(c.g * f), cl(c.b * f)};
}

unsigned hash2(int x, int y) {
    unsigned h = static_cast<unsigned>(x) * 374761393u + static_cast<unsigned>(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
double rand01(unsigned h) { return (h & 0xFFFF) / 65535.0; }

// ---------------------------------------------------------------------------
// Canvas (shared by the team) plus blend, downsample and BMP output
// ---------------------------------------------------------------------------
class Canvas {
public:
    Canvas(int width, int height, Color background)
        : width_(width), height_(height), pixels_(width * height, background) {}

    int width() const { return width_; }
    int height() const { return height_; }

    void plot(int x, int y, Color color) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) pixels_[y * width_ + x] = color;
    }
    void blend(int x, int y, Color color, double alpha) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            Color& p = pixels_[y * width_ + x];
            p = mix(p, color, alpha);
        }
    }
    Color get(int x, int y) const { return pixels_[y * width_ + x]; }

    Canvas downsample(int f) const {
        Canvas out(width_ / f, height_ / f, {0, 0, 0});
        for (int y = 0; y < out.height_; ++y)
            for (int x = 0; x < out.width_; ++x) {
                int r = 0, g = 0, b = 0;
                for (int j = 0; j < f; ++j)
                    for (int i = 0; i < f; ++i) {
                        const Color& c = pixels_[(y * f + j) * width_ + (x * f + i)];
                        r += c.r; g += c.g; b += c.b;
                    }
                int n = f * f;
                out.pixels_[y * out.width_ + x] = {static_cast<unsigned char>(r / n),
                                                    static_cast<unsigned char>(g / n),
                                                    static_cast<unsigned char>(b / n)};
            }
        return out;
    }

    void saveBMP(const char* filename) const {
        std::ofstream out(filename, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot create output image");
        int rowBytes = (width_ * 3 + 3) & ~3;
        int imageSize = rowBytes * height_;
        unsigned char header[54] = {0};
        auto put32 = [&](int pos, int v) {
            for (int i = 0; i < 4; ++i) header[pos + i] = static_cast<unsigned char>((v >> (8 * i)) & 0xFF);
        };
        header[0] = 'B'; header[1] = 'M';
        put32(2, 54 + imageSize); put32(10, 54); put32(14, 40);
        put32(18, width_); put32(22, height_);
        header[26] = 1; header[28] = 24; put32(34, imageSize);
        out.write(reinterpret_cast<char*>(header), 54);
        std::vector<unsigned char> row(rowBytes, 0);
        for (int y = height_ - 1; y >= 0; --y) {
            for (int x = 0; x < width_; ++x) {
                const Color& c = pixels_[y * width_ + x];
                row[x * 3] = c.b; row[x * 3 + 1] = c.g; row[x * 3 + 2] = c.r;
            }
            out.write(reinterpret_cast<char*>(row.data()), rowBytes);
        }
    }
    // The picture as RGB bytes, bottom row first (the order OpenGL expects).
    std::vector<unsigned char> toRGB() const {
        std::vector<unsigned char> out;
        out.reserve(static_cast<size_t>(width_) * height_ * 3);
        for (int y = height_ - 1; y >= 0; --y)
            for (int x = 0; x < width_; ++x) {
                const Color& c = pixels_[y * width_ + x];
                out.push_back(c.r); out.push_back(c.g); out.push_back(c.b);
            }
        return out;
    }
private:
    int width_, height_;
    std::vector<Color> pixels_;
};

// ---------------------------------------------------------------------------
// Shared drawing algorithms
// ---------------------------------------------------------------------------

// DDA line. Person 4: every line is clipped to the canvas first (Cohen-Sutherland).
void drawLineDDA(Canvas& canvas, Point a, Point b, Color color) {
    ClipWindow screen{0, 0, double(canvas.width() - 1), double(canvas.height() - 1)};
    double cx0 = a.x, cy0 = a.y, cx1 = b.x, cy1 = b.y;
    if (!cohenSutherlandClip(cx0, cy0, cx1, cy1, screen)) return;
    a = {static_cast<int>(std::lround(cx0)), static_cast<int>(std::lround(cy0))};
    b = {static_cast<int>(std::lround(cx1)), static_cast<int>(std::lround(cy1))};
    int dx = b.x - a.x, dy = b.y - a.y;
    int steps = std::max(std::abs(dx), std::abs(dy));
    double x = a.x, y = a.y;
    double xStep = steps ? static_cast<double>(dx) / steps : 0;
    double yStep = steps ? static_cast<double>(dy) / steps : 0;
    for (int i = 0; i <= steps; ++i) {
        canvas.plot(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), color);
        x += xStep; y += yStep;
    }
}

// Scanline polygon fill; alpha < 1 makes it see-through (used for shadows).
void fillPolygon(Canvas& canvas, const std::vector<Point>& vertices, Color color, double alpha = 1.0) {
    if (vertices.size() < 3) return;
    int yMin = vertices[0].y, yMax = vertices[0].y;
    for (Point p : vertices) { yMin = std::min(yMin, p.y); yMax = std::max(yMax, p.y); }
    yMin = std::max(yMin, 0);
    yMax = std::min(yMax, canvas.height() - 1);
    std::vector<double> hits;
    for (int y = yMin; y <= yMax; ++y) {
        hits.clear();
        for (size_t i = 0; i < vertices.size(); ++i) {
            Point a = vertices[i], b = vertices[(i + 1) % vertices.size()];
            if ((a.y <= y && y < b.y) || (b.y <= y && y < a.y))
                hits.push_back(a.x + static_cast<double>(y - a.y) * (b.x - a.x) / (b.y - a.y));
        }
        std::sort(hits.begin(), hits.end());
        for (size_t i = 0; i + 1 < hits.size(); i += 2)
            for (int x = static_cast<int>(std::ceil(hits[i])); x <= static_cast<int>(std::floor(hits[i + 1])); ++x) {
                if (alpha >= 1.0) canvas.plot(x, y, color);
                else canvas.blend(x, y, color, alpha);
            }
    }
}

void outlinePolygon(Canvas& canvas, const std::vector<Point>& v, Color outline) {
    for (size_t i = 0; i < v.size(); ++i) drawLineDDA(canvas, v[i], v[(i + 1) % v.size()], outline);
}

// Filled disc using the Midpoint Circle Algorithm (Person 3). It records the widest x
// of every row, then fills each row once, so see-through discs do not double up.
void fillCircleMidpoint(Canvas& canvas, int cx, int cy, int radius, Color color, double alpha = 1.0) {
    if (radius < 0) return;
    std::vector<int> half(radius + 1, 0);
    int x = 0, y = radius, p = 1 - radius;
    while (x <= y) {
        half[y] = std::max(half[y], x);
        half[x] = std::max(half[x], y);
        x++;
        if (p < 0) p += 2 * x + 1;
        else { y--; p += 2 * (x - y) + 1; }
    }
    for (int dy = -radius; dy <= radius; ++dy) {
        int w = half[std::abs(dy)];
        for (int px = cx - w; px <= cx + w; ++px) {
            if (alpha >= 1.0) canvas.plot(px, cy + dy, color);
            else canvas.blend(px, cy + dy, color, alpha);
        }
    }
}

// ---------------------------------------------------------------------------
// Camera: perspective projection
// ---------------------------------------------------------------------------
const int SS = 3;                                     // supersampling factor
const int OUT_W = 800, OUT_H = 450;                   // size of the saved frames
const int W = OUT_W * SS, H = OUT_H * SS;             // size we really draw at
const double PX = W / 1280.0;                         // design pixels -> real pixels

double CAM_X = 450, CAM_Y = 790;                      // camera ground position (moves in play mode)
double CAM_H = 330;                                  // camera height in the world (changes with the view)
double PITCH = 0.50;                                  // look-down angle in radians (changes with the view)
const double FOCAL = 610.0 * PX;
const double SCREEN_CX = W / 2.0, SCREEN_CY = H * 0.595;
double CP = std::cos(PITCH), SP = std::sin(PITCH);   // cos and sin of PITCH, kept in step with it
double HORIZON_Y = SCREEN_CY - FOCAL * std::tan(PITCH);
// The sun stays at its broadcast-view height, so it stays in the sky in every view.
const double SUN_Y = SCREEN_CY - FOCAL * std::tan(0.50) - 55.0 * PX;

const Color HAZE{196, 214, 232};

struct Proj { double x, y, z; };

// World point (X, Y on the ground, h = height above ground) -> screen pixel.
// Points closer to the camera than this (in depth) are behind or at the lens and are not drawn.
const double NEAR_Z = 30.0;

Proj project(double X, double Y, double h) {
    double rx = X - CAM_X, fwd = CAM_Y - Y, up = h - CAM_H;
    double z = fwd * CP - up * SP;
    double yc = fwd * SP + up * CP;
    return {SCREEN_CX + FOCAL * rx / z, SCREEN_CY - FOCAL * yc / z, z};
}

Point scr(double X, double Y, double h = 0) {
    Proj p = project(X, Y, h);
    return {static_cast<int>(std::lround(p.x)), static_cast<int>(std::lround(p.y))};
}

Color fogged(Color c, double z) {
    return mix(c, HAZE, 0.88 * (1.0 - std::exp(-z / 4500.0)));
}

// ---------------------------------------------------------------------------
// Track (Person 1): four cubic Bezier segments forming a closed loop
// ---------------------------------------------------------------------------
const int SEGMENT_COUNT = 4;
const int TRACK_N = 480;
const int SAMPLES_PER_SEG = TRACK_N / SEGMENT_COUNT;
const double ROAD_HALF = 70.0;

const Point TRACK_ANCHORS[SEGMENT_COUNT] = {{450, 115}, {770, 265}, {480, 478}, {132, 308}};
const Point TRACK_HANDLES[SEGMENT_COUNT] = {{150, -30}, {30, 125}, {-155, 15}, {-15, -145}};

void getControlPoints(int index, Point& p0, Point& p1, Point& p2, Point& p3) {
    int next = (index + 1) % SEGMENT_COUNT;
    p0 = TRACK_ANCHORS[index];
    p3 = TRACK_ANCHORS[next];
    p1 = {p0.x + TRACK_HANDLES[index].x, p0.y + TRACK_HANDLES[index].y};
    p2 = {p3.x - TRACK_HANDLES[next].x, p3.y - TRACK_HANDLES[next].y};
}

struct Frame { double x, y, tx, ty, nx, ny; };   // point, direction, sideways normal
std::vector<Frame> g_frames;
std::vector<double> g_cum;                       // distance along the track at each sample
double g_trackLength = 0;

// Bernstein formula B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3
void buildFrames() {
    g_frames.clear();
    for (int seg = 0; seg < SEGMENT_COUNT; ++seg) {
        Point p0, p1, p2, p3;
        getControlPoints(seg, p0, p1, p2, p3);
        for (int k = 0; k < SAMPLES_PER_SEG; ++k) {
            double t = double(k) / SAMPLES_PER_SEG, u = 1 - t;
            double b0 = u * u * u, b1 = 3 * u * u * t, b2 = 3 * u * t * t, b3 = t * t * t;
            Frame f;
            f.x = b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x;
            f.y = b0 * p0.y + b1 * p1.y + b2 * p2.y + b3 * p3.y;
            double dx = 3 * u * u * (p1.x - p0.x) + 6 * u * t * (p2.x - p1.x) + 3 * t * t * (p3.x - p2.x);
            double dy = 3 * u * u * (p1.y - p0.y) + 6 * u * t * (p2.y - p1.y) + 3 * t * t * (p3.y - p2.y);
            double len = std::sqrt(dx * dx + dy * dy);
            f.tx = dx / len; f.ty = dy / len;
            f.nx = -f.ty; f.ny = f.tx;           // + side is the inside of the loop
            g_frames.push_back(f);
        }
    }
    // Distance along the track, so cars can move at a steady real speed.
    g_cum.assign(TRACK_N + 1, 0.0);
    for (int i = 0; i < TRACK_N; ++i) {
        const Frame& a = g_frames[i];
        const Frame& b = g_frames[(i + 1) % TRACK_N];
        g_cum[i + 1] = g_cum[i] + std::hypot(b.x - a.x, b.y - a.y);
    }
    g_trackLength = g_cum[TRACK_N];
}

// Position and direction at distance u along the track (wraps around).
Frame frameAt(double u) {
    u = std::fmod(u, g_trackLength);
    if (u < 0) u += g_trackLength;
    int i = static_cast<int>(std::upper_bound(g_cum.begin(), g_cum.end(), u) - g_cum.begin()) - 1;
    i = std::max(0, std::min(TRACK_N - 1, i));
    int j = (i + 1) % TRACK_N;
    double f = (u - g_cum[i]) / (g_cum[i + 1] - g_cum[i]);
    Frame r;
    r.x = g_frames[i].x + (g_frames[j].x - g_frames[i].x) * f;
    r.y = g_frames[i].y + (g_frames[j].y - g_frames[i].y) * f;
    double tx = g_frames[i].tx + (g_frames[j].tx - g_frames[i].tx) * f;
    double ty = g_frames[i].ty + (g_frames[j].ty - g_frames[i].ty) * f;
    double len = std::hypot(tx, ty);
    r.tx = tx / len; r.ty = ty / len;
    r.nx = -r.ty; r.ny = r.tx;
    return r;
}

Vec2 offsetPoint(const Frame& f, double d) { return {f.x + f.nx * d, f.y + f.ny * d}; }

double nearestTrack(double X, double Y, int& index) {
    double best = 1e18;
    index = 0;
    for (int i = 0; i < TRACK_N; ++i) {
        double d = std::hypot(X - g_frames[i].x, Y - g_frames[i].y);
        if (d < best) { best = d; index = i; }
    }
    return best;
}

void drawRibbon(Canvas& c, double d0, double d1, double h,
                const std::function<bool(int, Color&)>& style, double alpha = 1.0) {
    for (int i = 0; i < TRACK_N; ++i) {
        Color col;
        if (!style(i, col)) continue;
        int j = (i + 1) % TRACK_N;
        Vec2 a0 = offsetPoint(g_frames[i], d0), a1 = offsetPoint(g_frames[i], d1);
        Vec2 b0 = offsetPoint(g_frames[j], d0), b1 = offsetPoint(g_frames[j], d1);
        double z = project((a0.x + b1.x) / 2, (a0.y + b1.y) / 2, h).z;
        // Skip quads that touch the near plane: their screen points would be wrong.
        if (project(a0.x, a0.y, h).z < NEAR_Z || project(a1.x, a1.y, h).z < NEAR_Z ||
            project(b0.x, b0.y, h).z < NEAR_Z || project(b1.x, b1.y, h).z < NEAR_Z) continue;
        fillPolygon(c, {scr(a0.x, a0.y, h), scr(a1.x, a1.y, h), scr(b1.x, b1.y, h), scr(b0.x, b0.y, h)},
                    fogged(col, z), alpha);
    }
}

const int START_INDEX = 122;   // start/finish line where the track runs towards the camera

void drawTrackSurface(Canvas& c) {
    const Color asphalt{58, 62, 70}, asphalt2{63, 67, 76}, rubber{44, 47, 54};
    const Color red{205, 42, 48}, white{244, 246, 250}, gravel{190, 178, 150};
    for (int side = -1; side <= 1; side += 2) {
        double a = side * 80.0, b = side * 94.0;
        drawRibbon(c, std::min(a, b), std::max(a, b), 0, [&](int, Color& col) { col = gravel; return true; });
        double k0 = side * ROAD_HALF, k1 = side * (ROAD_HALF + 10);
        drawRibbon(c, std::min(k0, k1), std::max(k0, k1), 0,
                   [&](int i, Color& col) { col = ((i / 5) % 2) ? red : white; return true; });
    }
    drawRibbon(c, -ROAD_HALF, ROAD_HALF, 0,
               [&](int i, Color& col) { col = ((i / 12) % 2) ? asphalt : asphalt2; return true; });
    drawRibbon(c, -26, 26, 0, [&](int, Color& col) { col = rubber; return true; }, 0.55);
    for (int side = -1; side <= 1; side += 2) {
        double a = side * (ROAD_HALF - 6), b = side * (ROAD_HALF - 2);
        drawRibbon(c, std::min(a, b), std::max(a, b), 0, [&](int, Color& col) { col = white; return true; });
    }
    drawRibbon(c, -1.6, 1.6, 0, [&](int i, Color& col) { col = {250, 205, 60}; return (i % 14) < 7; });

    // Chequered start/finish line.
    for (int row = 0; row < 2; ++row)
        for (int cell = 0; cell < 10; ++cell) {
            double d0 = -ROAD_HALF + cell * (2 * ROAD_HALF / 10), d1 = d0 + 2 * ROAD_HALF / 10;
            int i0 = START_INDEX + row * 3, i1 = i0 + 3;
            Vec2 a0 = offsetPoint(g_frames[i0], d0), a1 = offsetPoint(g_frames[i0], d1);
            Vec2 b0 = offsetPoint(g_frames[i1], d0), b1 = offsetPoint(g_frames[i1], d1);
            Color col = ((cell + row) % 2) ? Color{20, 22, 28} : white;
            double z = project(a0.x, a0.y, 0).z;
            fillPolygon(c, {scr(a0.x, a0.y), scr(a1.x, a1.y), scr(b1.x, b1.y), scr(b0.x, b0.y)}, fogged(col, z));
        }
}

// ---------------------------------------------------------------------------
// Sky, hills and ground
// ---------------------------------------------------------------------------
void drawCloud(Canvas& c, double x, double y, double s) {
    struct Blob { double dx, dy, r; };
    const Blob blobs[] = {{0, 0, 38}, {42, 7, 30}, {-44, 9, 28}, {18, -18, 30}, {-18, -13, 26}};
    for (const Blob& b : blobs)
        fillCircleMidpoint(c, static_cast<int>(x + b.dx * s), static_cast<int>(y + b.dy * s),
                           static_cast<int>(b.r * s), {255, 255, 255}, 0.5);
}

void drawHillLayer(Canvas& c, double baseY, double amp, int seed, Color color) {
    const int anchors = 7;
    double ax[anchors], ay[anchors];
    for (int i = 0; i < anchors; ++i) {
        ax[i] = i * double(W) / (anchors - 1);
        ay[i] = baseY - amp * (0.3 + 0.7 * rand01(hash2(seed, i)));
    }
    std::vector<Point> poly;
    double handle = double(W) / (anchors - 1) / 2.6;
    for (int i = 0; i + 1 < anchors; ++i)
        for (int k = 0; k < 40; ++k) {
            double t = k / 40.0, u = 1 - t;
            double x = u*u*u*ax[i] + 3*u*u*t*(ax[i] + handle) + 3*u*t*t*(ax[i+1] - handle) + t*t*t*ax[i+1];
            double y = u*u*u*ay[i] + 3*u*u*t*ay[i] + 3*u*t*t*ay[i+1] + t*t*t*ay[i+1];
            poly.push_back({static_cast<int>(x), static_cast<int>(y)});
        }
    poly.push_back({W, static_cast<int>(HORIZON_Y) + 12});
    poly.push_back({0, static_cast<int>(HORIZON_Y) + 12});
    fillPolygon(c, poly, color);
}

void drawSkyAndGround(Canvas& c) {
    const Color skyTop{46, 102, 200}, skyHorizon{188, 215, 240};
    const double sunX = W * 0.78, sunY = SUN_Y;
    const double sigma = 150.0 * PX;

    for (int sy = 0; sy < H; ++sy) {
        double yc = (SCREEN_CY - (sy + 0.5)) / FOCAL;
        if (-SP + yc * CP < -1e-9) continue;
        double g = clamp01(sy / std::max(1.0, HORIZON_Y));
        Color base = mix(skyTop, skyHorizon, std::pow(g, 1.5));
        for (int sx = 0; sx < W; ++sx) {
            double dx = sx - sunX, dy = sy - sunY;
            double glow = std::exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma));
            c.plot(sx, sy, mix(base, {255, 240, 205}, glow * 0.75));
        }
    }
    fillCircleMidpoint(c, static_cast<int>(sunX), static_cast<int>(sunY), static_cast<int>(26 * PX), {255, 247, 222});
    drawCloud(c, W * 0.18, HORIZON_Y * 0.30, 1.6 * PX);
    drawCloud(c, W * 0.50, HORIZON_Y * 0.18, 1.2 * PX);
    drawCloud(c, W * 0.90, HORIZON_Y * 0.35, 1.0 * PX);
    drawHillLayer(c, HORIZON_Y, 80.0 * PX, 7, {150, 172, 205});
    drawHillLayer(c, HORIZON_Y, 52.0 * PX, 19, {118, 146, 176});
    drawHillLayer(c, HORIZON_Y, 30.0 * PX, 33, {84, 128, 104});

    // Ground: every pixel is traced back from the camera to the ground plane.
    const Color grassA{82, 152, 74}, grassB{92, 166, 84};
    for (int sy = 0; sy < H; ++sy) {
        double yc = (SCREEN_CY - (sy + 0.5)) / FOCAL;
        double up = -SP + yc * CP;
        if (up >= -1e-9) continue;
        double fwd = CP + yc * SP;
        double t = -CAM_H / up;
        double Y = CAM_Y - fwd * t;
        double fade = 1.0 / (1.0 + t / 700.0);
        for (int sx = 0; sx < W; ++sx) {
            double X = CAM_X + ((sx + 0.5 - SCREEN_CX) / FOCAL) * t;
            Color base = (static_cast<int>(std::floor(Y / 45.0)) & 1) ? grassA : grassB;
            double noise = rand01(hash2(static_cast<int>(std::floor(X / 5)), static_cast<int>(std::floor(Y / 5)))) - 0.5;
            c.plot(sx, sy, fogged(shade(base, 1.0 + noise * 0.14 * fade), t));
        }
    }
}

// ---------------------------------------------------------------------------
// Trackside objects (drawn far-to-near with the cars)
// ---------------------------------------------------------------------------
struct Item { double key; std::function<void(Canvas&)> draw; };   // key = world Y (small = far)

std::vector<Point> groundEllipse(double X, double Y, double along, double across, double dx, double dy) {
    std::vector<Point> pts;
    for (int k = 0; k < 28; ++k) {
        double a = 2 * PI * k / 28;
        double lx = std::cos(a) * along, ly = std::sin(a) * across;
        pts.push_back(scr(X + lx * dx - ly * dy, Y + lx * dy + ly * dx));
    }
    return pts;
}

void drawTree(Canvas& c, double X, double Y, int kind) {
    Proj base = project(X, Y, 0);
    double s = FOCAL / base.z;
    const Color trunk{98, 70, 48};
    fillPolygon(c, groundEllipse(X + 6, Y + 2, 24, 12, 1, 0), {20, 40, 24}, 0.30);
    fillPolygon(c, {scr(X - 3, Y, 0), scr(X + 3, Y, 0), scr(X + 3, Y, 24), scr(X - 3, Y, 24)}, fogged(trunk, base.z));
    if (kind == 0) {
        for (int k = 0; k < 3; ++k) {
            double baseH = 14 + k * 20, topH = baseH + 36, hw = 24 - k * 5;
            Point apex = scr(X, Y, topH), mid = scr(X, Y, baseH);
            Point l = scr(X - hw, Y, baseH), r = scr(X + hw, Y, baseH);
            fillPolygon(c, {apex, l, mid}, fogged({28, 90, 52}, base.z));
            fillPolygon(c, {apex, mid, r}, fogged({52, 128, 70}, base.z));
        }
    } else {
        Proj p = project(X, Y, 42);
        fillCircleMidpoint(c, static_cast<int>(p.x), static_cast<int>(p.y), static_cast<int>(22 * s), fogged({34, 104, 56}, base.z));
        fillCircleMidpoint(c, static_cast<int>(p.x + 4 * s), static_cast<int>(p.y - 4 * s), static_cast<int>(16 * s), fogged({62, 142, 66}, base.z));
        fillCircleMidpoint(c, static_cast<int>(p.x + 8 * s), static_cast<int>(p.y - 8 * s), static_cast<int>(8 * s), {150, 205, 110}, 0.45);
    }
}

// Grandstand ranges (sample indices) on the far side of the circuit.
const int STAND_A0 = 412, STAND_A1 = 480, STAND_B0 = 0, STAND_B1 = 70;
bool inStandRange(int i) { return (i >= STAND_A0 && i < STAND_A1) || (i >= STAND_B0 && i < STAND_B1); }

void addWalls(std::vector<Item>& items) {
    // Advertising boards in team colours along both sides of the track.
    const Color panels[] = {{236, 238, 244}, {205, 42, 48}, {236, 238, 244}, {32, 56, 140},
                            {236, 238, 244}, {250, 205, 50}};
    for (int side = -1; side <= 1; side += 2)
        for (int i = 0; i < TRACK_N; ++i) {
            int j = (i + 1) % TRACK_N;
            Vec2 a = offsetPoint(g_frames[i], side * 98.0), b = offsetPoint(g_frames[j], side * 98.0);
            Vec2 a2 = offsetPoint(g_frames[i], side * 104.0), b2 = offsetPoint(g_frames[j], side * 104.0);
            Color col = panels[(i / 10) % 6];
            items.push_back({(a.y + b.y) / 2, [a, b, a2, b2, col](Canvas& c) {
                double z = project(a.x, a.y, 0).z;
                fillPolygon(c, {scr(a.x, a.y, 0), scr(b.x, b.y, 0), scr(b.x, b.y, 14), scr(a.x, a.y, 14)}, fogged(col, z));
                fillPolygon(c, {scr(a.x, a.y, 14), scr(b.x, b.y, 14), scr(b2.x, b2.y, 14), scr(a2.x, a2.y, 14)},
                            fogged(shade(col, 1.15), z));
            }});
        }
}

// Tiered grandstands with a colourful crowd.
void addStands(std::vector<Item>& items, int i0, int i1) {
    const Color crowd[] = {{214, 48, 54}, {240, 240, 244}, {40, 98, 210}, {250, 205, 60},
                           {230, 150, 120}, {60, 60, 70}, {40, 168, 96}};
    const int tiers = 4;
    for (int i = i0; i < i1; ++i) {
        int j = (i + 1) % TRACK_N;
        for (int k = 0; k < tiers; ++k) {
            double d0 = -(114 + k * 13), d1 = -(114 + (k + 1) * 13);
            double hPrev = (k == 0) ? 0 : 12 + (k - 1) * 11, hTop = 12 + k * 11;
            Vec2 a0 = offsetPoint(g_frames[i], d0), b0 = offsetPoint(g_frames[j], d0);
            Vec2 a1 = offsetPoint(g_frames[i], d1), b1 = offsetPoint(g_frames[j], d1);
            Color seat = crowd[hash2(i, k) % 7];
            items.push_back({(a0.y + b0.y) / 2 - 0.01 * (tiers - k), [=](Canvas& c) {
                double z = project(a0.x, a0.y, 0).z;
                fillPolygon(c, {scr(a0.x, a0.y, hPrev), scr(b0.x, b0.y, hPrev), scr(b0.x, b0.y, hTop), scr(a0.x, a0.y, hTop)},
                            fogged({70, 76, 96}, z));
                fillPolygon(c, {scr(a0.x, a0.y, hTop), scr(b0.x, b0.y, hTop), scr(b1.x, b1.y, hTop), scr(a1.x, a1.y, hTop)},
                            fogged(seat, z));
            }});
        }
    }
}

void addGantry(std::vector<Item>& items) {
    const Frame& f = g_frames[START_INDEX + 2];
    Vec2 p1 = offsetPoint(f, -112.0), p2 = offsetPoint(f, 112.0);
    auto pillar = [](Canvas& c, Vec2 p) {
        double z = project(p.x, p.y, 0).z;
        fillPolygon(c, {scr(p.x - 4, p.y, 0), scr(p.x + 4, p.y, 0), scr(p.x + 4, p.y, 92), scr(p.x - 4, p.y, 92)},
                    fogged({70, 76, 90}, z));
        fillPolygon(c, {scr(p.x + 1, p.y, 0), scr(p.x + 4, p.y, 0), scr(p.x + 4, p.y, 92), scr(p.x + 1, p.y, 92)},
                    fogged({110, 118, 134}, z));
    };
    items.push_back({p1.y, [=](Canvas& c) { pillar(c, p1); }});
    items.push_back({p2.y, [=](Canvas& c) { pillar(c, p2); }});
    items.push_back({std::min(p1.y, p2.y) - 1, [p1, p2](Canvas& c) {
        double z = project(p1.x, p1.y, 0).z;
        fillPolygon(c, {scr(p1.x, p1.y, 66), scr(p2.x, p2.y, 66), scr(p2.x, p2.y, 92), scr(p1.x, p1.y, 92)},
                    fogged({22, 34, 70}, z));
        const int cells = 24;
        for (int k = 0; k < cells; ++k) {
            double t0 = double(k) / cells, t1 = double(k + 1) / cells;
            double x0 = p1.x + (p2.x - p1.x) * t0, y0 = p1.y + (p2.y - p1.y) * t0;
            double x1 = p1.x + (p2.x - p1.x) * t1, y1 = p1.y + (p2.y - p1.y) * t1;
            for (int row = 0; row < 2; ++row) {
                double h0 = 80 + row * 6, h1 = h0 + 6;
                Color col = ((k + row) % 2) ? Color{20, 22, 28} : Color{244, 246, 250};
                fillPolygon(c, {scr(x0, y0, h0), scr(x1, y1, h0), scr(x1, y1, h1), scr(x0, y0, h1)}, fogged(col, z));
            }
        }
        fillPolygon(c, {scr(p1.x, p1.y, 70), scr(p2.x, p2.y, 70), scr(p2.x, p2.y, 76), scr(p1.x, p1.y, 76)},
                    fogged({205, 42, 48}, z));
    }});
}

void addTrees(std::vector<Item>& items) {
    unsigned seed = 12345;
    auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return (seed >> 8) / 16777216.0; };
    int placed = 0;
    for (int tries = 0; tries < 6000 && placed < 75; ++tries) {
        double X = -450 + next() * 1800, Y = -420 + next() * 940;
        int idx;
        double d = nearestTrack(X, Y, idx);
        if (d < 125) continue;
        if (inStandRange(idx) && d < 200) continue;
        Proj p = project(X, Y, 0);
        if (p.x < -60 * PX || p.x > W + 60 * PX || p.y > H) continue;
        int kind = next() < 0.6 ? 0 : 1;
        items.push_back({Y, [X, Y, kind](Canvas& c) { drawTree(c, X, Y, kind); }});
        ++placed;
    }
}

// ---------------------------------------------------------------------------
// Formula 1 car model.
// The car is described as parts (boxes, wheels, a helmet) with a height range.
// It is cut into thin horizontal slices; each slice is a top-view picture.
// ---------------------------------------------------------------------------
const double SPRU = 3.0;                    // slice pixels per world unit
const double PAD_U = 58, PAD_V = 26;        // slice covers u in [-58,58], v in [-26,26]
const int SLICE_W = static_cast<int>(2 * PAD_U * SPRU), SLICE_H = static_cast<int>(2 * PAD_V * SPRU);
const double LAYER_STEP = 0.8;
const int LAYERS = 25;                      // heights 0 .. 19.2
const Color KEY{255, 0, 255};               // "transparent" colour inside slices

enum PartKind { PRISM, WHEEL, SPHERE };
struct Part {
    PartKind kind;
    std::vector<Vec2> pts;                  // PRISM outline in car coordinates (u forward, v sideways)
    double h0 = 0, h1 = 0;                  // PRISM height range
    Color col{0, 0, 0};
    double a = 0, b = 0, c = 0, d = 0;      // WHEEL: a=centre u, b=radius, c..d = v range (both sides)
                                            // SPHERE: a=u, b=v, c=centre height, d=radius
};

void addPrism(std::vector<Part>& parts, std::vector<Vec2> pts, double h0, double h1, Color col, bool mirror = false) {
    Part p; p.kind = PRISM; p.pts = pts; p.h0 = h0; p.h1 = h1; p.col = col;
    parts.push_back(p);
    if (mirror) {
        for (Vec2& v : p.pts) v.y = -v.y;
        parts.push_back(p);
    }
}

void addWheel(std::vector<Part>& parts, double u, double radius, double v0, double v1) {
    Part p; p.kind = WHEEL; p.col = {34, 34, 38};
    p.a = u; p.b = radius; p.c = v0; p.d = v1;
    parts.push_back(p);
}

std::vector<Part> buildCarParts(Color body, Color accent) {
    std::vector<Part> P;
    const Color dark{30, 32, 38}, arm{58, 60, 68};

    addPrism(P, {{-42, -16}, {-42, 16}, {30, 16}, {44, 8}, {44, -8}, {30, -16}}, 0, 1.6, {24, 26, 30});    // floor
    addWheel(P, -30, 9.5, 13, 22.5);                                                                       // rear tyres
    addWheel(P, 28, 8.5, 11.5, 19.5);                                                                      // front tyres
    addPrism(P, {{26, 7}, {30, 7}, {30, 11.5}, {26, 11.5}}, 4, 5.3, arm, true);                            // front arms
    addPrism(P, {{-32, 8}, {-28, 8}, {-28, 13}, {-32, 13}}, 4, 5.3, arm, true);                            // rear arms
    addPrism(P, {{-38, -7}, {-38, 7}, {-18, 9}, {-6, 9}, {8, 8}, {8, -8}, {-6, -9}, {-18, -9}}, 2, 9.5, body); // monocoque
    addPrism(P, {{-30, 8}, {-28, 13.5}, {0, 14}, {10, 10}, {10, 8}}, 2.4, 8.5, shade(body, 0.92), true);   // sidepods
    addPrism(P, {{-26, 9}, {-8, 9}, {-8, 13.6}, {-26, 13.6}}, 6.4, 8.6, accent, true);                     // sidepod stripe
    addPrism(P, {{8, -8}, {8, 8}, {24, 5}, {46, 2.6}, {46, -2.6}, {24, -5}}, 2, 6.5, body);                // nose
    addPrism(P, {{17, -5.4}, {22, -4.6}, {22, 4.6}, {17, 5.4}}, 5.8, 6.9, accent);                         // nose band
    addPrism(P, {{-36, -4}, {-36, 4}, {-14, 4.8}, {-6, 3}, {-6, -3}, {-14, -4.8}}, 9.5, 15, body);         // engine cover
    addPrism(P, {{-17, -3}, {-17, 3}, {-10, 3}, {-10, -3}}, 15, 19.2, dark);                               // airbox
    addPrism(P, {{-7, 4.6}, {4, 4.6}, {4, 5.8}, {-7, 5.8}}, 13.4, 15.6, dark, true);                       // halo sides
    addPrism(P, {{3.4, -5.8}, {5, -5.8}, {5, 5.8}, {3.4, 5.8}}, 13.4, 15.6, dark);                         // halo front
    {   Part h; h.kind = SPHERE; h.col = accent; h.a = -2; h.b = 0; h.c = 12.2; h.d = 3.4; P.push_back(h); } // helmet
    addPrism(P, {{44, -21}, {44, 21}, {50, 21}, {50, -21}}, 1.4, 2.7, dark);                               // front wing
    addPrism(P, {{50, -21}, {50, 21}, {54, 20}, {54, -20}}, 2, 3.5, body);                                 // front flap
    addPrism(P, {{44, 21}, {54, 21}, {54, 22.6}, {44, 22.6}}, 1.4, 7, accent, true);                       // front end plates
    addPrism(P, {{-54, -17}, {-54, 17}, {-46, 17}, {-46, -17}}, 13.4, 14.6, dark);                         // rear wing
    addPrism(P, {{-54, -17}, {-54, 17}, {-50, 17}, {-50, -17}}, 14.6, 16, body);                           // rear flap
    addPrism(P, {{-48, -2}, {-48, 2}, {-44, 2}, {-44, -2}}, 3, 13.4, dark);                                // rear wing pillar
    addPrism(P, {{-54, 17}, {-46, 17}, {-46, 18.6}, {-54, 18.6}}, 6, 17.5, accent, true);                  // rear end plates
    addPrism(P, {{-40, -1.5}, {-40, 1.5}, {-38, 1.5}, {-38, -1.5}}, 4, 6.4, {230, 30, 30});                // rear light
    return P;
}

Point slicePt(double u, double v) {
    return {static_cast<int>(std::lround((u + PAD_U) * SPRU)), static_cast<int>(std::lround((v + PAD_V) * SPRU))};
}

// One top-view slice of the car at height h.
Canvas renderSlice(const std::vector<Part>& parts, double h) {
    Canvas s(SLICE_W, SLICE_H, KEY);
    for (const Part& p : parts) {
        if (p.kind == PRISM) {
            if (h < p.h0 || h > p.h1) continue;
            bool top = (h + LAYER_STEP > p.h1);
            std::vector<Point> poly;
            for (const Vec2& v : p.pts) poly.push_back(slicePt(v.x, v.y));
            fillPolygon(s, poly, top ? shade(p.col, 1.12) : shade(p.col, 0.78));
            if (top) outlinePolygon(s, poly, shade(p.col, 0.5));
        } else if (p.kind == WHEEL) {
            double R = p.b;
            if (h < 0 || h > 2 * R) continue;
            double half = std::sqrt(std::max(0.0, R * R - (h - R) * (h - R)));
            bool top = (h + LAYER_STEP > 2 * R);
            Color col = top ? Color{58, 58, 64} : shade(p.col, 0.9 + 0.5 * (h / (2 * R)));
            for (int side = -1; side <= 1; side += 2) {
                double v0 = side * p.c, v1 = side * p.d;
                fillPolygon(s, {slicePt(p.a - half, v0), slicePt(p.a + half, v0), slicePt(p.a + half, v1), slicePt(p.a - half, v1)}, col);
            }
        } else {
            double dz = h - p.c;
            if (std::fabs(dz) > p.d) continue;
            double r = std::sqrt(p.d * p.d - dz * dz);
            Point c = slicePt(p.a, p.b);
            fillCircleMidpoint(s, c.x, c.y, static_cast<int>(std::lround(r * SPRU)), dz > p.d - 1.0 ? shade(p.col, 1.25) : p.col);
        }
    }
    return s;
}

struct CarModel { std::vector<Canvas> slices; };

CarModel makeCar(Color body, Color accent) {
    std::vector<Part> parts = buildCarParts(body, accent);
    CarModel m;
    for (int k = 0; k < LAYERS; ++k) m.slices.push_back(renderSlice(parts, k * LAYER_STEP));
    return m;
}

// Draw one slice lying on the plane at height h. For every screen pixel we trace the
// camera ray down to that plane, convert to car coordinates, and read the slice.
void drawSlice(Canvas& dst, const Canvas& slice, double X0, double Y0, double ux, double uy,
               double h, double fogAmount) {
    double nx = -uy, ny = ux;
    double minx = 1e18, maxx = -1e18, miny = 1e18, maxy = -1e18;
    for (int a = -1; a <= 1; a += 2)
        for (int b = -1; b <= 1; b += 2) {
            double u = a * PAD_U, v = b * PAD_V;
            Proj p = project(X0 + u * ux + v * nx, Y0 + u * uy + v * ny, h);
            minx = std::min(minx, p.x); maxx = std::max(maxx, p.x);
            miny = std::min(miny, p.y); maxy = std::max(maxy, p.y);
        }
    int y0 = std::max(0, static_cast<int>(miny)), y1 = std::min(dst.height() - 1, static_cast<int>(maxy) + 1);
    int x0 = std::max(0, static_cast<int>(minx)), x1 = std::min(dst.width() - 1, static_cast<int>(maxx) + 1);
    for (int sy = y0; sy <= y1; ++sy) {
        double yc = (SCREEN_CY - (sy + 0.5)) / FOCAL;
        double up = -SP + yc * CP;
        if (up >= -1e-9) continue;
        double fwd = CP + yc * SP;
        double t = (h - CAM_H) / up;
        double Y = CAM_Y - fwd * t;
        for (int sx = x0; sx <= x1; ++sx) {
            double X = CAM_X + ((sx + 0.5 - SCREEN_CX) / FOCAL) * t;
            double dX = X - X0, dY = Y - Y0;
            double u = dX * ux + dY * uy, v = dX * nx + dY * ny;
            int px = static_cast<int>(std::floor((u + PAD_U) * SPRU));
            int py = static_cast<int>(std::floor((v + PAD_V) * SPRU));
            if (px < 0 || py < 0 || px >= slice.width() || py >= slice.height()) continue;
            Color s = slice.get(px, py);
            if (s.r == KEY.r && s.g == KEY.g && s.b == KEY.b) continue;
            dst.plot(sx, sy, mix(s, HAZE, fogAmount));
        }
    }
}

void drawCar(Canvas& c, const CarModel& car, double X, double Y, double ux, double uy) {
    double nx = -uy, ny = ux;
    // Soft shadow on the road, pushed away from the sun (which is on the right).
    for (int pass = 0; pass < 2; ++pass) {
        double grow = pass == 0 ? 1.0 : 0.82;
        std::vector<Point> pts;
        const double corners[4][2] = {{-52, -21}, {-52, 21}, {52, 21}, {52, -21}};
        for (const auto& k : corners) {
            double u = k[0] * grow, v = k[1] * grow;
            pts.push_back(scr(X + u * ux + v * nx - 7, Y + u * uy + v * ny + 3, 0));
        }
        fillPolygon(c, pts, {12, 20, 16}, 0.22);
    }
    double fogAmount = 0.55 * (1.0 - std::exp(-project(X, Y, 0).z / 4500.0));
    for (int k = 0; k < LAYERS; ++k)
        drawSlice(c, car.slices[k], X, Y, ux, uy, k * LAYER_STEP, fogAmount);
}

// ---------------------------------------------------------------------------
// GIF writer (hand-written): colour palette, ordered dithering, LZW compression.
// ---------------------------------------------------------------------------
class GifWriter {
public:
    void begin(const char* filename, int w, int h, const std::vector<Color>& palette) {
        out_.open(filename, std::ios::binary);
        if (!out_) throw std::runtime_error("Cannot create GIF");
        w_ = w; h_ = h;
        out_.write("GIF89a", 6);
        put16(w); put16(h);
        out_.put(static_cast<char>(0xF7)); out_.put(0); out_.put(0);       // 256-colour global table
        for (int i = 0; i < 256; ++i) {
            Color c = i < static_cast<int>(palette.size()) ? palette[i] : Color{0, 0, 0};
            out_.put(static_cast<char>(c.r)); out_.put(static_cast<char>(c.g)); out_.put(static_cast<char>(c.b));
        }
        out_.write("\x21\xFF\x0B" "NETSCAPE2.0" "\x03\x01\x00\x00\x00", 19);  // loop forever
        // Nearest palette colour for every 6-bit-per-channel colour.
        lut_.assign(1 << 18, 0);
        for (int i = 0; i < (1 << 18); ++i) {
            int r = ((i >> 12) & 63) * 4 + 2, g = ((i >> 6) & 63) * 4 + 2, b = (i & 63) * 4 + 2;
            int best = 0, bestD = 1 << 30;
            for (int k = 0; k < static_cast<int>(palette.size()); ++k) {
                int dr = r - palette[k].r, dg = g - palette[k].g, db = b - palette[k].b;
                int d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
                if (d < bestD) { bestD = d; best = k; }
            }
            lut_[i] = static_cast<unsigned char>(best);
        }
        table_.assign(4096 * 256, -1);
    }

    void addFrame(const Canvas& c, int delayCentiseconds) {
        std::vector<unsigned char> pix(static_cast<size_t>(w_) * h_);
        for (int y = 0; y < h_; ++y)
            for (int x = 0; x < w_; ++x) {
                Color p = c.get(x, y);
                double d = (bayer(x & 7, y & 7) / 63.0 - 0.5) * 9.0;
                int r = clampByte(p.r + d), g = clampByte(p.g + d), b = clampByte(p.b + d);
                pix[static_cast<size_t>(y) * w_ + x] = lut_[((r >> 2) << 12) | ((g >> 2) << 6) | (b >> 2)];
            }
        out_.write("\x21\xF9\x04\x04", 4);
        put16(delayCentiseconds);
        out_.put(0); out_.put(0);
        out_.put(0x2C); put16(0); put16(0); put16(w_); put16(h_); out_.put(0);
        writeLZW(pix);
    }

    void end() { out_.put(0x3B); out_.close(); }

private:
    std::ofstream out_;
    int w_ = 0, h_ = 0;
    std::vector<unsigned char> lut_;
    std::vector<int> table_;

    void put16(int v) { out_.put(static_cast<char>(v & 255)); out_.put(static_cast<char>((v >> 8) & 255)); }
    static int clampByte(double v) { return v < 0 ? 0 : (v > 255 ? 255 : static_cast<int>(v)); }
    static int bayer(int x, int y) {          // 8x8 ordered-dither pattern, values 0..63
        int v = 0, xx = x ^ y;
        for (int b = 2; b >= 0; --b) { v = (v << 1) | ((y >> b) & 1); v = (v << 1) | ((xx >> b) & 1); }
        return v;
    }

    void writeLZW(const std::vector<unsigned char>& pix) {
        out_.put(8);                                           // minimum code size
        std::vector<unsigned char> block;
        unsigned acc = 0; int nacc = 0;
        auto flushBlock = [&]() {
            if (block.empty()) return;
            out_.put(static_cast<char>(block.size()));
            out_.write(reinterpret_cast<char*>(block.data()), static_cast<std::streamsize>(block.size()));
            block.clear();
        };
        auto emit = [&](int code, int bits) {
            acc |= static_cast<unsigned>(code) << nacc; nacc += bits;
            while (nacc >= 8) {
                block.push_back(static_cast<unsigned char>(acc & 255));
                acc >>= 8; nacc -= 8;
                if (block.size() == 255) flushBlock();
            }
        };
        const int CLEAR = 256, EOI = 257;
        int next = 258, bits = 9;
        std::fill(table_.begin(), table_.end(), -1);
        emit(CLEAR, bits);
        int prefix = pix[0];
        for (size_t i = 1; i < pix.size(); ++i) {
            int c = pix[i], key = (prefix << 8) | c;
            if (table_[key] >= 0) { prefix = table_[key]; continue; }
            emit(prefix, bits);
            if (next > (1 << bits) - 1 && bits < 12) ++bits;
            if (next < 4096) table_[key] = next++;
            else { emit(CLEAR, bits); std::fill(table_.begin(), table_.end(), -1); next = 258; bits = 9; }
            prefix = c;
        }
        emit(prefix, bits);
        if (next > (1 << bits) - 1 && bits < 12) ++bits;
        emit(EOI, bits);
        if (nacc > 0) block.push_back(static_cast<unsigned char>(acc & 255));
        flushBlock();
        out_.put(0);
    }
};

// Median-cut palette: repeatedly split the colour box with the most pixels.
std::vector<Color> buildPalette(const Canvas& img) {
    struct Bin { int c[3]; long n; };
    std::vector<long> hist(32768, 0);
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x) {
            Color p = img.get(x, y);
            ++hist[((p.r >> 3) << 10) | ((p.g >> 3) << 5) | (p.b >> 3)];
        }
    std::vector<Bin> bins;
    for (int i = 0; i < 32768; ++i)
        if (hist[i]) bins.push_back({{(i >> 10) & 31, (i >> 5) & 31, i & 31}, hist[i]});

    struct Box { int lo, hi; };
    std::vector<Box> boxes{{0, static_cast<int>(bins.size())}};
    while (boxes.size() < 256) {
        int pick = -1; double bestScore = 0; int pickCh = 0;
        for (int bi = 0; bi < static_cast<int>(boxes.size()); ++bi) {
            if (boxes[bi].hi - boxes[bi].lo < 2) continue;
            int mn[3] = {31, 31, 31}, mx[3] = {0, 0, 0}; long pop = 0;
            for (int i = boxes[bi].lo; i < boxes[bi].hi; ++i) {
                pop += bins[i].n;
                for (int ch = 0; ch < 3; ++ch) { mn[ch] = std::min(mn[ch], bins[i].c[ch]); mx[ch] = std::max(mx[ch], bins[i].c[ch]); }
            }
            int ch = 0, range = mx[0] - mn[0];
            for (int k = 1; k < 3; ++k) if (mx[k] - mn[k] > range) { range = mx[k] - mn[k]; ch = k; }
            double score = static_cast<double>(pop) * (range + 1);
            if (score > bestScore) { bestScore = score; pick = bi; pickCh = ch; }
        }
        if (pick < 0) break;
        Box bx = boxes[pick];
        std::sort(bins.begin() + bx.lo, bins.begin() + bx.hi,
                  [pickCh](const Bin& a, const Bin& b) { return a.c[pickCh] < b.c[pickCh]; });
        long total = 0;
        for (int i = bx.lo; i < bx.hi; ++i) total += bins[i].n;
        long acc = 0; int mid = bx.lo + 1;
        for (int i = bx.lo; i < bx.hi - 1; ++i) { acc += bins[i].n; mid = i + 1; if (acc * 2 >= total) break; }
        boxes[pick] = {bx.lo, mid};
        boxes.push_back({mid, bx.hi});
    }
    std::vector<Color> palette;
    for (const Box& bx : boxes) {
        double r = 0, g = 0, b = 0, n = 0;
        for (int i = bx.lo; i < bx.hi; ++i) {
            r += bins[i].c[0] * bins[i].n; g += bins[i].c[1] * bins[i].n; b += bins[i].c[2] * bins[i].n; n += bins[i].n;
        }
        if (n == 0) continue;
        palette.push_back({static_cast<unsigned char>(std::min(255.0, r / n * 8 + 4)),
                           static_cast<unsigned char>(std::min(255.0, g / n * 8 + 4)),
                           static_cast<unsigned char>(std::min(255.0, b / n * 8 + 4))});
    }
    return palette;
}

void applyVignette(Canvas& c) {
    for (int y = 0; y < c.height(); ++y)
        for (int x = 0; x < c.width(); ++x) {
            double nx = (x - c.width() / 2.0) / (c.width() / 2.0);
            double ny = (y - c.height() / 2.0) / (c.height() / 2.0);
            c.plot(x, y, shade(c.get(x, y), 1.0 - 0.22 * (nx * nx + ny * ny) / 2.0));
        }
}

// ---------------------------------------------------------------------------
// The race
// ---------------------------------------------------------------------------
struct Team {
    Color body, accent;
    double start;      // starting position along the lap (0..1)
    double lane;       // which side of the track
    double surge;      // how much the speed rises and falls during the lap
    double phase;
};

// The seven teams. Team 0 (red) is the car the player drives in play mode.
std::vector<Team> makeTeams() {
    return {
        {{214, 40, 48},  {245, 245, 245}, 0.00, -24, 0.20, 0.0},
        {{20, 150, 150}, {210, 215, 220}, 0.14,  24, 0.16, 1.3},
        {{240, 128, 34}, {30, 60, 150},   0.28, -24, 0.22, 2.4},
        {{32, 48, 120},  {250, 205, 50},  0.43,  24, 0.18, 3.1},
        {{20, 130, 80},  {240, 240, 240}, 0.57, -24, 0.20, 4.2},
        {{200, 205, 212},{214, 40, 48},   0.71,  24, 0.16, 5.0},
        {{110, 60, 170}, {240, 120, 190}, 0.86, -24, 0.22, 5.8},
    };
}

// Sky and track, painted once per picture (the camera may have moved).
Canvas buildBase() {
    Canvas base(W, H, {0, 0, 0});
    drawSkyAndGround(base);
    drawTrackSurface(base);
    return base;
}

// ---------------------------------------------------------------------------
// Race simulation. Each car has a speed and a lane that change every frame.
// A car slows down behind a slower car and changes lane when it is stuck.
// A safety check then makes sure no two cars ever overlap.
// ---------------------------------------------------------------------------
const double CAR_LEN = 104.0, CAR_WID = 42.0;   // car size (matches the drawn model)
const double MIN_GAP = 30.0;                    // space kept between bumpers
const double SAFE_GAP = 140.0;                  // closer than this, a car watches the car ahead
const double FOLLOW_GAIN = 0.2;                 // how strongly a car matches the speed of the car ahead
const double MAX_ACCEL = 1.0, MAX_BRAKE = 3.0;  // most a car can speed up or slow down per frame
const double LANE_RATE = 1.2;                   // most a car can move sideways per frame
const double LANE_LIMIT = 60.0;                 // how far across the track a car may go
const int LANE_COOLDOWN = 90;                   // frames a car waits before changing lane again
const double PLAYER_STEP_SPEED = 0.002;         // change per W / S press (lap fraction per frame)
const double PLAYER_MAX_SPEED = 0.03;           // fastest the player's car can be asked to go
const double PLAYER_LANE_STEP = 6.0;            // sideways change per A / D press

struct Racer {
    double s = 0;            // distance round the lap (not wrapped, so it can keep growing)
    double v = 0;            // speed in world units per frame
    double lane = 0;         // sideways offset from the track centre
    double targetLane = 0;   // the lane the car is moving towards
    double base = 0;         // cruising speed (one lap in 100 frames)
    double surge = 0, phase = 0;   // how the wished speed rises and falls
    bool player = false;
    int cooldown = 0;        // frames until this car may change lane again
};

struct Race {
    std::vector<Racer> racers;     // racers[0] is the player's car
    int frame = 0;                 // frames simulated so far
    double playerSpeed = 0.01;     // speed the player asks for (lap fraction per frame)
    bool playerBlocked = false;    // true in a frame where the player was held back
    int corrections = 0;           // how many times the safety check had to move a car
    int laneChanges = 0;           // lane changes made by AI cars
};

// Signed distance from a to b along the lap, in the range (-L/2, L/2].
double lapDelta(double a, double b) {
    const double L = g_trackLength;
    double d = std::fmod(b - a, L);
    if (d > L / 2) d -= L;
    if (d <= -L / 2) d += L;
    return d;
}

// Start a race from the team table. Every car starts at its grid position.
Race makeRace(const std::vector<Team>& teams) {
    Race r;
    for (const Team& t : teams) {
        Racer c;
        c.base = g_trackLength / 100.0;
        c.v = c.base;
        c.s = t.start * g_trackLength;
        c.surge = t.surge;
        c.phase = t.phase;
        c.lane = c.targetLane = t.lane;
        r.racers.push_back(c);
    }
    r.racers[0].player = true;
    return r;
}

// Speed a car would like to drive at this frame.
double desiredSpeed(const Race& r, const Racer& c) {
    if (c.player) return r.playerSpeed * g_trackLength;
    // Same rise and fall as the first version of the animation.
    return c.base * (1.0 + c.surge * std::cos(2 * PI * r.frame / 100.0 + c.phase));
}

// The nearest car ahead of car i in its lane band, or -1 if there is none.
int leaderOf(const Race& r, int i) {
    int best = -1;
    double bestDist = 1e18;
    for (int j = 0; j < static_cast<int>(r.racers.size()); ++j) {
        if (j == i) continue;
        if (std::fabs(r.racers[j].lane - r.racers[i].lane) >= CAR_WID + 6) continue;
        double d = lapDelta(r.racers[i].s, r.racers[j].s);
        if (d > 0 && d < bestDist) { bestDist = d; best = j; }
    }
    return best;
}

// True if no other car is beside, or close behind or ahead of, the lane `side`.
bool laneFree(const Race& r, int self, double side) {
    for (int j = 0; j < static_cast<int>(r.racers.size()); ++j) {
        if (j == self) continue;
        if (std::fabs(r.racers[j].lane - side) >= CAR_WID + 6) continue;
        if (std::fabs(lapDelta(r.racers[self].s, r.racers[j].s)) < CAR_LEN + 2 * MIN_GAP) return false;
    }
    return true;
}

// True if cars i and j overlap or are closer than the minimum gap.
bool tooClose(const Race& r, int i, int j) {
    const Racer& a = r.racers[i];
    const Racer& b = r.racers[j];
    if (std::fabs(a.lane - b.lane) >= CAR_WID) return false;      // side by side with room
    return std::fabs(lapDelta(a.s, b.s)) < CAR_LEN + MIN_GAP;
}

// Safety check. If two cars are too close, the one behind is moved back
// until the gap is right. Returns how many pairs had to be fixed.
int keepApart(Race& r) {
    int fixes = 0;
    const double need = CAR_LEN + MIN_GAP;
    const int n = static_cast<int>(r.racers.size());
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) {
            if (!tooClose(r, i, j)) continue;
            double d = lapDelta(r.racers[i].s, r.racers[j].s);   // where j is, seen from i
            int back = (d > 0) ? i : j;                          // the car that must give way
            int front = (d > 0) ? j : i;
            double dist = lapDelta(r.racers[back].s, r.racers[front].s);
            r.racers[back].s += dist - need;                     // move back to the right gap
            r.racers[back].v = std::min(r.racers[back].v, r.racers[front].v);
            if (r.racers[back].player) r.playerBlocked = true;
            ++fixes;
        }
    return fixes;
}

// Advance the race by one frame.
void stepRace(Race& r) {
    r.playerBlocked = false;
    for (int i = 0; i < static_cast<int>(r.racers.size()); ++i) {
        Racer& c = r.racers[i];
        double wish = desiredSpeed(r, c);
        double want = wish;
        int ahead = leaderOf(r, i);
        if (ahead >= 0) {
            const Racer& lead = r.racers[ahead];
            double gap = lapDelta(c.s, lead.s) - CAR_LEN;          // bumper-to-bumper gap
            if (gap < SAFE_GAP) {
                double follow = lead.v + (gap - MIN_GAP) * FOLLOW_GAIN;
                want = std::min(want, std::max(0.0, follow));
            }
            // A car held up by a slower car tries the other side of the track.
            bool stuck = gap < SAFE_GAP / 2 && lead.v < 0.9 * wish;   // the car ahead is holding it up
            if (!c.player && c.cooldown == 0 && stuck && laneFree(r, i, -c.targetLane)) {
                c.targetLane = -c.targetLane;
                c.cooldown = LANE_COOLDOWN;
                ++r.laneChanges;
            }
        }
        // Speed up or slow down, but not by more than the car can manage in one frame.
        c.v += std::max(-MAX_BRAKE, std::min(MAX_ACCEL, want - c.v));
        if (c.v < 0) c.v = 0;
    }
    for (Racer& c : r.racers) {
        c.s += c.v;
        c.lane += std::max(-LANE_RATE, std::min(LANE_RATE, c.targetLane - c.lane));
        if (c.cooldown > 0) --c.cooldown;
    }
    ++r.frame;
    for (int pass = 0; pass < 3; ++pass) r.corrections += keepApart(r);
}

// How far apart two race states are: the largest change in any car's place
// (distance round the lap, or sideways). Zero means the two states match.
double loopSeam(const Race& a, const Race& b) {
    double seam = 0;
    for (size_t i = 0; i < a.racers.size(); ++i) {
        seam = std::max(seam, std::fabs(lapDelta(a.racers[i].s, b.racers[i].s)));
        seam = std::max(seam, std::fabs(a.racers[i].lane - b.racers[i].lane));
    }
    return seam;
}

// Choose the frame to start the GIF from. Starting at frame o and ending 100
// frames later should give the smallest jump back to the start. Returns o.
int findLoopStart(const std::vector<Team>& teams, double& bestSeam) {
    const int SEARCH = 600;              // how many starting frames to try
    Race scan = makeRace(teams);
    int best = 0;
    bestSeam = 1e18;
    for (int o = 0; o < SEARCH; ++o) {
        Race later = scan;
        for (int f = 0; f < 100; ++f) stepRace(later);
        double seam = loopSeam(scan, later);
        if (seam < bestSeam) { bestSeam = seam; best = o; }
        stepRace(scan);
    }
    return best;
}

// Where a racer is drawn: its depth key, index, frame and lane offset.
struct CarPlace { double key; int index; Frame fr; double lane; };

CarPlace placeOf(const Racer& c, int index) {
    Frame fr = frameAt(c.s);
    Vec2 pos = offsetPoint(fr, c.lane);
    return {pos.y, index, fr, c.lane};
}

std::vector<CarPlace> placesOf(const Race& r) {
    std::vector<CarPlace> places;
    for (int i = 0; i < static_cast<int>(r.racers.size()); ++i)
        places.push_back(placeOf(r.racers[i], i));
    return places;
}

// ---------------------------------------------------------------------------
// Camera views. Each view sets the camera height and tilt, and where it stands.
// ---------------------------------------------------------------------------
enum CameraMode { VIEW_BROADCAST, VIEW_FOLLOW, VIEW_CLOSE, VIEW_TOP, VIEW_LOW, VIEW_COUNT };

struct ViewSetting {
    const char* name;
    bool follow;      // true: the camera follows the player's car
    double height;    // camera height
    double pitch;     // tilt: larger looks down more steeply
};

// For a followed view the camera stands at height / tan(pitch) behind the car.
// That puts the car in the middle of the picture.
const ViewSetting VIEWS[VIEW_COUNT] = {
    {"Broadcast", false, 330, 0.50},   // the original fixed view
    {"Follow",    true,  250, 0.70},
    {"Close-up",  true,  120, 0.785},
    {"Top",       true,  700, 1.20},   // looking almost straight down
    {"Low chase", true,  40,  0.25},   // close to the ground, looking along the track
};

// Set the camera height and tilt, and keep the values derived from the tilt in step.
void setLens(double height, double pitch) {
    CAM_H = height;
    PITCH = pitch;
    CP = std::cos(pitch);
    SP = std::sin(pitch);
    HORIZON_Y = SCREEN_CY - FOCAL * std::tan(pitch);
}

// Move the camera for the chosen view. The car it follows is the player's.
void setCamera(CameraMode mode, const Racer& car) {
    const ViewSetting& vs = VIEWS[mode];
    setLens(vs.height, vs.pitch);
    if (!vs.follow) {
        CAM_X = 450; CAM_Y = 790;       // same spot as the original fixed view
        return;
    }
    Vec2 pos = offsetPoint(frameAt(car.s), car.lane);
    CAM_X = pos.x;                       // stay level with the car
    CAM_Y = pos.y + vs.height / std::tan(vs.pitch);   // and stand back so the car is centred
}

// Draw one picture: the base, then cars and scenery from far to near. The
// result is anti-aliased and vignetted, ready to save or show.
Canvas renderWorld(const Canvas& base, const std::vector<Item>& scenery,
                   std::vector<CarPlace> places, const std::vector<CarModel>& cars) {
    std::sort(places.begin(), places.end(), [](const CarPlace& a, const CarPlace& b) { return a.key < b.key; });
    Canvas frame = base;
    size_t next = 0;
    auto drawNextCar = [&]() {
        const CarPlace& cp = places[next++];
        Vec2 pos = offsetPoint(cp.fr, cp.lane);
        if (pos.y <= CAM_Y) drawCar(frame, cars[cp.index], pos.x, pos.y, cp.fr.tx, cp.fr.ty);
    };
    // Anything with a world Y beyond the camera is behind it and is skipped.
    for (const Item& it : scenery) {
        while (next < places.size() && places[next].key <= it.key) drawNextCar();
        if (it.key <= CAM_Y) it.draw(frame);
    }
    while (next < places.size()) drawNextCar();

    Canvas out = frame.downsample(SS);  // anti-aliasing
    applyVignette(out);
    return out;
}

// Everything the scene needs that does not change from frame to frame.
struct World {
    std::vector<Item> scenery;
    std::vector<CarModel> cars;
    std::vector<Team> teams;
};

World buildWorld() {
    World w;
    addTrees(w.scenery);
    addWalls(w.scenery);
    addStands(w.scenery, STAND_A0, STAND_A1);
    addStands(w.scenery, STAND_B0, STAND_B1);
    addGantry(w.scenery);
    std::sort(w.scenery.begin(), w.scenery.end(), [](const Item& a, const Item& b) { return a.key < b.key; });
    w.teams = makeTeams();
    for (const Team& t : w.teams) w.cars.push_back(makeCar(t.body, t.accent));
    return w;
}

// What play mode remembers between key presses.
struct PlayState {
    Race race;
    CameraMode view = VIEW_BROADCAST;
    std::string note;   // message about the last key pressed
};

// Draw the picture for the current state (camera, cars and scenery).
Canvas renderState(const PlayState& st, const World& w) {
    setCamera(st.view, st.race.racers[0]);
    Canvas base = buildBase();
    return renderWorld(base, w.scenery, placesOf(st.race), w.cars);
}

// One line of status shown under the picture.
std::string statusLine(const PlayState& st) {
    return "Frame " + std::to_string(st.race.frame) +
           "  speed " + std::to_string(int(st.race.playerSpeed * 1000)) +
           "  lane " + std::to_string(int(st.race.racers[0].lane)) +
           "  view " + VIEWS[st.view].name;
}

// Turn the image into text: each character is the average brightness of a block of pixels.
void printTextView(const Canvas& img, int cols, int rows) {
    const std::string ramp = " .:-=+*#%@";
    const int blockW = img.width() / cols, blockH = img.height() / rows;
    std::string text = "\033[2J\033[H";  // clear the terminal, cursor to top-left
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            double sum = 0;
            for (int y = 0; y < blockH; ++y)
                for (int x = 0; x < blockW; ++x) {
                    Color c = img.get(col * blockW + x, row * blockH + y);
                    sum += (c.r + c.g + c.b) / 3.0;
                }
            double level = sum / (blockW * blockH) / 256.0;   // 0..1
            int index = std::min(static_cast<int>(level * ramp.size()), static_cast<int>(ramp.size()) - 1);
            text += ramp[index];
        }
        text += '\n';
    }
    std::cout << text;
}

// Apply one command. W/S/A/D and Space/Enter each play one frame of the race.
// C changes the camera without moving time. Returns false when the player quits.
// F (save) is handled by the caller.
bool applyKey(PlayState& st, char key, bool advance = true) {
    st.note.clear();
    Race& r = st.race;
    if (key == 'q') return false;
    if (key == 'c') {
        st.view = static_cast<CameraMode>((st.view + 1) % VIEW_COUNT);
        return true;
    }
    if (key == 'w') {
        r.playerSpeed = std::min(PLAYER_MAX_SPEED, r.playerSpeed + PLAYER_STEP_SPEED);
    } else if (key == 's') {
        r.playerSpeed = std::max(0.0, r.playerSpeed - PLAYER_STEP_SPEED);
    } else if (key == 'a') {
        r.racers[0].targetLane = std::max(-LANE_LIMIT, r.racers[0].targetLane - PLAYER_LANE_STEP);
    } else if (key == 'd') {
        r.racers[0].targetLane = std::min(LANE_LIMIT, r.racers[0].targetLane + PLAYER_LANE_STEP);
    } else if (key != '\n' && key != ' ') {
        st.note = "Unknown key. Use W, S, A, D, C, F, Space or Q.";
        return true;
    }
    if (advance) stepRace(r);
    if (r.playerBlocked) st.note = "Contact: the car ahead holds you back.";
    return true;
}

// An arrow key arrives in the terminal as ESC [ A/B/C/D. Turn the last letter
// into the matching W/S/D/A command so arrows and letters share one code path.
char arrowToKey(char letter) {
    if (letter == 'A') return 'w';   // Up    = faster
    if (letter == 'B') return 's';   // Down  = slower
    if (letter == 'C') return 'd';   // Right = move right
    if (letter == 'D') return 'a';   // Left  = move left
    return '?';
}

// Terminal play mode. Type a key and press Enter. Each command is one frame of the race.
int runTextPlay(const World& w) {
    const int COLS = 100, ROWS = 30;      // size of the text picture
    PlayState st;
    st.race = makeRace(w.teams);
    std::string line;
    while (true) {
        printTextView(renderState(st, w), COLS, ROWS);
        std::cout << statusLine(st) << "\n";
        if (!st.note.empty()) std::cout << st.note << "\n";
        std::cout << "Up/W faster  Down/S slower  Left/Right (A/D) across track  C camera  F save image\n"
                  << "Enter = next frame   Q = quit\n> " << std::flush;

        if (!std::getline(std::cin, line)) break;
        char key = line.empty() ? '\n' : static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        if (line.size() >= 3 && line[0] == 27 && line[1] == '[') key = arrowToKey(line[2]);
        if (key == 'f') {
            renderState(st, w).saveBMP("f1_view.bmp");
            st.note = "Saved f1_view.bmp";
        } else if (!applyKey(st, key)) {
            break;
        }
    }
    return 0;
}

// Test mode (--check): run the race with no drawing and report collisions,
// gaps and the loop seam. Used to check that the cars never clip.
// Run one test scenario and print its results. When `aggressive` is set, the
// player drives at top speed and keeps swapping lanes, to test the contact rules.
int checkScenario(const World& w, const char* title, bool aggressive) {
    const int FRAMES = 5000;
    Race r = makeRace(w.teams);
    if (aggressive) r.playerSpeed = PLAYER_MAX_SPEED;
    int overlapFrames = 0;
    double minGap = 1e18;
    int playerBlockedFrames = 0;
    for (int f = 0; f < FRAMES; ++f) {
        if (aggressive && f % 60 == 0) r.racers[0].targetLane = (r.racers[0].targetLane > 0) ? -LANE_LIMIT : LANE_LIMIT;
        stepRace(r);
        if (r.playerBlocked) ++playerBlockedFrames;
        bool bad = false;
        for (int i = 0; i < static_cast<int>(r.racers.size()); ++i)
            for (int j = i + 1; j < static_cast<int>(r.racers.size()); ++j) {
                if (std::fabs(r.racers[i].lane - r.racers[j].lane) >= CAR_WID) continue;
                double gap = std::fabs(lapDelta(r.racers[i].s, r.racers[j].s)) - CAR_LEN;
                if (gap < 0) bad = true;
                minGap = std::min(minGap, gap);
            }
        if (bad) ++overlapFrames;
    }
    std::cout << "== " << title << " ==\n"
              << "Frames simulated:             " << FRAMES << "\n"
              << "Frames with overlapping cars: " << overlapFrames << "\n"
              << "Smallest bumper gap seen:     " << int(minGap) << " units\n"
              << "Safety corrections:           " << r.corrections << "\n"
              << "AI lane changes:              " << r.laneChanges << "\n"
              << "Frames the player was held:   " << playerBlockedFrames << "\n";
    return overlapFrames;
}

// Test mode (--check): run the race with no drawing and report collisions,
// gaps and the loop seam. Used to check that the cars never clip.
int runCheck(const World& w) {
    int bad = checkScenario(w, "Normal driving", false);
    bad += checkScenario(w, "Player flat out, weaving", true);

    // Loop seam for the GIF: the best starting frame and the jump it leaves.
    double seam = 0;
    int startFrame = findLoopStart(w.teams, seam);
    std::cout << "GIF start frame: " << startFrame << ", loop seam " << seam
              << " units (0 = seamless)\n";
    return bad == 0 ? 0 : 1;
}

#ifdef F1_USE_GLUT
// ---------------------------------------------------------------------------
// Window play mode (OpenGL + GLUT). Built only with -DF1_USE_GLUT.
// The picture is still drawn by our own code; OpenGL only shows the pixels
// and reports key presses. The window uses file-level state because GLUT
// callbacks are plain functions with no place to pass data.
// ---------------------------------------------------------------------------
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

const World* g_world = nullptr;
PlayState g_state;
bool g_paused = false;
int g_lastTick = 0, g_pendingMs = 0;
int g_windowWidth = OUT_W, g_windowHeight = OUT_H;
const int FRAME_MS = 40; // fixed simulation step: 25 frames per second

// Keep a 16:9 picture centred in any window size.
void resizeWindow(int width, int height) {
    g_windowWidth = std::max(1, width);
    g_windowHeight = std::max(1, height);
    glutPostRedisplay();
}

// Redraw the picture and write the status text on top.
void drawWindow() {
    Canvas img = renderState(g_state, *g_world);
    std::vector<unsigned char> rgb = img.toRGB();
    glViewport(0, 0, g_windowWidth, g_windowHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    double scale = std::min(double(g_windowWidth) / OUT_W, double(g_windowHeight) / OUT_H);
    int width = int(OUT_W * scale), height = int(OUT_H * scale);
    glViewport((g_windowWidth-width)/2, (g_windowHeight-height)/2, width, height);
    glPixelZoom(float(scale), float(scale));
    glRasterPos2i(-1, -1);                       // bottom-left of the window
    glDrawPixels(img.width(), img.height(), GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
    glPixelZoom(1, 1);

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(-0.98f, -0.93f);
    std::string status = statusLine(g_state) + (g_paused ? "  PAUSED" : "  RUNNING");
    for (char ch : status) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ch);
    glRasterPos2f(-0.98f, -0.84f);
    const std::string help = "Arrows/WASD drive C camera P/Space pause R restart Enter step F save Q quit";
    for (char ch : help) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ch);
    if (!g_state.note.empty()) {
        glRasterPos2f(-0.98f, -0.75f);
        for (char ch : g_state.note) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ch);
    }
    glutSwapBuffers();
}

// Window controls change settings; only the timer advances the running race.
void onKey(unsigned char key, int, int) {
    char k = (key == 13) ? '\n' : static_cast<char>(std::tolower(key));
    if (k == 27 || k == 'q') std::exit(0);
    if (k == 'p' || k == ' ') {
        g_paused = !g_paused;
        g_pendingMs = 0;
        g_lastTick = glutGet(GLUT_ELAPSED_TIME);
    } else if (k == 'r') {
        g_state = PlayState{};
        g_state.race = makeRace(g_world->teams);
        g_paused = true; // show the starting grid until the user resumes
        g_pendingMs = 0;
        g_lastTick = glutGet(GLUT_ELAPSED_TIME);
    } else if (k == '\n') {
        if (g_paused) stepRace(g_state.race);
    } else if (k == 'f') {
        renderState(g_state, *g_world).saveBMP("f1_view.bmp");
        g_state.note = "Saved f1_view.bmp";
    } else {
        applyKey(g_state, k, false);
    }
    glutPostRedisplay();
}

// Arrow keys: GLUT reports them separately from letters. We reuse applyKey
// without advancing, because the timer owns the race clock in window mode.
void onSpecialKey(int key, int, int) {
    char k = 0;
    if (key == GLUT_KEY_UP) k = 'w';
    else if (key == GLUT_KEY_DOWN) k = 's';
    else if (key == GLUT_KEY_LEFT) k = 'a';
    else if (key == GLUT_KEY_RIGHT) k = 'd';
    if (k == 0) return;
    applyKey(g_state, k, false);
    glutPostRedisplay();
}

// Elapsed time keeps motion independent of rendering speed. Cap catch-up after
// long stalls so restoring a window cannot skip a large section of the race.
void animateWindow(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    int elapsed = std::max(0, std::min(250, now - g_lastTick));
    g_lastTick = now;
    if (!g_paused) {
        g_pendingMs += elapsed;
        bool advanced = false;
        while (g_pendingMs >= FRAME_MS) {
            stepRace(g_state.race);
            g_pendingMs -= FRAME_MS;
            advanced = true;
        }
        if (advanced) glutPostRedisplay();
    }
    glutTimerFunc(16, animateWindow, 0);
}

int runWindow(const World& w) {
    g_world = &w;
    g_state.race = makeRace(w.teams);
    int argc = 1;
    char name[] = "f1_race";
    char* argv[] = {name, nullptr};
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(OUT_W, OUT_H);
    glutCreateWindow("F1 Race - arrows/WASD drive, P/Space pause, R restart, C camera");
    glutDisplayFunc(drawWindow);
    glutKeyboardFunc(onKey);
    glutSpecialFunc(onSpecialKey);
    glutReshapeFunc(resizeWindow);
    g_lastTick = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, animateWindow, 0);
    glutMainLoop();
    return 0;
}
#endif

int main(int argc, char** argv) {
    const int FRAMES = 100;          // one lap = 100 frames
    const int DELAY_CS = 4;          // 4/100 s per frame = 25 frames per second

    buildFrames();
    World world = buildWorld();
    const std::string mode = argc > 1 ? argv[1] : "";

    if (mode == "--check") return runCheck(world);
    if (mode == "--play") {
#ifdef F1_USE_GLUT
        return runWindow(world);
#else
        return runTextPlay(world);
#endif
    }

    // Race recording for the GIF: one frame is drawn, then the race moves on one frame.
    Canvas base = buildBase();
    double seam = 0;
    Race race = makeRace(world.teams);
    for (int f = findLoopStart(world.teams, seam); f > 0; --f) stepRace(race);
    GifWriter gif;
    for (int f = 0; f < FRAMES; ++f) {
        Canvas out = renderWorld(base, world.scenery, placesOf(race), world.cars);
        if (f == 0) {
            out.saveBMP("f1_frame.bmp");
            gif.begin("f1_race.gif", out.width(), out.height(), buildPalette(out));
        }
        gif.addFrame(out, DELAY_CS);
        stepRace(race);
        std::cout << "Frame " << (f + 1) << " / " << FRAMES << "\r" << std::flush;
    }
    gif.end();
    std::cout << "\nCreated f1_race.gif and f1_frame.bmp (" << OUT_W << " x " << OUT_H << ")\n";
    return 0;
}
