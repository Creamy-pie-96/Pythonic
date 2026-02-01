# pythonicDraw.hpp - Developer Guide

This document explains the internal workings of the `pythonicDraw.hpp` module for high-resolution terminal graphics and media rendering. It is intended for maintainers and contributors, not end users.

## Overview

`pythonicDraw.hpp` provides:

- High-resolution terminal graphics using Unicode Braille characters (U+2800-U+28FF)
- Fast block-based rendering for images and real-time video
- Double-buffering and ANSI escape codes for flicker-free output
- FFmpeg integration for video streaming
- ImageMagick integration for image format conversion
- RAII terminal state management for robust cleanup

## Braille Graphics System

### Braille Encoding

- Each Unicode Braille character represents a 2×4 pixel grid (8 dots per cell)
- Dots are mapped to bits in a byte; codepoint = 0x2800 + bit_pattern
- The layout:

| Col 0 | Col 1 |
| ----- | ----- | ----------------- |
| [1]   | [4]   | Row 0 (bits 0, 3) |
| [2]   | [5]   | Row 1 (bits 1, 4) |
| [3]   | [6]   | Row 2 (bits 2, 5) |
| [7]   | [8]   | Row 3 (bits 6, 7) |

### Lookup Table

- `BrailleLUT` precomputes all 256 possible braille patterns to UTF-8 strings for fast rendering
- Used by `braille_to_utf8(uint8_t bits)`

### Canvas

- `BrailleCanvas` manages a 2D grid of character cells
- Each cell stores a byte (8 bits for 8 dots)
- Drawing operations:
  - `set_pixel(x, y, on)` - set/clear a single pixel
  - `set_block(char_x, char_y, pixels)` - set a 2×4 block in one operation
  - `set_block_gray(char_x, char_y, gray[8], threshold)` - optimized for video frames
  - `load_frame_fast(data, width, height, threshold)` - bulk load for images/videos
- Rendering: `render()` returns the full terminal string

## Terminal Control

- Uses ANSI escape codes for cursor movement, screen clearing, hiding/showing cursor
- Double-buffering: clears screen, draws frame, moves cursor to top-left
- `ansi` namespace provides helpers

## Image Rendering

- Detects image files by extension
- Converts any image to PPM using ImageMagick's `convert` command
- Loads PPM, binarizes to braille, renders to terminal
- Functions:
  - `is_image_file(filename)`
  - `convert_to_ppm(input_file, max_width)`
  - `render_image(filename, max_width, threshold)`
  - `print_image(filename, max_width, threshold)`

## DOT Graph Rendering

- Converts DOT graph string to PPM using Graphviz
- Renders as braille graphics
- Functions:
  - `dot_to_ppm(dot_content, width)`
  - `render_dot(dot_content, max_width, threshold)`
  - `print_dot(dot_content, max_width, threshold)`

## Video Streaming

- Detects video files by extension
- Uses FFmpeg to decode frames to grayscale
- Renders each frame using `BrailleCanvas` and double-buffering
- `VideoPlayer` class:
  - Loads video, extracts info (resolution, fps, duration)
  - Plays video in terminal (blocking or async)
  - Uses `set_block_gray` for fast frame rendering
  - RAII `TerminalStateGuard` ensures terminal is restored on exit/interruption
- Functions:
  - `is_video_file(filename)`
  - `play_video(filename, width, threshold)`
  - `print_video_info(filename)`
  - `print_media(filename, max_width, threshold)` (auto-detects image/video)

## Terminal State Management

- `TerminalStateGuard` hides cursor, resets attributes, restores state on destruction
- Prevents terminal corruption if playback is interrupted

## Performance Notes

- Block-based rendering is 8x faster than per-pixel
- Precomputed lookup table eliminates string conversion overhead
- Double-buffering avoids flicker
- All external commands (ImageMagick, FFmpeg) are called via `popen` for portability

## Extending/Debugging

- To add new formats, extend `is_image_file`/`is_video_file` and conversion logic
- For debugging, use `print_video_info` and `BrailleCanvas::render()`
- All file I/O and external calls are wrapped for error handling

## Dependencies

- ImageMagick (convert)
- FFmpeg (ffmpeg, ffprobe)
- Graphviz (dot) for DOT graph rendering

## Example Usage

```cpp
#include <pythonic/pythonicDraw.hpp>
using namespace pythonic::draw;

BrailleCanvas canvas(80, 40);
canvas.line(0, 0, 159, 159);
std::cout << canvas.render();

print_image("image.png", 80);
play_video("video.mp4", 80);
```

## Troubleshooting

- If images/videos do not render, check external tool availability (`convert`, `ffmpeg`)
- If terminal is corrupted after playback, ensure `TerminalStateGuard` is used
- For performance, prefer block-based APIs for bulk pixel operations

---

This guide is for maintainers. For user documentation, see the main README and print.md.
