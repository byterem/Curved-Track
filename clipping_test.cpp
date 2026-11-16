#include "clipping.h"
#include <cassert>
#include <iostream>

void check(double x0, double y0, double x1, double y1, bool visible,
           double a, double b, double c, double d) {
    assert(cohenSutherlandClip(x0, y0, x1, y1, {0, 0, 10, 10}) == visible);
    if (visible) {
        assert(std::abs(x0-a) < 1e-9 && std::abs(y0-b) < 1e-9);
        assert(std::abs(x1-c) < 1e-9 && std::abs(y1-d) < 1e-9);
    }
}

int main() {
    check(2, 3, 8, 9, true, 2, 3, 8, 9); // inside
    check(-5, 5, 15, 5, true, 0, 5, 10, 5); // horizontal
    check(5, -5, 5, 15, true, 5, 0, 5, 10); // vertical
    check(-5, -5, 15, 15, true, 0, 0, 10, 10); // corner crossing
    check(15, 15, -5, -5, true, 10, 10, 0, 0); // reversed
    check(-5, 0, 15, 0, true, 0, 0, 10, 0); // boundary
    check(5, 5, 5, 5, true, 5, 5, 5, 5); // single point
    check(-1, 5, -1, 5, false, 0, 0, 0, 0);
    check(-2, 2, 2, -3, false, 0, 0, 0, 0); // different outcodes, misses corner
    check(-5, 12, 15, 12, false, 0, 0, 0, 0);
    std::cout << "10 clipping cases passed\n";
}
