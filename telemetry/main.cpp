#include "Visualizer.hpp"
#include "udpListener.hpp"
#include "ReferenceTracker.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    bool referenceLap = false;

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--reference-lap") {
            referenceLap = true;
            std::cout << "Tracking this lap as reference for track calibration.\n";
        }
    }

    // Start UDP listener in background thread
    std::thread listenerThread(startUDPListener);
    listenerThread.detach();

    // Give the listener a moment to bind to the socket
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if(referenceLap) {
        std::cout << "Reference lap mode enabled. Use the track calibration tool after completing the lap.\n";
    }

    Visualizer visualizer(1200, 700);
    ReferenceTracker refTracker;

    if (!visualizer.init()) {
        std::cerr << "Failed to initialize visualizer\n";
        return 1;
    }

    std::cout << "Visualizer initialized. Waiting for telemetry...\n";

    // Main loop
    while (visualizer.update()) {
        if(referenceLap) {
            refTracker.update();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }

    if(referenceLap) {
        refTracker.saveReferenceLap();
    }

    visualizer.shutdown();
    std::cout << "Visualizer closed.\n";
    return 0;
}
