#include "bezier.h"

#include <cmath>

// Half the width of the road in pixels.
static const float TRACK_HALF_WIDTH = 46.0f;

// How many points we calculate for each Bezier segment. More points make
// the curves and the road edges look smoother.
static const int POINTS_PER_SEGMENT = 60;

// The anchors the track passes through, in order, forming a closed loop.
static const Point2D ANCHORS[] = {
    {200.0f, 130.0f},
    {420.0f, 110.0f},
    {650.0f, 140.0f},
    {830.0f, 260.0f},
    {860.0f, 450.0f},
    {740.0f, 590.0f},
    {520.0f, 610.0f},
    {300.0f, 580.0f},
    {170.0f, 450.0f},
    {150.0f, 280.0f},
};
static const int ANCHOR_COUNT = 10;

// One point on a cubic Bezier curve for t between 0 and 1.
static Point2D bezierPoint(Point2D p0, Point2D p1, Point2D p2, Point2D p3, float t) {
    float u = 1.0f - t;
    float a = u * u * u;
    float b = 3.0f * u * u * t;
    float c = 3.0f * u * t * t;
    float d = t * t * t;

    Point2D point;
    point.x = a * p0.x + b * p1.x + c * p2.x + d * p3.x;
    point.y = a * p0.y + b * p1.y + c * p2.y + d * p3.y;
    return point;
}

// The two inner control points of the cubic segment that goes from
// anchor i to the next anchor. They lie along the curve direction, which
// is what keeps the joined segments smooth at every anchor.
static void segmentControlPoints(int i, Point2D& control1, Point2D& control2) {
    Point2D start = ANCHORS[i];
    Point2D end = ANCHORS[(i + 1) % ANCHOR_COUNT];
    Point2D before = ANCHORS[(i - 1 + ANCHOR_COUNT) % ANCHOR_COUNT];
    Point2D after = ANCHORS[(i + 2) % ANCHOR_COUNT];

    control1.x = start.x + (end.x - before.x) / 6.0f;
    control1.y = start.y + (end.y - before.y) / 6.0f;
    control2.x = end.x - (after.x - start.x) / 6.0f;
    control2.y = end.y - (after.y - start.y) / 6.0f;
}

// Calculates all the centre line points of the closed track.
static std::vector<Point2D> sampleCentreLine() {
    std::vector<Point2D> centre;
    for (int i = 0; i < ANCHOR_COUNT; i++) {
        Point2D control1;
        Point2D control2;
        segmentControlPoints(i, control1, control2);

        Point2D start = ANCHORS[i];
        Point2D end = ANCHORS[(i + 1) % ANCHOR_COUNT];
        // We stop before t = 1 so the next segment's start is not repeated.
        for (int step = 0; step < POINTS_PER_SEGMENT; step++) {
            float t = step / static_cast<float>(POINTS_PER_SEGMENT);
            centre.push_back(bezierPoint(start, control1, control2, end, t));
        }
    }
    return centre;
}

// Builds the inner and outer road edges by moving each centre point
// sideways along the track normal, then measures how long the track is.
Track buildTrack() {
    std::vector<Point2D> centre = sampleCentreLine();
    int count = static_cast<int>(centre.size());
    Track track;
    track.length = 0.0f;

    for (int i = 0; i < count; i++) {
        // Using the point before and after gives a smooth direction.
        Point2D previous = centre[(i - 1 + count) % count];
        Point2D next = centre[(i + 1) % count];
        float dx = next.x - previous.x;
        float dy = next.y - previous.y;
        float length = std::sqrt(dx * dx + dy * dy);

        // Normal is the direction turned by 90 degrees.
        Point2D normal = {-dy / length, dx / length};

        TrackPoint point;
        point.center = centre[i];
        point.inner.x = centre[i].x - normal.x * TRACK_HALF_WIDTH;
        point.inner.y = centre[i].y - normal.y * TRACK_HALF_WIDTH;
        point.outer.x = centre[i].x + normal.x * TRACK_HALF_WIDTH;
        point.outer.y = centre[i].y + normal.y * TRACK_HALF_WIDTH;
        point.distance = track.length;
        point.angle = std::atan2(dy, dx);

        track.points.push_back(point);

        // Add the step to the next point to the total distance.
        float stepX = centre[(i + 1) % count].x - centre[i].x;
        float stepY = centre[(i + 1) % count].y - centre[i].y;
        track.length += std::sqrt(stepX * stepX + stepY * stepY);
    }
    return track;
}

// Finds the track point just behind the given distance.
int trackIndexAt(const Track& track, float distance) {
    int count = static_cast<int>(track.points.size());
    int index = 0;
    while (index + 1 < count && track.points[index + 1].distance <= distance) {
        index++;
    }
    return index;
}
