cmake_minimum_required(VERSION 3.10)
project(UsingPythonic)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Pythonic library (header-only)
# FAILS if not installed
find_package(pythonic REQUIRED)

# Add your executable
# --> Replace 'demoapp' with your project executable name
# --> Add all your .cpp files here if you have multiple
add_executable(demoapp demo.cpp)
# For multi-file projects:
# add_executable(demoapp main.cpp file1.cpp file2.cpp)
# or use file(GLOB SOURCES *.cpp) and then add_executable(demoapp ${SOURCES})


# Link libraries to your target
# --> Add other libraries here if your project depends on them
target_link_libraries(demoapp PRIVATE pythonic::pythonic)

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
