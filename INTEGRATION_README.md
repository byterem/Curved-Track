# Person 4: clipping and integration

Checked against upstream main `337d643`. The track is merged (PR #2);
the wheels are on `origin/feature/wheels` (`4342b05`).

## Course component integration

`main.cpp` combines Person 1's track and Person 2/3's body and wheels.
It has one Canvas and one main function. The original component files are
preserved. `clipping.h` clips each DDA line before rasterization, including
the wheel fill lines. Pixel bounds checks remain for polygons and circles.

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o scene.exe
.\scene.exe
g++ -std=c++17 -Wall -Wextra -pedantic clipping_test.cpp -o clipping_test.exe
.\clipping_test.exe
```

Output: `scene.ppm`. This is a static CP1 integration. CP2's homogeneous
translation/rotation matrices are still outstanding; do not mark CP2 complete.
`prepare_integration.py` records how the components were assembled; running it
again overwrites main.cpp, so only use it intentionally.

## Optional F1 animation supplied in the pasted chat

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic f1_race.cpp -o f1_race.exe
.\f1_race.exe
```

Output: `f1_race.gif` (100 frames, 800x450, four-second repeating lap),
and `f1_frame.bmp`. No external C++ libraries are needed.
Cars follow arc-length samples of four Bezier segments and face the tangent.
The lane and speed oscillations repeat after one lap, allowing a seamless loop.

This demo uses perspective, layered 3D car models, fog and supersampling.
Perspective and supersampling are covered by Topics 9 and 10 of the supplied
lecture notes, but exceed the repository's baseline checkpoint scope.
It does not implement the required
CP2 homogeneous matrix exercise. The F1 body and tyres are replacement models,
not the original teammates' side-view car and midpoint wheel routines.
Keep this demo separate from the course integration and credit it accurately.

Compile each entry point separately; compiling every .cpp together causes
duplicate types, functions and main definitions.

## Verification

Both programs compile with C++17, Wall, Wextra and pedantic without warnings.
The clipper has ten geometric checks. The GIF was independently decoded using
Pillow: all 100 frames loaded, 99 differed from the first, duration 4000ms,
infinite looping. Pillow is a verification tool only, not a runtime dependency.

Development branch: `feature/clipping-integration`. Review the integration via a PR
before merging into main. The standalone wheels branch remains separate; its
drawing routines are included in the integrated main.cpp.
