# Pythonic C++ Library

Welcome! Pythonic is a modern C++20 library that brings the expressive power and clean syntax of Python to C++. It aims to make C++ code feel as natural and productive as Python, while retaining the performance and flexibility of C++.

## What is Pythonic?

**Pythonic** is a C++ library designed for developers who love Python's dynamic features but need the speed and control of C++. It provides:

- Dynamic typing and flexible containers
- Python-style slicing, comprehensions, and string methods
- Easy-to-use data structures (list, dict, set, etc.)
- Graph algorithms and **interactive graph visualization** with ImGui
- Math utilities and more
- A familiar, readable syntax—no weird hacks, just clean C++

## Why use this library?

- Write C++ code that feels like Python—concise, readable, and expressive
- Rapid prototyping and algorithm development in C++
- Seamless integration with modern C++ projects
- Great for teaching, competitive programming, and research

## User Guides & Documentation

- For a complete step-by-step guide on cloning, building, installing, and using this library in your own project, see the detailed [Getting Started Guide](docs/examples/README.md).
- **Note:** If you don't want to install, you can simply add the `include/pythonic` directory to your project's include path and use `target_include_directories` in your CMakeLists.txt.
- For a detailed user guide, [check out the documentation](docs/index.md).

## Optional: Graph Viewer Dependencies

The library includes an **optional** interactive graph visualization feature. To enable it, you need to install the following dependencies:

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install libglfw3-dev libgl1-mesa-dev
git clone https://github.com/ocornut/imgui.git external/imgui
```

### macOS:
```bash
brew install glfw
git clone https://github.com/ocornut/imgui.git external/imgui
```

### Windows (vcpkg):
```bash
vcpkg install glfw3:x64-windows
git clone https://github.com/ocornut/imgui.git external/imgui
```

Then build with:
```bash
cmake -B build -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON
cmake --build build
```

Use in code:
```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::vars;

var g = graph(5);
g.add_edge(0, 1);
g.add_edge(1, 2);
g.show();  // Opens interactive viewer!
```

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
