#ifndef CURVED_TRACK_CLIPPING_H
#define CURVED_TRACK_CLIPPING_H

#include <cmath>

struct ClipWindow { double xmin, ymin, xmax, ymax; };

// Four independent bits describe which window edges a point lies beyond.
inline int computeCode(double x, double y, const ClipWindow& w) {
    int code = 0;
    if (x < w.xmin) code |= 1;
    if (x > w.xmax) code |= 2;
    if (y < w.ymin) code |= 4;
    if (y > w.ymax) code |= 8;
    return code;
}

// Clip in place; accept boundary points and reject invalid input windows.
inline bool cohenSutherlandClip(double& x0, double& y0, double& x1,
                                double& y1, const ClipWindow& w) {
    if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) ||
        !std::isfinite(y1) || !std::isfinite(w.xmin) || !std::isfinite(w.ymin) ||
        !std::isfinite(w.xmax) || !std::isfinite(w.ymax) ||
        w.xmin > w.xmax || w.ymin > w.ymax) return false;
    int a = computeCode(x0, y0, w), b = computeCode(x1, y1, w);
    while (true) {
        if (!(a | b)) return true;
        if (a & b) return false;
        int outside = a ? a : b;
        double x, y;
        if (outside & 8) {
            x = x0 + (x1 - x0) * (w.ymax - y0) / (y1 - y0); y = w.ymax;
        } else if (outside & 4) {
            x = x0 + (x1 - x0) * (w.ymin - y0) / (y1 - y0); y = w.ymin;
        } else if (outside & 2) {
            y = y0 + (y1 - y0) * (w.xmax - x0) / (x1 - x0); x = w.xmax;
        } else {
            y = y0 + (y1 - y0) * (w.xmin - x0) / (x1 - x0); x = w.xmin;
        }
        if (outside == a) { x0 = x; y0 = y; a = computeCode(x0, y0, w); }
        else { x1 = x; y1 = y; b = computeCode(x1, y1, w); }
    }
}

#endif
