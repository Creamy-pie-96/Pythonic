cmake_minimum_required(VERSION 3.10)
project(pythonic_benchmark)

# Require C++20 for concepts, ranges, and other features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add optimizations for fair comparison
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include/pythonic)

add_executable(benchmark benchmark.cpp)

# Optional: install target
install(TARGETS benchmark DESTINATION bin)
