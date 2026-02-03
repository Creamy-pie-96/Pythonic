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

// Print with RenderConfig (simplified API)
print("photo.png", RenderConfig().set_mode(Mode::colored).set_max_width(120));

// Print video in terminal
print("video.mp4");  // Plays video with braille rendering
```

---

## Text Printing

| Function                 | Description                                                                                                              | Example                        |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------ | ------------------------------ |
| `print(args...)`         | Print any number of arguments, space-separated                                                                           | `print("Hello", 42, "World");` |
| `pprint(v, indent_step)` | Pretty-print `var` with indentation (uses `var.pretty_str()` for formatted output, including graphs as 2D visualization) | `pprint(my_dict, 4);`          |

**Parameters:**

- `args...` - Any number of arguments of any type
- `v` - The `var` object to pretty-print (lists, dicts, graphs, etc.)
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

## RenderConfig (Unified Configuration API)

**Modern simplified API** using `RenderConfig` class for all rendering parameters with builder pattern.

### RenderConfig Methods

| Method                     | Description                 | Default                  | Example                                    |
| -------------------------- | --------------------------- | ------------------------ | ------------------------------------------ |
| `set_type(Type)`           | Set media type              | `Type::auto_detect`      | `config.set_type(Type::video)`             |
| `set_mode(Mode)`           | Set render mode             | `Mode::bw_dot`           | `config.set_mode(Mode::colored)`           |
| `set_parser(Parser)`       | Set backend parser          | `Parser::default_parser` | `config.set_parser(Parser::opencv)`        |
| `set_format(Format)`       | Set export format           | `Format::text`           | `config.set_format(Format::video)`         |
| `set_threshold(int)`       | Set B&W threshold           | `128`                    | `config.set_threshold(100)`                |
| `set_max_width(int)`       | Set terminal width          | `80`                     | `config.set_max_width(120)`                |
| `set_dithering(Dithering)` | Set dithering algorithm     | `Dithering::none`        | `config.set_dithering(Dithering::ordered)` |
| `set_grayscale_dots(bool)` | Use grayscale dots          | `false`                  | `config.set_grayscale_dots(true)`          |
| `set_invert(bool)`         | Invert colors               | `false`                  | `config.set_invert(true)`                  |
| `set_fps(int)`             | Set target FPS (0=original) | `0`                      | `config.set_fps(30)`                       |
| `set_start_time(double)`   | Set video start time (sec)  | `-1.0` (beginning)       | `config.set_start_time(60.0)`              |
| `set_end_time(double)`     | Set video end time (sec)    | `-1.0` (end)             | `config.set_end_time(120.0)`               |
| `set_audio(Audio)`         | Set audio mode              | `Audio::off`             | `config.set_audio(Audio::on)`              |
| `set_shell(Shell)`         | Set interactive mode        | `Shell::noninteractive`  | `config.set_shell(Shell::interactive)`     |
| `set_pause_key(char)`      | Set pause key               | `'p'`                    | `config.set_pause_key(' ')`                |
| `set_stop_key(char)`       | Set stop key                | `'s'`                    | `config.set_stop_key('q')`                 |
| `set_gpu(bool)`            | Enable GPU acceleration     | `true`                   | `config.set_gpu(false)`                    |
| `with_audio()`             | Enable audio (shorthand)    | -                        | `config.with_audio()`                      |
| `interactive()`            | Enable interactive mode     | -                        | `config.interactive()`                     |
| `no_gpu()`                 | Disable GPU (shorthand)     | -                        | `config.no_gpu()`                          |

### Dithering Enum

| Dithering            | Description                     | Quality | Use Case            |
| -------------------- | ------------------------------- | ------- | ------------------- |
| `Dithering::none`    | No dithering (default)          | Fast    | Clean graphics      |
| `Dithering::ordered` | Ordered/Bayer dithering         | Good    | Photos, gradients   |
| `Dithering::floyd`   | Floyd-Steinberg error diffusion | Best    | High-quality images |

### RenderConfig Usage

```cpp
// Print with config (recommended modern API)
print(filepath, RenderConfig config)

// Example 1: Simple one-liner with builder pattern
print("photo.png", RenderConfig().set_mode(Mode::colored).set_max_width(120));

// Example 2: Video with custom settings
print("movie.mp4", RenderConfig()
    .set_mode(Mode::colored)
    .with_audio()
    .interactive()
    .set_start_time(60)
    .set_end_time(120));

// Example 3: High-quality grayscale with dithering
print("photo.jpg", RenderConfig()
    .set_mode(Mode::grayscale_dot)
    .set_dithering(Dithering::floyd)
    .set_max_width(160));

// Example 4: Multiple configurations
RenderConfig config;
config.mode = Mode::colored_dot;
config.dithering = Dithering::ordered;
config.max_width = 100;
print("image.png", config);
```

---

## Traditional Media API

The traditional print API with individual parameters is still supported for backward compatibility.

### Basic Signatures

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

| Mode                  | Description                      | Resolution  | Colors    | Use Case                      |
| --------------------- | -------------------------------- | ----------- | --------- | ----------------------------- |
| `Mode::bw_dot`        | Braille patterns (default)       | 8× terminal | B&W       | Highest detail, low bandwidth |
| `Mode::bw`            | Half-block characters (▀▄█)      | 2× terminal | B&W       | Basic B&W rendering           |
| `Mode::colored`       | Half-block with color            | 2× terminal | 24-bit    | Best color clarity            |
| `Mode::colored_dot`   | Braille with color               | 8× terminal | 24-bit    | High detail + color           |
| `Mode::grayscale_dot` | Grayscale braille with dithering | 8× terminal | Grayscale | Smooth gradients              |
| `Mode::flood_dot`     | Colored braille, flood-filled    | 8× terminal | 24-bit    | Smoothest appearance          |

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

### Complete Traditional API Signature

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
// Black & white braille (highest detail) - DEFAULT
print("photo.png");

// True color blocks (best color)
print("photo.png", Type::image, Mode::colored);

// Colored braille (detail + color)
print("photo.png", Type::image, Mode::colored_dot);

// Using RenderConfig (modern API)
print("photo.png", RenderConfig().set_mode(Mode::colored));
```

### Example 2: Video Playback with Audio

```cpp
// Play video with audio in true color
print("video.mp4", Type::video, Mode::colored,
      Parser::default_parser, Audio::on);

// Modern API with RenderConfig
print("video.mp4", RenderConfig()
    .set_mode(Mode::colored)
    .with_audio());

// Play video with interactive controls
print("video.mp4", RenderConfig()
    .set_mode(Mode::bw_dot)
    .interactive()
    .set_pause_key('p')
    .set_stop_key('s'));
```

### Example 3: Webcam Capture

```cpp
// Capture from default webcam (device 0)
print("0", Type::webcam);

// Capture from specific device (Linux)
print("/dev/video0", Type::webcam, Mode::colored);

// Using RenderConfig
print("0", RenderConfig()
    .set_type(Type::webcam)
    .set_mode(Mode::colored_dot)
    .set_max_width(160));
```

### Example 4: Video Clip Export

```cpp
// Extract clip from 1:00 to 2:00
print("movie.mp4", RenderConfig()
    .set_start_time(60)
    .set_end_time(120)
    .with_audio()
    .set_mode(Mode::colored));
```

### Example 5: High-Quality Photo with Dithering

```cpp
// Best quality grayscale
print("photo.jpg", RenderConfig()
    .set_mode(Mode::grayscale_dot)
    .set_dithering(Dithering::floyd)
    .set_max_width(160));
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
- Use `Mode::grayscale_dot` with dithering for high-quality photos
- Use `Mode::flood_dot` for smoothest appearance

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
