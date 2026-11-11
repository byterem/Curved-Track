// ICS 3204: Track component (Person 1)
// Pure C++17, no OpenGL and no external libraries.
// Draws a closed race track made of four cubic Bezier curves into a Canvas
// and saves it as a PPM image.
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------------
// Shared types and canvas (copied from Person 2's car_body.cpp so the two
// files can be merged without changes)
// ---------------------------------------------------------------------------
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
// ---------------------------------------------------------------------------
// Track
// ---------------------------------------------------------------------------

const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 600;

const double ROAD_WIDTH = 140.0;                 // enough for Person 2's 184x76 car
const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;

const int SEGMENT_COUNT = 4;                     // four Bezier segments form the loop
const int STEPS_PER_SEGMENT = 50;                // samples of t per segment
const int CENTRE_POINT_COUNT = SEGMENT_COUNT * STEPS_PER_SEGMENT;

const int DIRECTION_SCALE = 1000;                // keeps a unit direction in ints
const int DASH_PERIOD = 8;                       // samples in one dash plus one gap
const int DASH_LENGTH = 4;                       // of which this many are drawn

const double EDGE_LINE_WIDTH = 3.0;              // thickness of a white edge line, in pixels

const Color BACKGROUND_COLOR{198, 222, 198};     // light grass green
const Color ROAD_COLOR{62, 68, 78};              // dark grey tarmac
const Color EDGE_COLOR{248, 250, 252};           // white road edges
const Color CENTRE_LINE_COLOR{250, 205, 60};     // dashed yellow line

// The four points where the segments meet, going clockwise from the top.
// They are not evenly spaced: the bottom point sits to the right and the left
// point sits high, so the circuit is a rounded oval with a long sweeping bend
// instead of a plain circle.
const Point TRACK_ANCHORS[SEGMENT_COUNT] = {
    {450, 115},   // top
    {770, 265},   // right
    {480, 478},   // bottom
    {132, 308}    // left
};

// Handle offset of each anchor: where that anchor's control point sits relative
// to the anchor, which sets the direction the track leaves it. Both segments
// that meet at an anchor use the same handle, so their control points lie on
// one straight line through the shared anchor and the join is smooth. Short
// handles give a tighter turn (here the right side), long handles a gentler
// sweep (the bottom), which is what makes the circuit interesting to drive.
const Point TRACK_HANDLES[SEGMENT_COUNT] = {
    {150, -30},
    {30, 125},
    {-155, 15},
    {-15, -145}
};

// Control points of one segment: P0 is its start, P3 its end, P1 and P2 are
// the handles that bend the curve.
void getControlPoints(int index, Point& p0, Point& p1, Point& p2, Point& p3) {
    int next = (index + 1) % SEGMENT_COUNT;
    p0 = TRACK_ANCHORS[index];
    p3 = TRACK_ANCHORS[next];
    p1 = {p0.x + TRACK_HANDLES[index].x, p0.y + TRACK_HANDLES[index].y};
    p2 = {p3.x - TRACK_HANDLES[next].x, p3.y - TRACK_HANDLES[next].y};
}

// Cubic Bezier point with the Bernstein formula:
// B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3
Point bezierPoint(Point p0, Point p1, Point p2, Point p3, double t) {
    double u = 1.0 - t;
    double b0 = u * u * u;
    double b1 = 3.0 * u * u * t;
    double b2 = 3.0 * u * t * t;
    double b3 = t * t * t;
    double x = b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x;
    double y = b0 * p0.y + b1 * p1.y + b2 * p2.y + b3 * p3.y;
    return {static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y))};
}

// Direction of the curve at sample `index`, using the derivative of the
// Bernstein formula. It is more accurate than subtracting two rounded points,
// which keeps the road edges smooth.
void sampleTangent(int index, double& dx, double& dy) {
    int segment = index / STEPS_PER_SEGMENT;
    double t = static_cast<double>(index % STEPS_PER_SEGMENT) / STEPS_PER_SEGMENT;
    Point p0, p1, p2, p3;
    getControlPoints(segment, p0, p1, p2, p3);
    double u = 1.0 - t;
    dx = 3.0 * u * u * (p1.x - p0.x) + 6.0 * u * t * (p2.x - p1.x) + 3.0 * t * t * (p3.x - p2.x);
    dy = 3.0 * u * u * (p1.y - p0.y) + 6.0 * u * t * (p2.y - p1.y) + 3.0 * t * t * (p3.y - p2.y);
}

// Ordered points all the way round the loop. Each segment is sampled at 50
// steps; t = 1 is skipped because it is the same point as t = 0 of the next
// segment.
std::vector<Point> getTrackCentreLine() {
    std::vector<Point> centre;
    centre.reserve(CENTRE_POINT_COUNT);
    for (int segment = 0; segment < SEGMENT_COUNT; ++segment) {
        Point p0, p1, p2, p3;
        getControlPoints(segment, p0, p1, p2, p3);
        for (int step = 0; step < STEPS_PER_SEGMENT; ++step) {
            double t = static_cast<double>(step) / STEPS_PER_SEGMENT;
            centre.push_back(bezierPoint(p0, p1, p2, p3, t));
        }
    }
    return centre;
}

// Direction of travel at a centre-line point, as a unit vector scaled by
// DIRECTION_SCALE (so it can stay an integer Point). Turn it into an angle
// with atan2(direction.y, direction.x) when rotating the car in CP2.
Point getTrackDirection(int index) {
    int wrapped = ((index % CENTRE_POINT_COUNT) + CENTRE_POINT_COUNT) % CENTRE_POINT_COUNT;
    double dx, dy;
    sampleTangent(wrapped, dx, dy);
    double length = std::sqrt(dx * dx + dy * dy);
    return {static_cast<int>(std::lround(dx / length * DIRECTION_SCALE)),
            static_cast<int>(std::lround(dy / length * DIRECTION_SCALE))};
}

// Points running parallel to the centre line at a given distance from it.
// Turning the direction by 90 degrees gives the normal (-dy, dx); a positive
// distance moves inside the loop, a negative one moves outside.
std::vector<Point> offsetCentreLine(const std::vector<Point>& centre, double distance) {
    std::vector<Point> edge(centre.size());
    for (int i = 0; i < CENTRE_POINT_COUNT; ++i) {
        double dx, dy;
        sampleTangent(i, dx, dy);
        double length = std::sqrt(dx * dx + dy * dy);
        double nx = -dy / length;
        double ny = dx / length;
        edge[i] = {static_cast<int>(std::lround(centre[i].x + nx * distance)),
                   static_cast<int>(std::lround(centre[i].y + ny * distance))};
    }
    return edge;
}

// The road is filled slice by slice: each slice between two neighbouring
// samples is a quad built from the two edge points of both samples.
void drawRoadSurface(Canvas& canvas, const std::vector<Point>& inner,
                     const std::vector<Point>& outer, Color color) {
    for (int i = 0; i < CENTRE_POINT_COUNT; ++i) {
        int next = (i + 1) % CENTRE_POINT_COUNT;     // wraps around to close the loop
        fillPolygon(canvas, {inner[i], outer[i], outer[next], inner[next]}, color);
    }
}

// A white band along one road edge. It is filled with the same quad slices as
// the road, starting at the same boundary (distanceA) and going EDGE_LINE_WIDTH
// towards the middle of the road (distanceB). Sharing that first boundary keeps
// the band exactly on the road edge, and DDA lines along both sides of the band
// give it a crisp outline.
void drawEdgeBand(Canvas& canvas, const std::vector<Point>& centre,
                  double distanceA, double distanceB, Color color) {
    std::vector<Point> a = offsetCentreLine(centre, distanceA);
    std::vector<Point> b = offsetCentreLine(centre, distanceB);
    for (int i = 0; i < CENTRE_POINT_COUNT; ++i) {
        int next = (i + 1) % CENTRE_POINT_COUNT;
        fillPolygon(canvas, {a[i], b[i], b[next], a[next]}, color);
        drawLineDDA(canvas, a[i], a[next], color);
        drawLineDDA(canvas, b[i], b[next], color);
    }
}

// Dashed centre line: DASH_LENGTH samples are drawn, the rest of the period
// is left empty.
void drawCentreLine(Canvas& canvas, const std::vector<Point>& centre, Color color) {
    for (int i = 0; i < CENTRE_POINT_COUNT; ++i) {
        if (i % DASH_PERIOD >= DASH_LENGTH) continue;
        int next = (i + 1) % CENTRE_POINT_COUNT;
        drawLineDDA(canvas, centre[i], centre[next], color);
    }
}

// Draws the whole track: road surface first, then the edges and the centre
// line on top of it.
void drawTrack(Canvas& canvas) {
    std::vector<Point> centre = getTrackCentreLine();
    std::vector<Point> inner = offsetCentreLine(centre, HALF_ROAD_WIDTH);
    std::vector<Point> outer = offsetCentreLine(centre, -HALF_ROAD_WIDTH);
    drawRoadSurface(canvas, inner, outer, ROAD_COLOR);
    drawEdgeBand(canvas, centre, HALF_ROAD_WIDTH, HALF_ROAD_WIDTH - EDGE_LINE_WIDTH, EDGE_COLOR);
    drawEdgeBand(canvas, centre, -HALF_ROAD_WIDTH, -HALF_ROAD_WIDTH + EDGE_LINE_WIDTH, EDGE_COLOR);
    drawCentreLine(canvas, centre, CENTRE_LINE_COLOR);
}

int main() {
    Canvas canvas(SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
    drawTrack(canvas);
    canvas.savePPM("track.ppm");
    std::cout << "Created track.ppm (" << SCREEN_WIDTH << " x " << SCREEN_HEIGHT << ")\n";
    return 0;
}
