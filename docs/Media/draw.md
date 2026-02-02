# pythonicDraw Module

The `pythonicDraw.hpp` module provides terminal graphics capabilities for rendering images and videos using Unicode characters (braille patterns and block characters) with ANSI color support.

## Include

```cpp
#include <pythonic/pythonicDraw.hpp>
using namespace pythonic::draw;
```

Or use the full include:

```cpp
#include <pythonic/pythonic.hpp>
```

## Rendering Modes

| Mode            | Characters Used        | Description                                  |
| --------------- | ---------------------- | -------------------------------------------- |
| BW Braille      | `⠀⠁⠂...⣿`              | Highest resolution B&W (2x4 pixels per char) |
| BW Blocks       | `▀▄█ `                 | Lower resolution B&W (1x2 pixels per char)   |
| Colored Blocks  | `▀▄█ ` + ANSI color    | True color (24-bit) output                   |
| Colored Braille | `⠀⠁⠂...⣿` + ANSI color | Colored braille patterns                     |

## Canvas Classes

### BrailleCanvas

For black & white braille rendering (highest resolution).

```cpp
#include <pythonic/pythonicDraw.hpp>
using namespace pythonic::draw;

// Create canvas with character dimensions
BrailleCanvas canvas(80, 24);  // 80 chars wide, 24 chars tall

// Or create from pixel dimensions
BrailleCanvas canvas = BrailleCanvas::from_pixels(160, 96);

// Set individual pixels
canvas.set_pixel(10, 20);        // Turn on pixel at (10, 20)
canvas.set_pixel(10, 20, false); // Turn off pixel

// Load a grayscale frame (for video)
canvas.load_frame_fast(gray_data, width, height, threshold);

// Render to string
std::string output = canvas.render();
std::cout << output << std::endl;
```

### BWBlockCanvas

For black & white block character rendering.

```cpp
BWBlockCanvas canvas(80, 24);

// Load frame from grayscale data
canvas.load_frame_gray(gray_data, width, height, threshold);

// Or from RGB data
canvas.load_frame_rgb(rgb_data, width, height, threshold);

// Render
std::cout << canvas.render() << std::endl;
```

### ColorCanvas

For true color (24-bit) rendering using half-block characters.

```cpp
ColorCanvas canvas = ColorCanvas::from_pixels(160, 96);

// Load RGB frame
canvas.load_frame_rgb(rgb_data, width, height);

// Render with ANSI escape codes
std::cout << canvas.render() << std::endl;
```

### ColoredBrailleCanvas

For colored braille pattern rendering.

```cpp
ColoredBrailleCanvas canvas(80, 24);

// Load RGB frame (colors are averaged per braille cell)
canvas.load_frame_rgb(rgb_data, width, height, threshold);

// Render
std::cout << canvas.render() << std::endl;
```

## Image Functions

### print_image (BW Braille)

```cpp
// Basic usage
print_image("photo.png", 80, 128);  // width=80, threshold=128

// With just filename (defaults)
print_image("photo.png");
```

### print_image_colored

```cpp
// True color image rendering
print_image_colored("photo.png", 80);  // width=80
```

### print_image_opencv (OpenCV backend)

```cpp
// Using OpenCV for image loading (if compiled with OpenCV support)
print_image_opencv("photo.png", 80, 128, Mode::colored);
```

## Video Functions

### play_video (BW Braille)

```cpp
// Play video in black & white braille
play_video("video.mp4", 80, 128);  // width=80, threshold=128
```

### play_video_colored

```cpp
// Play video in true color
play_video_colored("video.mp4", 80);
```

### play_video_audio

```cpp
// Play video with audio (requires SDL2 or PortAudio)
play_video_audio("video.mp4", 80, Mode::colored);
```

### play_video_opencv

```cpp
// Play video using OpenCV backend
play_video_opencv("video.mp4", 80, Mode::bw_dot, 128);
```

### play_webcam

```cpp
// Capture from webcam (requires OpenCV)
play_webcam("0", 80, Mode::colored, 128);
play_webcam("/dev/video0", 80, Mode::bw_dot, 128);
```

## Video Player Classes

### VideoPlayer (BW Braille)

```cpp
VideoPlayer player("video.mp4", 80, 128);
player.play();  // Blocking playback
player.stop();  // Stop playback
```

### ColoredVideoPlayer

```cpp
ColoredVideoPlayer player("video.mp4", 80, 30.0);  // width=80, fps=30
player.play();
```

### OpenCVVideoPlayer

```cpp
OpenCVVideoPlayer player("video.mp4", 80, Mode::colored, 128);
player.play();

// For webcam
OpenCVVideoPlayer webcam("0", 80, Mode::colored);
webcam.play();
```

### AudioVideoPlayer

```cpp
// Plays video with synchronized audio
AudioVideoPlayer player("video.mp4", 80, Mode::colored, 0);  // 0 = auto fps
player.play();
```

## Helper Functions

### File Type Detection

```cpp
// Check if file is video
bool is_video = is_video_file("file.mp4");  // true

// Check if file is image
bool is_image = is_image_file("file.png");  // true

// Check Pythonic encrypted formats
bool is_pi = is_pythonic_image_file("photo.pi");  // true
bool is_pv = is_pythonic_video_file("video.pv");  // true

// Check webcam source
bool is_webcam = is_webcam_source("0");           // true
bool is_webcam = is_webcam_source("/dev/video0"); // true

// Parse webcam index
int idx = parse_webcam_index("0");            // 0
int idx = parse_webcam_index("/dev/video2");  // 2
```

### Video Info

```cpp
print_video_info("video.mp4");
// Output:
// Video: video.mp4
//   Resolution: 1920x1080
//   FPS: 30
//   Duration: 120.5 seconds
```

## ANSI Terminal Codes

The module provides ANSI escape code helpers:

```cpp
using namespace pythonic::draw::ansi;

// Control codes
std::cout << CURSOR_HOME;    // Move cursor to top-left
std::cout << CLEAR_SCREEN;   // Clear entire screen
std::cout << HIDE_CURSOR;    // Hide cursor
std::cout << SHOW_CURSOR;    // Show cursor
std::cout << RESET;          // Reset all attributes

// Color codes
std::cout << fg_color(255, 0, 0);  // Red foreground
std::cout << bg_color(0, 0, 255);  // Blue background

// Cursor positioning
std::cout << cursor_to(10, 5);  // Move to row 10, col 5
```

## Building with Features

### Basic (BW only, requires ImageMagick and FFmpeg)

```bash
cmake -B build
cmake --build build
```

### With True Color GPU Acceleration

```bash
cmake -B build -DPYTHONIC_ENABLE_OPENCL=ON
cmake --build build
```

### With Audio Support

```bash
cmake -B build -DPYTHONIC_ENABLE_SDL2_AUDIO=ON  # or -DPYTHONIC_ENABLE_PORTAUDIO=ON
cmake --build build
```

### With OpenCV (webcam support)

```bash
cmake -B build -DPYTHONIC_ENABLE_OPENCV=ON
cmake --build build
```

### Full Feature Build

```bash
cmake -B build -DPYTHONIC_ENABLE_SDL2_AUDIO=ON -DPYTHONIC_ENABLE_OPENCL=ON -DPYTHONIC_ENABLE_OPENCV=ON
cmake --build build
```

## Dependencies

| Feature                 | Required Packages   |
| ----------------------- | ------------------- |
| Basic BW rendering      | FFmpeg, ImageMagick |
| True color              | (none extra)        |
| OpenCL GPU acceleration | OpenCL SDK          |
| SDL2 audio              | SDL2                |
| PortAudio audio         | PortAudio           |
| OpenCV backend          | OpenCV 4.x          |
