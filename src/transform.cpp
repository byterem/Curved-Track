#include "transform.h"

#include <cmath>

// Identity matrix: a point stays where it is.
void setIdentity(Mat3 matrix) {
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            matrix[row][column] = (row == column) ? 1.0f : 0.0f;
        }
    }
}

// Translation matrix. The last column holds the movement.
void setTranslation(Mat3 matrix, float tx, float ty) {
    setIdentity(matrix);
    matrix[0][2] = tx;
    matrix[1][2] = ty;
}

// Rotation matrix around the origin.
void setRotation(Mat3 matrix, float angle) {
    setIdentity(matrix);
    float cosine = std::cos(angle);
    float sine = std::sin(angle);
    matrix[0][0] = cosine;
    matrix[0][1] = -sine;
    matrix[1][0] = sine;
    matrix[1][1] = cosine;
}

// out = a * b. Multiplying the matrices joins two transformations.
void multiplyMatrices(Mat3 out, const Mat3 a, const Mat3 b) {
    Mat3 result;
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            result[row][column] = a[row][0] * b[0][column] +
                                  a[row][1] * b[1][column] +
                                  a[row][2] * b[2][column];
        }
    }

    // Copy at the end, because out may be one of the inputs.
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            out[row][column] = result[row][column];
        }
    }
}

// Multiplies the homogeneous point (x, y, 1) by the matrix.
Point2D applyTransform(const Mat3 matrix, Point2D point) {
    Point2D moved;
    moved.x = matrix[0][0] * point.x + matrix[0][1] * point.y + matrix[0][2];
    moved.y = matrix[1][0] * point.x + matrix[1][1] * point.y + matrix[1][2];
    return moved;
}
