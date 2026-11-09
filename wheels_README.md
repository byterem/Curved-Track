# Wheels 

Pure C++ implementation of the car wheels using the **Midpoint Circle
Algorithm**. No OpenGL, no animation, and no external graphics library.
Built on top of the car body so the wheels can be checked in place.

## Build and test

```bash
g++ -std=c++17 -Wall -Wextra wheels.cpp -o wheels
./wheels
```

On Windows with MinGW, build the same way and run `wheels.exe`.

This creates `car_with_wheels.ppm` (a binary PPM image). Open it in an
image viewer that supports PPM, or convert it to PNG for a screenshot.
The generated image is not meant to be committed as source code.

## Graphics techniques

- **Midpoint Circle Algorithm:** uses an integer decision value to choose
  each pixel, so no floating point or trigonometry is needed.
- **8-way symmetry:** only one eighth of the circle is calculated and the
  points are mirrored into the other seven octants.
- **Filled discs:** the symmetric points are joined with horizontal DDA
  lines to fill the tyre, rim and hub.
- **DDA (Person 2):** reused for the four spokes.

## Wheel design

Each wheel is made of a dark tyre (radius 20), a grey rim (60% of the
radius), a hub (20% of the radius) and four spokes. The spokes make
rotation visible later in CP2.

## Integration contract

`drawCarWheels(Canvas& canvas, int originX, int originY)` uses the same
origin as `drawCarBody(canvas, originX, originY)`. Wheel centres in local
coordinates are (45, 94) and (144, 94), with radius 20. Call it after
`drawCarBody` so the wheels are drawn on top of the body.

`wheels.cpp` currently contains a copy of Person 2's `Canvas` and
`drawLineDDA` so it can be built and tested alone. At integration, these
should move to a shared header so the files can be compiled together.

## CP2 plan

The wheel centres and spokes are local points, so they can take the same
translation and rotation as the car body. Rotating the spokes about each
wheel centre will show the wheels spinning as the car moves.