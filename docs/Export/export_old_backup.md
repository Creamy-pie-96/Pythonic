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
export_media("photo.png", "output", Type::image, Format::image);  // Creates output.png

// Export video with ASCII rendering
export_media("video.mp4", "output_video", Type::video, Format::video);  // Creates output_video.mp4

// Export to encrypted Pythonic format
export_media("photo.png", "secret", Type::image, Format::pythonic);  // Creates secret.pi
```

---

## Export Function

| Function                                               | Description                    | Example                           |
| ------------------------------------------------------ | ------------------------------ | --------------------------------- |
| `export_media(input, output, type, format, mode, ...)` | Export media with full control | See below for detailed parameters |

### Basic Signatures

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
             config,          // Export configuration
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
| `config`      | `ExportConfig` | default             | Custom export configuration            |
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

---

## ExportConfig Class

Configure how ASCII art is rendered to images:

| Field             | Type   | Default       | Description                               |
| ----------------- | ------ | ------------- | ----------------------------------------- |
| `dot_size`        | `int`  | 2             | Dot radius in pixels                      |
| `dot_density`     | `int`  | 3             | Spacing multiplier (higher = more spaced) |
| `bg_color`        | `RGB`  | (0,0,0)       | Background color                          |
| `default_fg`      | `RGB`  | (255,255,255) | Default foreground color                  |
| `preserve_colors` | `bool` | `true`        | Use ANSI colors if present                |

**Builder Methods:**

```cpp
ExportConfig config;
config.set_dot_size(3)
      .set_density(4)
      .set_background(0, 0, 0)
      .set_foreground(255, 255, 255)
      .set_preserve_colors(true);
```

---

## Examples

### Example 1: Export Image to Text

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Auto-detect type, export as text
export_media("photo.png", "ascii_art");
// Creates: ascii_art.txt

// Explicit type and format
export_media("photo.jpg", "output", Type::image, Format::text);
// Creates: output.txt
```

### Example 2: Export Image as PNG

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Export with colored braille rendering
export_media("photo.png", "rendered",
             Type::image, Format::image, Mode::colored_dot);
// Creates: rendered.png (PNG showing colored braille art)

// Custom dot size and colors
ex::ExportConfig cfg;
cfg.set_dot_size(4)
   .set_density(3)
   .set_background(255, 255, 255)  // White background
   .set_foreground(0, 0, 0);       // Black dots

export_media("photo.png", "custom_render",
             Type::image, Format::image, Mode::bw_dot,
             80, 128, Audio::off, 0, cfg);
// Creates: custom_render.png with custom appearance
```

### Example 3: Export Video with ASCII Rendering

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Export full video with Braille rendering at 30 fps
export_media("input.mp4", "ascii_video",
             Type::video, Format::video, Mode::bw_dot,
             120,           // Terminal width
             128,           // Threshold
             Audio::on,     // Include audio
             30);           // 30 FPS
// Creates: ascii_video.mp4

// Export video segment (1:00 to 2:00)
export_media("input.mp4", "segment",
             Type::video, Format::video, Mode::colored,
             100, 128, Audio::on, 24,
             ex::ExportConfig(), true,
             60.0,    // Start at 1:00
             120.0);  // End at 2:00
// Creates: segment.mp4 (1 minute of colored ASCII video)
```

### Example 4: Export to Encrypted Format

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// Export image to encrypted .pi format
export_media("secret.png", "encrypted",
             Type::image, Format::pythonic);
// Creates: encrypted.pi

// Export video to encrypted .pv format
export_media("private.mp4", "encrypted_video",
             Type::video, Format::pythonic);
// Creates: encrypted_video.pv

// Later, use print() to view encrypted files
print("encrypted.pi");        // Auto-decrypts and displays
print("encrypted_video.pv");  // Auto-decrypts and plays
```

### Example 5: High-Quality Video Export

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

// High resolution colored video with custom config
ex::ExportConfig cfg;
cfg.set_dot_size(2)
   .set_density(4)
   .set_preserve_colors(true);

export_media("input.mp4", "hq_output",
             Type::video,
             Format::video,
             Mode::colored_dot,  // Colored braille for detail
             160,                // Wide terminal
             128,                // Standard threshold
             Audio::on,          // Include audio
             30,                 // 30 FPS
             cfg,                // Custom config
             true,               // Use GPU if available
             0,                  // Start from beginning
             -1);                // To end
// Creates: hq_output.mp4 (high-quality colored ASCII video)
```

### Example 6: Batch Export with Different Modes

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;

std::string input = "photo.png";

// Export in all modes for comparison
export_media(input, "bw_dot", Type::image, Format::image, Mode::bw_dot);
export_media(input, "bw_block", Type::image, Format::image, Mode::bw);
export_media(input, "colored", Type::image, Format::image, Mode::colored);
export_media(input, "colored_dot", Type::image, Format::image, Mode::colored_dot);

// Creates: bw_dot.png, bw_block.png, colored.png, colored_dot.png
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
- **ImageMagick required:** Image processing uses ImageMagick
- **GPU support:** OpenCL GPU acceleration optional (build with `PYTHONIC_ENABLE_OPENCL=ON`)

---

# Next Check

- [LiveDraw](../LiveDraw/livedraw.md)
