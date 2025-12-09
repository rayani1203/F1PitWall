#include <iostream>
#include <fstream>
#include <iomanip>
#include "packetStructs.hpp"
#include "packetWriters.hpp"
#include "../live/LiveTelemetry.hpp"

#include <string>
#include <unordered_map>
#include <cstring>

void writeCarTelemetryPacket(const uint8_t* data, std::ofstream& file) {
    if (!data || !file.is_open()) {
        file << "Invalid car telemetry packet data\n";
        return;
    }
    const PacketCarTelemetryData* packet = reinterpret_cast<const PacketCarTelemetryData*>(data);

    for (int i = 0; i < 22; ++i) {
        if (i != packet->m_header.m_playerCarIndex) continue;
        const CarTelemetryData& carData = packet->m_carTelemetryData[i];

        LiveInputSample sample;

        sample.throttle = carData.m_throttle;
        sample.brake = carData.m_brake;
        sample.steer = carData.m_steer;
        sample.timestampMs = static_cast<uint64_t>(packet->m_header.m_sessionTime*1000);

        g_liveInputs.push(sample);

        file << "Car " << i << ":\n";
        file << "  Speed: " << carData.m_speed << " km/h\n";
        file << "  Throttle: " << std::fixed << std::setprecision(4) << carData.m_throttle * 100 << "%\n";
        file << "  Steer: " << std::fixed << std::setprecision(4) << carData.m_steer << "\n";
        file << "  Brake: " << std::fixed << std::setprecision(4) << carData.m_brake * 100 << "%\n";
        file << "  Clutch: " << static_cast<int>(carData.m_clutch) << "%\n";
        file << "  Gear: " << static_cast<int>(carData.m_gear) << "\n";
        file << "  Engine RPM: " << carData.m_engineRPM << "\n";
        file << "  DRS: " << (carData.m_drs ? "On" : "Off") << "\n";
        file << "  Rev Lights Percent: " << static_cast<int>(carData.m_revLightsPercent) << "%\n";
        file << "\n";
    }
}

static std::string decodeEventCode(const char code[4]) {
    // Map of known event codes to descriptions
    static const std::unordered_map<std::string, std::string> map = {
        {"SSTA", "Session Started"},
        {"SEND", "Session Ended"},
        {"FTLP", "Fastest Lap"},
        {"RTMT", "Retirement"},
        {"DRSE", "DRS Enabled"},
        {"DRSD", "DRS Disabled"},
        {"TMPT", "Team mate in pits"},
        {"CHQF", "Chequered flag"},
        {"RCWN", "Race Winner"},
        {"PENA", "Penalty Issued"},
        {"SPTP", "Speed Trap Triggered"},
        {"STLG", "Start lights"},
        {"LGOT", "Lights out"},
        {"DTSV", "Drive through served"},
        {"SGSV", "Stop go served"},
        {"FLBK", "Flashback activated"},
        {"BUTN", "Button status"},
        {"RDFL", "Red Flag"},
        {"OVTK", "Overtake"}
    };

    std::string s(code, 4);
    auto it = map.find(s);
    if (it != map.end()) return it->second;
    return std::string("Unknown event (") + s + ")";
}

void writeEventPacket(const uint8_t* data, std::ofstream& file) {
    if (!data || !file.is_open()) {
        file << "Invalid event packet data\n";
        return;
    }
    const PacketEventData* packet = reinterpret_cast<const PacketEventData*>(data);

    char eventCode[5] = {0};
    std::memcpy(eventCode, packet->m_eventStringCode, 4);
    std::string eventDescription = decodeEventCode(eventCode);

    file << "Event: " << eventCode << " - " << eventDescription << "\n";

    // Parse event-specific details based on event code
    std::string code(eventCode, 4);

    if (code == "FTLP") {
        // Fastest Lap
        file << "  Vehicle Index: " << static_cast<int>(packet->m_eventDetails.FastestLap.vehicleIdx) << "\n";
        file << "  Lap Time: " << std::fixed << std::setprecision(2) << packet->m_eventDetails.FastestLap.lapTime << " s\n";
    }
    else if (code == "RTMT") {
        // Retirement
        file << "  Vehicle Index: " << static_cast<int>(packet->m_eventDetails.Retirement.vehicleIdx) << "\n";
    }
    else if (code == "TMPT") {
        // Team mate in pits
        file << "  Vehicle Index: " << static_cast<int>(packet->m_eventDetails.TeamMateInPits.vehicleIdx) << "\n";
    }
    else if (code == "RCWN") {
        // Race Winner
        file << "  Vehicle Index: " << static_cast<int>(packet->m_eventDetails.RaceWinner.vehicleIdx) << "\n";
    }
    else if (code == "PENA") {
        // Penalty Issued
        file << "  Penalty Type: " << static_cast<int>(packet->m_eventDetails.Penalty.penaltyType) << "\n";
        file << "  Infringement Type: " << static_cast<int>(packet->m_eventDetails.Penalty.infringementType) << "\n";
        file << "  Vehicle Index: " << static_cast<int>(packet->m_eventDetails.Penalty.vehicleIdx) << "\n";
        file << "  Other Vehicle Index: " << static_cast<int>(packet->m_eventDetails.Penalty.otherVehicleIdx) << "\n";
        file << "  Time: " << static_cast<int>(packet->m_eventDetails.Penalty.time) << " s\n";
        file << "  Lap Number: " << static_cast<int>(packet->m_eventDetails.Penalty.lapNum) << "\n";
        file << "  Places Gained: " << static_cast<int>(packet->m_eventDetails.Penalty.placesGained) << "\n";
    }
    else if (code == "SPTP") {
        // Speed Trap Triggered
        file << "  Vehicle Index: " << static_cast<int>(packet->m_eventDetails.SpeedTrap.vehicleIdx) << "\n";
        file << "  Speed: " << std::fixed << std::setprecision(1) << packet->m_eventDetails.SpeedTrap.speed << " km/h\n";
        file << "  Overall Fastest: " << static_cast<int>(packet->m_eventDetails.SpeedTrap.isOverallFastestInSession) << "\n";
        file << "  Driver Fastest: " << static_cast<int>(packet->m_eventDetails.SpeedTrap.isDriverFastestInSession) << "\n";
        file << "  Fastest Vehicle Index: " << static_cast<int>(packet->m_eventDetails.SpeedTrap.fastestVehicleIdxInSession) << "\n";
        file << "  Fastest Speed in Session: " << std::setprecision(1) << packet->m_eventDetails.SpeedTrap.fastestSpeedInSession << " km/h\n";
    }
    else if (code == "STLG") {
        // Start lights
        file << "  Number of Lights: " << static_cast<int>(packet->m_eventDetails.StartLIghts.numLights) << "\n";
    }
    else if (code == "DTSV") {
        // Drive through served
        file << "  Vehicle Index: " << static_cast<int>(packet->m_eventDetails.DriveThroughPenaltyServed.vehicleIdx) << "\n";
    }
    else if (code == "SGSV") {
        // Stop go served
        file << "  Vehicle Index: " << static_cast<int>(packet->m_eventDetails.StopGoPenaltyServed.vehicleIdx) << "\n";
    }
    else if (code == "FLBK") {
        // Flashback
        file << "  Flashback Frame ID: " << packet->m_eventDetails.Flashback.flashbackFrameIdentifier << "\n";
        file << "  Flashback Session Time: " << std::fixed << std::setprecision(2) << packet->m_eventDetails.Flashback.flashbackSessionTime << " s\n";
    }
    else if (code == "BUTN") {
        // Button status
        file << "  Button Status: 0x" << std::hex << packet->m_eventDetails.Buttons.buttonStatus << std::dec << "\n";
    }
    else if (code == "OVTK") {
        // Overtake
        file << "  Overtaking Vehicle Index: " << static_cast<int>(packet->m_eventDetails.Overtake.overtakingVehicleIdx) << "\n";
        file << "  Being Overtaken Vehicle Index: " << static_cast<int>(packet->m_eventDetails.Overtake.beingOvertakenVehicleIdx) << "\n";
    }
}

void writeMotionPacket(const uint8_t* data, std::ofstream& file) {
    if (!data || !file.is_open()) {
        file << "Invalid motion packet data\n";
        return;
    }

    const PacketMotionData* packet = reinterpret_cast<const PacketMotionData*>(data);

    for (int i = 0; i < 22; ++i) {
        if (i != packet->m_header.m_playerCarIndex) continue;
        const CarMotionData& motionData = packet->m_carMotionData[i];
        file << "Car " << i << ":\n";
        file << "  Position: (" << std::fixed << std::setprecision(2) 
             << motionData.m_worldPositionX << ", "
             << motionData.m_worldPositionY << ", "
             << motionData.m_worldPositionZ << ") m\n";
        file << "  Velocity: (" << std::setprecision(2)
             << motionData.m_worldVelocityX << ", "
             << motionData.m_worldVelocityY << ", "
             << motionData.m_worldVelocityZ << ") m/s\n";
        file << "  G-Forces - Lateral: " << std::setprecision(3) << motionData.m_gForceLateral
             << ", Longitudinal: " << motionData.m_gForceLongitudinal
             << ", Vertical: " << motionData.m_gForceVertical << "\n";
        file << "  Rotation (rad) - Yaw: " << std::setprecision(4) << motionData.m_yaw
             << ", Pitch: " << motionData.m_pitch
             << ", Roll: " << motionData.m_roll << "\n";
        file << "\n";
    }
}

void writeSessionPacket(const uint8_t* data, std::ofstream& file) {
    const PacketSessionData* packet = reinterpret_cast<const PacketSessionData*>(data);
    file << "Session:\n";
    file << "  Weather: " << static_cast<int>(packet->m_weather) << "\n";
    file << "  Track Temp: " << static_cast<int>(packet->m_trackTemperature) << " C\n";
    file << "  Air Temp: " << static_cast<int>(packet->m_airTemperature) << " C\n";
    file << "  Total Laps: " << static_cast<int>(packet->m_totalLaps) << "\n";
    file << "  Track Length: " << packet->m_trackLength << " m\n";
    file << "  Session Type: " << static_cast<int>(packet->m_sessionType) << "\n";
    file << "  Session Time Left: " << packet->m_sessionTimeLeft << " s\n";
    file << "  Session Duration: " << packet->m_sessionDuration << " s\n";
    file << "  Pit Speed Limit: " << static_cast<int>(packet->m_pitSpeedLimit) << " km/h\n";
}

void writeLapDataPacket(const uint8_t* data, std::ofstream& file) {
    const PacketLapData* packet = reinterpret_cast<const PacketLapData*>(data);
    file << "Lap Data:\n";
    for (int i = 0; i < 22; ++i) {
        if (i != packet->m_header.m_playerCarIndex) continue;
        const LapData& lap = packet->m_lapData[i];
        file << "Car " << i << ": Last=" << lap.m_lastLapTimeInMS
             << " ms, Current=" << lap.m_currentLapTimeInMS
             << " ms, Pos=" << static_cast<int>(lap.m_carPosition)
             << ", Lap#=" << static_cast<int>(lap.m_currentLapNum)
             << ", PitStatus=" << static_cast<int>(lap.m_pitStatus) << "\n";
    }
}

void writeParticipantsPacket(const uint8_t* data, std::ofstream& file) {
    const PacketParticipantsData* packet = reinterpret_cast<const PacketParticipantsData*>(data);
    file << "Participants: ActiveCars=" << static_cast<int>(packet->m_numActiveCars) << "\n";
    for (int i = 0; i < 22; ++i) {
        if (i != packet->m_header.m_playerCarIndex) continue;
        const ParticipantData& p = packet->m_participants[i];
        file << "  Car " << i << ": Name='" << p.m_name << "' AI=" << static_cast<int>(p.m_aiControlled)
             << ", Team=" << static_cast<int>(p.m_teamId) << ", Num=" << static_cast<int>(p.m_raceNumber) << "\n";
    }
}

void writeCarSetupsPacket(const uint8_t* data, std::ofstream& file) {
    const PacketCarSetupData* packet = reinterpret_cast<const PacketCarSetupData*>(data);
    file << "Car Setups:\n";
    for (int i = 0; i < 22; ++i) {
        if (i != packet->m_header.m_playerCarIndex) continue;
        const CarSetupData& s = packet->m_carSetups[i];
        file << "Car " << i << ": FrontWing=" << static_cast<int>(s.m_frontWing)
             << ", RearWing=" << static_cast<int>(s.m_rearWing)
             << ", BrakeBias=" << static_cast<int>(s.m_brakeBias)
             << ", FuelLoad=" << s.m_fuelLoad << "\n";
    }
}

void writeCarStatusPacket(const uint8_t* data, std::ofstream& file) {
    const PacketCarStatusData* packet = reinterpret_cast<const PacketCarStatusData*>(data);
    file << "Car Status:\n";
    for (int i = 0; i < 22; ++i) {
        if (i != packet->m_header.m_playerCarIndex) continue;
        const CarStatusData& s = packet->m_carStatusData[i];
        file << "Car " << i << ": Fuel=" << s.m_fuelInTank << "kg, FuelLaps=" << s.m_fuelRemainingLaps
             << ", MaxRPM=" << s.m_maxRPM << ", DRSAllowed=" << static_cast<int>(s.m_drsAllowed)
             << ", TyreCompound=" << static_cast<int>(s.m_actualTyreCompound) << "\n";
    }
}

void writeFinalClassificationPacket(const uint8_t* data, std::ofstream& file) {
    const PacketFinalClassificationData* packet = reinterpret_cast<const PacketFinalClassificationData*>(data);
    file << "Final Classification: NumCars=" << static_cast<int>(packet->m_numCars) << "\n";
    for (int i = 0; i < 22; ++i) {
        if (i != packet->m_header.m_playerCarIndex) continue;
        const FinalClassificationData& c = packet->m_classificationData[i];
        file << "Car " << i << ": Pos=" << static_cast<int>(c.m_position)
             << ", Laps=" << static_cast<int>(c.m_numLaps)
             << ", Grid=" << static_cast<int>(c.m_gridPosition)
             << ", Points=" << static_cast<int>(c.m_points)
             << ", BestLap=" << c.m_bestLapTimeInMS << " ms\n";
    }
}

void writeLobbyInfoPacket(const uint8_t* data, std::ofstream& file) {
    const PacketLobbyInfoData* packet = reinterpret_cast<const PacketLobbyInfoData*>(data);
    file << "Lobby Info: Players=" << static_cast<int>(packet->m_numPlayers) << "\n";
    for (int i = 0; i < 22; ++i) {
        const LobbyInfoData& p = packet->m_lobbyPlayers[i];
        file << "  Player " << i << ": Name='" << p.m_name << "', Car=" << static_cast<int>(p.m_carNumber)
             << ", Ready=" << static_cast<int>(p.m_readyStatus) << "\n";
    }
}

void writeCarDamagePacket(const uint8_t* data, std::ofstream& file) {
    const PacketCarDamageData* packet = reinterpret_cast<const PacketCarDamageData*>(data);
    file << "Car Damage:\n";
    for (int i = 0; i < 22; ++i) {
        if (i != packet->m_header.m_playerCarIndex) continue;
        const CarDamageData& d = packet->m_carDamageData[i];
        file << "Car " << i << ": TyreWear=[" << d.m_tyresWear[0] << "," << d.m_tyresWear[1]
             << "," << d.m_tyresWear[2] << "," << d.m_tyresWear[3] << "]"
             << ", FLWing=" << static_cast<int>(d.m_frontLeftWingDamage)
             << ", FRWing=" << static_cast<int>(d.m_frontRightWingDamage)
             << ", RearWing=" << static_cast<int>(d.m_rearWingDamage) << "\n";
    }
}

void writeSessionHistoryPacket(const uint8_t* data, std::ofstream& file) {
    const PacketSessionHistoryData* packet = reinterpret_cast<const PacketSessionHistoryData*>(data);
    file << "Session History: CarIdx=" << static_cast<int>(packet->m_carIdx)
         << ", NumLaps=" << static_cast<int>(packet->m_numLaps)
         << ", NumStints=" << static_cast<int>(packet->m_numTyreStints) << "\n";
    
    file << "Best Times: Lap#" << static_cast<int>(packet->m_bestLapTimeLapNum)
         << ", S1#" << static_cast<int>(packet->m_bestSector1LapNum)
         << ", S2#" << static_cast<int>(packet->m_bestSector2LapNum)
         << ", S3#" << static_cast<int>(packet->m_bestSector3LapNum) << "\n\n";

    // Lap History
    file << "Lap History:\n";
    for (int i = 0; i < packet->m_numLaps && i < 100; ++i) {
        const LapHistoryData& lap = packet->m_lapHistoryData[i];
        
        // Format total lap time
        uint32_t totalMs = lap.m_lapTimeInMS;
        uint32_t mins = totalMs / 60000;
        uint32_t secs = (totalMs % 60000) / 1000;
        uint32_t ms = totalMs % 1000;
        
        file << "  Lap " << i << ": " << mins << ":" 
             << std::setfill('0') << std::setw(2) << secs << "."
             << std::setw(3) << ms << std::setfill(' ');
        
        // Sector times
        file << " [S1: " << lap.m_sector1TimeMinutes << ":"
             << std::setfill('0') << std::setw(2) << (lap.m_sector1TimeInMS / 1000) << "."
             << std::setw(3) << (lap.m_sector1TimeInMS % 1000) << std::setfill(' ')
             << ", S2: " << lap.m_sector2TimeMinutes << ":"
             << std::setfill('0') << std::setw(2) << (lap.m_sector2TimeInMS / 1000) << "."
             << std::setw(3) << (lap.m_sector2TimeInMS % 1000) << std::setfill(' ')
             << ", S3: " << lap.m_sector3TimeMinutes << ":"
             << std::setfill('0') << std::setw(2) << (lap.m_sector3TimeInMS / 1000) << "."
             << std::setw(3) << (lap.m_sector3TimeInMS % 1000) << std::setfill(' ')
             << "]";
        
        // Validity flags
        file << " [";
        if (lap.m_lapValidBitFlags & 0x01) file << "LAP_VALID";
        else file << "LAP_INVALID";
        file << " ";
        if (lap.m_lapValidBitFlags & 0x02) file << "S1_VALID";
        else file << "S1_INVALID";
        file << " ";
        if (lap.m_lapValidBitFlags & 0x04) file << "S2_VALID";
        else file << "S2_INVALID";
        file << " ";
        if (lap.m_lapValidBitFlags & 0x08) file << "S3_VALID";
        else file << "S3_INVALID";
        file << "]\n";
    }
    
    // Tyre Stint History
    // if (packet->m_numTyreStints > 0) {
    //     file << "\nTyre Stint History:\n";
    //     for (int i = 0; i < packet->m_numTyreStints && i < 8; ++i) {
    //         const TyreStintHistoryData& stint = packet->m_tyreStintsHistoryData[i];
    //         file << "  Stint " << i << ": EndLap=" << static_cast<int>(stint.m_endLap)
    //              << ", ActualCompound=" << static_cast<int>(stint.m_tyreActualCompound)
    //              << ", VisualCompound=" << static_cast<int>(stint.m_tyreVisualCompound) << "\n";
    //     }
    // }
}

void writeTyreSetsPacket(const uint8_t* data, std::ofstream& file) {
    const PacketTyreSetsData* packet = reinterpret_cast<const PacketTyreSetsData*>(data);
    file << "Tyre Sets: CarIdx=" << static_cast<int>(packet->m_carIdx)
         << ", FittedIndex=" << static_cast<int>(packet->m_fittedIdx) << "\n";
    for (int i = 0; i < 20; ++i) {
        const TyreSetData& t = packet->m_tyreSetData[i];
        file << "  Set " << i << ": ActualCompound=" << static_cast<int>(t.m_actualTyreCompound)
             << ", VisualCompound=" << static_cast<int>(t.m_visualTyreCompound)
             << ", Wear=" << static_cast<int>(t.m_wear)
             << ", Available=" << static_cast<int>(t.m_available)
             << ", LifeSpan=" << static_cast<int>(t.m_lifeSpan) << "\n";
    }
}

void writeMotionExPacket(const uint8_t* data, std::ofstream& file) {
    const PacketMotionExData* packet = reinterpret_cast<const PacketMotionExData*>(data);
    file << "MotionEx: LocalVel=(" << packet->m_localVelocityX << ", " << packet->m_localVelocityY
         << ", " << packet->m_localVelocityZ << ")\n";
    file << "  AngularVel=(" << packet->m_angularVelocityX << ", " << packet->m_angularVelocityY
         << ", " << packet->m_angularVelocityZ << ")\n";
    file << "  FrontWheelsAngle=" << packet->m_frontWheelsAngle << "\n";
}

