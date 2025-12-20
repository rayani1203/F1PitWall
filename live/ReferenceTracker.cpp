#include "ReferenceTracker.hpp"
#include "LiveTelemetry.hpp"
#include "StaticInfo.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

ReferenceTracker::ReferenceTracker() {
}

ReferenceTracker::~ReferenceTracker() {
}

void ReferenceTracker::update() {
    LivePositionSample latest_sample;
    LiveTelemetry::peekLatestPosition(latest_sample);
    LiveInputSample inputHistory[512];
    size_t inputCount = LiveTelemetry::copyHistory(inputHistory, 512);

    if (!recordLap_){
        if(inputCount < 50) {
            return;
        }
        for (int i = 0; i < 50; ++i) {
            if (inputHistory[inputCount - 1 - i].speed > 1e-3) {
                return;
            }
        }
        recordLap_ = true;
        lapPositions_.clear();
        std::cout << "Started recording reference lap for track " << g_staticInfo.track_id << "\n";
    }
    
    
    lapPositions_.push_back({
        latest_sample.worldX,
        latest_sample.worldY,
        latest_sample.worldZ
    });

}

void ReferenceTracker::saveReferenceLap() {
    smoothReferenceLap(5);
    trackId_ = g_staticInfo.track_id;
    fs::path dir("tools/track_calibration/track_paths");
    fs::create_directories(dir);  // ensure directory exists

    fs::path filepath = dir / (std::to_string(trackId_) + "_reference_lap.bin");
    std::ofstream ofs(filepath, std::ios::binary);
    if (!ofs) {
        std::cerr << "Failed to open " << filepath << " for writing\n";
        return;
    }

    ofs.write(reinterpret_cast<const char*>(lapPositions_.data()), lapPositions_.size() * sizeof(Vec3));
    std::cout << "Saved reference lap (" << lapPositions_.size() << " samples) to " << filepath << "\n";
}

void ReferenceTracker::smoothReferenceLap(size_t window = 5) {
    if (lapPositions_.size() < 2) return;

    std::vector<Vec3> smoothed;
    smoothed.reserve(lapPositions_.size());
    for (size_t i = 0; i < lapPositions_.size(); ++i) {
        float sumX = 0, sumY = 0, sumZ = 0;
        size_t count = 0;
        // average over [i-window, i+window] points
        for (int j = (int)i - (int)window; j <= (int)i + (int)window; ++j) {
            if (j >= 0 && j < (int)lapPositions_.size()) {
                sumX += lapPositions_[j].x;
                sumY += lapPositions_[j].y;
                sumZ += lapPositions_[j].z;
                count++;
            }
        }
        smoothed.push_back({sumX / count, sumY / count, sumZ / count});
    }

    lapPositions_ = std::move(smoothed);
}


bool ReferenceTracker::loadReferenceLap(int trackId) {
    fs::path dir("tools/track_calibration/track_paths");
    fs::path filepath = dir / (std::to_string(trackId) + "_reference_lap.bin");

    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs) {
        std::cerr << "Failed to open " << filepath << " for reading\n";
        return false;
    }

    // Get file size
    ifs.seekg(0, std::ios::end);
    size_t fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    size_t numSamples = fileSize / sizeof(Vec3);
    lapPositions_.resize(numSamples);
    ifs.read(reinterpret_cast<char*>(lapPositions_.data()), fileSize);

    std::cout << "Loaded reference lap (" << lapPositions_.size() << " samples) from " << filepath << "\n";
    return true;
}