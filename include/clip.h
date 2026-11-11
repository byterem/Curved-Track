#ifndef CLIP_H
#define CLIP_H

// Cohen-Sutherland clipping against the window rectangle.
// The two end points are moved onto the window border when they are outside.
// Returns false when the whole line lies outside, so it should not be drawn.
bool clipLine(int& x1, int& y1, int& x2, int& y2);

#endif
