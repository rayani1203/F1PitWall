#pragma once
#include <vector>
#include <cstddef>

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

    // Plot history buffers
    std::vector<float> m_throttleHistory;
    std::vector<float> m_brakeHistory;
    std::vector<float> m_steerHistory;
    std::vector<double> m_timeHistory;
    
    static constexpr size_t MAX_HISTORY = 512;

    void updatePlotData();
    void drawUI();
};
