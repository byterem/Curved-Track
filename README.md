# F1 Racing on a Curved Track

## Description
A pure C++17 graphics project for ICS 3204 – Computer Graphics.
The active project is `f1_race.cpp`: open-wheel F1 cars drive around a closed
Bezier circuit, turn with the track and render through a perspective camera.
It exports a looping GIF using pixels calculated without external graphics
libraries or OpenGL. The original static 2D scene in `main.cpp` is retired from
active development and retained only as a historical component reference.

## Graphics Concepts Used
| Concept | Used For |
|---|---|
| DDA Line | Track edges and car details (Bresenham is not implemented) |
| Midpoint Circle Algorithm | Filled scenery discs and helmet slices |
| Polygons | F1 model slices, road and scenery |
| Bézier Curves | Shape of the track |
| Translation | F1 world placement |
| Rotation | Orienting the F1 model along the track tangent |
| Cohen-Sutherland Clipping | Keeping lines inside the screen |

## Project Progress
- [x] CP1: Basic components integrated with clipping
- [x] F1 animation: Closed-lap movement, tangent orientation and GIF output
- [ ] CP2: Explicit homogeneous transformation matrices for the F1 model
- [ ] CP3: OpenGL integration and animation
- [ ] CP4: Final demo

## Technologies
- C++
- OpenGL (from CP3)

## How to Run
1. Clone the repository:
   git clone https://github.com/byterem/Curved-Track.git
2. Open the project in your C++ editor
3. Compile: `g++ -std=c++17 -O2 f1_race.cpp -o f1_race`
4. Run `./f1_race` (Windows: `.\f1_race.exe`) to create `f1_race.gif` and `f1_frame.bmp`.

See [INTEGRATION_README.md](INTEGRATION_README.md) for clipping checks and the
F1 integration details. Compile each program separately.

### F1 animation and clipping checks

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic f1_race.cpp -o f1_race.exe
.\f1_race.exe
g++ -std=c++17 -Wall -Wextra -pedantic clipping_test.cpp -o clipping_test.exe
.\clipping_test.exe
```

The F1 program writes `f1_race.gif` (800 x 450, 100 frames, four-second repeat)
and `f1_frame.bmp`. Open the GIF in a browser to see the animation.
The historical 2D program writes `scene.ppm`; it is not the active submission path.
On Linux/macOS, omit `.exe` and run `./f1_race` or `./clipping_test`.
Do not compile all `.cpp` files together: the standalone components have duplicate
types, functions and entry points. Generated executables and images are ignored.

## Explanation using our lecture notes

References use the topic titles and PDF page numbers in the supplied
**Computer Graphics** notes compilation. The lecture files are reference
material and are not redistributed here.

| Lecture reference | Concept | Implementation |
| --- | --- | --- |
| Topic 2: Colour & Colour Models, p. 15 | RGB channels | `Color`, `Canvas` |
| Topic 4: Basic Geometric Primitives, pp. 9-12 | DDA | `drawLineDDA` |
| Topic 5: Basic Geometric Primitives Part 2, pp. 3-7 | Midpoint circle, symmetry | F1 `fillCircleMidpoint`; historical outline routine in `main.cpp` |
| Topic 5, pp. 11-12, 16 | Line clipping | `clipping.h` |
| Topic 6: Polygons and Polygon Filling, pp. 20-22 | Scanline filling | `fillPolygon` |
| Topic 7: Curves, pp. 4-7, 9-12 | Parametric and Bezier curves | F1 `buildFrames`, `frameAt` |
| Topic 8: 2D & 3D Transformations, pp. 4-10 | Translation and rotation | F1 orientation and placement equations |
| Topic 9: Projections, pp. 16-17 | Perspective | F1 `project` |
| Topic 10: Photo Realistic Rendering Techniques, p. 19 | Supersampling | F1 `Canvas::downsample` |

### Pixels, lines and circles

`Color` stores red, green and blue as bytes from 0 to 255. `Canvas::plot` writes
one pixel at array index `y * width + x`. The origin is the top-left corner:
x increases rightwards and y increases downwards. Bounds checks prevent writes
outside the buffer; they do not replace geometric clipping.

DDA calculates `dx=x1-x0`, `dy=y1-y0`, and `steps=max(abs(dx),abs(dy))`.
Starting at the first endpoint, it adds `dx/steps` and `dy/steps`, rounding each
position to choose a pixel. A zero-length line plots one point without division.
For example, `(2,2)` to `(8,5)` has six steps and increments `(1,0.5)`, matching
Topic 4, p. 11. In the F1 program, DDA draws model-slice polygon outlines.
Bresenham is the integer alternative taught in class; this code uses DDA.

The midpoint circle starts at `(0,r)` with decision value `p=1-r` and uses its
sign to choose the next pixel. One octant produces eight reflected positions:
`(xc +/- x,yc +/- y)` and `(xc +/- y,yc +/- x)`. The F1 filled-disc implementation
records the widest span for each symmetric row and fills each row once, avoiding
repeated blending. These discs draw scenery and helmet slices. F1 tyres use
model cross-sections rather than the retired side-view midpoint wheel routine.
The code increments x before updating p, using `2*x+1`, or `2*(x-y)+1` after
decrementing y. These are equivalent to the notes' old-coordinate updates
`2*x+3` and `2*(x-y)+5`; the update order explains the different constants.

### Polygons and the curved track

For each scanline, `fillPolygon` finds intersections with the polygon edges,
sorts the x coordinates, pairs them and fills between each pair. Horizontal
edges are excluded from intersection calculations. A half-open y interval avoids
counting shared vertices twice. DDA outlines the top polygons of F1 model slices.
The road surface uses the same filling method on quadrilateral strips.

Four cubic Bezier segments share endpoints to close the track. Each uses:

```text
B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t)t^2 P2 + t^3 P3
0 <= t <= 1
```

P0 and P3 are endpoints; P1 and P2 control the bend. Shared handle directions
produce smooth joins. The F1 track has 120 samples per segment, or 480
centre points. Offsetting along the normal by +/-70 world units makes a 140-unit-wide
road. Adjacent edge points form the filled strips.
Topic 7, p. 4 explains that equal changes in t need not cover equal distances.
The F1 program therefore accumulates distances between samples. `frameAt` finds
a requested distance in that table and interpolates position and tangent,
providing an approximate arc-length lookup.

### Person 4: Cohen-Sutherland clipping

The window is `[0,width-1] x [0,height-1]`. `computeCode` gives each endpoint a
four-bit outcode: left=1, right=2, y below the minimum=4, y above the maximum=8.
Zero means inside; boundaries count as inside. On the downward-y image grid,
bit 4 is visually above the screen and bit 8 below it. The numerical comparisons
remain consistent with the lecture's Cartesian bit convention.

`cohenSutherlandClip` repeats these decisions before DDA rasterization:

1. `(code0 | code1) == 0`: accept the remaining segment.
2. `(code0 & code1) != 0`: reject it because both endpoints share an outside edge.
3. Otherwise intersect an outside endpoint with a window edge, replace it,
   recalculate its outcode and repeat.

For a horizontal boundary yb, the intersection is
`x=x0+(x1-x0)*(yb-y0)/(y1-y0)`, `y=yb`.
For a vertical boundary xb, use
`y=y0+(y1-y0)*(xb-x0)/(x1-x0)`, `x=xb`.
For example, clipping `(-5,5)` to `(15,5)` against `[0,10] x [0,10]` gives
`(0,5)` to `(10,5)`; a horizontal line at y=12 is rejected entirely.

Fully invisible lines skip rasterization and crossing lines draw only the visible
portion. This clips every DDA call in the F1 program.
Polygon fills and directly plotted circle outlines still use Canvas bounds checks.
This is not polygon clipping or 3D near-plane clipping. Wu's algorithm smooths
rasterized lines; it is not a line clipping algorithm (see Topic 5, pp. 15-16).

### Transformations and movement

In the F1 program, a model point `(u,v)` is oriented along the unit tangent
`(tx,ty)` and placed at track position `(X,Y)`:

```text
worldX = X + u*tx - v*ty
worldY = Y + u*ty + v*tx
```

Putting `tx=cos(theta)` and `ty=sin(theta)` gives rotation followed by translation.
In Topic 8's homogeneous coordinate notation, the equivalent operation is:

```text
[x']   [cos(theta) -sin(theta) X] [u]
[y'] = [sin(theta)  cos(theta) Y] [v]
[1 ]   [0           0         1] [1]
```

This matrix explains the operation; the program evaluates the coordinate equations
directly. The remaining CP2 implementation task is to express the F1 model's
rotation and translation using reusable homogeneous matrices, then verify that
placement and heading match the existing animation over a complete lap.
The original 2D car does not need further animation work. On the image grid, positive rotation
appears clockwise because y points downwards.

Each F1 car moves one lap over 100 frames, facing the tangent. Periodic lane and
speed variations repeat at the loop boundary. Four centiseconds per frame gives
25 fps. These variations are illustrative and do not implement collision physics.

### Perspective, smooth edges and output

`project` moves world coordinates relative to the camera and its pitch, then uses
`screenX=cx+focal*xCamera/depth` and `screenY=cy-focal*yCamera/depth`.
Dividing by depth makes distant cars smaller, as in Topic 9. The fixed scene is
in front of the camera; there is no general near-plane polygon clipper.

The F1 scene is rendered at 2400 x 1350. Averaging each 3 x 3 block produces
800 x 450 pixels and reduces jagged edges. This is supersampling from Topic 10.
The cars use horizontal model slices; cars and scenery are ordered approximately
from far to near. There is no per-pixel depth buffer, so complex overlaps can
show ordering errors. Fog, colour shading and painted shadows are illustrative
effects rather than physically based light transport.

The palette builder, ordered dithering and LZW GIF writer are supporting output
code, not algorithms specifically demonstrated in the supplied lectures.
Perspective and supersampling were taught and are part of the active F1 project.
OpenGL remains deferred until CP3. Existing movement and orientation do not by
themselves establish completion of the explicit CP2 matrix implementation.

## Contributions and verification

Person 1's track comes from `track.cpp` on main. The original body and Person 3's
wheels come from `feature/wheels`; `main.cpp` combines these with one Canvas and
one entry point. Person 4 adds `clipping.h`, checks and integration documentation.
The standalone component files and integrated 2D main are preserved as historical
references; the current run command and development target are `f1_race.cpp`.

`f1_race.cpp` was supplied through the pasted Claude chat. It follows Person 1's
track geometry but replaces the original side-view body and wheels with F1 models.
It should not be credited as unchanged Person 2/3 car code. Person 4 supplied the
missing clipping header and verified the demo locally.
`prepare_integration.py` records the assembly process; rerunning it overwrites
`main.cpp`, so use it only intentionally.

Both programs compile with C++17 and `-Wall -Wextra -pedantic`. Ten clipping cases
cover inside, outside, horizontal, vertical, boundary, corner-crossing, reversed
and degenerate lines. All 100 generated GIF frames were independently decoded;
99 differ from the first and the total duration is 4000 ms.

## Team Members
- 192576 Sebudde Jeremy Mwesigwa
- 192797 Ahmed Najma Wanjiku
- 185318 Karani Shifla
- 186400 Hafswa Sufi


## Course
ICS 3204 – Computer Graphics, Strathmore University
