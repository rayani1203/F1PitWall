#include "../live/LiveTelemetry.hpp"
#include <imgui.h>
#include <implot.h>
#include <vector>

void DrawInputsWindow() {
    LiveInputSample history[512];
    size_t count = LiveTelemetry::copyHistory(history, 512);
    
    static std::vector<float> throttle;
    static std::vector<float> brake;
    throttle.resize(count);
    brake.resize(count);

    for (size_t i = 0; i < count; i++) {
        throttle[i] = history[i].throttle;
        brake[i]    = history[i].brake;
    }

    ImGui::Begin("Inputs");

    if (ImPlot::BeginPlot("Throttle/Brake History")) {
        ImPlot::PlotLine("Throttle", throttle.data(), throttle.size());
        ImPlot::PlotLine("Brake", brake.data(), brake.size());
        ImPlot::EndPlot();
    }

    ImGui::End();
}
