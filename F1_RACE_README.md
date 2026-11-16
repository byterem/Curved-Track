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

The program takes about 8 seconds and prints `Frame 1 / 100` ... `Frame 100 / 100`.
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
| `W` | Speed up the red car |
| `S` | Slow the red car down |
| `A` / `D` | Move the red car across the track |
| `C` | Change camera view: Broadcast, Follow, Close-up |
| `F` | Save the current picture as `f1_view.bmp` |
| Enter (no key) | Move the race forward one frame |
| `Q` | Quit |

The other six cars keep racing on their own. The red car's speed is the
distance it covers per frame, so press Enter more often for a faster race.

Camera views:

- **Broadcast**: the original fixed view.
- **Follow**: the camera stays level with the red car, behind it.
- **Close-up**: the same, but closer to the car.

The camera only moves across and along the track. Its height and tilt stay the
same, so it cannot turn or look from above.

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
