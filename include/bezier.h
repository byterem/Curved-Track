#ifndef BEZIER_H
#define BEZIER_H

#include <vector>
#include "common.h"

// One point of the track. It stores the centre line point, the two road
// edges next to it, and where it sits along the track.
struct TrackPoint {
    Point2D center;
    Point2D inner;
    Point2D outer;
    float distance;  // how far this point is from the start, along the centre line
    float angle;     // direction the track is heading here, in radians
};

// The whole closed track, made from several cubic Bezier segments.
struct Track {
    std::vector<TrackPoint> points;
    float length;  // total length of the centre line
};

// Calculates the track points from the Bezier segments.
Track buildTrack();

// Returns the index of the track point that is closest behind a given distance.
int trackIndexAt(const Track& track, float distance);

#endif
