#include <GL/glut.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "bezier.h"
#include "common.h"
#include "line.h"
#include "polygon.h"
#include "transform.h"

static const float PI = 3.14159265f;
static const float DEFAULT_SPEED = 3.0f;
static const float MIN_SPEED = 0.5f;
static const float MAX_SPEED = 12.0f;
static const float SPEED_STEP = 0.5f;
static const int FRAME_MS = 16;  // about 60 frames per second

// Animation state. These are globals because the GLUT callbacks
// cannot take extra parameters.
static Track gTrack;
static float gDistance = 0.0f;  // how far along the track the car is
static float gSpeed = DEFAULT_SPEED;
static bool gRunning = false;
static int gDirection = 1;  // +1 forward, -1 reverse

// Draws the inner and outer road edges with Bresenham lines.
static void drawTrackEdges() {
    int count = static_cast<int>(gTrack.points.size());
    for (int i = 0; i < count; i++) {
        const TrackPoint& a = gTrack.points[i];
        const TrackPoint& b = gTrack.points[(i + 1) % count];

        drawLineBresenham(static_cast<int>(std::lround(a.inner.x)),
                          static_cast<int>(std::lround(a.inner.y)),
                          static_cast<int>(std::lround(b.inner.x)),
                          static_cast<int>(std::lround(b.inner.y)));
        drawLineBresenham(static_cast<int>(std::lround(a.outer.x)),
                          static_cast<int>(std::lround(a.outer.y)),
                          static_cast<int>(std::lround(b.outer.x)),
                          static_cast<int>(std::lround(b.outer.y)));
    }
}

// Dashed middle line, drawn with DDA so the scene shows both line algorithms.
static void drawCentreLine() {
    const int DASH_LENGTH = 4;  // points drawn
    const int GAP_LENGTH = 5;   // points skipped
    int count = static_cast<int>(gTrack.points.size());

    for (int i = 0; i < count; i += DASH_LENGTH + GAP_LENGTH) {
        for (int k = 0; k < DASH_LENGTH; k++) {
            const Point2D& start = gTrack.points[(i + k) % count].center;
            const Point2D& end = gTrack.points[(i + k + 1) % count].center;
            drawLineDDA(static_cast<int>(std::lround(start.x)),
                        static_cast<int>(std::lround(start.y)),
                        static_cast<int>(std::lround(end.x)),
                        static_cast<int>(std::lround(end.y)));
        }
    }
}

// Builds the car matrix: turn the car to the track direction,
// then translate it to the point on the track.
static void computeCarMatrix(Mat3 matrix) {
    const TrackPoint& point = gTrack.points[trackIndexAt(gTrack, gDistance)];
    float angle = point.angle;
    if (gDirection < 0) {
        angle += PI;  // face the other way when driving in reverse
    }

    Mat3 rotation;
    Mat3 translation;
    setRotation(rotation, angle);
    setTranslation(translation, point.center.x, point.center.y);
    multiplyMatrices(matrix, translation, rotation);
}

// Draws text with the GLUT bitmap font at a screen position.
static void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
    }
}

// Shows the controls, the speed and the current state.
static void drawHud() {
    char speedLine[64];
    char stateLine[64];

    glColor3f(0.85f, 0.88f, 0.95f);
    drawText(16.0f, SCREEN_HEIGHT - 26.0f, "SPACE  : start / pause");
    drawText(16.0f, SCREEN_HEIGHT - 48.0f, "UP/DOWN: speed up / slow down");
    drawText(16.0f, SCREEN_HEIGHT - 70.0f, "R      : reverse direction");
    drawText(16.0f, SCREEN_HEIGHT - 92.0f, "0      : reset the car");
    drawText(16.0f, SCREEN_HEIGHT - 114.0f, "ESC    : quit");

    std::snprintf(speedLine, sizeof(speedLine), "Speed: %.1f", gSpeed);
    std::snprintf(stateLine, sizeof(stateLine), "%s   Direction: %s",
                  gRunning ? "Running" : "Paused",
                  (gDirection > 0) ? "forward" : "reverse");

    glColor3f(0.98f, 0.82f, 0.25f);
    drawText(16.0f, SCREEN_HEIGHT - 148.0f, speedLine);
    glColor3f(0.55f, 0.95f, 0.60f);
    drawText(16.0f, SCREEN_HEIGHT - 170.0f, stateLine);
}

// Draws one frame of the scene.
static void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.90f, 0.90f, 0.96f);
    glBegin(GL_POINTS);
    drawTrackEdges();
    glEnd();

    glColor3f(0.95f, 0.78f, 0.22f);
    glBegin(GL_POINTS);
    drawCentreLine();
    glEnd();

    Mat3 carMatrix;
    computeCarMatrix(carMatrix);

    glColor3f(0.30f, 0.85f, 1.00f);
    glBegin(GL_POINTS);
    drawCarBody(carMatrix);
    glEnd();

    glColor3f(1.00f, 0.58f, 0.20f);
    glBegin(GL_POINTS);
    drawCarWheels(carMatrix);
    glEnd();

    drawHud();
    glutSwapBuffers();
}

// Moves the car by one frame and keeps the distance inside the track.
static void update(int) {  // the callback value is not used
    if (gRunning) {
        gDistance += gDirection * gSpeed;
        if (gDistance < 0.0f) {
            gDistance += gTrack.length;
        } else if (gDistance >= gTrack.length) {
            gDistance -= gTrack.length;
        }
        glutPostRedisplay();
    }
    glutTimerFunc(FRAME_MS, update, 0);
}

// Puts the car back at the start, paused and facing forward.
static void resetCar() {
    gDistance = 0.0f;
    gSpeed = DEFAULT_SPEED;
    gDirection = 1;
    gRunning = false;
}

// Normal keys: SPACE, R, 0 and ESC.
static void keyPressed(unsigned char key, int, int) {
    if (key == ' ') {
        gRunning = !gRunning;
    } else if (key == 'r' || key == 'R') {
        gDirection = -gDirection;
    } else if (key == '0') {
        resetCar();
    } else if (key == 27) {
        std::exit(0);  // ESC
    }
    glutPostRedisplay();
}

// Arrow keys: UP is faster, DOWN is slower.
static void specialPressed(int key, int, int) {
    if (key == GLUT_KEY_UP) {
        gSpeed = std::min(MAX_SPEED, gSpeed + SPEED_STEP);
    } else if (key == GLUT_KEY_DOWN) {
        gSpeed = std::max(MIN_SPEED, gSpeed - SPEED_STEP);
    }
    glutPostRedisplay();
}

// Screen setup: dark background and a 2D coordinate system in pixels.
static void init() {
    glClearColor(0.09f, 0.10f, 0.14f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, SCREEN_WIDTH, 0.0, SCREEN_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPointSize(1.0f);
}

int main(int argc, char** argv) {
    gTrack = buildTrack();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutCreateWindow("Racing Car on a Curved Track - ICS 3204");

    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyPressed);
    glutSpecialFunc(specialPressed);
    glutTimerFunc(FRAME_MS, update, 0);

    glutMainLoop();
    return 0;
}
