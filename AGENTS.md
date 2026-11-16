# AGENTS.md

Instructions for AI coding agents working on this project.
Please read this before writing or changing any code.

## Project Summary
The active project is the F1 race animation in `f1_race.cpp` for ICS 3204 –
Computer Graphics. Cars follow a Bezier circuit and render through a perspective
camera to a GIF. The original static 2D scene and components are historical
references, retired from active development by the team.

This is a **student learning project**. Keep everything simple and easy to understand.

## Golden Rules
1. **Keep it simple.** Short functions, clear names, no clever tricks.
2. **Write the algorithms ourselves.** Do NOT use built-in drawing helpers (e.g. `glutSolidSphere`, `GL_LINES`, `GL_CIRCLE`) for the core shapes. The point is to show we understand the algorithms.
3. **Add short comments** explaining what each function does and why. Students must be able to explain the code.
4. **One change at a time.** Don't rewrite files you were not asked to touch.
5. **Ask before adding libraries.** Use standard C++ only until CP3.

## Project Phases (follow the current one)
| Phase | Environment | Goal |
|---|---|---|
| CP1 | Pure C++ | Draw the basic components (track, car body, wheels) |
| CP2 | Pure C++ | Add transformations and animation (translate, rotate) |
| CP3 | OpenGL | Move everything into OpenGL, add interaction |
| CP4 | OpenGL | Final demo and polish |

**Do not use OpenGL before CP3.** In CP1 and CP2, draw pixels into a simple 2D array and save the result as an image (e.g. `.ppm`) or print it to check the output.

## Algorithms to Use
| What | Algorithm |
|---|---|
| Straight lines | DDA or Bresenham |
| Wheels | Midpoint Circle (use 8-way symmetry) |
| Car body and window | Polygons drawn with line algorithms |
| Track | Bézier curve (calculate points using `t` from 0 to 1) |
| Keeping lines on screen | Cohen-Sutherland clipping |
| Moving the car | Translation (homogeneous matrix) |
| Turning the car | Rotation (homogeneous matrix) |

## Folder Structure
```
/src        C++ source files (.cpp)
/include    Header files (.h)
/docs       Screenshots and checkpoint notes
README.md   Project description
AGENTS.md   This file
```

## Suggested Files
- `line.cpp` – DDA and Bresenham
- `circle.cpp` – Midpoint Circle
- `polygon.cpp` – Car body and window
- `bezier.cpp` – Track curve
- `clip.cpp` – Cohen-Sutherland
- `transform.cpp` – Translate and rotate
- `f1_race.cpp` – Active F1 animation entry point
- `main.cpp` – Historical static 2D integration

## Coding Style
- Language: **C++11 or newer**
- Use `camelCase` for functions and variables (e.g. `drawWheel`)
- Use `UPPER_CASE` for constants (e.g. `SCREEN_WIDTH`)
- One function does one job
- Keep functions under about 40 lines
- No global variables unless really needed

## How to Build and Run
```
g++ -std=c++17 -O2 -Wall -Wextra -pedantic f1_race.cpp -o f1_race
./f1_race
```
Windows: use `f1_race.exe` and run `.\f1_race.exe`. Compile entry points
separately. Update this once OpenGL is added in CP3.

## Testing Your Work
- Each component (track, car, wheel) should be testable on its own
- Always check the output by looking at the image or screen
- Compare line results from DDA and Bresenham if unsure

## What NOT To Do
- Do not delete or rename files without asking
- Do not copy large code blocks that we cannot explain
- Do not add features that are not in the current phase
- Preserve the approved F1 perspective, layered models and supersampling.
  Keep further changes explainable using the lecture notes; OpenGL remains CP3.
- Do not commit build files or images larger than needed

## Git Rules
- Small commits with clear messages (e.g. `Add midpoint circle for wheels`)
- Work on your own branch, then merge
- Pull the latest changes before you start

## When You Are Unsure
Ask the student a short question before making big changes.
When you finish a task, give a short summary: what you changed and which file.

## Team Roles (CP1)
| Role | Component |
|---|---|
| Person 1 | Track (Bézier curve) |
| Person 2 | Car body (polygons + lines) |
| Person 3 | Wheels (Midpoint Circle) |
| Person 4 | Setup, clipping, GitHub, merging |
