cmake_minimum_required(VERSION 3.10)
project(UsingPythonic)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The graph viewer target that can be exported by the installed
# package requires OpenGL to be found by CMake before importing
# the `pythonic` config. Ensure OpenGL is available for downstream
# projects that link `pythonic::pythonic_graph_viewer`.

# Prefer modern GLVND OpenGL implementation and set CMP0072 policy
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()
set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)

# Find the Pythonic library (header-only)
# FAILS if not installed
find_package(pythonic REQUIRED)

# The viewer also depends on GLFW at link/config time; projects that
# link `pythonic::pythonic_graph_viewer` may want to ensure GLFW is
# available. Find it here to provide clearer error messages early.
find_package(glfw3 REQUIRED)

# Find ImageMagick for image rendering support
find_package(ImageMagick COMPONENTS Magick++ REQUIRED)

# Find FFmpeg for video rendering support
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libswscale libavutil)

# Add your executable
# --> Replace 'demoapp' with your project executable name
# --> Add all your .cpp files here if you have multiple
add_executable(demoapp demo.cpp)
# For multi-file projects:
# add_executable(demoapp main.cpp file1.cpp file2.cpp)
# or use file(GLOB SOURCES *.cpp) and then add_executable(demoapp ${SOURCES})


# Link libraries to your target
# --> Add other libraries here if your project depends on them
target_link_libraries(demoapp PRIVATE pythonic::pythonic ImageMagick::Magick++ ${FFMPEG_LIBRARIES})

# Add FFmpeg include directories
target_include_directories(demoapp PRIVATE ${FFMPEG_INCLUDE_DIRS})

# If you installed Pythonic with the optional Graph Viewer enabled
# (build/install Pythonic with `-DPYTHONIC_ENABLE_GRAPH_VIEWER=ON`),
# you can link the viewer target to enable `var::show()` GUI support:
#
#    target_link_libraries(demoapp PRIVATE pythonic::pythonic_graph_viewer)
#
# Note: The Graph Viewer requires an OpenGL/GLFW environment at runtime.
# Make sure development packages for GLFW/OpenGL are available when
# building Pythonic and that a desktop display (DISPLAY) is present
# when running the produced binary.

# ============================================================================
# Optional: Enable automatic SVG generation when using Graph::to_dot()
# Requires Graphviz to be installed on your system
# ============================================================================
# Option A: Enable manually (uncomment to enable):
# target_compile_definitions(demoapp PRIVATE GRAPHVIZ_AVAILABLE)

# Option B: Auto-detect Graphviz (recommended):
option(ENABLE_GRAPHVIZ "Enable Graphviz integration for Graph::to_dot()" ON)
if(ENABLE_GRAPHVIZ)
    find_program(GRAPHVIZ_DOT dot)
    if(GRAPHVIZ_DOT)
        message(STATUS "Graphviz found: ${GRAPHVIZ_DOT}")
        message(STATUS "  Graph::to_dot() will automatically generate SVG files")
        target_compile_definitions(demoapp PRIVATE GRAPHVIZ_AVAILABLE)
    else()
        message(STATUS "Graphviz not found (optional)")
        message(STATUS "  Graph::to_dot() will only create .dot files")
        message(STATUS "  Install Graphviz to enable automatic SVG generation")
    endif()
endif()

# Optional status message
message(STATUS "Using installed Pythonic package")
