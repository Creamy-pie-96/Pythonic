# Pythonic C++ Library - Quick Start Guide

This guide shows you how to use the Pythonic C++ library in your own projects.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Step 1: Clone the Pythonic Library](#step-1-clone-the-pythonic-library)
- [Step 2: Build and Install the Library](#step-2-build-and-install-the-library)
- [Step 3: Create Your Project](#step-3-create-your-project)
- [Step 4: Write Your Code](#step-4-write-your-code)
- [Step 5: Build Your Project](#step-5-build-your-project)
- [Step 6: Run Your Application](#step-6-run-your-application)
- [Alternative: Using Without Installing](#alternative-using-without-installing)

---

## Prerequisites

Before you begin, make sure you have:

- **C++20 compiler** (GCC 10+, Clang 10+, MSVC 2019+)
- **CMake 3.10 or later**
- **Git**

---

## Step 1: Clone the Pythonic Library

First, choose a location to install the Pythonic library (this can be anywhere on your system). For this guide, we'll use a folder called `pythonic_cpp_lib`:

### Linux / macOS:

```bash
mkdir -p ~/pythonic_cpp_lib
cd ~/pythonic_cpp_lib
git clone https://github.com/Creamy-pie-96/Pythonic.git
cd Pythonic
```

### Windows (Command Prompt):

```cmd
mkdir %USERPROFILE%\pythonic_cpp_lib
cd %USERPROFILE%\pythonic_cpp_lib
git clone https://github.com/Creamy-pie-96/Pythonic.git
cd Pythonic
```

### Windows (PowerShell):

```powershell
New-Item -ItemType Directory -Force -Path ~\pythonic_cpp_lib
cd ~\pythonic_cpp_lib
git clone https://github.com/Creamy-pie-96/Pythonic.git
cd Pythonic
```

**Note:** You can use any directory name and location you prefer - just remember where you put it!

---

## Step 2: Build and Install the Library

Now build and install the Pythonic library. The install will be placed in a subfolder called `install` within the Pythonic directory.

### Linux / macOS:

````bash
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../install -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON
cmake --build . --target install -j4
cd ../..    # Go back to pythonic_cpp_lib directory
pwd         # Remember this path - you'll need it later!

If you want to use the interactive Graph Viewer (`var::show()`), build
the library with graph viewer support enabled. This will compile the
GLFW/OpenGL/ImGui-backed viewer and expose the CMake target
`pythonic::pythonic_graph_viewer` for downstream projects.

Enable the viewer during configuration:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=../install -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON
cmake --build . --target install -j4
````

Required system dependencies (example on Debian/Ubuntu):

```bash
sudo apt update && sudo apt install -y build-essential cmake libglfw3-dev libx11-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libglu1-mesa-dev libgl1-mesa-dev
```

ImGui is included in the Pythonic repository under `external/imgui`.
If you vendor or replace it, ensure the viewer build picks up a compatible
ImGui + OpenGL loader.

## Platform notes and install hints

macOS (Homebrew)

```bash
# Install dependencies via Homebrew
brew update
brew install cmake glfw
# OpenGL is provided by the system (via frameworks). Build as usual with CMake.
```

Windows (Visual Studio + vcpkg)

Option A — vcpkg (recommended):

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat   # or bootstrap-vcpkg.sh on WSL

# Install GLFW and other deps for x64
./vcpkg install glfw3:x64-windows

# Configure CMake to use vcpkg toolchain when building your project
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<path-to-pythonic-install>
```

Option B — Visual Studio without vcpkg:

1. Install the "Desktop development with C++" workload in Visual Studio.
2. Download prebuilt GLFW binaries or build GLFW from source and point CMake
   to the GLFW install location via `CMAKE_PREFIX_PATH` or `-DGLFW_ROOT`.

On Windows, OpenGL is available via `opengl32.lib` (FindOpenGL handles it).

````

### Windows (Command Prompt):

```cmd
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=..\install -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON
cmake --build . --target install --config Release
cd ..\..
cd
````

### Windows (PowerShell):

```powershell
New-Item -ItemType Directory -Force -Path build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="..\install"
cmake --build . --target install --config Release
cd ..\..
Get-Location    # Remember this path - you'll need it later!
```

**After this step:**

- The library is installed at: `<your-pythonic_cpp_lib-path>/Pythonic/install/`
- Save or remember this full path - you'll use it when building your own projects

---

## Step 3: Create Your Project

Now create your own project in a **separate location** (anywhere you want - doesn't need to be near the Pythonic library):

### Linux / macOS:

```bash
mkdir -p ~/myproject
cd ~/myproject
```

### Windows (Command Prompt):

```cmd
mkdir %USERPROFILE%\myproject
cd %USERPROFILE%\myproject
```

### Windows (PowerShell):

```powershell
New-Item -ItemType Directory -Force -Path ~\myproject
cd ~\myproject
```

**Your directory structure now looks like:**

```
(anywhere on your system)
└── pythonic_cpp_lib/
    └── Pythonic/
        └── install/          ← Pythonic is installed here

(can be anywhere else on your system)
└── myproject/                ← Your project (we are here now)
```

---

## Step 4: Write Your Code

Create two files in your project directory:

### 4.1: Create `main.cpp`

```cpp
#include <pythonic/pythonic.hpp>

using namespace pythonic::vars;
using namespace pythonic::print;

int main() {
    var x = 42;
    var name = "Pythonic C++";

    print("Hello from", name);
    print("The answer is:", x);

    // Create a list
    var nums = list(1, 2, 3, 4, 5);
    print("Numbers:", nums);

    return 0;
}
```

Or you can also copy the demo.cpp from **example** dir

### 4.2: Create `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyApp)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If you plan to link the optional Graph Viewer (to enable `var::show()`),
# CMake needs to find OpenGL before importing the installed `pythonic`
# package because the exported viewer target references `OpenGL::GL`.
find_package(OpenGL REQUIRED)

# Find the Pythonic library
find_package(pythonic REQUIRED)

# Add your executable (replace main.cpp with your source files)
add_executable(myapp main.cpp)

# Link the Pythonic library
target_link_libraries(myapp PRIVATE pythonic::pythonic)

# If you built and installed Pythonic with the Graph Viewer enabled,
# link the viewer target to enable `var::show()` in your app:
target_link_libraries(myapp PRIVATE pythonic::pythonic_graph_viewer)
```

Copy paste it or we have a **CMakeLists.txt** file in **example** directory. You can directly use that too.

---

## Step 5: Build Your Project

Now configure and build your project. You need to tell CMake where you installed the Pythonic library.

**IMPORTANT - You Must Specify the Path:**  
Replace `<path-to-pythonic-install>` below with the **actual full path** to where you installed Pythonic in Step 2.

For example:

- Linux/macOS: `/home/username/pythonic_cpp_lib/Pythonic/install`
- Windows: `C:\Users\username\pythonic_cpp_lib\Pythonic\install`

### Linux / macOS:

```bash
mkdir build
cd build

# Replace <path-to-pythonic-install> with your actual path!
# Example: cmake .. -DCMAKE_PREFIX_PATH=/home/john/pythonic_cpp_lib/Pythonic/install
cmake .. -DCMAKE_PREFIX_PATH=<path-to-pythonic-install>

cmake --build . -j4
```

**Example with actual path:**

```bash
cmake .. -DCMAKE_PREFIX_PATH=$HOME/pythonic_cpp_lib/Pythonic/install
```

### Windows (Command Prompt):

```cmd
mkdir build
cd build

REM Replace <path-to-pythonic-install> with your actual path!
REM Example: cmake .. -DCMAKE_PREFIX_PATH=C:\Users\John\pythonic_cpp_lib\Pythonic\install
cmake .. -DCMAKE_PREFIX_PATH=<path-to-pythonic-install>

cmake --build . --config Release
```

**Example with actual path:**

```cmd
cmake .. -DCMAKE_PREFIX_PATH=%USERPROFILE%\pythonic_cpp_lib\Pythonic\install
```

### Windows (PowerShell):

```powershell
New-Item -ItemType Directory -Force -Path build
cd build

# Replace <path-to-pythonic-install> with your actual path!
# Example: cmake .. -DCMAKE_PREFIX_PATH="C:\Users\John\pythonic_cpp_lib\Pythonic\install"
cmake .. -DCMAKE_PREFIX_PATH="<path-to-pythonic-install>"

cmake --build . --config Release
```

**Example with actual path:**

```powershell
cmake .. -DCMAKE_PREFIX_PATH="$HOME\pythonic_cpp_lib\Pythonic\install"
```

---

**Finding Your Path:**

If you forgot where you installed Pythonic, you can find it:

**Linux/macOS:**

```bash
find ~ -name "pythonicConfig.cmake" 2>/dev/null
# This will show the full path - use everything before "/lib/cmake/pythonic/"
```

**Windows (PowerShell):**

```powershell
Get-ChildItem -Path $HOME -Filter "pythonicConfig.cmake" -Recurse -ErrorAction SilentlyContinue
# Look for the path, use everything before "\lib\cmake\pythonic\"
```

---

## Step 6: Run Your Application

## Using the Graph Viewer (`var::show()`)

If you built and installed Pythonic with `-DPYTHONIC_ENABLE_GRAPH_VIEWER=ON`,
your installed package exports the target `pythonic::pythonic_graph_viewer`.
Link that target in your project's `CMakeLists.txt` to enable the
interactive viewer API. Example CMake configuration:

```cmake
find_package(pythonic REQUIRED)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE pythonic::pythonic pythonic::pythonic_graph_viewer)
```

Minimal example that uses the viewer:

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::vars;

int main() {
        var g = graph(3);
        g.add_edge(0, 1, 1.0, 0.0, true);
        g.add_edge(1, 2, 1.0, 0.0, true);

        // Show opens an interactive window (may block until closed)
        g.show();

        return 0;
}
```

Runtime notes:

- The viewer opens a desktop window: ensure a graphical session (DISPLAY)
  is available on Linux. Headless servers without an X/Wayland display
  cannot show the GUI unless you use a virtual framebuffer (Xvfb).
- If you see missing link symbols when building your application, confirm
  system GLFW/OpenGL development packages were available when Pythonic was
  built and that Pythonic was installed with the viewer enabled.

Finally, run your compiled program:

### Linux / macOS:

```bash
./myapp
```

### Windows (Command Prompt):

```cmd
Release\myapp.exe
```

### Windows (PowerShell):

```powershell
.\Release\myapp.exe
```

You should see output like:

```
Hello from Pythonic C++
The answer is: 42
Numbers: [1, 2, 3, 4, 5]
```

---

## Alternative: Using Without Installing

If you don't want to install the library, you can include the headers directly:

### CMakeLists.txt (Direct Include Method)

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(myapp main.cpp)

# Point directly to the Pythonic include directory
target_include_directories(myapp PRIVATE
    ${CMAKE_SOURCE_DIR}/../Pythonic/include
)
target_compile_features(myapp PRIVATE cxx_std_20)
```

Then build without CMAKE_PREFIX_PATH:

### Linux / macOS:

```bash
mkdir build && cd build
cmake ..
cmake --build . -j4
./myapp
```

### Windows (Command Prompt):

```cmd
mkdir build && cd build
cmake ..
cmake --build . --config Release
Release\myapp.exe
```

---

## Troubleshooting

### "Could not find pythonic" Error

This is the most common error. It means CMake cannot find the installed package config files.

**Solution:**

1. **Find where you installed Pythonic:**

   **Linux/macOS:**

   ```bash
   find ~ -name "pythonicConfig.cmake" 2>/dev/null
   ```

   This will show something like: `/home/user/pythonic_cpp_lib/Pythonic/install/lib/cmake/pythonic/pythonicConfig.cmake`

   The path you need is everything **before** `/lib/cmake/pythonic/`:  
   → `/home/user/pythonic_cpp_lib/Pythonic/install`

   **Windows (PowerShell):**

   ```powershell
   Get-ChildItem -Path $HOME -Filter "pythonicConfig.cmake" -Recurse -ErrorAction SilentlyContinue
   ```

   This will show something like: `C:\Users\John\pythonic_cpp_lib\Pythonic\install\lib\cmake\pythonic\pythonicConfig.cmake`

   The path you need is everything **before** `\lib\cmake\pythonic\`:  
   → `C:\Users\John\pythonic_cpp_lib\Pythonic\install`

2. **Use that full path in your cmake command:**

   ```bash
   cmake .. -DCMAKE_PREFIX_PATH=/home/user/pythonic_cpp_lib/Pythonic/install
   ```

   _note_ you can add _-DCMAKE_BUILD_TYPE=Release_ if you want to build in release mode

3. **Verify the path is correct** before running cmake:

   **Linux/macOS:**

   ```bash
   ls /path/to/pythonic_cpp_lib/Pythonic/install/lib/cmake/pythonic/
   # Should show: pythonicConfig.cmake  pythonicConfigVersion.cmake  pythonicTargets.cmake
   ```

   **Windows:**

   ```cmd
   dir C:\path\to\pythonic_cpp_lib\Pythonic\install\lib\cmake\pythonic\
   ```

### "pythonic/pythonic.hpp: No such file or directory"

- Check that you're using `pythonic::pythonic` (with `::`) in `target_link_libraries`
- Verify headers were installed to `install/include/pythonic/`

### Compiler Errors about C++20

- Ensure your compiler supports C++20
- On older systems, you may need to update GCC/Clang

---

## Optional: Enabling Graphviz Support

The Graph class includes a `to_dot()` method that exports the graph structure to a DOT file (for visualization with Graphviz). By default, it only creates the `.dot` file.

If you have [Graphviz](https://graphviz.org/) installed on your system, you can enable **automatic SVG generation** when `to_dot()` is called.

### Auto-Detection (Recommended)

The CMakeLists.txt provided in this examples folder **automatically detects Graphviz**. When you run `cmake`, you'll see one of these messages:

```
-- Graphviz found: /usr/bin/dot
--   Graph::to_dot() will automatically generate SVG files
```

or:

```
-- Graphviz not found (optional)
--   Graph::to_dot() will only create .dot files
--   Install Graphviz to enable automatic SVG generation
```

To disable the auto-detection, configure with:

```bash
cmake .. -DCMAKE_PREFIX_PATH=<path> -DENABLE_GRAPHVIZ=OFF
```

### Manual Enable (Alternative)

If you're creating your own CMakeLists.txt from scratch, add this line after `add_executable()`:

```cmake
target_compile_definitions(myapp PRIVATE GRAPHVIZ_AVAILABLE)
```

Or define it in your code before including the library:

```cpp
#define GRAPHVIZ_AVAILABLE
#include <pythonic/pythonic.hpp>
```

### Example Usage

```cpp
#include <pythonic/pythonic.hpp>

using namespace pythonic::vars;

int main() {
    var g = graph(4);
    g.add_edge(0, 1, 1.0);
    g.add_edge(1, 2, 2.0);
    g.add_edge(2, 3, 3.0);
    g.add_edge(3, 0, 4.0);

    // If GRAPHVIZ_AVAILABLE: Creates my_graph.dot AND my_graph.svg
    // Otherwise: Creates only my_graph.dot
    g.to_dot("my_graph.dot");
    return 0;
}
```

**Without Graphviz installed**, you can still manually convert DOT files:

```bash
# Install Graphviz first (one-time):
# Ubuntu/Debian: sudo apt install graphviz
# macOS: brew install graphviz
# Windows: Download from https://graphviz.org/download/

# Then convert manually:
dot -Tsvg my_graph.dot -o my_graph.svg
dot -Tpng my_graph.dot -o my_graph.png
```

---

## What's Next?

Check out the full documentation in the main [README.md](../README.md) for all the features:

- Dynamic typing with `var`
- Python-style containers (lists, dicts, sets)
- Slicing, comprehensions, ranges
- Graph algorithms
- Math library
- And much more!

Enjoy coding with Pythonic C++!
