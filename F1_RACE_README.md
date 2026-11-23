# F1 Race Animation: Quick Start

`f1_race.cpp` draws a racing scene in pure C++17 (no OpenGL, no drawing libraries).
Each frame is drawn pixel by pixel into an array, and the frames are saved as an
animated GIF. The program runs one lap of a Bezier track and loops it.

## How to Launch

Requirements: a C++17 compiler (`g++`, or MinGW on Windows).

Linux / macOS:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic f1_race.cpp -o f1_race
./f1_race
```

Windows (PowerShell):

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic f1_race.cpp -o f1_race.exe
.\f1_race.exe
```

The program takes about 7 seconds and prints `Frame 1 / 100` ... `Frame 100 / 100`.
When it finishes it creates two files in the same folder:

| File | What it is |
|---|---|
| `f1_race.gif` | The animation: 800 x 450, 100 frames, 4 seconds, loops. Open it in a browser. |
| `f1_frame.bmp` | The first frame as a still image. |

These generated files are ignored by git.

## Play Mode (interactive)

Run the program with `--play` to control the red car and change the camera:

```bash
./f1_race --play
```

The race is drawn as text in the terminal. Each command is one line: type a
key and press Enter.

| Key | Action |
|---|---|
| `Up arrow` or `W` | Speed up the red car (and play one frame) |
| `Down arrow` or `S` | Slow the red car down (and play one frame) |
| `Left arrow` / `Right arrow` or `A` / `D` | Move the red car left / right across the track (and play one frame) |
| Enter or Space | Play one frame with no change |
| `C` | Change camera view (does not move time) |
| `F` | Save the current picture as `f1_view.bmp` |
| `Q` | Quit |

In the terminal, an arrow key only works after you press Enter, because the
terminal sends a whole line at a time. For smooth driving use the window
version below, where arrow keys react instantly.

Every driving key press is one frame of the race, so pressing a key is how time
passes.

The other six cars race on their own. They slow down behind a slower car, and
when one is held up it tries the other side of the track. No two cars can pass
through each other. If you drive into a car ahead, you are held back and the
message `Contact` appears.

Camera views:

- **Broadcast**: the original fixed view.
- **Follow**: the camera follows the red car from behind.
- **Close-up**: the same, but closer to the car.
- **Top**: looking almost straight down on the car.
- **Low chase**: close to the ground, looking along the track.

In the followed views the car stays in the middle of the picture.

### Collision check

`--check` runs the race with no drawing and reports whether any cars overlap,
the smallest gap between cars, and how far the GIF loop jumps:

```bash
./f1_race --check
```

### Play mode in a window (OpenGL + GLUT)

The same controls also work in a window. Build it with the GLUT flag, which
links OpenGL and GLUT (on Linux, install freeglut first):

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -DF1_USE_GLUT f1_race.cpp -o f1_race_gl -lglut -lGL
./f1_race_gl --play
```

Windows with MSYS2 MinGW64 and freeglut:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -DF1_USE_GLUT f1_race.cpp -o f1_race_gl.exe -lfreeglut -lopengl32
.\f1_race_gl.exe --play
```

If missing, install `mingw-w64-x86_64-freeglut` through the MSYS2 MinGW64
terminal. `C:\msys64\mingw64\bin` must be on PATH for the compiler and DLLs.

The window runs continuously, targeting 25 simulation steps per second. Click
inside it for keyboard focus. Window controls differ from the terminal mode:

| Key | Window action |
| --- | --- |
| P / Space | Pause or resume |
| R | Reset race, camera and controls; remain paused at frame 0 |
| Enter | Advance one frame while paused |
| Up / W | Increase player speed |
| Down / S | Decrease player speed |
| Left / A | Move one lane left |
| Right / D | Move one lane right |
| C | Cycle Broadcast, Follow, Close-up, Top and Low chase |
| F | Save `f1_view.bmp` |
| Q / Escape | Quit |

The arrow keys are mapped onto W/A/S/D, so both work the same way. Paused
control changes do not advance time. The timer uses elapsed time and fixed
40 ms simulation steps; long stalls are capped to avoid excessive catch-up.
The window preserves the image proportions when resized. Rendering frame rate
depends on CPU performance because our C++ algorithms draw every pixel.
OpenGL displays those pixels; it does not replace the drawing algorithms.

Play mode does not write the GIF. Running `./f1_race` with no argument still
makes `f1_race.gif` as before.

## What Each Part Does

The source file is split into sections. Here is what the main pieces do.

| Part | Where | Job |
|---|---|---|
| Settings | top of file | Constants such as `OUT_W`/`OUT_H` (image size), `SS` (supersampling), `PITCH` and `FOCAL` (camera). |
| `Color`, `Point`, `Vec2` | top | Small structs for colours and positions. |
| `Canvas` | class | A 2D pixel array with `plot` and `downsample`. Supersampling draws at 3x size and shrinks the result for smoother edges. |
| `drawLineDDA` | line drawing | Draws straight lines with the DDA algorithm. |
| `fillPolygon` | polygons | Fills shapes with scanline filling. |
| `fillCircleMidpoint` | circles | Draws filled circles with the Midpoint Circle algorithm, using 8-way symmetry. |
| `project`, `scr` | camera | Perspective projection: turns 3D world points into screen pixels. |
| `buildFrames`, `frameAt` | track | Builds the Bezier track and gives the position and direction at any point `u` along it. |
| `nearestTrack` | track | Finds the nearest track point, used to place objects beside the road. |
| `drawTrackSurface` | scenery | Draws the road surface. |
| `drawSkyAndGround` | scenery | Draws sky, hills, and ground. |
| `addWalls`, `addStands`, `addGantry`, `addTrees` | scenery | Adds track-side objects to a draw list. |
| `buildCarParts`, `makeCar` | car | Describes the F1 car as stacked horizontal slices (layered model). |
| `drawCar` | car | Draws the car at its position and rotates it to follow the track direction. |
| `GifWriter` | output | Writes the frames as an animated GIF (LZW compression). |
| `buildPalette` | output | Chooses the 256 GIF colours from the frame. |
| `applyVignette` | effects | Darkens the corners of the picture. |
| `main` | entry | Sets up the teams and cars, runs the 100-frame loop, and saves the GIF and BMP. |

### How a frame is made

1. Draw sky and ground.
2. Collect the scenery and car items, sort them from far to near (painter's algorithm).
3. Draw each item in that order, so near objects cover far ones.
4. Shrink the supersampled canvas to 800 x 450 and add the vignette.
5. Store the frame in the GIF.

## Changing Things

- Make the image smaller or larger: change `OUT_W` and `OUT_H`.
- Change the number of frames: change `FRAMES` in `main`.
- Change the animation speed: change `DELAY_CS` in `main`.
- Adjust the camera: change `CAM_X`, `CAM_Y`, `CAM_H`, or `PITCH`.

Rebuild after each change.

## Notes

- Compile only `f1_race.cpp` on its own. The other `.cpp` files have their own `main`
  and will clash if built together.
- The historical static 2D scene (`main.cpp`) and the clipping test (`clipping_test.cpp`)
  are separate programs with their own build commands. See [README.md](README.md).
