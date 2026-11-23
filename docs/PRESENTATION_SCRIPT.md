# Presentation Script — F1 Racing on a Curved Track

**Course:** ICS 3204 – Computer Graphics, Strathmore University  
**Team:** Jeremy (Person 1), Najma (Person 2), Shifla (Person 3), Hafswa (Person 4)  
**Format:** 12 minutes including a live demo, then Q&A  
**Assumption:** no slide deck exists yet, so this file also gives the slide plan.

---

## 1. Timing and slide map

| Time | Speaker | Section | Slide | Visual |
|---|---|---|---|---|
| 0:00–0:45 | Jeremy | Title and what we built | 1 | One good frame from `f1_frame.bmp` |
| 0:45–2:00 | Jeremy | The Bézier track | 2 | `docs/track.png` + the Bézier formula |
| 2:00–3:00 | Jeremy | Arc length and car placement | 3 | `g_cum` / `frameAt` snippet + transform equation |
| 3:00–4:10 | Najma | Canvas, DDA and scanline fill | 4 | `Canvas::plot`, `drawLineDDA`, `fillPolygon` snippets |
| 4:10–5:00 | Najma | The original 2D car body | 5 | `car_body` screenshot / `scene.ppm` render |
| 5:00–6:00 | Shifla | Midpoint circle and wheels | 6 | Circle diagram + `p = 1 - r` code |
| 6:00–7:10 | Shifla | The layered F1 car | 7 | Diagram of 25 slices + close-up of a car |
| 7:10–8:00 | Shifla | Animation and transforms | 8 | `stepRace` flow + rotation/translation matrix |
| 8:00–9:00 | Hafswa | Cohen–Sutherland clipping | 9 | 4-bit outcode table + before/after line |
| 9:00–10:00 | Hafswa | Perspective and supersampling | 10 | Pinhole diagram + 3× downsample |
| 10:00–11:00 | Hafswa | Race logic, GIF and verification | 11 | `--check` output + GIF stats |
| 11:00–12:00 | Hafswa + all | Live demo and close | 12 | The GLUT window, then `f1_race.gif` |

**Rule:** each speaker ends with one sentence that hands over to the next person.
The handover lines are written into the script.

---

## 2. Before you start (10-minute checklist)

- [ ] Build the GIF version and run it once:
  `g++ -std=c++17 -O2 -Wall -Wextra -pedantic f1_race.cpp -o f1_race && ./f1_race`
- [ ] Build the window version:
  Linux: `g++ -std=c++17 -O2 -DF1_USE_GLUT f1_race.cpp -o f1_race_gl -lglut -lGL`
  Windows: `g++ -std=c++17 -O2 -DF1_USE_GLUT f1_race.cpp -o f1_race_gl.exe -lfreeglut -lopengl32`
- [ ] Open `f1_race.gif` in a browser tab. Keep it as the backup demo.
- [ ] Have `f1_frame.bmp`, `f1_view.bmp` and `docs/track.png` ready to show.
- [ ] Terminal font at least 18 pt; close notifications; test the projector.
- [ ] One person drives the demo (Hafswa). The others only narrate when called.
- [ ] Timekeeper: Najma, with a phone timer and the timing table above.
- [ ] Do a full rehearsal once with the demo, then once without slides.

**Wording to avoid**

- Do **not** say “OpenGL draws the cars.” OpenGL only displays a CPU-rendered pixel array.
- Do **not** say “Bresenham.” This project uses DDA; Bresenham is only mentioned as the alternative.
- Do **not** say “the follow camera is always behind the car.” The camera always looks along −Y, so it re-centres on the car but does not rotate with the car’s heading.
- Do **not** say “DDA draws the track edges.” The track edges are filled ribbons; DDA outlines the car slices and the historical body details.
- Do **not** claim the F1 car model is the original Person 2/3 code. It is a replacement model that uses the same primitives. The README says this; the presentation should too.
- Say “about eight to ten seconds” for GIF generation, not a fixed “seven seconds.”

---

## 3. The script

### Slide 1 — Jeremy (0:00–0:45)

**On the slide:** project title, team names, one race frame, the line
“Pure C++17 · no drawing library · 100 frames · 4-second loop.”

**SAY:**
> Good morning. We are Jeremy, Najma, Shifla and Hafswa, and this is our ICS 3204 project: a Formula 1 race written in pure C++17. There is no game engine and no graphics library in the renderer. Every pixel is placed by our own code: lines, circles, polygon fills, curves, clipping and a perspective camera. The program produces a four-second looping GIF, and it also has an OpenGL window where you can drive the red car. One important point before we start: OpenGL only displays the pixels we computed. It does not draw the scene for us.

---

### Slide 2 — Jeremy (0:45–2:00)

**On the slide:** `docs/track.png`, the four anchors/handles, and the formula
`B(t) = (1−t)³P₀ + 3(1−t)²tP₁ + 3(1−t)t²P₂ + t³P₃`.

**SAY:**
> The track is Person 1’s component. It is four cubic Bézier segments joined into a closed loop. Each segment uses the Bernstein form: P0 and P3 are the anchors where segments meet, and P1 and P2 are handles that bend the curve. Both segments at an anchor share the same handle direction, so the join is smooth — there is no kink. We sample each segment at 120 values of t, which gives 480 centre points around the lap. The tangent comes from the derivative B′(t), not from subtracting rounded points, so it stays accurate between samples. Offsetting each point 70 units along the normal gives a 140-unit-wide road. Neighbouring points form quads, and our scanline fill paints them.

**Handover:**
> A curve alone does not tell a car how fast to move, so I’ll show how we turn it into distance.

---

### Slide 3 — Jeremy (2:00–3:00)

**On the slide:** the `g_cum` table idea, the `frameAt` lookup, and
`worldX = X + u·tx − v·ty`, `worldY = Y + u·ty + v·tx`.

**SAY:**
> Equal steps in t do not mean equal distances — the curve covers more ground where it is straight. So we build a table of accumulated distance, `g_cum`, and `frameAt(u)` looks up a distance along the lap and interpolates the position and tangent. That gives steady speed and makes the loop close cleanly. To place a car, we take a model point (u, v), rotate it by the tangent, and translate it to the track. That is rotation followed by translation: the homogeneous matrix from Topic 8, written as two equations. Najma will show the primitives underneath.

---

### Slide 4 — Najma (3:00–4:10)

**On the slide:** three short snippets — `Canvas::plot`, `drawLineDDA`, `fillPolygon`.

**SAY:**
> Everything is drawn on a Canvas: a flat array of RGB pixels, addressed as `y × width + x`. `plot()` writes one pixel and checks the bounds. For lines we use DDA: compute `dx`, `dy`, and `steps = max(|dx|, |dy|)`, then add `dx/steps` and `dy/steps` each step and round to the nearest pixel. For filled shapes we use scanline fill: for each row, find where the polygon edges cross that row, sort the x values, and fill between each pair. The half-open y rule stops a shared vertex from being counted twice. These two routines — DDA and scanline fill — are the base of everything else.

**Handover:**
> Person 2 used exactly these two routines to build the original car body.

---

### Slide 5 — Najma (4:10–5:00)

**On the slide:** the side-view car image and a labelled polygon list
(body, two windows, door seam, lights).

**SAY:**
> Person 2’s component is the original side-view car. The body is an eight-vertex polygon, the windows are two more polygons, and DDA draws the door seam, the handle and the light outlines. It was built in local coordinates with an origin, so it could be moved later. That component is retired from the final animation, but it is the historical CP1 deliverable, and it shows polygons, DDA and scanline fill working together. In the final F1 program the car is a different, 3D model — Shifla will explain that.

---

### Slide 6 — Shifla (5:00–6:00)

**On the slide:** an octant diagram, the 8 symmetric points, and
`p = 1 − r`, `p += 2x + 1`, `p += 2(x − y) + 1`.

**SAY:**
> Person 3’s component is the wheel, and it uses the midpoint circle algorithm. Start at (0, r) with decision value `p = 1 − r`. At each step, if p is negative the midpoint is inside the circle, so we keep y and update `p += 2x + 1`. Otherwise we move y down and update `p += 2(x − y) + 1`. We only compute one octant; eight-way symmetry gives the other seven points. For a filled disc, we record the widest span on each row and fill each row once, so transparent discs do not blend twice. In the final scene these discs draw the sun, clouds, trees and the helmet.

**Handover:**
> The final car uses those circles, but in a very different way from the 2D wheel.

---

### Slide 7 — Shifla (6:00–7:10)

**On the slide:** a stack of 25 slices next to a rendered car, with the labels
“top-view slice”, “height 0.8”, “25 layers”.

**SAY:**
> The final car is a replacement model for the animation, and it is built like a loaf of bread: 25 horizontal slices, 0.8 units apart. Each slice is a top-view picture made from the same polygons and circles. To draw the car, we put each slice on its own height plane, and for every screen pixel we trace the camera ray down to that plane, convert the point into car coordinates, and read the slice. That is why the car looks solid from any direction as it turns. The tyres are wheel cross-sections and the helmet is a circle slice. It costs more than a 2D sprite, but it uses only algorithms from class.

---

### Slide 8 — Shifla (7:10–8:00)

**On the slide:** the `stepRace` flow (speed → distance → frame → draw) and the
3×3 homogeneous rotation+translation matrix.

**SAY:**
> Each car has a distance `s`, a speed `v` and a lane offset. Every frame we advance `s` by `v`, look up the track frame, and orient the car with the tangent. That is the same rotation and translation from slide 3, applied to every vertex of every slice. The seven teams have different colours, start positions, lanes and speed surges. The red car is the player’s in play mode. The lane and speed changes repeat every 100 frames, which is what lets the GIF loop. Hafswa will cover clipping and the camera.

---

### Slide 9 — Hafswa (8:00–9:00)

**On the slide:** the 4-bit outcode table and one line before/after clipping.

**SAY:**
> Person 4’s job is clipping. In the active F1 program, every DDA line goes through Cohen–Sutherland before it is rasterized. Each endpoint gets a four-bit code: left, right, top, bottom. If both codes are zero, accept the line. If they share a bit, reject it — both endpoints are outside the same edge. Otherwise, intersect the outside endpoint with that edge, replace it, and repeat. An off-screen line is never rasterized, and a crossing line draws only its visible part. We test ten cases: inside, horizontal, vertical, corner crossing, reversed, boundary, single point, and three rejections. Polygon fills still use Canvas bounds checks; this is line clipping, not polygon clipping.

---

### Slide 10 — Hafswa (9:00–10:00)

**On the slide:** a pinhole-camera diagram and a 3×3 block averaging diagram.

**SAY:**
> The camera is a pinhole. For a world point, we subtract the camera position, rotate by the pitch, and get a depth `z` and a vertical value. Then `screenX = cx + focal·x/z` and `screenY = cy − focal·y/z`. Dividing by z is what makes distant cars smaller. We render at 2400 × 1350 and average each 3 × 3 block down to 800 × 450. That is supersampling from Topic 10: nine times the pixels, but most jagged edges disappear. Fog, shading and the vignette are extra polish, not lecture requirements.

---

### Slide 11 — Hafswa (10:00–11:00)

**On the slide:** the actual `--check` output and the GIF facts:
100 frames · 40 ms · 25 fps · 4 s · 800×450 · seam 4.3 × 10⁻¹².

**SAY:**
> The race is a small simulation. A car watches the car ahead in its lane, slows when it is close, and tries the other side when it is stuck. A safety check then pushes any overlapping car back to a minimum gap. We ran 5,000 normal frames and 5,000 aggressive frames, and zero frames ended with overlapping cars. The GIF has 100 frames at four centiseconds each — 25 frames per second, four seconds per loop. We search for the starting frame with the smallest jump back to the start; the seam is about 4 × 10⁻¹² units, which is effectively zero. The GIF writer is hand-written: median-cut palette, ordered dithering, LZW compression.

---

### Slide 12 — Hafswa + all (11:00–12:00)

**On the slide:** the controls list and the GitHub link.

**SAY:**
> Let’s show it. This is the live window; the red car is ours. `C` cycles the cameras, `W` and `S` change speed, `A` and `D` change lane, `P` pauses, `Enter` steps one frame, `R` resets, and `F` saves a screenshot. The other six cars drive themselves.
>
> *(Run the demo — see the run sheet below.)*
>
> That is the project. The code and the README — with lecture references for the course algorithms — are on GitHub. We’re happy to take questions.

---

## 4. Live demo run sheet (60–75 seconds)

1. **Start clean.** Press `R` (reset, stays paused), then `P` to resume.
2. **Broadcast view**, 10 s: point out the seven cars, the kerbs, the grandstands.
3. **`C` → Follow**, 8 s: “the camera re-centres on the red car.”
4. **`C` → Close-up**, 5 s: point out the stacked slices and the helmet.
5. **`C` → Top**, 5 s: “the same 3D model from above.”
6. **`C` → Low chase**, 5 s: “close to the ground.”
7. **`P` to pause**, press `Enter` three times: “single-step while paused.”
8. **`P` to resume**, press `A`/`D` to change lane and `W` to speed up, 8 s.
9. **`F`** to save `f1_view.bmp`, then `Q` to quit.
10. Switch to the browser and let `f1_race.gif` play for 10 s.
11. Return to the title slide for questions.

**If the window fails:** do not debug live. Say “the recording uses the same
renderer,” open `f1_race.gif`, and continue. The terminal mode
(`./f1_race --play`) is the second fallback.

---

## 5. Q&A preparation

Route each question to its owner. Do not let one person answer everything.

| Question | Who leads | Short answer |
|---|---|---|
| Why DDA and not Bresenham? | Najma | The brief allows either; we implemented DDA. Bresenham is the integer alternative. Clipping runs before DDA, so off-screen lines are skipped. |
| Why no matrix class? | Jeremy | The two equations are the rotation+translation matrix. The homogeneous form is in the README. A reusable matrix class is an optional extension, not a requirement. |
| How does the 3D car work without a depth buffer? | Shifla | 25 stacked top-view slices, drawn far-to-near with the painter’s algorithm. There is no per-pixel z-buffer, so complex overlaps can be wrong; the README says this. |
| Is OpenGL drawing the scene? | Hafswa | No. `glDrawPixels` displays a CPU-rendered RGB array; GLUT only handles input and the timer. |
| How do you know cars never overlap? | Hafswa | `--check` runs 5,000 normal + 5,000 aggressive frames; zero frames end with an overlap. The safety net reports how often it corrects a car. |
| How is the loop seamless? | Jeremy | `findLoopStart` tries 600 starting frames and keeps the one with the smallest 100-frame seam. The measured seam is 4.3 × 10⁻¹² units. |
| Why 3× supersampling? | Hafswa | It is a simple quality/cost trade: 9× the pixels, averaged down. It removes most jagged edges without a more complex anti-aliasing method. |
| What exactly is clipped? | Hafswa | DDA lines only. Polygon fills and circles use Canvas bounds checks. We do not implement polygon clipping or a 3D near-plane clipper. |
| Why 480 track samples but only 100 animation frames? | Jeremy | 480 is the geometry resolution; 100 is the animation length. Cars move by arc length, not one sample per frame. |
| What would you improve next? | Hafswa | Cache the background for a fixed camera, add polygon/near-plane clipping, add a real depth buffer, and make the chase camera rotate with the car’s heading. |
| What did you use AI for? | Jeremy | Be honest: the F1 model and animation were adapted from a prototype shared in a chat; the team integrated it, verified it, and can explain every algorithm. The original track, body, wheels and clipping work are the team’s. |

---

## 6. Numbers to memorise

| Number | Meaning |
|---|---|
| 4 | Bézier segments in the closed track |
| 120 / 480 | Samples per segment / centre points around the lap |
| 140 | Road width in world units (±70) |
| 7 | Teams and cars |
| 25 / 0.8 | Slices per car / height step (top of car 19.2 units) |
| 800 × 450 | Saved frame size |
| 2400 × 1350 | Render size before supersampling |
| 3 × 3 | Supersampling block |
| 100 / 40 ms | GIF frames / delay per frame |
| 25 fps / 4 s | Playback rate / loop length |
| 10 | Cohen–Sutherland test cases |
| 10,000 | Frames simulated by `--check` |
| 43 / 30 | Smallest bumper gap in the normal / aggressive run (units) |
| 2,075 | Safety corrections in the aggressive run |
| 0 | Overlapping frames reported |
| 4.3 × 10⁻¹² | GIF loop seam in world units |

---

## 7. If you have only 8 minutes

Cut these, in order:

1. Slide 5 (historical car body) — mention it in one sentence during slide 4.
2. Slide 7 (slice model) — keep one image and two sentences.
3. Shorten the demo to 40 seconds: Broadcast → `C` twice → pause/step → GIF.
4. Keep slides 2, 3, 6, 9, 10 and 11; they carry the course concepts.

Do **not** cut the clipping slide or the verification numbers — they are the
strongest evidence that the team understands the algorithms.

---

## 8. Rehearsal notes

- Rehearse the handovers separately. A clean handover saves five seconds each time.
- Say the formula once, slowly, then explain the symbols. Do not read code line by line.
- Point at the screen when you mention a car, a kerb or a slice.
- The demo is the proof; keep talking during it, but do not narrate every key press.
- Leave 10 seconds of silence at the end of the demo before inviting questions.
- If a question is about a file you did not write, say who wrote it and pass it on.
