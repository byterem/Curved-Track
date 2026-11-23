@echo off
rem Easy launcher for Windows (needs freeglut installed, e.g. with MSYS2).
rem Falls back to terminal mode if the window version fails to build.
g++ -std=c++17 -O2 -DF1_USE_GLUT f1_race.cpp -o f1_race_gl.exe -lfreeglut -lopengl32 -lglu32 2>nul && (f1_race_gl.exe --play & goto :eof)
g++ -std=c++17 -O2 f1_race.cpp -o f1_race.exe && f1_race.exe --play
