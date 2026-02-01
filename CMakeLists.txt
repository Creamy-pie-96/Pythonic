cmake_minimum_required(VERSION 3.10)
project(Pythonic)

# Require C++20 for concepts, ranges, and other features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Optional: Detect Graphviz for Graph::to_dot() SVG auto-generation
# ============================================================================
option(PYTHONIC_ENABLE_GRAPHVIZ "Enable Graphviz integration for automatic SVG generation" ON)

if(PYTHONIC_ENABLE_GRAPHVIZ)
    find_program(GRAPHVIZ_DOT_EXECUTABLE dot)
    if(GRAPHVIZ_DOT_EXECUTABLE)
        message(STATUS "Found Graphviz: ${GRAPHVIZ_DOT_EXECUTABLE}")
        message(STATUS "  Graph::to_dot() will automatically generate SVG files")
        set(PYTHONIC_GRAPHVIZ_FOUND TRUE)
    else()
        message(STATUS "Graphviz not found (optional)")
        message(STATUS "  Graph::to_dot() will only create .dot files")
        message(STATUS "  Install Graphviz and rebuild to enable automatic SVG generation")
        message(STATUS "  Or use: dot -Tsvg graph.dot -o graph.svg")
        set(PYTHONIC_GRAPHVIZ_FOUND FALSE)
    endif()
endif()

# ============================================================================
# Optional: SDL2 Audio for video playback with sound
# ============================================================================
option(PYTHONIC_ENABLE_SDL2_AUDIO "Enable SDL2 audio for video playback with sound" OFF)

if(PYTHONIC_ENABLE_SDL2_AUDIO)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        message(STATUS "Found SDL2: ${SDL2_LIBRARIES}")
        message(STATUS "  Audio playback enabled for video files")
        set(PYTHONIC_SDL2_FOUND TRUE)
    else()
        message(STATUS "SDL2 not found - trying pkg-config")
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2 QUIET sdl2)
            if(SDL2_FOUND)
                message(STATUS "Found SDL2 via pkg-config")
                set(PYTHONIC_SDL2_FOUND TRUE)
            endif()
        endif()
        if(NOT PYTHONIC_SDL2_FOUND)
            message(WARNING "SDL2 not found. Audio playback disabled.")
            message(STATUS "  Install SDL2: sudo apt install libsdl2-dev (Ubuntu)")
            message(STATUS "  Or: brew install sdl2 (macOS)")
            set(PYTHONIC_ENABLE_SDL2_AUDIO OFF)
        endif()
    endif()
endif()

# ============================================================================
# Optional: PortAudio for video playback with sound (alternative to SDL2)
# ============================================================================
option(PYTHONIC_ENABLE_PORTAUDIO "Enable PortAudio for video playback with sound" OFF)

if(PYTHONIC_ENABLE_PORTAUDIO AND NOT PYTHONIC_ENABLE_SDL2_AUDIO)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
    endif()
    
    if(NOT PORTAUDIO_FOUND)
        find_library(PORTAUDIO_LIBRARIES portaudio)
        find_path(PORTAUDIO_INCLUDE_DIRS portaudio.h)
        if(PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
            set(PORTAUDIO_FOUND TRUE)
        endif()
    endif()
    
    if(PORTAUDIO_FOUND)
        message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARIES}")
        message(STATUS "  Audio playback enabled for video files")
        set(PYTHONIC_PORTAUDIO_FOUND TRUE)
    else()
        message(WARNING "PortAudio not found. Audio playback disabled.")
        message(STATUS "  Install PortAudio: sudo apt install libportaudio2 portaudio19-dev (Ubuntu)")
        message(STATUS "  Or: brew install portaudio (macOS)")
        set(PYTHONIC_ENABLE_PORTAUDIO OFF)
    endif()
elseif(PYTHONIC_ENABLE_PORTAUDIO AND PYTHONIC_ENABLE_SDL2_AUDIO)
    message(STATUS "Both SDL2 and PortAudio enabled - using SDL2 (preferred)")
    set(PYTHONIC_ENABLE_PORTAUDIO OFF)
endif()

# ============================================================================
# Optional: OpenCL for GPU-accelerated video rendering
# ============================================================================
option(PYTHONIC_ENABLE_OPENCL "Enable OpenCL for GPU-accelerated video rendering" OFF)

if(PYTHONIC_ENABLE_OPENCL)
    find_package(OpenCL QUIET)
    if(OpenCL_FOUND)
        message(STATUS "Found OpenCL: ${OpenCL_LIBRARIES}")
        message(STATUS "  GPU-accelerated rendering enabled")
        set(PYTHONIC_OPENCL_FOUND TRUE)
    else()
        message(WARNING "OpenCL not found. GPU acceleration disabled.")
        message(STATUS "  Install OpenCL: sudo apt install ocl-icd-opencl-dev (Ubuntu)")
        message(STATUS "  Or: brew install opencl-headers (macOS)")
        set(PYTHONIC_ENABLE_OPENCL OFF)
    endif()
endif()

# Create a library
add_library(pythonic src/pythonicDispatchStubs.cpp)

# Set the include directory for the library
# Use generator expressions for build/install interface
target_include_directories(pythonic PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

# Link audio libraries if enabled
if(PYTHONIC_SDL2_FOUND)
    target_compile_definitions(pythonic PUBLIC PYTHONIC_ENABLE_SDL2_AUDIO)
    target_include_directories(pythonic PUBLIC ${SDL2_INCLUDE_DIRS})
    target_link_libraries(pythonic PUBLIC ${SDL2_LIBRARIES})
elseif(PYTHONIC_PORTAUDIO_FOUND)
    target_compile_definitions(pythonic PUBLIC PYTHONIC_ENABLE_PORTAUDIO)
    target_include_directories(pythonic PUBLIC ${PORTAUDIO_INCLUDE_DIRS})
    target_link_libraries(pythonic PUBLIC ${PORTAUDIO_LIBRARIES})
endif()

# Link OpenCL if enabled
if(PYTHONIC_OPENCL_FOUND)
    target_compile_definitions(pythonic PUBLIC PYTHONIC_ENABLE_OPENCL)
    target_include_directories(pythonic PUBLIC ${OpenCL_INCLUDE_DIRS})
    target_link_libraries(pythonic PUBLIC ${OpenCL_LIBRARIES})
endif()

# Precompiled headers to speed up compilation
target_precompile_headers(pythonic PRIVATE
    <algorithm>
    <stdexcept>
    <pythonic/pythonicDispatchForwardDecls.hpp>
    <pythonic/pythonicError.hpp>
    <pythonic/pythonicOverflow.hpp>
    <pythonic/pythonicPromotion.hpp>
    <pythonic/pythonicVars.hpp>
)

# Add Graphviz compile definition if found
if(PYTHONIC_GRAPHVIZ_FOUND)
    target_compile_definitions(pythonic PUBLIC GRAPHVIZ_AVAILABLE)
endif()

# Require C++20 for interface users
target_compile_features(pythonic PUBLIC cxx_std_20)

# Optionally, add installation rules for sharing your library
install(TARGETS pythonic EXPORT pythonic-targets)
install(DIRECTORY include/ DESTINATION include)

# Export and package configuration for consumers using find_package(pythonic)
include(CMakePackageConfigHelpers)

set(PYTHONIC_VERSION "0.1.0")

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/pythonicConfigVersion.cmake"
	VERSION ${PYTHONIC_VERSION}
	COMPATIBILITY AnyNewerVersion
)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/pythonicConfig.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/pythonicConfig.cmake"
	@ONLY
)

install(EXPORT pythonic-targets
	FILE pythonicTargets.cmake
	NAMESPACE pythonic::
	DESTINATION lib/cmake/pythonic
	EXPORT_LINK_INTERFACE_LIBRARIES
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/pythonicConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/pythonicConfigVersion.cmake"
	DESTINATION lib/cmake/pythonic
)

# Build demo executable from docs/examples/demo.cpp
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/examples/demo.cpp")
#     add_executable(demo docs/examples/demo.cpp)
#     target_link_libraries(demo PRIVATE pythonic)
#     message(STATUS "Demo executable configured: run with './demo' after building")
# endif()

# ============================================================================
# Optional: Interactive Graph Viewer
# ============================================================================
option(PYTHONIC_ENABLE_GRAPH_VIEWER "Enable interactive graph viewer (requires GLFW, OpenGL)" OFF)

if(PYTHONIC_ENABLE_GRAPH_VIEWER)
    message(STATUS "Graph viewer enabled, looking for dependencies...")
    
    # Prefer modern GLVND OpenGL implementation and set CMP0072 policy
    if(POLICY CMP0072)
        cmake_policy(SET CMP0072 NEW)
    endif()
    set(OpenGL_GL_PREFERENCE GLVND)

    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    
    message(STATUS "  OpenGL: Found")
    message(STATUS "  GLFW:   Found")
    
    # ImGui sources - bundled in external/imgui
    set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
    
    if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
        message(FATAL_ERROR 
            "ImGui not found at ${IMGUI_DIR}\n"
            "Please run: git clone https://github.com/ocornut/imgui.git ${IMGUI_DIR}\n"
            "Or copy ImGui sources from examples/graph_visualization/external/imgui")
    endif()
    
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
    
    # Graph viewer static library
    add_library(pythonic_graph_viewer STATIC
        src/graph_viewer.cpp
        ${IMGUI_SOURCES}
    )
    
    target_compile_features(pythonic_graph_viewer PUBLIC cxx_std_20)
    target_compile_definitions(pythonic_graph_viewer PUBLIC PYTHONIC_ENABLE_GRAPH_VIEWER)
    
    # Precompiled headers to speed up compilation
    target_precompile_headers(pythonic_graph_viewer PRIVATE
        <algorithm>
        <stdexcept>
        <pythonic/pythonicDispatchForwardDecls.hpp>
        <pythonic/pythonicError.hpp>
        <pythonic/pythonicOverflow.hpp>
        <pythonic/pythonicPromotion.hpp>
        <pythonic/pythonicVars.hpp>
        <pythonic/graph_viewer.hpp>
    )
    
    # Use PRIVATE for external paths to avoid installation path issues
    target_include_directories(pythonic_graph_viewer
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
    )
    
    target_link_libraries(pythonic_graph_viewer PUBLIC
        pythonic
        glfw
        OpenGL::GL
    )
    
    # Install graph viewer library
    install(TARGETS pythonic_graph_viewer EXPORT pythonic-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
    
    message(STATUS "Graph viewer configured successfully!")
    message(STATUS "  Use: target_link_libraries(your_app pythonic_graph_viewer)")
    message(STATUS "  In code: var g = graph(5); g.show();")

    # Example Executable
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/graph_viewer_example.cpp")
        add_executable(graph_viewer_example examples/graph_viewer_example.cpp)
        target_link_libraries(graph_viewer_example PRIVATE pythonic_graph_viewer)
    endif()
    
    # CLI Test Executable  
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/test_graph_viewer_cli.cpp")
        add_executable(test_graph_cli test/test_graph_viewer_cli.cpp)
        target_link_libraries(test_graph_cli PRIVATE pythonic_graph_viewer)
    endif()
else()
    message(STATUS "Graph viewer disabled (enable with -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON)")
endif()
