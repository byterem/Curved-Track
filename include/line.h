#ifndef LINE_H
#define LINE_H

// Draws one pixel. Must be called between glBegin(GL_POINTS) and glEnd().
void plotPixel(int x, int y);

// DDA line: steps along the line in small equal steps.
void drawLineDDA(int x1, int y1, int x2, int y2);

// Bresenham line: uses only integer maths and an error term.
void drawLineBresenham(int x1, int y1, int x2, int y2);

#endif
