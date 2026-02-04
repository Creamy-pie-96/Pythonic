[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Print](../Print/print.md)

# Export (emit)

Export media files (images, videos) as ASCII/Braille art to text, PNG, video, or encrypted Pythonic format.

---

## Quick Start

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Export image to text file (default)
emit("photo.png", "output");  // → output.txt

// Export image as rendered PNG
emit("photo.png", "output", RenderConfig().set_format(Format::image));

// Export video with audio
emit("video.mp4", "output", RenderConfig()
    .set_format(Format::video)
    .with_audio());

// Export with pixel-level control
emit("photo.png", "output",
    RenderConfig().set_format(Format::image),
    ExportConfig().set_dot_size(4));
```

---

## API

```cpp
bool emit(input_path, output_name, RenderConfig = {}, ExportConfig = {})
```

| Parameter      | Type          | Default          | Description                                       |
| -------------- | ------------- | ---------------- | ------------------------------------------------- |
| `input_path`   | `std::string` | Required         | Path to source media file                         |
| `output_name`  | `std::string` | Required         | Output filename (extension auto-added)            |
| `RenderConfig` | config object | `RenderConfig()` | Controls rendering (mode, format, width, etc.)    |
| `ExportConfig` | config object | `ExportConfig()` | Controls pixel output (dot_size, density, colors) |

**Returns:** `true` on success, `false` on failure

---

## Text-to-Braille Art Export

Export text strings as braille art using `TextArt` tag:

```cpp
bool emit(TextArt, text, output_name, TextConfig = {})
```

| Parameter     | Type          | Default        | Description                       |
| ------------- | ------------- | -------------- | --------------------------------- |
| `TextArt`     | tag           | Required       | Tag indicating text-to-braille    |
| `text`        | `std::string` | Required       | Text to render                    |
| `output_name` | `std::string` | Required       | Output filename (.txt auto-added) |
| `TextConfig`  | config object | `TextConfig()` | Text rendering options            |

### TextConfig

| Field       | Type      | Default        | Description              |
| ----------- | --------- | -------------- | ------------------------ |
| `mode`      | `Mode`    | `Mode::bw_dot` | Rendering mode           |
| `fg_r`      | `uint8_t` | `255`          | Foreground red (0-255)   |
| `fg_g`      | `uint8_t` | `255`          | Foreground green (0-255) |
| `fg_b`      | `uint8_t` | `255`          | Foreground blue (0-255)  |
| `max_width` | `int`     | `0`            | Max width (0=auto)       |

### Examples

```cpp
// Export text as braille art
emit(TextArt, "Hello World!", "hello");  // → hello.txt

// Colored text
emit(TextArt, "COLORED!", "color_text",
    TextConfig{.mode = Mode::colored});

// Custom color (red)
emit(TextArt, "RED TEXT", "red_text",
    TextConfig{
        .mode = Mode::colored,
        .fg_r = 255,
        .fg_g = 0,
        .fg_b = 0
    });

// Multi-line text
emit(TextArt, "Line 1\nLine 2\nLine 3", "multiline");
```

---

## RenderConfig

Controls how media is rendered to ASCII/Braille art.

| Method                     | Default             | Description                               |
| -------------------------- | ------------------- | ----------------------------------------- |
| `set_type(Type)`           | `Type::auto_detect` | Force media type (image/video)            |
| `set_format(Format)`       | `Format::text`      | Output format (text/image/video/pythonic) |
| `set_mode(Mode)`           | `Mode::bw_dot`      | Rendering style                           |
| `set_max_width(int)`       | `80`                | Output width in characters                |
| `set_threshold(int)`       | `128`               | B&W brightness threshold (0-255)          |
| `set_dithering(Dithering)` | `Dithering::none`   | Dithering algorithm                       |
| `set_fps(int)`             | `0` (original)      | Video frame rate                          |
| `set_start_time(double)`   | `-1.0` (beginning)  | Video start time (seconds)                |
| `set_end_time(double)`     | `-1.0` (end)        | Video end time (seconds)                  |
| `set_audio(Audio)`         | `Audio::off`        | Include audio track                       |
| `with_audio()`             | —                   | Shorthand for `Audio::on`                 |
| `set_gpu(bool)`            | `true`              | GPU acceleration                          |
| `no_gpu()`                 | —                   | Disable GPU acceleration                  |

---

## Export (emit)Config

Controls pixel-level rendering for PNG/video output.

| Method                            | Default           | Description                    |
| --------------------------------- | ----------------- | ------------------------------ |
| `set_dot_size(int)`               | `2`               | Dot radius in pixels           |
| `set_density(int)`                | `3`               | Spacing multiplier             |
| `set_background(uint8_t r, g, b)` | `(0, 0, 0)`       | Background color (RGB)         |
| `set_foreground(uint8_t r, g, b)` | `(255, 255, 255)` | Default foreground color (RGB) |
| `set_preserve_colors(bool)`       | `true`            | Use ANSI colors from source    |

---

## Format Enum

| Format             | Extension | Description                  |
| ------------------ | --------- | ---------------------------- |
| `Format::text`     | .txt      | Plain text ASCII/Braille art |
| `Format::image`    | .png      | Rendered PNG image           |
| `Format::video`    | .mp4      | Video with ASCII frames      |
| `Format::pythonic` | .pi/.pv   | Encrypted Pythonic format    |

---

## Mode Enum

| Mode                      | Description                         | Resolution  | Colors    |
| ------------------------- | ----------------------------------- | ----------- | --------- |
| `Mode::bw_dot`            | Braille patterns (default)          | 8× terminal | B&W       |
| `Mode::bw`                | Half-block characters               | 2× terminal | B&W       |
| `Mode::colored`           | Half-block with color               | 2× terminal | 24-bit    |
| `Mode::colored_dot`       | Braille with color                  | 8× terminal | 24-bit    |
| `Mode::grayscale_dot`     | Grayscale braille                   | 8× terminal | Grayscale |
| `Mode::bw_dithered`       | B&W braille with dithering          | 8× terminal | B&W       |
| `Mode::flood_dot`         | Flood-filled braille (grayscale)    | 8× terminal | Grayscale |
| `Mode::flood_dot_colored` | Flood-filled braille with color     | 8× terminal | 24-bit    |
| `Mode::colored_dithered`  | Colored braille with ordered dither | 8× terminal | 24-bit    |

---

## Dithering Enum

Controls dithering algorithm for `Mode::bw_dithered` and `Mode::colored_dithered`:

| Dithering                    | Description                       | Quality | Best For     |
| ---------------------------- | --------------------------------- | ------- | ------------ |
| `Dithering::none`            | Simple threshold (no dithering)   | Fast    | Binary       |
| `Dithering::ordered`         | Ordered/Bayer dithering (default) | Good    | Video        |
| `Dithering::floyd_steinberg` | Floyd-Steinberg error diffusion   | Best    | Still images |

**Usage example:**

```cpp
// Export with Floyd-Steinberg dithering for best quality
emit("photo.png", "output", RenderConfig()
    .set_mode(Mode::colored_dithered)
    .set_dithering(Dithering::floyd_steinberg)
    .set_format(Format::image));
```

---

## Examples

### Export (emit) Image to Text

```cpp
emit("photo.png", "output");
// → output.txt

emit("photo.png", "wide", RenderConfig().set_max_width(120));
// → wide.txt (wider output)
```

### Export (emit) Image as PNG

```cpp
emit("photo.png", "rendered", RenderConfig()
    .set_format(Format::image)
    .set_mode(Mode::colored_dot));
// → rendered.png

// Custom pixel rendering
emit("photo.png", "custom",
    RenderConfig().set_format(Format::image),
    ExportConfig()
        .set_dot_size(4)
        .set_background(255, 255, 255)  // White bg
        .set_foreground(0, 0, 0));      // Black dots
// → custom.png
```

### Export (emit) Video

```cpp
// Full video with audio
emit("video.mp4", "ascii_video", RenderConfig()
    .set_format(Format::video)
    .set_mode(Mode::colored)
    .with_audio());
// → ascii_video.mp4

// Video clip (1:00 to 2:00)
emit("movie.mp4", "clip", RenderConfig()
    .set_format(Format::video)
    .set_start_time(60)
    .set_end_time(120)
    .with_audio());
// → clip.mp4
```

### Export (emit) with Dithering

```cpp
emit("photo.jpg", "dithered", RenderConfig()
    .set_format(Format::image)
    .set_mode(Mode::grayscale_dot)
    .set_dithering(Dithering::floyd));
// → dithered.png (high-quality grayscale)
```

### Export (emit) to Encrypted Format

```cpp
emit("secret.png", "encrypted", RenderConfig()
    .set_format(Format::pythonic));
// → encrypted.pi

// View later with print()
print("encrypted.pi");  // Auto-decrypts
```

---

## Notes

- **Extension auto-added:** Based on format (`.txt`, `.png`, `.mp4`, `.pi/.pv`)
- **FFmpeg required:** For video export
- **GPU acceleration:** Uses hardware encoder if available (h264_nvenc, h264_qsv, etc.)
- **Multi-threaded:** Rendering uses available CPU cores for speed

---

# Next

- [LiveDraw](../LiveDraw/livedraw.md)
