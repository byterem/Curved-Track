#ifndef COMMON_H
#define COMMON_H

// Window size in pixels. The scene is drawn in this same coordinate range,
// with (0,0) at the bottom-left corner.
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 720;

// A point in 2D space (also used as a direction vector).
struct Point2D {
    float x;
    float y;
};

// A 3x3 homogeneous transformation matrix, row by row: m[row][column].
// A point is stored as the column vector (x, y, 1).
typedef float Mat3[3][3];

#endif
