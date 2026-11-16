"""Assemble the existing team components without changing their source files."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent
wheels = subprocess.check_output(
    ['git', 'show', 'origin/feature/wheels:wheels.cpp'], cwd=root, text=True)
track = (root / 'track.cpp').read_text(encoding='utf-8')
shared = wheels[:wheels.index('int main()')]
shared = shared.replace('#include <vector>', '#include <vector>\n#include "clipping.h"')
shared = shared.replace('    void plot(int x, int y, Color color) {',
    '    int width() const { return width_; }\n'
    '    int height() const { return height_; }\n\n'
    '    void plot(int x, int y, Color color) {')
shared = shared.replace('    int dx = b.x - a.x, dy = b.y - a.y;',
    '    ClipWindow window{0, 0, double(canvas.width()-1), double(canvas.height()-1)};\n'
    '    double x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;\n'
    '    if (!cohenSutherlandClip(x0, y0, x1, y1, window)) return;\n'
    '    a = {int(std::lround(x0)), int(std::lround(y0))};\n'
    '    b = {int(std::lround(x1)), int(std::lround(y1))};\n'
    '    int dx = b.x - a.x, dy = b.y - a.y;')
track_part = track[track.index('const int SCREEN_WIDTH'):track.index('int main()')]
entry = '''
// CP1 integration: original side-view car on the lower straight.
// CP2 transformations remain a separate checkpoint.
int main() {
    Canvas canvas(SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
    drawTrack(canvas);
    drawCarBody(canvas, 388, 407);
    drawCarWheels(canvas, 388, 407);
    canvas.savePPM("scene.ppm");
    std::cout << "Created scene.ppm (900 x 600)\\n";
}
'''
(root / 'main.cpp').write_text(
    '// Integrated by Person 4; body/wheels from feature/wheels, track from main.\n'
    + shared + '\n// Person 1: original Bezier track.\n' + track_part + entry,
    encoding='utf-8')
