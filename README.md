# F1 Telemetry Visualizer

A real-time visualization tool for F1 2023 telemetry data with live plots of driver inputs (throttle, brake, steering).

## Architecture

### Components

1. **UDP Listener** (`core/udpListener.cpp`)
   - Listens on port 20777 for F1 2023 telemetry packets
   - Validates packet format and routes to packet writers
   - Runs in a background thread
   - Pushes live telemetry data to the ring buffer

2. **Live Ring Buffer** (`live/RingBuffer.hpp`, `live/LiveTelemetry.hpp`)
   - Thread-safe circular buffer (lock-free, atomic-based)
   - Stores last 512 `LiveInputSample` entries
   - Provides `copyHistory()` for snapshot reads
   - Data: throttle, brake, steer, timestamp

3. **Visualizer** (`live/Visualizer.hpp`, `live/Visualizer.cpp`)
   - ImGui + ImPlot-based GUI
   - OpenGL 3.3 rendering via GLFW
   - Real-time line plots of throttle/brake/steer
   - Live statistics (current values, max values, steer range)
   - 60 FPS update loop

4. **Packet Writers** (`core/packetWriters.cpp`)
   - Decodes all 14 F1 packet types
   - Writes human-readable telemetry to log files
   - Pushes car telemetry samples to ring buffer

## Building

```bash
cd /Users/rayani1203/Documents/F1
./build.sh
```

This will:
- Build GLFW (if not already built)
- Compile all sources
- Link against ImGui, ImPlot, GLFW, and OpenGL
- Output: `build/telemetry_viz`

## Running

```bash
./build/telemetry_viz
```

The visualizer will:
1. Start the UDP listener on port 20777
2. Open a 1200x700 window with ImGui/ImPlot
3. Display real-time plots as telemetry packets arrive
4. Show live driver input values and statistics

**Requires:** F1 2023 game running with UDP telemetry enabled on the same network.

## Data Flow

```
F1 2023 Game
    ↓
UDP Port 20777
    ↓
startUDPListener() thread
    ↓
Packet validation & decode
    ↓
writeCarTelemetryPacket()
    ├→ Write to log files
    └→ g_liveInputs.push(sample)
    ↓
RingBuffer<LiveInputSample, 512>
    ↓
Visualizer main loop
    ├→ copyHistory(samples, 512)
    ├→ Update plot buffers
    └→ Render ImPlot graphs
```

## UI Layout

**Input Controls Window:**
- Line plot showing throttle, brake, and steer over time
- Live value display (current throttle %, brake %, steer position)
- Sample count in buffer

**Statistics Window:**
- Maximum throttle percentage
- Maximum brake percentage
- Steer range (min/max values)

## Customization

- **Window size:** Edit `Visualizer::Visualizer(int width, int height)` in main.cpp
- **Buffer size:** Change `RingBuffer<LiveInputSample, 512>` template parameter
- **Plot scaling:** Modify `ImPlot::SetupAxes()` flags in Visualizer::drawUI()
- **Update rate:** Adjust sleep duration in main loop (currently 16ms = 60 FPS)

## Files Structure

```
F1/
├── core/
│   ├── udpListener.cpp          # Socket listener + packet dispatch
│   ├── udpListener.hpp          # startUDPListener() declaration
│   ├── packetStructs.hpp        # Binary packet format definitions
│   ├── packetWriters.cpp        # Packet decoding & file output
│   └── packetWriters.hpp        # Writer function declarations
├── live/
│   ├── RingBuffer.hpp           # Lock-free circular buffer
│   ├── LiveTelemetry.hpp        # Global buffer & copyHistory()
│   ├── LiveTelemetry.cpp        # Implementation
│   ├── Visualizer.hpp           # Visualizer class interface
│   └── Visualizer.cpp           # ImGui/ImPlot implementation
├── telemetry/
│   └── main.cpp                 # Application entry point
├── build.sh                     # Build script
└── thirdparty/
    ├── glfw/                    # Window/input library
    ├── imgui/                   # GUI framework
    ├── implot/                  # Plotting library
    └── glad/                    # OpenGL loader
```

## Dependencies

- C++17 compiler
- GLFW 3.3+ (included in thirdparty)
- ImGui (included in thirdparty)
- ImPlot (included in thirdparty)
- GLAD (OpenGL loader)
- OpenGL 3.3+
- macOS frameworks: Cocoa, Metal, QuartzCore, etc.

## Thread Safety

- **Ring buffer:** Uses `std::atomic<size_t>` for write-index synchronization
- **UDP listener:** Runs in detached background thread
- **Visualizer:** Polls ring buffer in main thread (non-blocking reads)
- **No mutexes:** Lockfree design for minimal latency

## Known Limitations

- OpenGL 3.3 deprecated on macOS 10.14+; consider Metal backend migration
- Ring buffer can miss samples if buffer overflows (oldest samples discarded)
- Single-car telemetry (player car only) currently displayed
