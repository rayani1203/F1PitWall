#pragma once
#include <string>

class StaticInfo {
public:
    StaticInfo();
    ~StaticInfo();

    int track_id = -1;

    std::string getTrackName();
};