#!/bin/bash
# Easy launcher: builds the game and starts play mode.
# Uses the window version (arrow keys) if GLUT is installed,
# otherwise falls back to the terminal version.
cd "$(dirname "$0")" || exit 1

if g++ -std=c++17 -O2 -DF1_USE_GLUT f1_race.cpp -o f1_race_gl -lglut -lGL 2>/dev/null; then
    ./f1_race_gl --play
else
    echo "GLUT not found, using terminal mode (install freeglut for the window)."
    g++ -std=c++17 -O2 f1_race.cpp -o f1_race || exit 1
    ./f1_race --play
fi
