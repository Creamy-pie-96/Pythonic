cmake_minimum_required(VERSION 3.10)
project(Pythonic)

# Require C++20 for concepts, ranges, and other features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create a header-only interface library
add_library(pythonic INTERFACE)

# Set the include directory for the library
# This makes headers available for users of the library
# Adjust path if your include/pythonic is elsewhere
set(PYTHONIC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(pythonic INTERFACE "${PYTHONIC_INCLUDE_DIR}")

# Require C++20 for interface users
target_compile_features(pythonic INTERFACE cxx_std_20)

# Optionally, add installation rules for sharing your library
install(TARGETS pythonic EXPORT pythonic-targets)
install(DIRECTORY include/ DESTINATION include)

# Example: Add a test executable (if you have test.cpp in test/)
# add_executable(test_runner test/test.cpp)
# target_link_libraries(test_runner PRIVATE pythonic)
