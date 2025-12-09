#pragma once
#include <cstdint>
#include "RingBuffer.hpp"

struct LiveInputSample {
    float throttle;  // 0-1
    float brake;     // 0-1
    float steer;     // -1 to 1
    uint64_t timestampMs;
};

// The global ring buffer (size tunable)
extern RingBuffer<LiveInputSample, 512> g_liveInputs;

class LiveTelemetry {
public:
    static size_t copyHistory(LiveInputSample* out, size_t maxCount);
};
