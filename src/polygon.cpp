#include "polygon.h"

#include <cmath>

#include "circle.h"
#include "line.h"
#include "transform.h"

// Half the car length is about 45 pixels; the wheels stick out a little.
static const int WHEEL_RADIUS = 9;

// Car body in the car's own coordinates: the car points along +x.
static const Point2D CAR_BODY[] = {
    {-42.0f, -16.0f},
    {22.0f, -16.0f},
    {40.0f, -8.0f},
    {40.0f, 8.0f},
    {22.0f, 16.0f},
    {-42.0f, 16.0f},
};
static const int BODY_COUNT = 6;

// The window (cockpit) sits inside the body.
static const Point2D CAR_WINDOW[] = {
    {-8.0f, -10.0f},
    {14.0f, -10.0f},
    {22.0f, -4.0f},
    {22.0f, 4.0f},
    {14.0f, 10.0f},
    {-8.0f, 10.0f},
};
static const int WINDOW_COUNT = 6;

// The two wheels, one on each side of the car.
static const Point2D WHEELS[] = {
    {-22.0f, -21.0f},
    {-22.0f, 21.0f},
};
static const int WHEEL_COUNT = 2;

// Draws a closed polygon by joining every point to the next one.
void drawPolygon(const Point2D* points, int count) {
    for (int i = 0; i < count; i++) {
        Point2D start = points[i];
        Point2D end = points[(i + 1) % count];
        drawLineBresenham(static_cast<int>(std::lround(start.x)),
                          static_cast<int>(std::lround(start.y)),
                          static_cast<int>(std::lround(end.x)),
                          static_cast<int>(std::lround(end.y)));
    }
}

// Moves every corner of a polygon with the car matrix, then draws it.
static void drawTransformedPolygon(const Point2D* points, int count, const Mat3 matrix) {
    Point2D moved[8];  // bigger than any polygon we use here
    for (int i = 0; i < count; i++) {
        moved[i] = applyTransform(matrix, points[i]);
    }
    drawPolygon(moved, count);
}

// Car body and window: two polygons drawn with line algorithms.
void drawCarBody(const Mat3 matrix) {
    drawTransformedPolygon(CAR_BODY, BODY_COUNT, matrix);
    drawTransformedPolygon(CAR_WINDOW, WINDOW_COUNT, matrix);
}

// Wheels are circles, so turning the car does not change their shape.
// We only move their centres and draw a midpoint circle at each one.
void drawCarWheels(const Mat3 matrix) {
    for (int i = 0; i < WHEEL_COUNT; i++) {
        Point2D center = applyTransform(matrix, WHEELS[i]);
        drawCircleMidpoint(static_cast<int>(std::lround(center.x)),
                           static_cast<int>(std::lround(center.y)),
                           WHEEL_RADIUS);
    }
}
