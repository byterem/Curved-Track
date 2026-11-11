#include "circle.h"

#include "line.h"

// Draws the 8 symmetric pixels of one calculated point.
static void plotEightPoints(int centerX, int centerY, int x, int y) {
    plotPixel(centerX + x, centerY + y);
    plotPixel(centerX + y, centerY + x);
    plotPixel(centerX + y, centerY - x);
    plotPixel(centerX + x, centerY - y);
    plotPixel(centerX - x, centerY - y);
    plotPixel(centerX - y, centerY - x);
    plotPixel(centerX - y, centerY + x);
    plotPixel(centerX - x, centerY + y);
}

// Midpoint circle algorithm.
// We walk from the top of the circle towards 45 degrees and use the
// decision variable to choose between the two next pixels.
void drawCircleMidpoint(int centerX, int centerY, int radius) {
    int x = 0;
    int y = radius;
    int decision = 1 - radius;

    plotEightPoints(centerX, centerY, x, y);
    while (x < y) {
        x++;
        if (decision < 0) {
            decision += 2 * x + 1;           // keep the same y
        } else {
            y--;
            decision += 2 * (x - y) + 1;     // step inwards too
        }
        plotEightPoints(centerX, centerY, x, y);
    }
}
