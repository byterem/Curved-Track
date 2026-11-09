# Car Body – Person 2 (CP1)

Pure C++ implementation of a stylized 2D racing car body with **polygons** and the **DDA line drawing algorithm**. A scanline polygon fill is used so shapes are visibly filled. No OpenGL, no animation, and no external graphics library.

## Build and test

```bash
g++ -std=c++17 -Wall -Wextra car_body.cpp -o car_body
./car_body
```

On Windows with MinGW, build the same way and run `car_body.exe`.

This creates `car_body.ppm` (a binary PPM image). Open it in an image viewer that supports PPM, or convert it to PNG for a screenshot. The generated PPM is not meant to be committed as source code.

## Graphics techniques

- **Polygons:** an eight-vertex car silhouette, two window polygons and light polygons.
- **DDA:** determines individual pixel locations for the edges, door seam and handle.
- **Scanline filling:** colors the interiors of the polygons.
- **Boundary safety:** `Canvas::plot` ignores pixels outside the image bounds (this is not Cohen–Sutherland line clipping).

## Integration contract

`drawCarBody(Canvas& canvas, int originX, int originY)` draws the body at a configurable location, in a local coordinate system. Person 3 adds the wheels using a separate midpoint-circle function. Before integration, the team should agree on a shared `Canvas` interface and consistent scale. The body is 184 pixels wide and 76 pixels tall, from local y=18 to 94. Suggested wheel centers: (45, 94) and (144, 94), radius 20.

## Evidence for checkpoint

1. Screenshot showing the generated car body image.
2. Screenshot showing successful compilation and the output message.
3. Commit containing this source file and README.

## CP2 plan

Apply translation and rotation to local vertices before drawing. To rotate the entire car including wheels, share the same transform with Person 3, instead of moving individual components independently.
