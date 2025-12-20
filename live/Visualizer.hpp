#pragma once
#include <vector>
#include <cstddef>
#include "ReferenceTracker.hpp"

class Visualizer {
public:
    Visualizer(int windowWidth = 1200, int windowHeight = 700);
    ~Visualizer();

    // Main loop - returns false when window should close
    bool update();

    // Initialize ImGui/GLFW
    bool init();

    // Cleanup
    void shutdown();

private:
    int m_windowWidth;
    int m_windowHeight;
    void* m_window;  // GLFWwindow*
    bool have_loaded_reference = false;

    // Plot history buffers
    // Use double for all plot arrays so ImPlot can take xs and ys with the same type
    std::vector<double> m_throttleHistory;
    std::vector<double> m_brakeHistory;
    std::vector<double> m_steerHistory;
    std::vector<double> m_timeHistory;
    std::vector<int> m_clutchHistory;
    std::vector<int> m_drsHistory;
    std::vector<int> m_gearHistory;
    
    std::vector<Vec3> referenceLap_;
    
    static constexpr size_t MAX_HISTORY = 512;

    void updatePlotData();
    void drawUI();
    void drawMiniMap();
};
