cmake_minimum_required(VERSION 3.10)
project(UsingPythonic)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threading support (required for std::thread in video export)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

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

# Add SDL2 before pythonic to ensure SDL2::SDL2 target is available if needed
find_package(SDL2 REQUIRED)

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

# ============================================================================
# Optional: SDL2 for audio playback in videos
# ============================================================================
pkg_check_modules(SDL2 QUIET sdl2)
if(SDL2_FOUND)
    message(STATUS "SDL2 found - audio playback enabled")
endif()

# ============================================================================
# Optional: OpenCL for GPU-accelerated video rendering
# ============================================================================
find_package(OpenCL QUIET)
if(OpenCL_FOUND)
    message(STATUS "OpenCL found - GPU acceleration enabled")
endif()

# ============================================================================
# Optional: OpenCV for advanced image/video processing and webcam support
# ============================================================================
find_package(OpenCV QUIET COMPONENTS core imgproc videoio imgcodecs)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found (${OpenCV_VERSION}) - webcam support enabled")
endif()

# Add your executable
# --> Replace 'demoapp' with your project executable name
# --> Add all your .cpp files here if you have multiple
add_executable(demoapp demo.cpp)
# For multi-file projects:
# add_executable(demoapp main.cpp file1.cpp file2.cpp)
# or use file(GLOB SOURCES *.cpp) and then add_executable(demoapp ${SOURCES})


# Link libraries to your target
# --> Add other libraries here if your project depends on them
target_link_libraries(demoapp PRIVATE pythonic::pythonic ImageMagick::Magick++ ${FFMPEG_LIBRARIES} Threads::Threads)

# Add FFmpeg include directories
target_include_directories(demoapp PRIVATE ${FFMPEG_INCLUDE_DIRS})

# Link optional SDL2 for audio playback
if(SDL2_FOUND)
    target_compile_definitions(demoapp PRIVATE PYTHONIC_ENABLE_SDL2_AUDIO)
    target_include_directories(demoapp PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(demoapp PRIVATE ${SDL2_LIBRARIES})
endif()

# Link optional OpenCL for GPU-accelerated video rendering
if(OpenCL_FOUND)
    target_compile_definitions(demoapp PRIVATE PYTHONIC_ENABLE_OPENCL)
    target_include_directories(demoapp PRIVATE ${OpenCL_INCLUDE_DIRS})
    target_link_libraries(demoapp PRIVATE ${OpenCL_LIBRARIES})
endif()

# Link optional OpenCV for advanced image/video processing and webcam
if(OpenCV_FOUND)
    target_compile_definitions(demoapp PRIVATE PYTHONIC_ENABLE_OPENCV)
    target_include_directories(demoapp PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(demoapp PRIVATE ${OpenCV_LIBS})
endif()

# ============================================================================
# Optional: Enable evdev for true simultaneous key detection (Linux only)
# ============================================================================
option(ENABLE_EVDEV "Enable evdev for true multi-key detection (Linux only)" ON)
if(ENABLE_EVDEV AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Evdev enabled - true simultaneous key detection available")
    message(STATUS "  Note: Requires read access to /dev/input/event* (input group)")
    target_compile_definitions(demoapp PRIVATE PYTHONIC_USE_EVDEV)
endif()

# Note: On Windows, true multi-key detection is automatic via GetAsyncKeyState
if(WIN32)
    message(STATUS "Windows detected - true multi-key detection via GetAsyncKeyState")
endif()

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
