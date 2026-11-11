#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "common.h"

// Fills the matrix with the identity matrix (does nothing to a point).
void setIdentity(Mat3 matrix);

// Translation matrix: moves a point by (tx, ty).
void setTranslation(Mat3 matrix, float tx, float ty);

// Rotation matrix: turns a point around the origin by angle in radians.
void setRotation(Mat3 matrix, float angle);

// out = a * b. Applying out to a point means applying b first, then a.
void multiplyMatrices(Mat3 out, const Mat3 a, const Mat3 b);

// Applies a transformation matrix to one point.
Point2D applyTransform(const Mat3 matrix, Point2D point);

#endif
