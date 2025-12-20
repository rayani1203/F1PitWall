#include "Visualizer.hpp"
#include "LiveTelemetry.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <iostream>
#include <cmath>
#include <string>

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

std::vector<Vec3> loadReferenceLap(int trackId) {
    if (trackId < 0) {
        return {};
    }
    ReferenceTracker tracker;
    if (tracker.loadReferenceLap(trackId)) {
        return tracker.getLapPositions();
    } else {
        return {};
    }
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

    // Load reference lap
    std::vector<Vec3> loaded_reference = loadReferenceLap(g_staticInfo.track_id);
    if (loaded_reference.size() > 0) {
        have_loaded_reference = true;
        referenceLap_ = loaded_reference;
        std::cout << "Loaded reference lap for track " << g_staticInfo.track_id << "\n";
    }
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
    m_clutchHistory.clear();
    m_drsHistory.clear();
    m_gearHistory.clear();

    for (size_t i = 0; i < count; i++) {
        m_throttleHistory.push_back(samples[i].throttle);
        m_brakeHistory.push_back(samples[i].brake);
        m_steerHistory.push_back(samples[i].steer);
        m_clutchHistory.push_back(static_cast<int>(samples[i].clutch));
        m_drsHistory.push_back(static_cast<int>(samples[i].drs));
        m_gearHistory.push_back(static_cast<int>(samples[i].gear));
        m_timeHistory.push_back(samples[i].timestampMs / 1000.0);  // Convert ms to seconds
    }
}

void Visualizer::drawMiniMap() {
    if (!have_loaded_reference){
        std::vector<Vec3> loaded_reference = loadReferenceLap(g_staticInfo.track_id);
        if (loaded_reference.size() > 0) {
            have_loaded_reference = true;
            referenceLap_ = loaded_reference;
            std::cout << "Loaded reference lap for track " << g_staticInfo.track_id << "\n";
        } else {
            return;
        }
    }

    ImGui::Begin("Mini Map");

    static std::vector<float> xs, zs; // top-down XZ view
    xs.resize(referenceLap_.size());
    zs.resize(referenceLap_.size());

    float minX = referenceLap_[0].x, maxX = referenceLap_[0].x;
    float minZ = referenceLap_[0].z, maxZ = referenceLap_[0].z;

    for (size_t i = 0; i < referenceLap_.size(); ++i) {
        xs[i] = referenceLap_[i].x;
        zs[i] = referenceLap_[i].z;

        if (xs[i] < minX) minX = xs[i];
        if (xs[i] > maxX) maxX = xs[i];
        if (zs[i] < minZ) minZ = zs[i];
        if (zs[i] > maxZ) maxZ = zs[i];
    }

    // Add 5% buffer around edges
    float xBuffer = (maxX - minX) * 0.05f;
    float zBuffer = (maxZ - minZ) * 0.05f;

    if (ImPlot::BeginPlot("Track", ImVec2(300, 300),
                          ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText |
                          ImPlotFlags_NoBoxSelect)) {

        // Set axes limits with buffer
        ImPlot::SetupAxisLimits(ImAxis_X1, minX - xBuffer, maxX + xBuffer);
        ImPlot::SetupAxisLimits(ImAxis_Y1, minZ - zBuffer, maxZ + zBuffer);

        // Remove tick labels/decorations
        ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoDecorations);

        // Plot thick line
        ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 8.0f); // much thicker
        ImPlot::PlotLine("Reference Lap", xs.data(), zs.data(), (int)xs.size());
        ImPlot::PopStyleVar();


        LivePositionSample latest_position;
        LiveTelemetry::peekLatestPosition(latest_position);
        float carX = latest_position.worldX;
        float carZ = latest_position.worldZ;
        ImPlot::PlotScatter("Car Position", &carX, &carZ, 1);

        ImPlot::EndPlot();
    }

    ImGui::End();
}

void Visualizer::drawUI() {
    // Main plot window
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(m_windowWidth, m_windowHeight), ImGuiCond_FirstUseEver);
    ImGui::Begin("Driver Inputs");

    if (ImPlot::BeginPlot("Throttle / Brake / Steer", ImVec2(-1, 400))) {
        // Setup axes with time X axis and auto-fit Y
        ImPlot::SetupAxes("Time (s)", "Value", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Outside);

        if (!m_timeHistory.empty()) {
            int numSamples = (int)m_timeHistory.size();

            // Compute seconds-ago X values so newest point is 0 and older points are negative
            std::vector<double> xvals(numSamples);
            double tEnd = m_timeHistory.back();
            for (int i = 0; i < numSamples; ++i) {
                xvals[i] = m_timeHistory[i] - tEnd; // negative or zero
            }

            double xmin = xvals.front(); // oldest (most negative)
            double xmax = 0.0; // newest
            // Must set axis limits before plotting
            ImPlot::SetupAxisLimits(ImAxis_X1, xmin, xmax, ImGuiCond_Always);
            // Ensure Y axis leaves room for markers slightly above 1.0
            ImPlot::SetupAxisLimits(ImAxis_Y1, -1.2, 1.3, ImGuiCond_Always);

            // Plot using seconds-ago X values
            ImPlot::PlotLine("Throttle", xvals.data(), m_throttleHistory.data(), numSamples);
            ImPlot::PlotLine("Brake", xvals.data(), m_brakeHistory.data(), numSamples);
            ImPlot::PlotLine("Steer", xvals.data(), m_steerHistory.data(), numSamples);

            // Build marker lists for gear-change events (detect when gear differs from previous sample)
            std::vector<double> gearUpX, gearUpY;
            std::vector<double> gearDownX, gearDownY;
            for (int i = 1; i < numSamples; ++i) {
                int prev = m_gearHistory[i-1];
                int cur = m_gearHistory[i];
                if (cur != prev) {
                    double x = xvals[i];
                    if (cur > prev) {
                        gearUpX.push_back(x);
                        gearUpY.push_back(1.15);
                    } else {
                        gearDownX.push_back(x);
                        gearDownY.push_back(1.15);
                    }
                }
            }

            // Draw DRS as shaded horizontal bands spanning contiguous DRS-on ranges
            ImVec2 plotPos = ImPlot::GetPlotPos();
            ImVec2 plotSize = ImPlot::GetPlotSize();
            double xMin = xmin;
            double xMax = xmax;
            double yMin = -1.2;
            double yMax = 1.3;
            auto xToPixel = [&](double x) {
                return plotPos.x + (float)((x - xMin) / (xMax - xMin) * plotSize.x);
            };
            auto yToPixel = [&](double y) {
                // ImPlot's origin is top-left of plot area; invert Y mapping
                return plotPos.y + plotSize.y - (float)((y - yMin) / (yMax - yMin) * plotSize.y);
            };

            ImDrawList* plotDL = ImPlot::GetPlotDrawList();
            const float bandTop = yToPixel(1.05);
            const float bandBottom = yToPixel(0.85);
            // Scan for contiguous DRS-on ranges
            int idx = 0;
            while (idx < numSamples) {
                // find start
                while (idx < numSamples && m_drsHistory[idx] == 0) ++idx;
                if (idx >= numSamples) break;
                int start = idx;
                while (idx < numSamples && m_drsHistory[idx] != 0) ++idx;
                int end = idx - 1;

                double xStart = xvals[start];
                double xEnd = xvals[end];
                float px0 = xToPixel(xStart);
                float px1 = xToPixel(xEnd);
                // Clamp within plot region
                if (px1 < plotPos.x) continue;
                if (px0 > plotPos.x + plotSize.x) continue;
                if (px0 < plotPos.x) px0 = plotPos.x;
                if (px1 > plotPos.x + plotSize.x) px1 = plotPos.x + plotSize.x;

                ImU32 fillCol = ImGui::GetColorU32(ImVec4(0.0f, 0.6f, 1.0f, 0.12f));
                ImU32 borderCol = ImGui::GetColorU32(ImVec4(0.0f, 0.6f, 1.0f, 0.35f));
                plotDL->AddRectFilled(ImVec2(px0, bandTop), ImVec2(px1, bandBottom), fillCol);
                plotDL->AddRect(ImVec2(px0, bandTop), ImVec2(px1, bandBottom), borderCol, 0.0f, 0, 1.0f);
            }

            if (!gearUpX.empty()) {
                ImVec4 upCol = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 7, upCol);
                ImPlot::PlotScatter("Upshift", gearUpX.data(), gearUpY.data(), (int)gearUpX.size());
            }
            if (!gearDownX.empty()) {
                ImVec4 downCol = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 7, downCol);
                ImPlot::PlotScatter("Downshift", gearDownX.data(), gearDownY.data(), (int)gearDownX.size());
            }
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

    // Car Inputs live window - show latest full telemetry sample (peekLatest)
    {
        LiveInputSample latest;
        if (LiveTelemetry::peekLatest(latest)) {
            ImGui::Begin("Car Inputs");
            // Big speed + gear
            // Prepare gear label
            std::string gearLabel;
            if (latest.gear == -1) gearLabel = "R";
            else if (latest.gear == 0) gearLabel = "N";
            else gearLabel = std::to_string(static_cast<int>(latest.gear));

            ImGui::PushFont(ImGui::GetFont());
            ImGui::Text("%u km/h", latest.speed);
            ImGui::SameLine(200);
            ImGui::Text("Gear: %s", gearLabel.c_str());
            ImGui::PopFont();

            // RPM bar (colored, with redline)
            ImGui::Separator();
            const int redline = 11500; // approximate
            float rpmNorm = (float)latest.engineRPM / (float)redline;
            if (rpmNorm > 1.0f) rpmNorm = 1.0f;
            ImU32 rpmColor = ImGui::GetColorU32(ImVec4(rpmNorm, 1.0f - rpmNorm, 0.0f, 1.0f));
            ImGui::Text("RPM: %u", latest.engineRPM);
            ImGui::SameLine(200);
            // Rev lights: draw a horizontal LED strip (16 LEDs)
            const int numLEDs = 16;
            int lit = (int)std::round((latest.revLightsPercent / 100.0f) * numLEDs);
            if (lit < 0) lit = 0; if (lit > numLEDs) lit = numLEDs;
            ImVec2 ledPos = ImGui::GetCursorScreenPos();
            float ledW = 12.0f;
            float ledH = 8.0f;
            float spacing = 4.0f;
            float totalW = numLEDs * ledW + (numLEDs - 1) * spacing;
            // center the strip
            ImVec2 winAvail = ImGui::GetContentRegionAvail();
            float startX = ledPos.x + (winAvail.x - totalW) * 0.5f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            for (int i = 0; i < numLEDs; ++i) {
                float x = startX + i * (ledW + spacing);
                ImVec2 a(x, ledPos.y);
                ImVec2 b(x + ledW, ledPos.y + ledH);
                if (i < lit) {
                    // gradient from yellow -> red
                    float t = (float)i / (float)(numLEDs - 1);
                    ImU32 col = ImGui::GetColorU32(ImVec4(1.0f, 0.8f * (1.0f - t), 0.0f + 0.5f * t, 1.0f));
                    dl->AddRectFilled(a, b, col, 2.0f);
                } else {
                    dl->AddRectFilled(a, b, ImGui::GetColorU32(ImVec4(0.12f,0.12f,0.12f,1.0f)), 2.0f);
                }
                dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(0,0,0,0.25f)), 2.0f);
            }
            // reserve space for the drawn LEDs
            ImGui::Dummy(ImVec2(totalW, ledH + 4.0f));
            ImGui::Text("Clutch: %u%%", latest.clutch);
            ImGui::Text("DRS: %s", latest.drs ? "On" : "Off");

            // Throttle & Brake bars side-by-side
            ImGui::Spacing();
            float barWidth = ImGui::GetContentRegionAvail().x * 0.48f;
            ImGui::BeginGroup();
            ImGui::Text("Throttle");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::ProgressBar(latest.throttle, ImVec2(barWidth, 0));
            ImGui::PopStyleColor();
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("Brake");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::ProgressBar(latest.brake, ImVec2(barWidth, 0));
            ImGui::PopStyleColor();
            ImGui::EndGroup();

            // Steering wheel radial indicator
            ImGui::Spacing();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 cur = ImGui::GetCursorScreenPos();
            float wheelSize = 80.0f;
            ImVec2 center = ImVec2(cur.x + wheelSize + 10, cur.y + wheelSize + 10);
            // Reserve space
            ImGui::Dummy(ImVec2(wheelSize * 2 + 20, wheelSize * 2 + 20));
            // Draw wheel
            ImU32 wheelCol = ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
            draw->AddCircleFilled(center, wheelSize, wheelCol);
            draw->AddCircle(center, wheelSize, ImGui::GetColorU32(ImVec4(0.8f,0.8f,0.8f,1.0f)), 32, 2.0f);
            // Steering angle line
            const float PI = 3.14159265f;
            // Map steering -1..1 to approx -270..270 degrees, but rotate so 0 points up
            float angle = latest.steer * 1.5f * PI - 0.5f * PI; // 0 -> -90deg (up)
            float lx = cosf(angle) * (wheelSize * 0.6f);
            float ly = sinf(angle) * (wheelSize * 0.6f);
            ImVec2 p2 = ImVec2(center.x + lx, center.y + ly);
            draw->AddLine(center, p2, ImGui::GetColorU32(ImVec4(1.0f,0.6f,0.0f,1.0f)), 3.0f);
            // Steering numeric
            ImGui::SetCursorScreenPos(ImVec2(center.x - 40, center.y + wheelSize + 8));
            ImGui::Text("Steer: %.2f", latest.steer);

            ImGui::End();
        }
    }

    ImGui::Text("Samples in buffer: %zu / %zu", m_throttleHistory.size(), MAX_HISTORY);

    ImGui::End();

    // Stats window
    ImGui::Begin("Statistics");

    if (!m_throttleHistory.empty()) {
        double throttleMax = *std::max_element(m_throttleHistory.begin(), m_throttleHistory.end());
        double brakeMax = *std::max_element(m_brakeHistory.begin(), m_brakeHistory.end());
        double steerMax = *std::max_element(m_steerHistory.begin(), m_steerHistory.end());
        double steerMin = *std::min_element(m_steerHistory.begin(), m_steerHistory.end());

        ImGui::Text("Throttle Max: %.2f%%", throttleMax * 100.0);
        ImGui::Text("Brake Max: %.2f%%", brakeMax * 100.0);
        ImGui::Text("Steer Range: [%.2f, %.2f]", steerMin, steerMax);
    }

    drawMiniMap();

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
