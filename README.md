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

## Optional: Media Rendering Dependencies

For terminal media rendering (images and videos), you need:

- **ImageMagick** - For image format conversion
- **FFmpeg** - For video decoding

### Ubuntu/Debian:

```bash
sudo apt-get install imagemagick ffmpeg
```

### macOS:

```bash
brew install imagemagick ffmpeg
```

### Usage:

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Print image in braille (black & white, high resolution)
print("photo.png", Type::image);

// Print image in true color (block characters)
print("photo.png", Type::image, Mode::colored);

// Print image in colored braille (combines color + high resolution)
print("photo.png", Type::image, Mode::colored_dot);

// Play video in true color
print("video.mp4", Type::video, Mode::colored);
```

### Rendering Modes

Pythonic supports four rendering modes for images and videos:

| Mode                | Description                 | Resolution             | Colors                         |
| ------------------- | --------------------------- | ---------------------- | ------------------------------ |
| `Mode::bw_dot`      | Braille patterns (default)  | 8x terminal resolution | B&W                            |
| `Mode::bw`          | Block characters (▀▄█)      | 2x terminal resolution | B&W                            |
| `Mode::colored`     | Block characters with color | 2x terminal resolution | True color                     |
| `Mode::colored_dot` | Braille with color          | 8x terminal resolution | True color (averaged per cell) |

## Optional: Audio Playback for Videos

To play videos with synchronized audio, you need SDL2 or PortAudio:

### Ubuntu/Debian:

```bash
# SDL2 (recommended)
sudo apt-get install libsdl2-dev

# Or PortAudio
sudo apt-get install portaudio19-dev libportaudio2
```

### macOS:

```bash
# SDL2 (recommended)
brew install sdl2

# Or PortAudio
brew install portaudio
```

### Windows (vcpkg):

```bash
# SDL2 (recommended)
vcpkg install sdl2:x64-windows

# Or PortAudio
vcpkg install portaudio:x64-windows
```

Then build with audio support:

```bash
# With SDL2
cmake -B build -DPYTHONIC_ENABLE_SDL2_AUDIO=ON
cmake --build build

# Or with PortAudio
cmake -B build -DPYTHONIC_ENABLE_PORTAUDIO=ON
cmake --build build
```

Use in code:

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Play video with audio
print("video.mp4", Type::video, Mode::bw_dot, Parser::default_parser, Audio::on);

// Play video with audio in true color
print("video.mp4", Type::video, Mode::colored, Parser::default_parser, Audio::on);
```

> **Note:** If audio libraries are not available, `Audio::on` will automatically fall back to silent video playback.

## Optional: GPU-Accelerated Video Rendering

For smoother colored video playback, you can enable OpenCL GPU acceleration:

### Ubuntu/Debian:

```bash
sudo apt-get install ocl-icd-opencl-dev opencl-clhpp-headers
```

### macOS:

OpenCL is included with macOS - no additional installation needed.

### Windows (vcpkg):

```bash
vcpkg install opencl:x64-windows
```

Then build with OpenCL support:

```bash
cmake -B build -DPYTHONIC_ENABLE_OPENCL=ON
cmake --build build
```

> **Note:** OpenCL support is optional. If not available, video rendering will use CPU with optimized buffering.

## Optional: OpenCV for Webcam and Advanced Processing

For webcam capture and advanced image/video processing, you can enable OpenCV support:

### Ubuntu/Debian:

```bash
sudo apt-get install libopencv-dev
```

### macOS:

```bash
brew install opencv
```

### Windows (vcpkg):

```bash
vcpkg install opencv:x64-windows
```

Then build with OpenCV support:

```bash
cmake -B build -DPYTHONIC_ENABLE_OPENCV=ON
cmake --build build
```

Use in code:

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Capture from webcam (requires OpenCV)
print("0", Type::webcam);  // Use device 0
print("/dev/video0", Type::webcam);  // Linux device path

// Use OpenCV backend for image/video processing
print("photo.png", Type::image, Mode::colored, Parser::opencv);
print("video.mp4", Type::video, Mode::colored_dot, Parser::opencv);
```

### Parser Backends

| Parser                   | Description                                        | Supports             |
| ------------------------ | -------------------------------------------------- | -------------------- |
| `Parser::default_parser` | FFmpeg for video, ImageMagick for images (default) | All media formats    |
| `Parser::opencv`         | OpenCV for everything                              | Media files + webcam |

> **Note:** Webcam capture always requires OpenCV. If OpenCV is not available, an exception is thrown for webcam sources.

## Proprietary Media Format (.pi, .pv)

Pythonic includes a media encryption system for protecting images and videos:

```cpp
#include <pythonic/pythonicMedia.hpp>
using namespace pythonic::media;

// Convert image to encrypted Pythonic format
convert("photo.jpg");  // Creates photo.pi

// Convert video to encrypted Pythonic format
convert("video.mp4");  // Creates video.pv

// Revert back to original format
revert("photo.pi");   // Creates photo_restored.jpg
revert("video.pv");   // Creates video_restored.mp4

// Print encrypted files directly (auto-detected)
print("photo.pi");    // Works like original image
print("video.pv");    // Works like original video
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
