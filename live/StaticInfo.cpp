#include "StaticInfo.hpp"
#include "../core/utils.hpp"

StaticInfo::StaticInfo()
    : track_id(-1) // Default to invalid track ID
{}

StaticInfo::~StaticInfo() {}

std::string StaticInfo::getTrackName() {
    auto it = IDtoTrack.find(track_id);
    if (it != IDtoTrack.end()) {
        return it->second;
    }
    return "Unknown Track";
}