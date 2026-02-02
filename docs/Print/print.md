[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Include and namespaces](../Includes_and_namespaces/incl_name.md)

# Print and Pretty Print

This page documents the `print` and `pprint` helpers in Pythonic, using a clear format with detailed examples.

---

## Overview

The `print` and `pprint` helpers in Pythonic provide a convenient way to output `var` objects and other types to the console or files. These functions are designed to handle a wide range of data types, including primitive values, containers, nested structures, **images, and videos**, with a focus on readability and ease of use.

---

## `print`

### Function Signatures and Parameters

| Function                                                                                                             | Description                                                                                                                                         |
| -------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `print(args...)`                                                                                                     | Prints any number of arguments (of any type), separated by a space, ending with a newline. For `var` types, uses the simple `str()` representation. |
| `print(filepath)`                                                                                                    | Auto-detects file type and renders accordingly (image/video/text).                                                                                  |
| `print(filepath, Type::type)`                                                                                        | Prints media files with explicit type hint. Uses BW braille (Mode::bw_dot) rendering by default.                                                    |
| `print(filepath, Type::type, Mode::mode)`                                                                            | Prints media files with explicit type and render mode.                                                                                              |
| `print(filepath, Type::type, Mode::mode, Parser::parser)`                                                            | Prints media files with explicit type, mode, and parser backend.                                                                                    |
| `print(filepath, Type::type, Mode::mode, Parser::parser, Audio::mode)`                                               | Prints media files with explicit type, mode, parser, and audio control.                                                                             |
| `print(filepath, Type::type, Mode::mode, Parser::parser, Audio::mode, width, threshold)`                             | Full control over rendering with custom width and threshold.                                                                                        |
| `print(filepath, Type::type, Mode::mode, Parser::parser, Audio::mode, width, threshold, shell, pause_key, stop_key)` | Full control including shell mode and video playback controls.                                                                                      |
| `pprint(var v, size_t indent_step=2)`                                                                                | Pretty-prints a `var` object with indentation. `indent_step` sets the number of spaces per indent level (default: 2).                               |

**Parameters:**

- `args...` (print): Any number of arguments of any type.
- `v` (pprint): The `var` object to pretty-print.
- `indent_step` (pprint): (Optional) Number of spaces for each indentation level. Default is 2.
- `shell`: (Optional) Shell mode. `Shell::noninteractive` (default) disables keyboard controls. `Shell::interactive` enables them.
- `pause_key`: (Optional) Key to pause/resume video playback. Default is `'p'`. Use `'\0'` to disable.
- `stop_key`: (Optional) Key to stop video playback. Default is `'s'`. Use `'\0'` to disable.

> **Note:** The current implementation does not support custom separators or end characters for `print`. Output always ends with a newline, and arguments are separated by a single space.

### Usage Examples

```cpp
#include "pythonic/pythonic.hpp"
using namespace Pythonic;

int main() {
    var a = 42;
    var b = "Hello, World!";
    var c = {1, 2, 3};

    print(a); // Output: 42
    print(b, c); // Output: Hello, World! [1, 2, 3]
    print(a, b, c, "\n", "---"); // Output: 42 Hello, World! [1, 2, 3] ---
    return 0;
}
```

---

## Media Printing

### Overview

Pythonic can render **images** and **videos** directly in the terminal using high-resolution braille Unicode characters or true color (24-bit ANSI) rendering. Each terminal character cell represents a 2×4 pixel grid in braille mode, providing 8x the resolution of standard ASCII art.

### Type Enum

Use the `Type` enum to specify how to handle media files:

| Type                | Description                                          |
| ------------------- | ---------------------------------------------------- |
| `Type::auto_detect` | Automatically detect file type from extension        |
| `Type::image`       | Force treat as image (render as braille or colored)  |
| `Type::video`       | Force treat as video (play with real-time rendering) |
| `Type::video_info`  | Show video metadata only (no playback)               |
| `Type::webcam`      | Capture from webcam (requires OpenCV)                |
| `Type::text`        | Force treat as plain text                            |

### Mode Enum

Use the `Mode` enum to control the visual output:

| Mode                | Description                                                                    |
| ------------------- | ------------------------------------------------------------------------------ |
| `Mode::bw`          | Black and white using half-block characters (▀▄█, 1×2 pixels per char)         |
| `Mode::bw_dot`      | Black and white using braille characters (default, 2×4 pixels per char)        |
| `Mode::colored`     | True color (24-bit ANSI) using half-block characters (1×2 pixels per char)     |
| `Mode::colored_dot` | True color using braille characters with averaged colors (2×4 pixels per char) |

### Parser Enum

Use the `Parser` enum to select the backend for media processing:

| Parser                   | Description                                          |
| ------------------------ | ---------------------------------------------------- |
| `Parser::default_parser` | FFmpeg for video, ImageMagick for images (default)   |
| `Parser::opencv`         | OpenCV for everything (also supports webcam capture) |

### Audio Enum

Use the `Audio` enum to control audio playback for videos:

| Audio        | Description                                            |
| ------------ | ------------------------------------------------------ |
| `Audio::off` | No audio playback (default, silent video)              |
| `Audio::on`  | Enable synchronized audio (requires SDL2 or PortAudio) |

### Shell Enum

Use the `Shell` enum to control keyboard input handling during video playback:

| Shell                   | Description                                                  |
| ----------------------- | ------------------------------------------------------------ |
| `Shell::noninteractive` | No keyboard input handling (default, safe for scripts/pipes) |
| `Shell::interactive`    | Enable keyboard controls for pause/stop                      |

> **Important:** By default, video playback uses `Shell::noninteractive` to avoid terminal issues when running in scripts, CI/CD pipelines, or non-interactive environments. Use `Shell::interactive` only when running in a real terminal where user input is expected.

### Supported Formats

**Images:** PNG, JPG, JPEG, GIF, BMP, PPM, PGM, PBM, `.pi` (Pythonic Image)

**Videos:** MP4, AVI, MKV, MOV, WEBM, FLV, WMV, M4V, GIF, `.pv` (Pythonic Video)

**Webcam:** Device index ("0", "1") or path ("/dev/video0", "/dev/video1")

### Dependencies

Media printing requires these external tools:

- **ImageMagick** - For image format conversion (`convert` command) - used with `Parser::default_parser`
- **FFmpeg** - For video decoding (`ffmpeg` and `ffprobe` commands) - used with `Parser::default_parser`
- **OpenCV** - For all media operations including webcam capture - used with `Parser::opencv`

For OpenCV support:

- **Ubuntu:** `sudo apt install libopencv-dev`
- **macOS:** `brew install opencv`
- Build with: `cmake -B build -DPYTHONIC_ENABLE_OPENCV=ON`

For audio playback (video_audio):

- **SDL2** - `sudo apt install libsdl2-dev` (Ubuntu) or `brew install sdl2` (macOS)
- Or **PortAudio** - `sudo apt install portaudio19-dev` (Ubuntu) or `brew install portaudio` (macOS)

See the [Installation Guide](../examples/readme.md) for installation instructions.

### Image Printing Examples

```cpp
#include "pythonic/pythonic.hpp"
using namespace Pythonic;

int main() {
    // Auto-detect from extension (default BW braille)
    print("photo.png");

    // Explicit type hint with black & white braille dots
    print("photo.png", Type::image);

    // Different rendering modes
    print("photo.png", Type::image, Mode::bw);          // BW half-blocks
    print("photo.png", Type::image, Mode::bw_dot);      // BW braille (default)
    print("photo.png", Type::image, Mode::colored);     // True color half-blocks
    print("photo.png", Type::image, Mode::colored_dot); // True color braille

    // With custom width (100 chars)
    print("photo.jpg", Type::image, Mode::colored, 100);

    // BW with custom width and threshold
    print("photo.jpg", Type::image, Mode::bw_dot, 80, 128);

    // Using OpenCV parser instead of ImageMagick
    print("photo.png", Type::image, Mode::colored, Parser::opencv);

    // Pythonic encrypted format (.pi files)
    print("secret.pi", Type::image, Mode::colored);

    return 0;
}
```

### Video Printing Examples

```cpp
#include "pythonic/pythonic.hpp"
using namespace Pythonic;

int main() {
    // Show video information only
    print("video.mp4", Type::video_info);
    // Output:
    // Video: video.mp4
    //   Resolution: 1920x1080
    //   FPS: 30
    //   Duration: 120.5 seconds

    // Play video in BW braille (default, silent)
    print("video.mp4", Type::video);

    // Different rendering modes
    print("video.mp4", Type::video, Mode::bw);          // BW half-blocks
    print("video.mp4", Type::video, Mode::bw_dot);      // BW braille (default)
    print("video.mp4", Type::video, Mode::colored);     // True color half-blocks
    print("video.mp4", Type::video, Mode::colored_dot); // True color braille

    // Play video with audio (requires SDL2 or PortAudio)
    print("video.mp4", Type::video, Mode::bw_dot, Audio::on);

    // Play video with audio in true color
    print("video.mp4", Type::video, Mode::colored, Audio::on);

    // Play with custom width (silent)
    print("video.mp4", Type::video, Mode::colored, Audio::off, 60);

    // Using OpenCV parser instead of FFmpeg
    print("video.mp4", Type::video, Mode::colored, Parser::opencv);

    // Pythonic encrypted format (.pv files)
    print("secret.pv", Type::video, Mode::colored);

    return 0;
}
```

### Video Playback Controls

During video playback in **interactive mode** (`Shell::interactive`), you can control the player using keyboard keys:

- **Pause/Resume**: Press the `pause_key` (default: `'p'`) to toggle between paused and playing states
- **Stop**: Press the `stop_key` (default: `'s'`) to stop playback and continue program execution

> **Note:** By default, keyboard controls are disabled (`Shell::noninteractive`) for safety in scripts and non-interactive environments. You must explicitly enable them with `Shell::interactive`.

```cpp
#include "pythonic/pythonic.hpp"
using namespace Pythonic;

int main() {
    // Default: no keyboard controls (safe for scripts)
    print("video.mp4", Type::video);

    // Enable keyboard controls for interactive use
    // Full signature: print(filepath, type, mode, parser, audio, width, threshold, shell, pause_key, stop_key)
    print("video.mp4", Type::video, Mode::colored, Parser::default_parser,
          Audio::off, 80, 128, Shell::interactive, 'p', 's');

    // Interactive with custom keys: 'p' to pause, 'q' to quit
    print("video.mp4", Type::video, Mode::colored, Parser::default_parser,
          Audio::off, 80, 128, Shell::interactive, 'p', 'q');

    // Interactive with only stop key (no pause)
    print("video.mp4", Type::video, Mode::colored, Parser::default_parser,
          Audio::off, 80, 128, Shell::interactive, '\0', 'x');

    std::cout << "Video finished or stopped, program continues..." << std::endl;
    return 0;
}
```

When paused, a `[PAUSED - Press 'p' to resume]` indicator is shown at the top of the screen.

### Audio Playback Requirements

To enable audio playback with `Audio::on`, you need to:

1. **Install an audio library:**
   - **SDL2 (recommended):** `sudo apt install libsdl2-dev` (Ubuntu) or `brew install sdl2` (macOS)
   - **PortAudio:** `sudo apt install portaudio19-dev` (Ubuntu) or `brew install portaudio` (macOS)

2. **Rebuild with audio support:**

   ```bash
   # With SDL2
   cmake -B build -DPYTHONIC_ENABLE_SDL2_AUDIO=ON
   cmake --build build

   # Or with PortAudio
   cmake -B build -DPYTHONIC_ENABLE_PORTAUDIO=ON
   cmake --build build
   ```

> **Note:** If audio libraries are not available, `Audio::on` will fall back to silent video playback.

### Webcam Examples

Webcam support requires OpenCV. Build with OpenCV enabled:

```bash
cmake -B build -DPYTHONIC_ENABLE_OPENCV=ON
cmake --build build
```

```cpp
#include "pythonic/pythonic.hpp"
using namespace Pythonic;

int main() {
    // Open default webcam (device 0)
    print("0", Type::webcam);

    // Colored braille rendering
    print("0", Type::webcam, Mode::colored_dot);

    // Different rendering modes
    print("0", Type::webcam, Mode::bw);          // BW half-blocks
    print("0", Type::webcam, Mode::colored);     // True color half-blocks

    // Device path (Linux)
    print("/dev/video0", Type::webcam, Mode::colored);

    // With custom width
    print("0", Type::webcam, Mode::colored_dot, 80);

    return 0;
}
```

### Pythonic Encrypted Format Examples

Pythonic supports proprietary encrypted media formats for secure distribution:

```cpp
#include "pythonic/pythonic.hpp"
using namespace Pythonic;

int main() {
    // Printing .pi (Pythonic Image) files
    print("secret.pi", Type::image, Mode::colored);

    // Printing .pv (Pythonic Video) files
    print("secret.pv", Type::video, Mode::colored);
    print("secret.pv", Type::video, Mode::colored_dot, Audio::on);

    // These files are encrypted and can only be played with this library
    // See docs/Media/media.md for how to create .pi/.pv files

    return 0;
}
```

### Technical Details

- **Braille characters** (U+2800-U+28FF): Each character contains 8 dots arranged in a 2×4 grid
- **Half-block characters** (▀ U+2580): Used for colored rendering with 2 vertical pixels per char
- **True color**: Uses ANSI 24-bit color codes (`\033[38;2;R;G;Bm` for foreground)
- **Double-buffering**: Uses ANSI escape codes (`\033[H`) to avoid screen flickering during video playback
- **Frame rate**: Video plays at the source's native FPS (or can be overridden)
- **Threshold**: Values below threshold appear as "off" (dark), above as "on" (bright) - BW mode only
- **Audio sync**: Video and audio run in separate threads with buffered synchronization

---

## `export_media` - Exporting ASCII Art

### Overview

The `export_media` function converts images and videos to ASCII/Braille art and saves them in various formats. This is the complement to `print` - while `print` displays to the terminal, `export_media` saves to files.

### Function Signature

```cpp
bool export_media(
    const std::string& input_path,
    const std::string& output_name,
    Type type = Type::auto_detect,
    Format format = Format::text,
    Mode mode = Mode::bw_dot,
    int max_width = 80,
    int threshold = 128,
    Audio audio = Audio::off,
    int fps = 0,
    const ExportConfig& config = ExportConfig()  // Customize rendering style
);
```

### Parameters

| Parameter     | Type           | Default             | Description                                                        |
| ------------- | -------------- | ------------------- | ------------------------------------------------------------------ |
| `input_path`  | `std::string`  | required            | Path to source media file (image or video)                         |
| `output_name` | `std::string`  | required            | Output filename (extension will be added/replaced based on format) |
| `type`        | `Type`         | `Type::auto_detect` | Media type hint (auto_detect, image, video)                        |
| `format`      | `Format`       | `Format::text`      | Output format (see Format enum below)                              |
| `mode`        | `Mode`         | `Mode::bw_dot`      | Render mode (bw, bw_dot, colored, colored_dot)                     |
| `max_width`   | `int`          | `80`                | Width of rendered output in characters                             |
| `threshold`   | `int`          | `128`               | Brightness threshold for BW modes (0-255)                          |
| `audio`       | `Audio`        | `Audio::off`        | Whether to include audio in video exports                          |
| `fps`         | `int`          | `0`                 | Frame rate for video export (0 = original fps)                     |
| `config`      | `ExportConfig` | `ExportConfig()`    | Rendering customization (dot_size, density, colors)                |

### Format Enum

| Format             | Extension | Description                                           |
| ------------------ | --------- | ----------------------------------------------------- |
| `Format::text`     | `.txt`    | Plain text file with ASCII/Braille characters         |
| `Format::image`    | `.png`    | PNG image with rendered Braille dots as actual pixels |
| `Format::video`    | `.mp4`    | MP4 video with rendered ASCII art frames              |
| `Format::pythonic` | `.pi/.pv` | Proprietary encrypted Pythonic format                 |

### ExportConfig - Customizing Rendering Style

Use `ExportConfig` to customize how ASCII/Braille art is rendered to images and videos:

```cpp
struct ExportConfig {
    int dot_size = 2;                    // Dot radius in pixels
    int dot_density = 3;                 // Spacing multiplier (higher = more spaced out)
    RGB bg_color = RGB(0, 0, 0);         // Background color (default: black)
    RGB default_fg = RGB(255, 255, 255); // Default foreground color (default: white)
    bool preserve_colors = true;         // Use ANSI colors if present in source
};
```

**Configuration Options:**

| Option            | Default            | Description                                                     |
| ----------------- | ------------------ | --------------------------------------------------------------- |
| `dot_size`        | `2`                | Radius of rendered Braille dots in pixels                       |
| `dot_density`     | `3`                | Spacing between dots (higher = more spaced out, lower = denser) |
| `bg_color`        | `RGB(0,0,0)`       | Background color where no dots are rendered                     |
| `default_fg`      | `RGB(255,255,255)` | Default dot color when source has no ANSI colors                |
| `preserve_colors` | `true`             | Whether to use ANSI colors from source (colored modes)          |

**Builder Pattern:**

```cpp
ExportConfig config;
config.set_dot_size(3)
      .set_density(2)
      .set_background(20, 20, 30)      // Dark blue-ish background
      .set_foreground(0, 255, 128)     // Bright green dots
      .set_preserve_colors(true);
```

**RGB Helper Struct:**

```cpp
struct RGB {
    uint8_t r, g, b;
    RGB();                              // Default: black (0, 0, 0)
    RGB(uint8_t r, uint8_t g, uint8_t b);
};

// Examples
RGB black;                   // 0, 0, 0
RGB white(255, 255, 255);
RGB red(255, 0, 0);
RGB custom(128, 64, 192);    // Purple-ish
```

### Usage Examples

#### Exporting Images

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main() {
    // Export to text file (default)
    export_media("photo.png", "output");  // Creates output.txt

    // Export to PNG with rendered Braille dots
    export_media("photo.png", "output", Type::image, Format::image);  // Creates output.png

    // Export with specific render mode
    export_media("photo.png", "output", Type::image, Format::image, Mode::colored);
    export_media("photo.png", "output", Type::image, Format::image, Mode::colored_dot);
    export_media("photo.png", "output", Type::image, Format::image, Mode::bw);
    export_media("photo.png", "output", Type::image, Format::image, Mode::bw_dot);

    // Export with custom width and threshold
    export_media("photo.png", "output", Type::image, Format::image,
                 Mode::bw_dot, 100, 100);  // 100 chars wide, threshold=100

    // Export to Pythonic encrypted format
    export_media("photo.png", "output", Type::image, Format::pythonic);  // Creates output.pi

    return 0;
}
```

#### Exporting Videos

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main() {
    // Export video to ASCII art video
    export_media("video.mp4", "output", Type::video, Format::video);  // Creates output.mp4

    // Export with specific render mode
    export_media("video.mp4", "output", Type::video, Format::video, Mode::colored);

    // Export with audio
    export_media("video.mp4", "output", Type::video, Format::video,
                 Mode::colored, 80, 128, Audio::on);

    // Export with custom fps
    export_media("video.mp4", "output", Type::video, Format::video,
                 Mode::colored, 80, 128, Audio::on, 24);  // 24 fps

    // Use original video fps (default when fps=0)
    export_media("video.mp4", "output", Type::video, Format::video,
                 Mode::colored, 80, 128, Audio::on, 0);  // Original fps

    // Export to Pythonic encrypted format
    export_media("video.mp4", "output", Type::video, Format::pythonic);  // Creates output.pv

    return 0;
}
```

#### Custom Styling with ExportConfig

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;
using namespace pythonic::ex;

int main() {
    // Create custom export config
    ExportConfig config;
    config.set_dot_size(4)             // Larger dots
          .set_density(2)               // Tighter spacing
          .set_background(32, 32, 48)   // Deep purple background
          .set_foreground(0, 255, 128)  // Bright green default color
          .set_preserve_colors(true);   // Keep original image colors

    // Export image with custom styling
    export_media("photo.png", "styled_output", Type::image, Format::image,
                 Mode::colored_dot, 100, 128, Audio::off, 0, config);

    // Export video with custom styling
    export_media("video.mp4", "styled_video", Type::video, Format::video,
                 Mode::colored, 80, 128, Audio::on, 0, config);

    return 0;
}
```

### Complete Export Example

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main() {
    std::string input_video = "media/video.mp4";

    // Export in all 4 render modes
    export_media(input_video, "export_bw", Type::video, Format::video,
                 Mode::bw, 80, 128, Audio::on);

    export_media(input_video, "export_bw_dot", Type::video, Format::video,
                 Mode::bw_dot, 80, 128, Audio::on);

    export_media(input_video, "export_colored", Type::video, Format::video,
                 Mode::colored, 80, 128, Audio::on);

    export_media(input_video, "export_colored_dot", Type::video, Format::video,
                 Mode::colored_dot, 80, 128, Audio::on);

    std::cout << "All exports complete!" << std::endl;
    return 0;
}
```

### How Video Export Works

1. **Frame extraction**: FFmpeg extracts frames from the source video
2. **ASCII rendering**: Each frame is rendered to ASCII/Braille art using the specified mode
3. **Image rendering**: The ASCII art is rendered to actual pixels (proper Braille dot rendering)
4. **Video encoding**: All frames are combined back into an MP4 video
5. **Audio muxing** (optional): Audio track is extracted and muxed with the video

### Output Quality

The exported video/image quality depends on:

- **Width** (`max_width`): Higher = more detail, larger file
- **Mode**: `colored` modes preserve more visual information
- **FPS**: Higher = smoother video, larger file

### Dependencies

Video export requires **FFmpeg** to be installed:

```bash
# Ubuntu/Debian
sudo apt install ffmpeg

# macOS
brew install ffmpeg
```

Image export requires **ImageMagick** for PNG conversion:

```bash
# Ubuntu/Debian
sudo apt install imagemagick

# macOS
brew install imagemagick
```

---

## `pprint`

### Function Signature

```cpp
pprint(const var& v, size_t indent_step = 2);
```

### Parameters

- `v`: The `var` object to pretty-print.
- `indent_step`: (Optional) Number of spaces to use for each indentation level. Default is 2. Increase for wider indentation.

### Features

- Outputs nested structures in a human-readable format.
- Automatically applies indentation and line breaks for clarity.
- Ideal for debugging and inspecting large or deeply nested data.
- Allows customizing indentation width via `indent_step`.

### Example

```cpp
#include "pythonicPrint.hpp"
#include "pythonicVars.hpp"

int main() {
    var nested = {"key1": {"subkey1": 1, "subkey2": 2}, "key2": [1, 2, 3]};

    pprint(nested);
    // Output:
    // {
    //     "key1": {
    //         "subkey1": 1,
    //         "subkey2": 2
    //     },
    //     "key2": [
    //         1,
    //         2,
    //         3
    //     ]
    // }

    return 0;
}
```

---

## Integration with `str()`, `toString()`, `pretty_str()`, and `operator<<`

- `print` relies on `str()` and `operator<<` to format output. This ensures that even large containers are displayed in a readable format.
- `pprint` uses `pretty_str()` to format nested structures with indentation and line breaks.
- Custom types can implement `toString()` or `operator<<` to define their own string representation for `print` and `pprint`.

### Example

```cpp
#include "pythonicPrint.hpp"
#include "pythonicVars.hpp"

struct CustomType {
    int x;
    int y;

    friend std::ostream& operator<<(std::ostream& os, const CustomType& obj) {
        return os << "CustomType(" << obj.x << ", " << obj.y << ")";
    }
};

int main() {
    CustomType obj = {10, 20};

    print(obj); // Output: CustomType(10, 20)

    return 0;
}
```

---

## File Output Helpers

Both `print` and `pprint` can redirect their output to files or streams. This is useful for logging or saving data for later analysis.

### Example

```cpp
#include "pythonicPrint.hpp"
#include "pythonicVars.hpp"
#include <fstream>

int main() {
    var data = {"name": "Alice", "age": 30, "hobbies": {"reading", "hiking"}};

    std::ofstream file("output.txt");
    if (file.is_open()) {
        print(file, data); // Writes to file
        pprint(file, data); // Pretty-prints to file
        file.close();
    }

    return 0;
}
```

---

## Print Text on Plots (`Figure::print`)

The plotting library includes a `print()` method for adding text annotations directly onto graphs. This is different from the global `print()` function—it's a method on `Figure` objects.

### Basic Usage

```cpp
#include "pythonic/pythonicPlot.hpp"
using namespace pythonic::plot;

Figure fig(80, 24);

// Plot some data
fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");

// Add text at data coordinates
fig.print("Maximum", 1.57, 1.0, "yellow");  // Text at (π/2, 1)
fig.print("Minimum", -1.57, -1.0, "red");   // Text at (-π/2, -1)

std::cout << fig.render();
```

### Parameters

| Parameter | Type                    | Description                          |
| --------- | ----------------------- | ------------------------------------ |
| `text`    | `std::string`           | The text to display                  |
| `x`       | `double`                | X position in **data coordinates**   |
| `y`       | `double`                | Y position in **data coordinates**   |
| `color`   | `std::string` or `RGBA` | Text color (e.g., `"cyan"`, `"red"`) |

### Pixel Font

Text is rendered using a compact 3×5 pixel Braille font:

- **Numbers**: 0-9
- **Letters**: A-Z, a-z
- **Symbols**: . , : - + = ( ) \* / \_

### Example: Annotated Graph

```cpp
Figure fig(80, 24);
fig.title("Annotated Sine Wave");

fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");
fig.print("Peak", PI/2, 1.0, "green");
fig.print("Zero", 0, 0, "yellow");
fig.print("Trough", -PI/2, -1.0, "red");

std::cout << fig.render();
```

### Method Chaining

```cpp
fig.title("My Plot")
   .plot([](double x) { return x*x; }, -3, 3, "cyan", "x²")
   .print("Vertex", 0, 0, "yellow")
   .print("Rising", 2, 4, "green");
```

See also: [Plot Documentation](../Plot/plot.md)

---

## Performance Considerations

While `print` and `pprint` are convenient for debugging and output, they may not be suitable for performance-sensitive logging. For large `var` objects or containers, consider serializing only the necessary parts to minimize overhead.

# Next Check

- [Var](../Var/var.md)
