#include "Visualizer.hpp"
#include "LiveTelemetry.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <iostream>
#include <cmath>

Visualizer::Visualizer(int windowWidth, int windowHeight)
    : m_windowWidth(windowWidth), m_windowHeight(windowHeight), m_window(nullptr) {
    m_throttleHistory.reserve(MAX_HISTORY);
    m_brakeHistory.reserve(MAX_HISTORY);
    m_steerHistory.reserve(MAX_HISTORY);
    m_timeHistory.reserve(MAX_HISTORY);
}

Visualizer::~Visualizer() {
    shutdown();
}

bool Visualizer::init() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    // GL 3.3 core
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(m_windowWidth, m_windowHeight, 
                                          "F1 Telemetry Visualizer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    m_window = window;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return true;
}

void Visualizer::shutdown() {
    if (m_window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        glfwDestroyWindow(static_cast<GLFWwindow*>(m_window));
        glfwTerminate();
        m_window = nullptr;
    }
}

void Visualizer::updatePlotData() {
    LiveInputSample samples[512];
    size_t count = LiveTelemetry::copyHistory(samples, 512);

    m_throttleHistory.clear();
    m_brakeHistory.clear();
    m_steerHistory.clear();
    m_timeHistory.clear();

    for (size_t i = 0; i < count; i++) {
        m_throttleHistory.push_back(samples[i].throttle);
        m_brakeHistory.push_back(samples[i].brake);
        m_steerHistory.push_back(samples[i].steer);
        m_timeHistory.push_back(static_cast<double>(i) * 0.016);  // ~60Hz, 16ms per frame
    }
}

void Visualizer::drawUI() {
    // Main plot window
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(m_windowWidth, m_windowHeight), ImGuiCond_FirstUseEver);
    ImGui::Begin("Driver Inputs");

    if (ImPlot::BeginPlot("Throttle / Brake / Steer", ImVec2(-1, 400))) {
        // Setup axes with auto-fitting for Y and manual X to show latest data
        ImPlot::SetupAxes("Sample", "Value", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Outside);

        if (!m_throttleHistory.empty()) {
            int numSamples = (int)m_throttleHistory.size();
            
            // Pan to show latest data on the right side (last 300 samples)
            // Must be called BEFORE PlotLine
            int startSample = std::max(0, numSamples - 300);
            ImPlot::SetupAxisLimits(ImAxis_X1, startSample - 10, numSamples + 10, ImGuiCond_Always);
            
            ImPlot::PlotLine("Throttle", m_throttleHistory.data(), numSamples);
            ImPlot::PlotLine("Brake", m_brakeHistory.data(), numSamples);
            ImPlot::PlotLine("Steer", m_steerHistory.data(), numSamples);
        }

        ImPlot::EndPlot();
    }

    // Stats panel
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Live Values:");
    
    if (!m_throttleHistory.empty()) {
        ImGui::Text("Throttle: %.2f%%", m_throttleHistory.back() * 100.0f);
        ImGui::SameLine(200);
        ImGui::Text("Brake: %.2f%%", m_brakeHistory.back() * 100.0f);
        ImGui::SameLine(400);
        ImGui::Text("Steer: %.2f", m_steerHistory.back());
    } else {
        ImGui::Text("Waiting for telemetry data...");
    }

    ImGui::Text("Samples in buffer: %zu / %zu", m_throttleHistory.size(), MAX_HISTORY);

    ImGui::End();

    // Stats window
    ImGui::Begin("Statistics");

    if (!m_throttleHistory.empty()) {
        float throttleMax = *std::max_element(m_throttleHistory.begin(), m_throttleHistory.end());
        float brakeMax = *std::max_element(m_brakeHistory.begin(), m_brakeHistory.end());
        float steerMax = *std::max_element(m_steerHistory.begin(), m_steerHistory.end());
        float steerMin = *std::min_element(m_steerHistory.begin(), m_steerHistory.end());

        ImGui::Text("Throttle Max: %.2f%%", throttleMax * 100.0f);
        ImGui::Text("Brake Max: %.2f%%", brakeMax * 100.0f);
        ImGui::Text("Steer Range: [%.2f, %.2f]", steerMin, steerMax);
    }

    ImGui::End();
}

bool Visualizer::update() {
    GLFWwindow* window = static_cast<GLFWwindow*>(m_window);
    
    if (glfwWindowShouldClose(window)) {
        return false;
    }

    // Start frame
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Update plot data from ring buffer
    updatePlotData();

    // Draw UI
    drawUI();

    // Render
    ImGui::Render();
    
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    
    return true;
}
