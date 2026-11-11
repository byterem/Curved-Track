#ifndef POLYGON_H
#define POLYGON_H

#include "common.h"

// Draws a closed outline polygon by joining the points with Bresenham lines.
// Must be called between glBegin(GL_POINTS) and glEnd().
void drawPolygon(const Point2D* points, int count);

// Draws the car body and its window using the given transformation matrix.
void drawCarBody(const Mat3 matrix);

// Draws the two wheels (midpoint circles) of the car.
void drawCarWheels(const Mat3 matrix);

#endif
