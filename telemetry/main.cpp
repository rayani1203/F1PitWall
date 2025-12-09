#include "Visualizer.hpp"
#include "udpListener.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Start UDP listener in background thread
    std::thread listenerThread(startUDPListener);
    listenerThread.detach();

    // Give the listener a moment to bind to the socket
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Visualizer visualizer(1200, 700);

    if (!visualizer.init()) {
        std::cerr << "Failed to initialize visualizer\n";
        return 1;
    }

    std::cout << "Visualizer initialized. Waiting for telemetry...\n";

    // Main loop
    while (visualizer.update()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }

    visualizer.shutdown();
    std::cout << "Visualizer closed.\n";
    return 0;
}
