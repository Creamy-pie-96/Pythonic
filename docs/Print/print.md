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

| Function                                                                | Description                                                                                                                                         |
| ----------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `print(args...)`                                                        | Prints any number of arguments (of any type), separated by a space, ending with a newline. For `var` types, uses the simple `str()` representation. |
| `print(filepath)`                                                       | Auto-detects file type and renders accordingly (image/video/text).                                                                                  |
| `print(filepath, Type::type)`                                           | Prints media files with explicit type hint. Uses black & white (braille) rendering by default.                                                      |
| `print(filepath, Type::type, Render::mode)`                             | Prints media files with explicit type and render mode (`Render::BW` or `Render::colored`).                                                          |
| `print(filepath, Type::type, Render::mode, Audio::mode)`                | Prints media files with explicit type, render mode, and audio control.                                                                              |
| `print(filepath, Type::type, Render::mode, Audio::mode, width, thresh)` | Full control over rendering with custom width and threshold.                                                                                        |
| `pprint(var v, size_t indent_step=2)`                                   | Pretty-prints a `var` object with indentation. `indent_step` sets the number of spaces per indent level (default: 2).                               |

**Parameters:**

- `args...` (print): Any number of arguments of any type.
- `v` (pprint): The `var` object to pretty-print.
- `indent_step` (pprint): (Optional) Number of spaces for each indentation level. Default is 2.

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
| `Type::text`        | Force treat as plain text                            |

### Render Enum

Use the `Render` enum to control the visual output:

| Render            | Description                                                                |
| ----------------- | -------------------------------------------------------------------------- |
| `Render::BW`      | Black and white using braille characters (default, 2×4 pixels per char)    |
| `Render::colored` | True color (24-bit ANSI) using half-block characters (1×2 pixels per char) |

### Audio Enum

Use the `Audio` enum to control audio playback for videos:

| Audio        | Description                                            |
| ------------ | ------------------------------------------------------ |
| `Audio::off` | No audio playback (default, silent video)              |
| `Audio::on`  | Enable synchronized audio (requires SDL2 or PortAudio) |

### Supported Formats

**Images:** PNG, JPG, JPEG, GIF, BMP, PPM, PGM, PBM

**Videos:** MP4, AVI, MKV, MOV, WEBM, FLV, WMV, M4V, GIF

### Dependencies

Media printing requires these external tools:

- **ImageMagick** - For image format conversion (`convert` command)
- **FFmpeg** - For video decoding (`ffmpeg` and `ffprobe` commands)

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

    // Explicit type hint with black & white braille
    print("photo.png", Type::image);

    // True color rendering (24-bit ANSI)
    print("photo.png", Type::image, Render::colored);

    // Full control: colored, 100 char width
    print("photo.jpg", Type::image, Render::colored, 100);

    // BW with custom width and threshold
    print("photo.jpg", Type::image, Render::BW, 80, 128);

    return 0;
}
```

    print("photo.jpg", Type::image, 80, 128);
    // Parameters: filepath, type, max_width=80, threshold=128

    return 0;

}

````

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

    // Play video in true color (silent)
    print("video.mp4", Type::video, Render::colored);

    // Play video with audio (requires SDL2 or PortAudio)
    print("video.mp4", Type::video, Render::BW, Audio::on);

    // Play video with audio in true color
    print("video.mp4", Type::video, Render::colored, Audio::on);

    // Play with custom width (silent)
    print("video.mp4", Type::video, Render::colored, Audio::off, 60);

    return 0;
}
````

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

### Technical Details

- **Braille characters** (U+2800-U+28FF): Each character contains 8 dots arranged in a 2×4 grid
- **Half-block characters** (▀ U+2580): Used for colored rendering with 2 vertical pixels per char
- **True color**: Uses ANSI 24-bit color codes (`\033[38;2;R;G;Bm` for foreground)
- **Double-buffering**: Uses ANSI escape codes (`\033[H`) to avoid screen flickering during video playback
- **Frame rate**: Video plays at the source's native FPS (or can be overridden)
- **Threshold**: Values below threshold appear as "off" (dark), above as "on" (bright) - BW mode only
- **Audio sync**: Video and audio run in separate threads with buffered synchronization

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

## Performance Considerations

While `print` and `pprint` are convenient for debugging and output, they may not be suitable for performance-sensitive logging. For large `var` objects or containers, consider serializing only the necessary parts to minimize overhead.

# Next Check

- [Var](../Var/var.md)
