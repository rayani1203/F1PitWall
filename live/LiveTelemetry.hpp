#pragma once
#include <cstdint>
#include "RingBuffer.hpp"
#include "StaticInfo.hpp"

struct LiveInputSample {
    // Basic inputs
    float throttle;  // 0-1
    float brake;     // 0-1
    float steer;     // -1 to 1
    uint64_t timestampMs;

    // Extra telemetry
    uint16_t speed;           // km/h
    uint16_t engineRPM;       // RPM
    uint8_t clutch;           // 0-100
    int8_t gear;              // -1,0,1+
    uint8_t drs;              // 0/1
    uint8_t revLightsPercent; // 0-100
};

struct LivePositionSample {
    float worldX;
    float worldY;
    float worldZ;
    uint64_t timestampMs;
};

// The global ring buffer (size tunable)
extern RingBuffer<LiveInputSample, 512> g_liveInputs;
extern StaticInfo g_staticInfo;
extern RingBuffer<LivePositionSample, 512> g_livePositions;

class LiveTelemetry {
public:
    static size_t copyHistory(LiveInputSample* out, size_t maxCount);
    // Return the most recent sample if available (does not copy history)
    static bool peekLatest(LiveInputSample& out);

    static size_t copyPositionHistory(LivePositionSample* out, size_t maxCount);
    static bool peekLatestPosition(LivePositionSample& out);
};
