# Track – Person 1 (CP1)

Pure C++ implementation of a closed 2D race track built from **four cubic Bézier curves**. The road surface is filled with **scanline polygon fill** and both edges are drawn with the **DDA line drawing algorithm**. No OpenGL, no animation, and no external graphics library.

## Build and test

```bash
g++ -std=c++17 -Wall -Wextra track.cpp -o track
./track
```

On Windows with MinGW, build the same way and run `track.exe`.

This creates `track.ppm` (a binary PPM image, 900 x 600). Open it in an image viewer that supports PPM, or convert it to PNG for a screenshot in `docs/`:

```bash
convert track.ppm docs/track.png
# or, with Python: python3 -c "from PIL import Image; Image.open('track.ppm').save('docs/track.png')"
```

`track.ppm` is generated output and is not committed as source. `docs/track.png` is the screenshot.

## What it draws

- A closed circuit that looks like a rounded oval with one long sweeping bend, made of four Bézier segments joined end to end.
- A road **140 pixels wide**, wide enough for Person 2's car (184 x 76), so the car has room to sit on the road in CP2.
- A dark grey road surface, a white 3-pixel band along the inner and outer edges, and a dashed yellow centre line, all on a light grass-green background.

## Graphics techniques

- **Bézier curves:** each segment uses the Bernstein form `B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3`, sampled at 50 steps of `t` from 0 to 1. Four segments x 50 steps = **200 ordered centre points** around the loop (the duplicate point where two segments meet is sampled once).
- **Smooth joins:** both segments that meet at an anchor share the same handle offset, so the incoming control point, the shared anchor and the outgoing control point lie on one straight line. The measured join angle is 0 degrees, so there are no kinks.
- **Track width:** the direction of the curve is taken from the derivative `B'(t) = 3(1-t)^2 (P1-P0) + 6(1-t)t (P2-P1) + 3t^2 (P3-P2)`. Rotating it by 90 degrees gives the normal `(-dy, dx)`, and offsetting each centre point by +/- 70 pixels gives the inner and outer edge points. The tightest curve has a radius of about 107 pixels, so the road never pinches shut or crosses itself.
- **Scanline filling:** the road is filled one slice at a time; each slice is the quad `(inner[i], outer[i], outer[i+1], inner[i+1])`, filled by `fillPolygon`.
- **DDA:** draws the edge lines by stepping along the line and rounding each pixel position.
- **Edges that line up:** a thin band of white pixels would be hard to see from a single DDA line, and a line and a filled area round to different pixels. So each edge is a filled band that starts on the exact road boundary (the same slice vertices the road uses) and reaches 3 pixels towards the middle of the road, with a DDA line drawn along both sides of the band.
- **Boundary safety:** `Canvas::plot` ignores pixels outside the image bounds (this is not Cohen-Sutherland line clipping; Person 4 adds that later).

## Integration contract

The names below match Person 2's `car_body.cpp` (`Color`, `Point`, `Canvas`, `drawLineDDA`, `fillPolygon`), so the files can be compiled together without changes.

```cpp
Point bezierPoint(Point p0, Point p1, Point p2, Point p3, double t);
void drawTrack(Canvas& canvas);
std::vector<Point> getTrackCentreLine();
Point getTrackDirection(int index);
```

- `bezierPoint(p0, p1, p2, p3, t)` – the point on one cubic Bézier segment at `t` in `[0, 1]`.
- `drawTrack(canvas)` – draws the whole track (surface, edges, centre line) onto an existing canvas.
- `getTrackCentreLine()` – 200 ordered points around the loop, starting at the top of the circuit and going clockwise. Index `i + 1` is the next point along the road, and `i = 199` wraps back to `i = 0`, so the loop never ends. For CP2, give the car a position index and increase it by a fixed step each frame.
- `getTrackDirection(index)` – the unit direction of travel at that centre point, scaled by 1000 so it can stay an integer `Point`. Use `atan2(direction.y, direction.x)` for the car's rotation angle. The index wraps, so any integer is accepted.

Suggested CP2 starting point: `centre = getTrackCentreLine()`, place the car with its centre at `centre[index]`, and rotate it by `atan2(getTrackDirection(index).y, getTrackDirection(index).x)`.

## CP2 plan

Use `getTrackCentreLine()` for translation and `getTrackDirection()` for rotation, then apply a homogeneous translation and rotation matrix to the car's local vertices (the same transform for the body, the windows and Person 3's wheels). The track itself does not change in CP2, so `drawTrack` can be called every frame to redraw the background.
