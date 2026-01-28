cmake_minimum_required(VERSION 3.10)
project(UsingPythonic)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Pythonic library (header-only)
find_package(pythonic REQUIRED)

# ImGui and OpenGL dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# ImGui sources
set(IMGUI_DIR "external/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add main executable
add_executable(mainapp main.cpp ${IMGUI_SOURCES})

# Include directories
target_include_directories(mainapp PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Link libraries
target_link_libraries(mainapp PRIVATE
    pythonic::pythonic
    glfw
    OpenGL::GL
)

message(STATUS "Build configured for Synapse with ImGui")
