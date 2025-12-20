#include "LiveTelemetry.hpp"
#include "RingBuffer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

RingBuffer<LiveInputSample, 512> g_liveInputs;
RingBuffer<LivePositionSample, 512> g_livePositions;
StaticInfo g_staticInfo;

size_t LiveTelemetry::copyHistory(LiveInputSample* out, size_t maxCount) {
    return g_liveInputs.copy(out, maxCount);
}

bool LiveTelemetry::peekLatest(LiveInputSample& out) {
    return g_liveInputs.peekLatest(out);
}

size_t LiveTelemetry::copyPositionHistory(LivePositionSample* out, size_t maxCount) {
    return g_livePositions.copy(out, maxCount);
}
bool LiveTelemetry::peekLatestPosition(LivePositionSample& out) {
    return g_livePositions.peekLatest(out);
}
