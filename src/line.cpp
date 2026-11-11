#include <GL/glut.h>

#include <algorithm>
#include <cmath>

#include "clip.h"
#include "common.h"
#include "line.h"

// Puts a single pixel on the screen. Every shape we draw ends up here,
// so this is also the last place where we can refuse to draw off screen.
void plotPixel(int x, int y) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }
    glVertex2i(x, y);
}

// DDA line: find the number of steps from the longer side, then move
// a little in x and y on every step.
void drawLineDDA(int x1, int y1, int x2, int y2) {
    if (!clipLine(x1, y1, x2, y2)) {
        return;  // the line is completely outside the window
    }

    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) {
        plotPixel(x1, y1);
        return;
    }

    float xStep = dx / static_cast<float>(steps);
    float yStep = dy / static_cast<float>(steps);
    float x = static_cast<float>(x1);
    float y = static_cast<float>(y1);

    for (int i = 0; i <= steps; i++) {
        plotPixel(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)));
        x += xStep;
        y += yStep;
    }
}

// Bresenham line: integer only. The error term decides when to step in
// the second direction, so the pixels follow the real line closely.
void drawLineBresenham(int x1, int y1, int x2, int y2) {
    if (!clipLine(x1, y1, x2, y2)) {
        return;  // the line is completely outside the window
    }

    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int stepX = (x1 < x2) ? 1 : -1;
    int stepY = (y1 < y2) ? 1 : -1;
    int error = dx - dy;

    while (true) {
        plotPixel(x1, y1);
        if (x1 == x2 && y1 == y2) {
            break;
        }

        int twiceError = 2 * error;
        if (twiceError > -dy) {
            error -= dy;
            x1 += stepX;
        }
        if (twiceError < dx) {
            error += dx;
            y1 += stepY;
        }
    }
}
