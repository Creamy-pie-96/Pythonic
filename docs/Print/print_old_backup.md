[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Plot](../Plot/plot.md)

# Print

This page documents all user-facing print functions in Pythonic for outputting data, including text, images, videos, and webcam streams, using a clear tabular format with concise examples.

---

## Quick Start

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Print text
print("Hello", "World", 42);  // Output: Hello World 42

// Pretty-print data structures
var data = dict({{"name", "Alice"}, {"age", 30}});
pprint(data);  // Formatted output with indentation

// Print image in terminal
print("photo.png");  // Auto-detects and renders as braille

// Print with config (simplified API)
print("photo.png", RenderConfig().set_mode(Mode::colored).set_max_width(120));

// Print video in terminal
print("video.mp4");  // Plays video with braille rendering
```

---

## Text Printing

| Function                 | Description                                    | Example                        |
| ------------------------ | ---------------------------------------------- | ------------------------------ |
| `print(args...)`         | Print any number of arguments, space-separated | `print("Hello", 42, "World");` |
| `pprint(v, indent_step)` | Pretty-print `var` with indentation (uses `var.pretty_str()` for formatted output, including graphs as 2D visualization) | `pprint(my_dict, 4);`          |

**Parameters:**

- `args...` - Any number of arguments of any type
- `v` - The `var` object to pretty-print  (lists, dicts, graphs, etc.)
- `indent_step` - Number of spaces per indent level (default: 2)

**Examples:**

```cpp
var a = 42;
var b = "Hello";
var c = list({1, 2, 3});

print(a, b, c);  // Output: 42 Hello [1, 2, 3]

var nested = dict({
    {"users", list({
        dict({{"name", "Alice"}, {"age", 30}}),
        dict({{"name", "Bob"}, {"age", 25}})
    })}
});

pprint(nested);
// Output (formatted):
// {
//   "users": [
//     {"name": "Alice", "age": 30},
//     {"name": "Bob", "age": 25}
//   ]
// }

// Pretty-print a graph (shows 2D visualization)
var g = Graph();
g.add_edge(1, 2);
g.add_edge(2, 3);
pprint(g);  // Shows graph as ASCII art
```

---

## RenderConfig (Unified Configuration)

Print images and videos directly in the terminal with various rendering modes.

### Basic Media Printing

| Function                                  | Description                       | Example                                           |
| ----------------------------------------- | --------------------------------- | ------------------------------------------------- |
| `print(filepath)`                         | Auto-detect and render media file | `print("photo.png");`                             |
| `print(filepath, Type::type)`             | Render with explicit type         | `print("video.mp4", Type::video);`                |
| `print(filepath, Type::type, Mode::mode)` | Render with type and mode         | `print("photo.png", Type::image, Mode::colored);` |

### Type Enum

Controls how the file is interpreted:

| Type                | Description                                   | Example                                 |
| ------------------- | --------------------------------------------- | --------------------------------------- |
| `Type::auto_detect` | Automatically detect from extension (default) | `print("file.png");`                    |
| `Type::image`       | Force treat as image                          | `print("data.bin", Type::image);`       |
| `Type::video`       | Force treat as video                          | `print("clip.avi", Type::video);`       |
| `Type::video_info`  | Show video metadata only                      | `print("video.mp4", Type::video_info);` |
| `Type::webcam`      | Capture from webcam (requires OpenCV)         | `print("0", Type::webcam);`             |
| `Type::text`        | Force treat as plain text                     | `print("file.txt", Type::text);`        |

### Mode Enum

Controls the visual rendering style:

| Mode                | Description                 | Resolution  | Colors | Use Case                      |
| ------------------- | --------------------------- | ----------- | ------ | ----------------------------- |
| `Mode::bw_dot`      | Braille patterns (default)  | 8× terminal | B&W    | Highest detail, low bandwidth |
| `Mode::bw`          | Half-block characters (▀▄█) | 2× terminal | B&W    | Basic B&W rendering           |
| `Mode::colored`     | Half-block with color       | 2× terminal | 24-bit | Best color clarity            |
| `Mode::colored_dot` | Braille with color          | 8× terminal | 24-bit | High detail + color           |

### Parser Enum

Controls the backend used for processing:

| Parser                   | Description                    | Supports               | Requirements        |
| ------------------------ | ------------------------------ | ---------------------- | ------------------- |
| `Parser::default_parser` | FFmpeg + ImageMagick (default) | Images, videos         | FFmpeg, ImageMagick |
| `Parser::opencv`         | OpenCV backend                 | Images, videos, webcam | OpenCV library      |

### Audio Enum

Controls audio playback for videos:

| Audio        | Description                  | Example                                                                              |
| ------------ | ---------------------------- | ------------------------------------------------------------------------------------ |
| `Audio::off` | Silent playback (default)    | `print("video.mp4");`                                                                |
| `Audio::on`  | Play with synchronized audio | `print("video.mp4", Type::video, Mode::colored, Parser::default_parser, Audio::on);` |

> **Note:** Audio requires SDL2 or PortAudio library. If unavailable, automatically falls back to silent playback.

---

## Complete Media API

```cpp
print(filepath,
      type,          // Type enum
      mode,          // Mode enum
      parser,        // Parser enum
      audio,         // Audio enum
      max_width,     // Terminal width (default: 80)
      threshold,     // B&W threshold 0-255 (default: 128)
      shell,         // Shell::interactive or Shell::noninteractive
      pause_key,     // Pause key (default: 'p', '\0' to disable)
      stop_key       // Stop key (default: 's', '\0' to disable)
);
```

---

## Examples

### Example 1: Image Rendering Modes

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Black & white braille (highest detail)
print("photo.png", Type::image, Mode::bw_dot);

// True color blocks (best color)
print("photo.png", Type::image, Mode::colored);

// Colored braille (detail + color)
print("photo.png", Type::image, Mode::colored_dot);
```

### Example 2: Video Playback with Audio

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Play video with audio in true color
print("video.mp4", Type::video, Mode::colored,
      Parser::default_parser, Audio::on);

// Play video with interactive controls
print("video.mp4", Type::video, Mode::bw_dot,
      Parser::default_parser, Audio::off, 120, 128,
      Shell::interactive);  // Press 'p' to pause, 's' to stop
```

### Example 3: Webcam Capture

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Capture from default webcam (device 0)
print("0", Type::webcam);

// Capture from specific device (Linux)
print("/dev/video0", Type::webcam, Mode::colored);

// High-res webcam capture
print("0", Type::webcam, Mode::colored_dot,
      Parser::opencv, Audio::off, 160);
```

### Example 4: Video Info

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Show video metadata without playing
print("video.mp4", Type::video_info);
// Output:
// Resolution: 1920x1080
// Duration: 1:23:45
// Frame rate: 30.00 fps
// Codec: h264
```

### Example 5: Proprietary Format (.pi, .pv)

```cpp
#include <pythonic/pythonic.hpp>
#include <pythonic/pythonicMedia.hpp>
using namespace Pythonic;

// Convert and encrypt image
pythonic::media::convert("photo.jpg");  // Creates photo.pi

// Print encrypted image (auto-detected)
print("photo.pi");  // Works like original image

// Convert back
pythonic::media::revert("photo.pi");  // Creates photo_restored.jpg
```

### Example 6: Custom Video Controls

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Custom pause/stop keys
print("video.mp4", Type::video, Mode::colored,
      Parser::default_parser, Audio::on, 80, 128,
      Shell::interactive,
      ' ',   // Space to pause/resume
      'q'    // 'q' to stop
);
```

---

## Resolution Comparison

| Terminal Size | Mode::bw_dot | Mode::bw  | Mode::colored | Mode::colored_dot |
| ------------- | ------------ | --------- | ------------- | ----------------- |
| 80×24 chars   | 160×96 px    | 80×48 px  | 80×48 px      | 160×96 px         |
| 120×40 chars  | 240×160 px   | 120×80 px | 120×80 px     | 240×160 px        |

**Recommendation:**

- Use `Mode::bw_dot` for line art, diagrams, text
- Use `Mode::colored` for photographs requiring color fidelity
- Use `Mode::colored_dot` for detailed images with color
- Use `Mode::bw` for simple graphics

---

## Shell Modes

| Mode                    | Description                    | Use Case                     |
| ----------------------- | ------------------------------ | ---------------------------- |
| `Shell::noninteractive` | No keyboard controls (default) | Scripts, automated rendering |
| `Shell::interactive`    | Enable pause/stop keys         | Interactive video playback   |

**Interactive Keys:**

- `p` (or custom `pause_key`) - Pause/resume playback
- `s` (or custom `stop_key`) - Stop playback and exit
- Use `'\0'` for either key to disable that control

---

# Next Check

- [Export](../Export/export.md)
