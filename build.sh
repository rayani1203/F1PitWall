#!/bin/bash

# F1 Telemetry Visualizer Build Script

set -e

THIRDPARTY_DIR="thirdparty"
BUILD_DIR="build"

# Create build directory
mkdir -p $BUILD_DIR

# Compiler flags
CFLAGS="-std=c++17 -fPIC -O2"
INCLUDE_DIRS="-Icore -Ilive -I$THIRDPARTY_DIR/imgui -I$THIRDPARTY_DIR/imgui/backends -I$THIRDPARTY_DIR/implot -I$THIRDPARTY_DIR/glfw/include -I$THIRDPARTY_DIR/glad/include -Itools"

# Source files
SOURCES="core/udpListener.cpp core/packetWriters.cpp live/ReferenceTracker.cpp live/LiveTelemetry.cpp live/Visualizer.cpp live/StaticInfo.cpp telemetry/main.cpp $THIRDPARTY_DIR/imgui/imgui.cpp $THIRDPARTY_DIR/imgui/imgui_demo.cpp $THIRDPARTY_DIR/imgui/imgui_draw.cpp $THIRDPARTY_DIR/imgui/imgui_tables.cpp $THIRDPARTY_DIR/imgui/imgui_widgets.cpp $THIRDPARTY_DIR/imgui/backends/imgui_impl_glfw.cpp $THIRDPARTY_DIR/imgui/backends/imgui_impl_opengl3.cpp $THIRDPARTY_DIR/implot/implot.cpp $THIRDPARTY_DIR/implot/implot_items.cpp $THIRDPARTY_DIR/glad/src/glad.c"

# Link libraries (macOS)
LIBS="-framework CoreFoundation -framework Cocoa -framework IOKit -framework OpenGL -framework Metal -framework MetalKit -framework QuartzCore"
GLFW_LIB="$THIRDPARTY_DIR/glfw/build/src/libglfw3.a"

# Compile
echo "Building F1 Telemetry Visualizer..."
clang++ $CFLAGS $INCLUDE_DIRS $SOURCES $GLFW_LIB $LIBS -o $BUILD_DIR/telemetry_viz

echo "Build complete: $BUILD_DIR/telemetry_viz"

# Build the calibration tool
CALIB_SOURCES="tools/track_calibration/track_calibration.cpp live/StaticInfo.cpp live/ReferenceTracker.cpp live/LiveTelemetry.cpp"
clang++ $CFLAGS $INCLUDE_DIRS $CALIB_SOURCES -o $BUILD_DIR/track_calibration

echo "Build complete: $BUILD_DIR/track_calibration"