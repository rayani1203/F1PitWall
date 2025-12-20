#include <iostream>
#include <string>
#include "../live/StaticInfo.hpp"
#include "../live/LiveTelemetry.hpp"
#include "../live/ReferenceTracker.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: calibrate_track <track_id>\n";
        return 1;
    }

    std::string trackId = argv[1];

    std::cout << "Track: " << trackId << "\n";

    // Load reference lap and print positions
    ReferenceTracker tracker;
    if (!tracker.loadReferenceLap(std::stoi(trackId))) {
        std::cerr << "Failed to load reference lap\n";
        return 1;
    }

    const auto& positions = tracker.getLapPositions();
    std::cout << "Loaded " << positions.size() << " positions:\n";
    for (size_t i = 0; i < positions.size(); ++i) {
        const auto& p = positions[i];
        std::cout << i << ": (" << p.x << ", " << p.y << ", " << p.z << ")\n";
    }

    return 0;
}
