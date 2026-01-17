cmake_minimum_required(VERSION 3.10)
project(Pythonic)

# Require C++20 for concepts, ranges, and other features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create a header-only interface library
add_library(pythonic INTERFACE)

# Set the include directory for the library
# Use generator expressions for build/install interface
target_include_directories(pythonic INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

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
