#include "LiveTelemetry.hpp"
#include "RingBuffer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

RingBuffer<LiveInputSample, 512> g_liveInputs;

size_t LiveTelemetry::copyHistory(LiveInputSample* out, size_t maxCount) {
    return g_liveInputs.copy(out, maxCount);
}

bool LiveTelemetry::peekLatest(LiveInputSample& out) {
    return g_liveInputs.peekLatest(out);
}
