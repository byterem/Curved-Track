#include <cmath>

#include "clip.h"
#include "common.h"

// Region codes: one bit per window border a point is outside of.
static const int INSIDE = 0;  // 0000
static const int LEFT = 1;    // 0001
static const int RIGHT = 2;   // 0010
static const int BOTTOM = 4;  // 0100
static const int TOP = 8;     // 1000

// Works out the region code of a point.
static int computeCode(int x, int y) {
    int code = INSIDE;
    if (x < 0) {
        code |= LEFT;
    } else if (x >= SCREEN_WIDTH) {
        code |= RIGHT;
    }
    if (y < 0) {
        code |= BOTTOM;
    } else if (y >= SCREEN_HEIGHT) {
        code |= TOP;
    }
    return code;
}

// Cohen-Sutherland clipping.
// Lines that are already inside are kept, lines on the same outside side
// are thrown away, and the rest are cut at one border and checked again.
bool clipLine(int& x1, int& y1, int& x2, int& y2) {
    float firstX = static_cast<float>(x1);
    float firstY = static_cast<float>(y1);
    float secondX = static_cast<float>(x2);
    float secondY = static_cast<float>(y2);

    int firstCode = computeCode(x1, y1);
    int secondCode = computeCode(x2, y2);

    while (true) {
        if ((firstCode | secondCode) == 0) {
            break;  // both points inside: keep the whole line
        }
        if ((firstCode & secondCode) != 0) {
            return false;  // both points outside the same border
        }

        // Pick the point that is outside and cut it to a window border.
        int outsideCode = (firstCode != 0) ? firstCode : secondCode;
        float x = 0.0f;
        float y = 0.0f;

        if (outsideCode & TOP) {
            y = static_cast<float>(SCREEN_HEIGHT - 1);
            x = firstX + (secondX - firstX) * (y - firstY) / (secondY - firstY);
        } else if (outsideCode & BOTTOM) {
            y = 0.0f;
            x = firstX + (secondX - firstX) * (y - firstY) / (secondY - firstY);
        } else if (outsideCode & RIGHT) {
            x = static_cast<float>(SCREEN_WIDTH - 1);
            y = firstY + (secondY - firstY) * (x - firstX) / (secondX - firstX);
        } else {
            x = 0.0f;
            y = firstY + (secondY - firstY) * (x - firstX) / (secondX - firstX);
        }

        // Store the new point and work out its code again.
        if (outsideCode == firstCode) {
            firstX = x;
            firstY = y;
            firstCode = computeCode(static_cast<int>(std::lround(firstX)),
                                    static_cast<int>(std::lround(firstY)));
        } else {
            secondX = x;
            secondY = y;
            secondCode = computeCode(static_cast<int>(std::lround(secondX)),
                                     static_cast<int>(std::lround(secondY)));
        }
    }

    x1 = static_cast<int>(std::lround(firstX));
    y1 = static_cast<int>(std::lround(firstY));
    x2 = static_cast<int>(std::lround(secondX));
    y2 = static_cast<int>(std::lround(secondY));
    return true;
}
