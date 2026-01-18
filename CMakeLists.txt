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

# Create a header-only interface library
add_library(pythonic INTERFACE)

# Set the include directory for the library
# Use generator expressions for build/install interface
target_include_directories(pythonic INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

# Add Graphviz compile definition if found
if(PYTHONIC_GRAPHVIZ_FOUND)
    target_compile_definitions(pythonic INTERFACE GRAPHVIZ_AVAILABLE)
endif()

# Require C++20 for interface users
target_compile_features(pythonic INTERFACE cxx_std_20)

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

# Example: Add a test executable (if you have test.cpp in test/)
# add_executable(test_runner test/test.cpp)
# target_link_libraries(test_runner PRIVATE pythonic)
