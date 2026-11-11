// ICS 3204: Car body component (Person 2)
// Pure C++17; produces a PPM image, no OpenGL or external libraries.
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

struct Color { unsigned char r, g, b; };
struct Point { int x, y; };

class Canvas {
public:
    Canvas(int width, int height, Color background)
        : width_(width), height_(height), pixels_(width * height, background) {}

    void plot(int x, int y, Color color) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_)
            pixels_[y * width_ + x] = color;
    }

    void savePPM(const char* filename) const {
        std::ofstream out(filename, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot create output image");
        out << "P6\n" << width_ << " " << height_ << "\n255\n";
        for (const Color& c : pixels_) {
            out.put(static_cast<char>(c.r));
            out.put(static_cast<char>(c.g));
            out.put(static_cast<char>(c.b));
        }
    }
private:
    int width_, height_;
    std::vector<Color> pixels_;
};

// Digital Differential Analyzer (DDA): calculates intermediate pixels along a line.
void drawLineDDA(Canvas& canvas, Point a, Point b, Color color) {
    int dx = b.x - a.x, dy = b.y - a.y;
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) { canvas.plot(a.x, a.y, color); return; }
    double x = a.x, y = a.y;
    double xStep = static_cast<double>(dx) / steps;
    double yStep = static_cast<double>(dy) / steps;
    for (int i = 0; i <= steps; ++i) {
        canvas.plot(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), color);
        x += xStep; y += yStep;
    }
}

// Scanline fill makes the polygons visible, instead of drawing outlines only.
void fillPolygon(Canvas& canvas, const std::vector<Point>& vertices, Color color) {
    if (vertices.size() < 3) return;
    int yMin = vertices[0].y, yMax = vertices[0].y;
    for (Point p : vertices) { yMin = std::min(yMin, p.y); yMax = std::max(yMax, p.y); }
    for (int y = yMin; y <= yMax; ++y) {
        std::vector<double> intersections;
        for (size_t i = 0; i < vertices.size(); ++i) {
            Point a = vertices[i], b = vertices[(i + 1) % vertices.size()];
            if ((a.y <= y && y < b.y) || (b.y <= y && y < a.y)) {
                double x = a.x + static_cast<double>(y - a.y) * (b.x - a.x) / (b.y - a.y);
                intersections.push_back(x);
            }
        }
        std::sort(intersections.begin(), intersections.end());
        for (size_t i = 0; i + 1 < intersections.size(); i += 2)
            for (int x = static_cast<int>(std::ceil(intersections[i]));
                 x <= static_cast<int>(std::floor(intersections[i + 1])); ++x)
                canvas.plot(x, y, color);
    }
}

void drawPolygon(Canvas& canvas, const std::vector<Point>& vertices,
                 Color fill, Color outline) {
    fillPolygon(canvas, vertices, fill);
    for (size_t i = 0; i < vertices.size(); ++i)
        drawLineDDA(canvas, vertices[i], vertices[(i + 1) % vertices.size()], outline);
}

// Draw only the body and windows. Wheels belong to Person 3.
// Coordinates are local to (originX, originY), which is useful for future movement.
void drawCarBody(Canvas& canvas, int originX, int originY) {
    auto poly = [=](std::initializer_list<Point> local) {
        std::vector<Point> result;
        for (Point p : local) result.push_back({p.x + originX, p.y + originY});
        return result;
    };
    const Color outline{28, 37, 54};
    const Color body{225, 60, 64};
    const Color glass{142, 213, 236};
    const Color trim{236, 244, 250};

    // Body silhouette: bonnet, cabin roof, rear, and lower chassis.
    drawPolygon(canvas, poly({{0, 54}, {22, 49}, {49, 18}, {131, 18},
                              {157, 49}, {184, 54}, {184, 94}, {0, 94}}), body, outline);
    // Window polygons.
    drawPolygon(canvas, poly({{53, 24}, {89, 24}, {89, 48}, {32, 48}}), glass, outline);
    drawPolygon(canvas, poly({{96, 24}, {127, 24}, {146, 48}, {96, 48}}), glass, outline);
    // Door seam and handles: DDA lines.
    drawLineDDA(canvas, {originX + 93, originY + 50},
                        {originX + 93, originY + 88}, outline);
    drawLineDDA(canvas, {originX + 100, originY + 58},
                        {originX + 114, originY + 58}, trim);
    // Front and rear lights.
    drawPolygon(canvas, poly({{176, 58}, {184, 60}, {184, 69}, {176, 69}}),
                {255, 221, 115}, outline);
    drawPolygon(canvas, poly({{0, 60}, {8, 58}, {8, 70}, {0, 70}}),
                {255, 130, 108}, outline);
}

int main() {
    Canvas canvas(640, 320, {243, 247, 251});
    drawCarBody(canvas, 220, 100);
    canvas.savePPM("car_body.ppm");
    std::cout << "Created car_body.ppm (640 x 320)\n";
    return 0;
}
