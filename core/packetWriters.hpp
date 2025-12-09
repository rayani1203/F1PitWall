#ifndef PACKET_WRITERS_HPP
#define PACKET_WRITERS_HPP

#include <cstdint>
#include <fstream>

void writeCarTelemetryPacket(const uint8_t* data, std::ofstream& file);
void writeEventPacket(const uint8_t* data, std::ofstream& file);
void writeMotionPacket(const uint8_t* data, std::ofstream& file);
void writeSessionPacket(const uint8_t* data, std::ofstream& file);
void writeLapDataPacket(const uint8_t* data, std::ofstream& file);
void writeParticipantsPacket(const uint8_t* data, std::ofstream& file);
void writeCarSetupsPacket(const uint8_t* data, std::ofstream& file);
void writeCarStatusPacket(const uint8_t* data, std::ofstream& file);
void writeFinalClassificationPacket(const uint8_t* data, std::ofstream& file);
void writeLobbyInfoPacket(const uint8_t* data, std::ofstream& file);
void writeCarDamagePacket(const uint8_t* data, std::ofstream& file);
void writeSessionHistoryPacket(const uint8_t* data, std::ofstream& file);
void writeTyreSetsPacket(const uint8_t* data, std::ofstream& file);
void writeMotionExPacket(const uint8_t* data, std::ofstream& file);
void callPacketTypeWriter(const uint8_t* data, std::ofstream& file, uint8_t packetId);



#endif // PACKET_WRITERS_HPP