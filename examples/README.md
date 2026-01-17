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

First, clone the Pythonic repository from GitHub:

### Linux / macOS:

```bash
git clone https://github.com/Creamy-pie-96/Pythonic.git
cd Pythonic
```

### Windows (Command Prompt):

```cmd
git clone https://github.com/Creamy-pie-96/Pythonic.git
cd Pythonic
```

### Windows (PowerShell):

```powershell
git clone https://github.com/Creamy-pie-96/Pythonic.git
cd Pythonic
```

---

## Step 2: Build and Install the Library

Now build and install the Pythonic library to a local directory. We'll install it to a folder called `install` in your workspace.

### Linux / macOS:

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../install
cmake --build . --target install -j4
```

### Windows (Command Prompt):

```cmd
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=..\install
cmake --build . --target install --config Release
```

### Windows (PowerShell):

```powershell
New-Item -ItemType Directory -Force -Path build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="..\install"
cmake --build . --target install --config Release
```

After this step, you'll have:

- Headers in: `install/include/pythonic/`
- CMake config in: `install/lib/cmake/pythonic/`

---

## Step 3: Create Your Project

Navigate back to your workspace and create a new project directory:

### Linux / macOS:

```bash
cd ..
cd ..
mkdir myproject
cd myproject
```

### Windows (Command Prompt):

```cmd
cd ..
cd ..
mkdir myproject
cd myproject
```

### Windows (PowerShell):

```powershell
cd ..
cd ..
New-Item -ItemType Directory -Force -Path myproject
cd myproject
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

# Find the Pythonic library
find_package(pythonic REQUIRED)

# Add your executable (replace main.cpp with your source files)
add_executable(myapp main.cpp)

# Link the Pythonic library
target_link_libraries(myapp PRIVATE pythonic::pythonic)
```

Copy paste it or we have a **CMakeLists.txt** file in **example** directory. You can directly use that too.

---

## Step 5: Build Your Project

Now configure and build your project, pointing CMake to where you installed Pythonic:

### Linux / macOS:

```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=../../Pythonic/install
cmake --build . -j4
```

### Windows (Command Prompt):

```cmd
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=..\..\Pythonic\install
cmake --build . --config Release
```

### Windows (PowerShell):

```powershell
New-Item -ItemType Directory -Force -Path build
cd build
cmake .. -DCMAKE_PREFIX_PATH="..\..\Pythonic\install"
cmake --build . --config Release
```

---

## Step 6: Run Your Application

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

- Make sure you set `-DCMAKE_PREFIX_PATH` to the correct install directory
- Verify that `install/lib/cmake/pythonic/pythonicConfig.cmake` exists

### "pythonic/pythonic.hpp: No such file or directory"

- Check that you're using `pythonic::pythonic` (with `::`) in `target_link_libraries`
- Verify headers were installed to `install/include/pythonic/`

### Compiler Errors about C++20

- Ensure your compiler supports C++20
- On older systems, you may need to update GCC/Clang

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
