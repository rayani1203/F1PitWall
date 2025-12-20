#pragma once
#include "LiveTelemetry.hpp"
#include <vector>
#include <string>
#include <fstream>

struct Vec3 {
    float x, y, z;
};

class ReferenceTracker {
public:
    ReferenceTracker();
    ~ReferenceTracker();

    void update();                      // append new samples from live telemetry
    void saveReferenceLap();            // save to binary file
    bool loadReferenceLap(int trackId);            // load from binary file
    void smoothReferenceLap(size_t window); // smooth loaded lap positions

    const std::vector<Vec3>& getLapPositions() const { return lapPositions_; }

private:
    std::vector<Vec3> lapPositions_;   // extracted positions for plotting
    int trackId_;
    bool recordLap_ = false;
};
