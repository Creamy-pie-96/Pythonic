# Pythonic C++ Library

Welcome! Pythonic is a modern C++20 library that brings the expressive power and clean syntax of Python to C++. It aims to make C++ code feel as natural and productive as Python, while retaining the performance and flexibility of C++.

## What is Pythonic?

**Pythonic** is a C++ library designed for developers who love Python's dynamic features but need the speed and control of C++. It provides:

- Dynamic typing and flexible containers
- Python-style slicing, comprehensions, and string methods
- Easy-to-use data structures (list, dict, set, etc.)
- Graph algorithms and **interactive graph visualization** with ImGui
- **Terminal media rendering** (images and videos in braille or true color)
- Math utilities and more
- A familiar, readable syntax—no weird hacks, just clean C++

## Why use this library?

- Write C++ code that feels like Python—concise, readable, and expressive
- Rapid prototyping and algorithm development in C++
- Seamless integration with modern C++ projects
- Great for teaching, competitive programming, and research

## Quick Example

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::vars;
using namespace Pythonic;

int main() {
    // Python-style dynamic variables
    var x = 42;
    var name = "Alice";
    var data = list({1, 2, 3, 4, 5});

    print("Hello,", name, "! Sum:", sum(data));

    // Terminal plotting
    pythonic::plot::plot([](double x) { return sin(x); }, -3.14, 3.14);

    // Print images/videos in terminal
    print("photo.png");   // Displays image as braille art
    print("video.mp4");   // Plays video in terminal

    return 0;
}
```

## User Guides & Documentation

- **[Getting Started Guide](docs/examples/README.md)** — Complete step-by-step guide on cloning, building, installing, and using this library (includes all optional dependencies)
- **[API Documentation](docs/index.md)** — Detailed user guide for all features

## Optional Features

The library includes several optional features that can be enabled during build:

| Feature            | Description                                | CMake Flag                          | Dependencies        |
| ------------------ | ------------------------------------------ | ----------------------------------- | ------------------- |
| **Graph Viewer**   | Interactive graph visualization with ImGui | `-DPYTHONIC_ENABLE_GRAPH_VIEWER=ON` | GLFW, OpenGL, ImGui |
| **Audio Playback** | Synchronized audio for video playback      | `-DPYTHONIC_ENABLE_SDL2_AUDIO=ON`   | SDL2 or PortAudio   |
| **GPU Rendering**  | GPU-accelerated video rendering            | `-DPYTHONIC_ENABLE_OPENCL=ON`       | OpenCL              |
| **OpenCV Support** | Webcam capture & advanced processing       | `-DPYTHONIC_ENABLE_OPENCV=ON`       | OpenCV              |

> **Note:** All features are optional. The core library works without any of these dependencies. See the [Getting Started Guide](docs/examples/README.md) for detailed installation instructions for each feature.

## Basic Installation

```bash
# Clone the repository
git clone https://github.com/Creamy-pie-96/Pythonic.git
cd Pythonic

# Build and install (core features only)
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../install
cmake --build . --target install

# Use in your project's CMakeLists.txt
find_package(pythonic REQUIRED)
target_link_libraries(your_target pythonic::pythonic)
```

For detailed installation with optional features, see the [Getting Started Guide](docs/examples/README.md).

## Contributing

See [CONTRIBUTING.md](docs/contributing.md) for guidelines on how to contribute to this project.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Disclaimer:

We’re looking for users to test the library and provide feedback!
Please open Issues or Discussions with any bug reports, questions, or feature ideas.
**Note:** Direct code contributions are not accepted at this time.

## Acknowledgments

Parts of this project, including some code generation and much of documentation, were assisted by GitHub Copilot (AI programming assistant). Copilot provided suggestions and support throughout the development process.

---

**AUTHOR:** MD. NASIF SADIK PRITHU
**GITHUB:** [Creamy-pie-96](https://github.com/Creamy-pie-96)

---
