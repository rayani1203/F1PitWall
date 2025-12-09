#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <sys/stat.h>
#include <dirent.h>
#include <map>
#include <memory>
#include "packetStructs.hpp"
#include "packetWriters.hpp"
// ===================== PACKET IDS =====================

enum PacketID : uint8_t {
    MOTION = 0,                    // Contains all motion data for player's car
    SESSION = 1,                   // Data about the session – track, time left
    LAP_DATA = 2,                  // Data about all the lap times of cars in the session
    EVENT = 3,                     // Various notable events that happen during a session
    PARTICIPANTS = 4,              // List of participants in the session
    CAR_SETUPS = 5,                // Packet detailing car setups for cars in the race
    CAR_TELEMETRY = 6,             // Telemetry data for all cars
    CAR_STATUS = 7,                // Status data for all cars
    FINAL_CLASSIFICATION = 8,      // Final classification confirmation at the end of a race
    LOBBY_INFO = 9,                // Information about players in a multiplayer lobby
    CAR_DAMAGE = 10,               // Damage status for all cars
    SESSION_HISTORY = 11,          // Lap and tyre data for session
    TYRE_SETS = 12,                // Extended tyre set data
    MOTION_EX = 13                 // Extended motion data for player car
};

// Helper function to get packet type name
std::string getPacketTypeName(uint8_t packetId) {
    switch (packetId) {
        case MOTION: return "motion";
        case SESSION: return "session";
        case LAP_DATA: return "lap_data";
        case EVENT: return "event";
        case PARTICIPANTS: return "participants";
        case CAR_SETUPS: return "car_setups";
        case CAR_TELEMETRY: return "car_telemetry";
        case CAR_STATUS: return "car_status";
        case FINAL_CLASSIFICATION: return "final_classification";
        case LOBBY_INFO: return "lobby_info";
        case CAR_DAMAGE: return "car_damage";
        case SESSION_HISTORY: return "session_history";
        case TYRE_SETS: return "tyre_sets";
        case MOTION_EX: return "motion_ex";
        default: return "unknown";
    }
}

// Global file handle cache
std::map<uint8_t, std::shared_ptr<std::ofstream>> fileCache;

// Helper function to get or create cached file handle
std::ofstream& getCachedFileHandle(uint8_t packetId) {
    if (fileCache.find(packetId) == fileCache.end()) {
        // Create telemetry directory if it doesn't exist
        struct stat st;
        if (stat("telemetry_data", &st) != 0) {
            mkdir("telemetry_data", 0755);
        }

        std::stringstream filename;
        filename << "telemetry_data/" << getPacketTypeName(packetId) << ".txt";

        auto filePtr = std::make_shared<std::ofstream>(filename.str(), std::ios::app);
        if (!filePtr->is_open()) {
            std::cerr << "Warning: Unable to open file: " << filename.str() << "\n";
        }
        fileCache[packetId] = filePtr;
    }

    return *fileCache[packetId];
}

void callPacketTypeWriter(const uint8_t* data, std::ofstream& file, uint8_t packetId) {
    // If this is a car telemetry packet, parse the telemetry entries into readable text
    if (packetId == CAR_TELEMETRY) {
        writeCarTelemetryPacket(data, file);
    } else if (packetId == MOTION) {
        writeMotionPacket(data, file);
    } else if (packetId == SESSION) {
        writeSessionPacket(data, file);
    } else if (packetId == LAP_DATA) {
        writeLapDataPacket(data, file);
    } else if (packetId == EVENT) {
        writeEventPacket(data, file);
    } else if (packetId == PARTICIPANTS) {
        writeParticipantsPacket(data, file);
    } else if (packetId == CAR_SETUPS) {
        writeCarSetupsPacket(data, file);
    } else if (packetId == CAR_STATUS) {
        writeCarStatusPacket(data, file);
    } else if (packetId == FINAL_CLASSIFICATION) {
        writeFinalClassificationPacket(data, file);
    } else if (packetId == LOBBY_INFO) {
        writeLobbyInfoPacket(data, file);
    } else if (packetId == CAR_DAMAGE) {
        writeCarDamagePacket(data, file);
    } else if (packetId == SESSION_HISTORY) {
        writeSessionHistoryPacket(data, file);
    } else if (packetId == TYRE_SETS) {
        writeTyreSetsPacket(data, file);
    } else if (packetId == MOTION_EX) {
        writeMotionExPacket(data, file);
    } else {
        file << "Unknown packet type: " << static_cast<int>(packetId);
    }
    file << "\n";
}

// Helper function to write packet to file
void writePacketToFile(uint8_t packetId, const uint8_t* data, size_t size) {
    try {
        if (size < sizeof(PacketHeader)) {
            std::cerr << "Packet too small to contain header (" << size << " bytes)\n";
            return;
        }

        std::ofstream& file = getCachedFileHandle(packetId);
        if (!file.is_open()) {
            return;
        }

        // timestamp
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        file << "==== " << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << " ====\n";

        callPacketTypeWriter(data, file, packetId);

        file.flush();  // Flush to disk but keep file open
        
    } catch (const std::exception& e) {
        std::cerr << "Error writing packet file: " << e.what() << "\n";
    }
}

// UDP listener thread function - call from main to start listening
void startUDPListener() {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(20777);
    addr.sin_addr.s_addr = INADDR_ANY;  // listen on all interfaces

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind socket\n";
        close(sock);
        return;
    }

    // Clear all text files in telemetry_data directory if it exists
    DIR* dir = opendir("telemetry_data");
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG && std::string(entry->d_name).find(".txt") != std::string::npos) {
                std::string filepath = "telemetry_data/" + std::string(entry->d_name);
                remove(filepath.c_str());
            }
        }
        closedir(dir);
    }

    std::cout << "UDP Listener: Listening on port 20777...\n";
    std::cout << "Waiting for F1 telemetry packets...\n\n";

    uint8_t buffer[2048];

    while (true) {
        ssize_t bytes = recv(sock, buffer, sizeof(buffer), 0);

        if (bytes <= 0) continue;

        if (bytes < sizeof(PacketHeader)) {
            std::cerr << "Packet too small to contain header\n";
            continue;
        }

        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

        // Verify packet format (should be 2023)
        if (header->m_packetFormat != 2023) {
            std::cerr << "Invalid packet format: " << header->m_packetFormat << "\n";
            continue;
        }

        // Get packet type name and write to file
        writePacketToFile(header->m_packetId, buffer, bytes);
    }

    close(sock);
}
