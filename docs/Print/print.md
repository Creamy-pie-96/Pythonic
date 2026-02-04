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

// Render media file with Media tag
print(Media, "photo.png");  // Renders image as braille
print(Media, "video.mp4");  // Plays video in terminal

// Render with RenderConfig
print(Media, "photo.png", RenderConfig().set_mode(Mode::colored).set_max_width(120));
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

## Media Printing (Media Tag)

Use `Media` tag to explicitly render media files:

| Function                         | Description                        |
| -------------------------------- | ---------------------------------- |
| `print(Media, filepath)`         | Render media with default settings |
| `print(Media, filepath, config)` | Render media with RenderConfig     |

```cpp
// Render image
print(Media, "photo.png");

// Render video with audio
print(Media, "video.mp4", RenderConfig().with_audio());

// Render with colored mode
print(Media, "image.jpg", RenderConfig().set_mode(Mode::colored));

// Render with all options
print(Media, "video.mp4", RenderConfig()
    .set_mode(Mode::colored_dot)
    .set_max_width(120)
    .with_audio()
    .interactive());
```

---

## Text-to-Braille Art (TextArt Tag)

Use `TextArt` tag to render text as braille art:

| Function                       | Description                       |
| ------------------------------ | --------------------------------- |
| `print(TextArt, text)`         | Render text with default settings |
| `print(TextArt, text, config)` | Render text with TextConfig       |

### TextConfig

Configuration for text-to-braille rendering:

| Field       | Type      | Default        | Description              |
| ----------- | --------- | -------------- | ------------------------ |
| `mode`      | `Mode`    | `Mode::bw_dot` | Rendering mode           |
| `fg_r`      | `uint8_t` | `255`          | Foreground red (0-255)   |
| `fg_g`      | `uint8_t` | `255`          | Foreground green (0-255) |
| `fg_b`      | `uint8_t` | `255`          | Foreground blue (0-255)  |
| `max_width` | `int`     | `0`            | Max width (0=auto)       |

### Examples

```cpp
// Basic text rendering
print(TextArt, "Hello World!");

// Multi-line text
print(TextArt, "Line 1\nLine 2\nLine 3");

// Colored text (white)
print(TextArt, "COLORED!", TextConfig{.mode = Mode::colored});

// Custom color - red
print(TextArt, "RED TEXT", TextConfig{
    .mode = Mode::colored,
    .fg_r = 255,
    .fg_g = 0,
    .fg_b = 0
});

// Cyan text
print(TextArt, "CYAN!", TextConfig{
    .mode = Mode::colored,
    .fg_r = 0,
    .fg_g = 255,
    .fg_b = 255
});
```

### Supported Characters

The built-in 3×5 pixel font supports:

- Uppercase letters: A-Z
- Lowercase letters: a-z
- Numbers: 0-9
- Symbols: `! " ' ( ) + , - . / : ; < = > ? @`
- Space character

---

## RenderConfig

Configuration class for media rendering with builder pattern.

| Method                          | Default                  | Description                   |
| ------------------------------- | ------------------------ | ----------------------------- |
| `set_type(Type)`                | `Type::auto_detect`      | Media type hint               |
| `set_mode(Mode)`                | `Mode::bw_dot`           | Rendering style               |
| `set_parser(Parser)`            | `Parser::default_parser` | Backend (FFmpeg/OpenCV)       |
| `set_max_width(int)`            | `80`                     | Terminal width                |
| `set_threshold(int)`            | `128`                    | B&W threshold (0-255)         |
| `set_dithering(Dithering)`      | `Dithering::none`        | Dithering algorithm           |
| `set_fps(int)`                  | `0` (original)           | Target FPS                    |
| `set_start_time(double)`        | `-1.0` (beginning)       | Video start (seconds)         |
| `set_end_time(double)`          | `-1.0` (end)             | Video end (seconds)           |
| `set_audio(Audio)`              | `Audio::off`             | Audio playback                |
| `set_shell(Shell)`              | `Shell::noninteractive`  | Keyboard controls             |
| `with_audio()`                  | —                        | Shorthand for Audio::on       |
| `interactive()`                 | —                        | Enable keyboard controls      |
| `set_volume(int)`               | `100`                    | Initial volume (0-100%)       |
| `set_volume_step(int)`          | `10`                     | Volume change per key (1-100) |
| `set_seek_frames(int)`          | `90`                     | Frames to seek per press      |
| `set_buffer_ahead_frames(int)`  | `60`                     | Frames to preload ahead       |
| `set_buffer_behind_frames(int)` | `90`                     | Frames to keep behind         |

---

## Interactive Video Playback

When `interactive()` is enabled, video playback runs in a multithreaded mode with non-blocking keyboard controls. This works with or without audio.
**Note** Interactive mode is not supported for opencv.

### Keyboard Controls

| Key       | Action                                       |
| --------- | -------------------------------------------- |
| ↑ (Up)    | Increase volume (default: 10%, configurable) |
| ↓ (Down)  | Decrease volume (default: 10%, configurable) |
| ← (Left)  | Seek backward (default: 90 frames)           |
| → (Right) | Seek forward (default: 90 frames)            |
| `p`       | Pause / Resume playback                      |
| `s`       | Stop playback                                |

### On-Screen Display

When interactive mode is enabled, the player shows:

1. **Progress Bar** (bottom): Shows playback position with time labels
   - White blocks: Played portion
   - Gray blocks: Remaining portion
   - Time format: `MM:SS` for start, current, and end times

2. **Volume Bar** (right side): 10-segment bar with braille patterns
   - Green (⣿): Low volume (0-40%)
   - Yellow (⣿): Medium volume (41-70%)
   - Red (⣿): High volume (71-100%)
   - Gray (⣀): Empty segments
   - Smooth gradient with partial braille characters

### Frame Buffering

The player uses a frame buffer for smooth seeking:

- **buffer_ahead_frames**: Preloaded frames for smooth playback (default: 60)
- **buffer_behind_frames**: Cached frames for backward seeking (default: 90)

### Example

```cpp
// Full-featured interactive video playback
print(Media, "movie.mp4", RenderConfig()
    .set_mode(Mode::colored_dot)
    .with_audio()
    .interactive()
    .set_volume(80)           // Start at 80% volume
    .set_volume_step(5)       // 5% volume change per keypress
    .set_seek_frames(120)     // Seek 4 seconds per keypress (at 30fps)
    .set_buffer_ahead_frames(90)
    .set_buffer_behind_frames(120));

// Interactive without audio (controls still work)
print(Media, "video.mp4", RenderConfig()
    .set_mode(Mode::colored)
    .interactive());           // Volume bar hidden when no audio
```

---

## Mode Enum

| Mode                      | Description                         | Resolution  | Colors    |
| ------------------------- | ----------------------------------- | ----------- | --------- |
| `Mode::bw_dot`            | Braille patterns                    | 8× terminal | B&W       |
| `Mode::bw`                | Half-block characters               | 2× terminal | B&W       |
| `Mode::colored`           | Half-block with color               | 2× terminal | 24-bit    |
| `Mode::colored_dot`       | Braille with color                  | 8× terminal | 24-bit    |
| `Mode::grayscale_dot`     | Grayscale braille                   | 8× terminal | Grayscale |
| `Mode::bw_dithered`       | B&W braille with dithering          | 8× terminal | B&W       |
| `Mode::flood_dot`         | Flood-filled braille (grayscale)    | 8× terminal | Grayscale |
| `Mode::flood_dot_colored` | Flood-filled braille with color     | 8× terminal | 24-bit    |
| `Mode::colored_dithered`  | Colored braille with ordered dither | 8× terminal | 24-bit    |

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

Controls dithering algorithm for `Mode::bw_dithered` and `Mode::colored_dithered`:

| Dithering                    | Description                       | Best For           |
| ---------------------------- | --------------------------------- | ------------------ |
| `Dithering::none`            | Simple threshold (no dithering)   | Fast binary output |
| `Dithering::ordered`         | Ordered/Bayer dithering (default) | Video, animation   |
| `Dithering::floyd_steinberg` | Floyd-Steinberg error diffusion   | Still images       |

**Usage example:**

```cpp
// Use Floyd-Steinberg for best still image quality
print(Media, "photo.png", RenderConfig()
    .set_mode(Mode::bw_dithered)
    .set_dithering(Dithering::floyd_steinberg));

// Use ordered dithering for video (faster, more stable)
print(Media, "video.mp4", RenderConfig()
    .set_mode(Mode::colored_dithered)
    .set_dithering(Dithering::ordered));
```

---

## Examples

### Image Rendering

```cpp
// Basic image render
print(Media, "photo.png");

// Colored rendering
print(Media, "photo.png", RenderConfig().set_mode(Mode::colored));

// High-quality with dithering
print(Media, "photo.jpg", RenderConfig()
    .set_mode(Mode::grayscale_dot)
    .set_dithering(Dithering::floyd)
    .set_max_width(160));
```

### Video Playback

```cpp
// Basic video
print(Media, "video.mp4");

// Video with audio
print(Media, "video.mp4", RenderConfig().with_audio());

// Interactive with keyboard controls (see Interactive Video Playback section)
print(Media, "movie.mp4", RenderConfig()
    .set_mode(Mode::colored)
    .with_audio()
    .interactive());

// Interactive without audio (volume controls hidden)
print(Media, "video.mp4", RenderConfig()
    .set_mode(Mode::colored_dot)
    .interactive());

// Play video segment (1:00 to 2:00)
print(Media, "movie.mp4", RenderConfig()
    .set_start_time(60)
    .set_end_time(120));

// Full interactive setup with custom controls
print(Media, "movie.mp4", RenderConfig()
    .set_mode(Mode::colored_dot)
    .with_audio()
    .interactive()
    .set_volume(70)           // 70% initial volume
    .set_seek_frames(60)      // Seek 2 seconds at 30fps
    .set_pause_key('p')
    .set_stop_key('s'));
```

### Webcam Capture

```cpp
// Default webcam
print(Media, "0", RenderConfig().set_type(Type::webcam));

// Colored webcam
print(Media, "0", RenderConfig()
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
