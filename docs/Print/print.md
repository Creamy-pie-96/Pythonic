[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Plot](../Plot/plot.md)

# Print

This page documents all user-facing print functions in Pythonic for outputting data, including text, images, videos, and webcam streams.

---

## Quick Start

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Print text (like std::cout)
print("Hello", "World", 42);  // Output: Hello World 42

// Pretty-print data structures
var data = dict({{"name", "Alice"}, {"age", 30}});
pprint(data);  // Formatted output with indentation

// Render media file with Draw tag
print(Draw, "photo.png");  // Renders image as braille
print(Draw, "video.mp4");  // Plays video in terminal

// Render with RenderConfig
print(Draw, "photo.png", RenderConfig().set_mode(Mode::colored).set_max_width(120));
```

---

## Text Printing

| Function                 | Description                           | Example                        |
| ------------------------ | ------------------------------------- | ------------------------------ |
| `print(args...)`         | Print arguments to stdout (like cout) | `print("Hello", 42, "World");` |
| `pprint(v, indent_step)` | Pretty-print `var` with indentation   | `pprint(my_dict, 4);`          |

```cpp
print("Hello");                    // Output: Hello
print("Value:", 42);               // Output: Value: 42
print(a, b, c);                    // Output: a b c (space-separated)

var nested = dict({{"x", 1}, {"y", 2}});
pprint(nested);                    // Formatted output
```

---

## Media Printing (Draw Tag)

Use `Draw` tag to explicitly render media files:

| Function                        | Description                        |
| ------------------------------- | ---------------------------------- |
| `print(Draw, filepath)`         | Render media with default settings |
| `print(Draw, filepath, config)` | Render media with RenderConfig     |

```cpp
// Render image
print(Draw, "photo.png");

// Render video with audio
print(Draw, "video.mp4", RenderConfig().with_audio());

// Render with colored mode
print(Draw, "image.jpg", RenderConfig().set_mode(Mode::colored));

// Render with all options
print(Draw, "video.mp4", RenderConfig()
    .set_mode(Mode::colored_dot)
    .set_max_width(120)
    .with_audio()
    .interactive());
```

---

## RenderConfig

Configuration class for media rendering with builder pattern.

| Method                     | Default                  | Description              |
| -------------------------- | ------------------------ | ------------------------ |
| `set_type(Type)`           | `Type::auto_detect`      | Media type hint          |
| `set_mode(Mode)`           | `Mode::bw_dot`           | Rendering style          |
| `set_parser(Parser)`       | `Parser::default_parser` | Backend (FFmpeg/OpenCV)  |
| `set_max_width(int)`       | `80`                     | Terminal width           |
| `set_threshold(int)`       | `128`                    | B&W threshold (0-255)    |
| `set_dithering(Dithering)` | `Dithering::none`        | Dithering algorithm      |
| `set_fps(int)`             | `0` (original)           | Target FPS               |
| `set_start_time(double)`   | `-1.0` (beginning)       | Video start (seconds)    |
| `set_end_time(double)`     | `-1.0` (end)             | Video end (seconds)      |
| `set_audio(Audio)`         | `Audio::off`             | Audio playback           |
| `set_shell(Shell)`         | `Shell::noninteractive`  | Keyboard controls        |
| `with_audio()`             | —                        | Shorthand for Audio::on  |
| `interactive()`            | —                        | Enable keyboard controls |

---

## Mode Enum

| Mode                  | Description           | Resolution  | Colors    |
| --------------------- | --------------------- | ----------- | --------- |
| `Mode::bw_dot`        | Braille patterns      | 8× terminal | B&W       |
| `Mode::bw`            | Half-block characters | 2× terminal | B&W       |
| `Mode::colored`       | Half-block with color | 2× terminal | 24-bit    |
| `Mode::colored_dot`   | Braille with color    | 8× terminal | 24-bit    |
| `Mode::grayscale_dot` | Grayscale braille     | 8× terminal | Grayscale |
| `Mode::flood_dot`     | Flood-filled braille  | 8× terminal | 24-bit    |

---

## Type Enum

| Type                | Description                |
| ------------------- | -------------------------- |
| `Type::auto_detect` | Detect from file extension |
| `Type::image`       | Force treat as image       |
| `Type::video`       | Force treat as video       |
| `Type::webcam`      | Capture from webcam        |
| `Type::video_info`  | Show video metadata only   |
| `Type::text`        | Force treat as plain text  |

---

## Dithering Enum

| Dithering            | Description                     |
| -------------------- | ------------------------------- |
| `Dithering::none`    | No dithering (default)          |
| `Dithering::ordered` | Ordered/Bayer dithering         |
| `Dithering::floyd`   | Floyd-Steinberg error diffusion |

---

## Examples

### Image Rendering

```cpp
// Basic image render
print(Draw, "photo.png");

// Colored rendering
print(Draw, "photo.png", RenderConfig().set_mode(Mode::colored));

// High-quality with dithering
print(Draw, "photo.jpg", RenderConfig()
    .set_mode(Mode::grayscale_dot)
    .set_dithering(Dithering::floyd)
    .set_max_width(160));
```

### Video Playback

```cpp
// Basic video
print(Draw, "video.mp4");

// Video with audio
print(Draw, "video.mp4", RenderConfig().with_audio());

// Interactive with controls
print(Draw, "movie.mp4", RenderConfig()
    .set_mode(Mode::colored)
    .with_audio()
    .interactive()
    .set_pause_key('p')
    .set_stop_key('s'));

// Play video segment (1:00 to 2:00)
print(Draw, "movie.mp4", RenderConfig()
    .set_start_time(60)
    .set_end_time(120));
```

### Webcam Capture

```cpp
// Default webcam
print(Draw, "0", RenderConfig().set_type(Type::webcam));

// Colored webcam
print(Draw, "0", RenderConfig()
    .set_type(Type::webcam)
    .set_mode(Mode::colored)
    .set_max_width(120));
```

---

## Resolution Comparison

| Terminal Size | bw_dot     | colored   | colored_dot |
| ------------- | ---------- | --------- | ----------- |
| 80×24 chars   | 160×96 px  | 80×48 px  | 160×96 px   |
| 120×40 chars  | 240×160 px | 120×80 px | 240×160 px  |

---

# Next

- [Export](../Export/export.md)
