#!/usr/bin/env python3
import struct

BIN = 'telemetry_data/session_history_last.bin'
LAYOUT = 'telemetry_data/session_history_layout.txt'

# This script parses PacketSessionHistoryData according to the project's
# packed C++ definitions (see core/packetStructs.hpp). It reads the binary
# dump written by the UDP listener and prints human-readable header, session
# history metadata, lap history entries (only up to reported m_numLaps) and
# tyre stint history entries (up to m_numTyreStints).

import os
import struct

# Sizes derived from the packed C++ structs in packetStructs.hpp:
# PacketHeader size = 29 bytes
HEADER_SIZE = 29
# Session history metadata after header: 7 bytes
SESSION_META_SIZE = 7
# LapHistoryData size (packed) = 14 bytes
LAP_HISTORY_SIZE = 14
# TyreStintHistoryData size = 3 bytes
TYRE_STINT_SIZE = 3


def hexdump(data, width=16):
    for i in range(0, len(data), width):
        chunk = data[i:i+width]
        hex_bytes = ' '.join(f"{b:02x}" for b in chunk)
        ascii_rep = ''.join((chr(b) if 32 <= b < 127 else '.') for b in chunk)
        print(f"{i:04x}  {hex_bytes:<{width*3}}  |{ascii_rep}|")


def main():
    try:
        with open(BIN, 'rb') as f:
            data = f.read()
    except FileNotFoundError:
        print('Binary dump not found:', BIN)
        return

    total = len(data)
    print('Total bytes:', total)
    if total == 0:
        print('Empty file')
        return

    print('\nHexdump (first 256 bytes):')
    hexdump(data[:256])

    if total < HEADER_SIZE + SESSION_META_SIZE:
        print(f'File too small: need at least {HEADER_SIZE+SESSION_META_SIZE} bytes')
        return

    # Parse PacketHeader fields (little-endian)
    off = 0
    packetFormat = struct.unpack_from('<H', data, off)[0]; off += 2
    gameYear = struct.unpack_from('<B', data, off)[0]; off += 1
    gameMajor = struct.unpack_from('<B', data, off)[0]; off += 1
    gameMinor = struct.unpack_from('<B', data, off)[0]; off += 1
    packetVersion = struct.unpack_from('<B', data, off)[0]; off += 1
    packetId = struct.unpack_from('<B', data, off)[0]; off += 1
    sessionUID = struct.unpack_from('<Q', data, off)[0]; off += 8
    sessionTime = struct.unpack_from('<f', data, off)[0]; off += 4
    frameIdentifier = struct.unpack_from('<I', data, off)[0]; off += 4
    overallFrameIdentifier = struct.unpack_from('<I', data, off)[0]; off += 4
    playerCarIndex = struct.unpack_from('<B', data, off)[0]; off += 1
    secondaryPlayerCarIndex = struct.unpack_from('<B', data, off)[0]; off += 1

    # Sanity: off should equal HEADER_SIZE
    print('\nParsed PacketHeader:')
    print(' packetFormat:', packetFormat)
    print(' gameYear:', gameYear)
    print(' gameMajorVersion:', gameMajor)
    print(' gameMinorVersion:', gameMinor)
    print(' packetVersion:', packetVersion)
    print(' packetId:', packetId)
    print(' sessionUID:', sessionUID)
    print(' sessionTime:', sessionTime)
    print(' frameIdentifier:', frameIdentifier)
    print(' overallFrameIdentifier:', overallFrameIdentifier)
    print(' playerCarIndex:', playerCarIndex)
    print(' secondaryPlayerCarIndex:', secondaryPlayerCarIndex)
    print(' header_bytes_parsed:', off, 'expected HEADER_SIZE:', HEADER_SIZE)

    # Session history metadata
    meta_off = off
    carIdx = struct.unpack_from('<B', data, meta_off)[0]; meta_off += 1
    numLaps = struct.unpack_from('<B', data, meta_off)[0]; meta_off += 1
    numTyreStints = struct.unpack_from('<B', data, meta_off)[0]; meta_off += 1
    bestLapNum = struct.unpack_from('<B', data, meta_off)[0]; meta_off += 1
    bestS1Num = struct.unpack_from('<B', data, meta_off)[0]; meta_off += 1
    bestS2Num = struct.unpack_from('<B', data, meta_off)[0]; meta_off += 1
    bestS3Num = struct.unpack_from('<B', data, meta_off)[0]; meta_off += 1

    print('\nSessionHistory metadata:')
    print(' carIdx:', carIdx)
    print(' numLaps:', numLaps)
    print(' numTyreStints:', numTyreStints)
    print(' bestLapNum:', bestLapNum)
    print(' bestSector1LapNum:', bestS1Num)
    print(' bestSector2LapNum:', bestS2Num)
    print(' bestSector3LapNum:', bestS3Num)

    base_lap_off = HEADER_SIZE + SESSION_META_SIZE
    print('\nComputed base_lap_off:', base_lap_off)

    # Parse lap history entries (only up to reported numLaps, but ensure within bounds)
    max_laps_in_packet = 100
    laps_to_read = min(numLaps, max_laps_in_packet)
    print(f'Parsing {laps_to_read} lap entries (each {LAP_HISTORY_SIZE} bytes)')

    for i in range(laps_to_read):
        lo = base_lap_off + i * LAP_HISTORY_SIZE
        hi = lo + LAP_HISTORY_SIZE
        if hi > total:
            print(f'  Lap {i}: out of bounds (need bytes {lo}-{hi-1}, total {total})')
            break

        lapTime = struct.unpack_from('<I', data, lo)[0]
        s1 = struct.unpack_from('<H', data, lo+4)[0]
        s1min = struct.unpack_from('<B', data, lo+6)[0]
        s2 = struct.unpack_from('<H', data, lo+7)[0]
        s2min = struct.unpack_from('<B', data, lo+9)[0]
        s3 = struct.unpack_from('<H', data, lo+10)[0]
        s3min = struct.unpack_from('<B', data, lo+12)[0]
        validFlags = struct.unpack_from('<B', data, lo+13)[0]

        # Convert times to seconds (and include minutes part)
        lap_sec = lapTime / 1000.0
        s1_sec = (s1min * 60) + (s1 / 1000.0)
        s2_sec = (s2min * 60) + (s2 / 1000.0)
        s3_sec = (s3min * 60) + (s3 / 1000.0)

        flags = []
        if validFlags & 0x01:
            flags.append('LapValid')
        if validFlags & 0x02:
            flags.append('S1Valid')
        if validFlags & 0x04:
            flags.append('S2Valid')
        if validFlags & 0x08:
            flags.append('S3Valid')

        print(f'  Lap {i+1:3d} @ bytes {lo}-{hi-1}: lapTime_ms={lapTime} ({lap_sec:.3f}s) flags=0x{validFlags:02x} {flags}')
        print(f'       S1: {s1} ms + {s1min} min -> {s1_sec:.3f}s')
        print(f'       S2: {s2} ms + {s2min} min -> {s2_sec:.3f}s')
        print(f'       S3: {s3} ms + {s3min} min -> {s3_sec:.3f}s')

    # Parse tyre stint history
    tyre_base = HEADER_SIZE + SESSION_META_SIZE + (max_laps_in_packet * LAP_HISTORY_SIZE)
    print('\nTyre stints start at offset (array base for 8 entries):', tyre_base)
    stints_to_read = min(numTyreStints, 8)
    for i in range(stints_to_read):
        to = tyre_base + i * TYRE_STINT_SIZE
        ti = to + TYRE_STINT_SIZE
        if ti > total:
            print(f'  Tyre stint {i}: out of bounds (need bytes {to}-{ti-1}, total {total})')
            break
        endLap, actual, visual = struct.unpack_from('<BBB', data, to)
        print(f'  TyreStint {i+1}: endLap={endLap} actual={actual} visual={visual}')

    print('\nDone')

if __name__ == '__main__':
    main()
