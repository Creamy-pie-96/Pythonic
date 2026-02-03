[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Print](../Print/print.md)

# Export

This page documents the user-facing export functions in Pythonic for converting media files to ASCII/Braille art format and saving them as images, videos, or text files.

---

## Quick Start

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Export image to text file
export_media("photo.png", "output");  // Creates output.txt

// Export as PNG with rendered Braille/ASCII art  
export_media("photo.png", "output", RenderConfig().set_format(Format::image));

// Export video with ASCII rendering using RenderConfig
export_media("video.mp4", "output", RenderConfig()
    .set_format(Format::video)
    .with_audio());

// Export to encrypted Pythonic format
export_media("photo.png", "secret", RenderConfig().set_format(Format::pythonic));
```

---

## Export with RenderConfig (Modern API)

**Simplified export API** using `RenderConfig` class (same as Print API).

```cpp
// Modern unified API
export_media(input_path, output_name, RenderConfig config)
```

All RenderConfig methods from [Print documentation](../Print/print.md#renderconfig-unified-configuration-api) apply, plus:

- `set_format(Format)` - Set output format (text, image, video, pythonic)
- `set_start_time(double)` - Video start time in seconds (-1=beginning)
- `set_end_time(double)` - Video end time in seconds (-1=end)
- `set_gpu(bool)` or `no_gpu()` - GPU acceleration control

### RenderConfig Export Examples

```cpp
// Export video clip with audio
export_media("movie.mp4", "clip", RenderConfig()
    .set_format(Format::video)
    .set_start_time(60)
    .set_end_time(120)
    .with_audio()
    .set_mode(Mode::colored));

// Export image with dithering
export_media("photo.jpg", "output", RenderConfig()
    .set_format(Format::image)
    .set_mode(Mode::grayscale_dot)
    .set_dithering(Dithering::floyd));

// Export to text with custom width
export_media("image.png", "ascii", RenderConfig()
    .set_format(Format::text)
    .set_max_width(120));
```

---

## Export Function (Traditional API)

| Function                                               | Description                    | Example                           |
| ------------------------------------------------------ | ------------------------------ | --------------------------------- |
| `export_media(input, output, type, format, mode, ...)` | Export media with full control | See below for detailed parameters |

### Traditional Signatures

```cpp
// Simple export (auto-detect, text format)
export_media(input_path, output_name);

// With type and format
export_media(input_path, output_name, Type::type, Format::format);

// With rendering mode
export_media(input_path, output_name, Type::type, Format::format, Mode::mode);

// Full control (video export with all options)
export_media(input_path, output_name,
             Type::type,      // Media type
             Format::format,  // Output format
             Mode::mode,      // Render mode
             max_width,       // Terminal width
             threshold,       // B&W threshold
             Audio::mode,     // Audio inclusion
             fps,             // Frame rate
             config,          // ExportConfig for image rendering
             use_gpu,         // GPU acceleration
             start_time,      // Start time (video)
             end_time         // End time (video)
);
```

---

## Parameters

| Parameter     | Type           | Default             | Description                            |
| ------------- | -------------- | ------------------- | -------------------------------------- |
| `input_path`  | `std::string`  | Required            | Path to source media file              |
| `output_name` | `std::string`  | Required            | Output filename (extension auto-added) |
| `type`        | `Type`         | `Type::auto_detect` | Media type hint                        |
| `format`      | `Format`       | `Format::text`      | Output file format                     |
| `mode`        | `Mode`         | `Mode::bw_dot`      | ASCII rendering mode                   |
| `max_width`   | `int`          | 80                  | Width in terminal characters           |
| `threshold`   | `int`          | 128                 | B&W threshold (0-255)                  |
| `audio`       | `Audio`        | `Audio::off`        | Include audio track (video only)       |
| `fps`         | `int`          | 0                   | Frame rate (0=use original)            |
| `config`      | `ExportConfig` | default             | Custom image rendering configuration   |
| `use_gpu`     | `bool`         | `true`              | Use GPU acceleration if available      |
| `start_time`  | `double`       | -1.0                | Start time in seconds (-1=beginning)   |
| `end_time`    | `double`       | -1.0                | End time in seconds (-1=end)           |

---

## Type Enum

| Type                | Description                          | Use Case                |
| ------------------- | ------------------------------------ | ----------------------- |
| `Type::auto_detect` | Auto-detect from extension (default) | Most cases              |
| `Type::image`       | Force treat as image                 | Binary files            |
| `Type::video`       | Force treat as video                 | Non-standard extensions |

---

## Format Enum

| Format             | Extension | Description               | Output                               |
| ------------------ | --------- | ------------------------- | ------------------------------------ |
| `Format::text`     | .txt      | Plain text file (default) | ASCII/Braille art as text            |
| `Format::image`    | .png      | PNG image file            | Rendered ASCII art as image          |
| `Format::video`    | .mp4      | MP4 video file            | Video with ASCII rendering per frame |
| `Format::pythonic` | .pi/.pv   | Encrypted Pythonic format | Encrypted media file                 |

---

## Mode Enum

Same as [Print Mode Enum](../Print/print.md#mode-enum):

| Mode                | Description                | Resolution  | Colors |
| ------------------- | -------------------------- | ----------- | ------ |
| `Mode::bw_dot`      | Braille patterns (default) | 8× terminal | B&W    |
| `Mode::bw`          | Half-block characters      | 2× terminal | B&W    |
| `Mode::colored`     | Half-block with color      | 2× terminal | 24-bit |
| `Mode::colored_dot` | Braille with color         | 8× terminal | 24-bit |
| `Mode::grayscale_dot` | Grayscale braille with dithering | 8× terminal | Grayscale |
| `Mode::flood_dot`   | Colored braille, flood-filled | 8× terminal | 24-bit |

---

## ExportConfig Class

**Advanced image rendering configuration** for fine-tuning how ASCII/Braille art is rendered to PNG images.

| Method                              | Description                          | Default       | Returns         |
| ----------------------------------- | ------------------------------------ | ------------- | --------------- |
| `set_dot_size(int)`                 | Set dot radius in pixels             | `2`           | `ExportConfig&` |
| `set_density(int)`                  | Set spacing multiplier               | `3`           | `ExportConfig&` |
| `set_background(uint8_t r, g, b)`   | Set background color                 | `(0, 0, 0)`   | `ExportConfig&` |
| `set_foreground(uint8_t r, g, b)`   | Set default foreground color         | `(255,255,255)` | `ExportConfig&` |
| `set_preserve_colors(bool)`         | Use ANSI colors from source          | `true`        | `ExportConfig&` |

### ExportConfig Fields

| Field             | Type   | Default       | Description                               |
| ----------------- | ------ | ------------- | ----------------------------------------- |
| `dot_size`        | `int`  | 2             | Dot radius in pixels for braille rendering |
| `dot_density`     | `int`  | 3             | Spacing multiplier (higher = more spaced) |
| `bg_color`        | `RGB`  | (0,0,0)       | Background color                          |
| `default_fg`      | `RGB`  | (255,255,255) | Default foreground color                  |
| `preserve_colors` | `bool` | `true`        | Use ANSI colors if present                |

### ExportConfig Usage

```cpp
// Builder pattern
ExportConfig config;
config.set_dot_size(4)
      .set_density(5)
      .set_background(255, 255, 255)  // White background
      .set_foreground(0, 0, 0)        // Black dots
      .set_preserve_colors(false);    // Ignore ANSI colors

// Use with traditional API
export_media("photo.png", "custom", 
             Type::image, Format::image, Mode::bw_dot,
             80, 128, Audio::off, 0, 
             config);  // Pass ExportConfig here
```

---

## Examples

### Example 1: Export Image to Text (Modern API)

```cpp
// Using RenderConfig
export_media("photo.png", "ascii_art", RenderConfig());
// Creates: ascii_art.txt

// With custom width
export_media("photo.png", "wide_art", RenderConfig().set_max_width(120));
```

### Example 2: Export Image as PNG

```cpp
// Modern API with RenderConfig
export_media("photo.png", "rendered", RenderConfig()
    .set_format(Format::image)
    .set_mode(Mode::colored_dot));
// Creates: rendered.png (PNG showing colored braille art)

// Traditional API with ExportConfig for fine control
ExportConfig cfg;
cfg.set_dot_size(4)
   .set_density(3)
   .set_background(255, 255, 255)  // White background
   .set_foreground(0, 0, 0);       // Black dots

export_media("photo.png", "custom_render",
             Type::image, Format::image, Mode::bw_dot,
             80, 128, Audio::off, 0, cfg);
// Creates: custom_render.png with custom appearance
```

### Example 3: Export Video with Modern API

```cpp
// Export full video with audio
export_media("input.mp4", "ascii_video", RenderConfig()
    .set_format(Format::video)
    .set_mode(Mode::colored)
    .with_audio()
    .set_max_width(120)
    .set_fps(30));
// Creates: ascii_video.mp4

// Export video segment (1:00 to 2:00)
export_media("input.mp4", "segment", RenderConfig()
    .set_format(Format::video)
    .set_mode(Mode::colored)
    .with_audio()
    .set_start_time(60.0)
    .set_end_time(120.0)
    .set_fps(24));
// Creates: segment.mp4 (1 minute clip)
```

### Example 4: Export with Dithering

```cpp
// High-quality grayscale with Floyd-Steinberg dithering
export_media("photo.jpg", "dithered", RenderConfig()
    .set_format(Format::image)
    .set_mode(Mode::grayscale_dot)
    .set_dithering(Dithering::floyd)
    .set_max_width(160));
// Creates: dithered.png
```

### Example 5: Export to Encrypted Format

```cpp
// Export image to encrypted .pi format
export_media("secret.png", "encrypted", RenderConfig()
    .set_format(Format::pythonic));
// Creates: encrypted.pi

// Export video to encrypted .pv format
export_media("private.mp4", "encrypted_video", RenderConfig()
    .set_format(Format::pythonic));
// Creates: encrypted_video.pv

// Later, use print() to view encrypted files
print("encrypted.pi");        // Auto-decrypts and displays
print("encrypted_video.pv");  // Auto-decrypts and plays
```

### Example 6: Batch Export with Different Modes

```cpp
std::string input = "photo.png";

// Export in all modes for comparison using modern API
export_media(input, "bw_dot", RenderConfig()
    .set_format(Format::image).set_mode(Mode::bw_dot));
export_media(input, "bw_block", RenderConfig()
    .set_format(Format::image).set_mode(Mode::bw));
export_media(input, "colored", RenderConfig()
    .set_format(Format::image).set_mode(Mode::colored));
export_media(input, "colored_dot", RenderConfig()
    .set_format(Format::image).set_mode(Mode::colored_dot));

// Creates: bw_dot.png, bw_block.png, colored.png, colored_dot.png
```

### Example 7: GPU Acceleration Control

```cpp
// Disable GPU for consistent results across systems
export_media("video.mp4", "cpu_only", RenderConfig()
    .set_format(Format::video)
    .no_gpu());

// Explicitly enable GPU
export_media("video.mp4", "gpu_fast", RenderConfig()
    .set_format(Format::video)
    .set_gpu(true));
```

---

## Video Export Process

When exporting video with `Format::video`:

1. **Frame Extraction** - FFmpeg extracts frames at target FPS
2. **ASCII Rendering** - Each frame rendered to ASCII/Braille (multi-threaded)
3. **Image Export** - ASCII art converted to PNG frames
4. **Video Encoding** - Frames combined into output video with FFmpeg
5. **Audio Merging** - Original audio track added if `Audio::on`

**Progress Display:**

- Real-time progress bar shows current stage and ETA
- Uses Unicode braille characters for smooth progress indication
- Automatically adjusts based on frame count

**Performance:**

- Multi-threaded rendering (uses CPU cores - 2)
- GPU acceleration for colored modes (if `use_gpu=true`)
- Optimized memory usage for large videos

---

## Return Value

All `export_media` functions return `bool`:

- `true` - Export successful
- `false` - Export failed (check console for error messages)

---

## Notes

- **Output filename:** Extension is automatically appended based on `format` parameter
- **Temporary files:** Created in `/tmp` or `/var/tmp` depending on video size
- **Memory usage:** Large videos use disk-based temp storage to avoid tmpfs limits
- **FFmpeg required:** Video export requires FFmpeg in system PATH
- **ImageMagick required:** Image processing uses ImageMagick for PNG export
- **GPU support:** OpenCL GPU acceleration optional (build with `PYTHONIC_ENABLE_OPENCL=ON`)

---

# Next Check

- [LiveDraw](../LiveDraw/livedraw.md)
