# pythonicDraw — Terminal Graphics Engine

## What This Module Does

`pythonicDraw.hpp` is the rendering engine that makes terminal graphics possible. It provides the foundational primitives for rendering images and videos to the terminal using Unicode characters (Braille patterns, half-blocks) and ANSI color codes.

**Core problem it solves**: You want to display images, play videos, or draw graphics in a terminal — which nominally only supports text characters.

---

## The Architecture: How It All Fits Together

```
┌─────────────────────────────────────────────────────────────────┐
│                      pythonicDraw.hpp                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐                                           │
│  │   Input Media   │                                           │
│  │  (Image/Video)  │                                           │
│  └────────┬────────┘                                           │
│           │                                                    │
│           ▼                                                    │
│  ┌─────────────────┐     External tools:                       │
│  │ FFmpeg/         │     - FFmpeg (video decode)               │
│  │ ImageMagick     │     - ImageMagick (image decode)          │
│  └────────┬────────┘     - Graphviz (DOT graphs)              │
│           │                                                    │
│           ▼                                                    │
│  ┌─────────────────┐                                           │
│  │   Raw Pixels    │     RGB or Grayscale                      │
│  │   (PPM/buffer)  │                                           │
│  └────────┬────────┘                                           │
│           │                                                    │
│  ┌────────┴────────┬────────────────────┬──────────────────┐  │
│  ▼                 ▼                    ▼                  ▼  │
│ ┌──────────┐  ┌──────────┐  ┌────────────────┐  ┌─────────┐  │
│ │BrailleCanvas│ │ColorCanvas│ │ColoredBraille │ │BWBlock  │  │
│ │(bw_dot)     │ │(colored) │ │Canvas(color_dot)│ │Canvas(bw)│ │
│ └──────┬─────┘  └────┬─────┘ └───────┬────────┘ └────┬────┘  │
│        │             │               │               │        │
│        └─────────────┴───────────────┴───────────────┘        │
│                             │                                  │
│                             ▼                                  │
│                    ┌─────────────────┐                        │
│                    │   ANSI String   │                        │
│                    │ (UTF-8 + colors)│                        │
│                    └─────────────────┘                        │
│                             │                                  │
│                             ▼                                  │
│                    ┌─────────────────┐                        │
│                    │    Terminal     │                        │
│                    └─────────────────┘                        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Part 1: The Four Rendering Modes

### Mode Enum

```cpp
enum class Mode {
    bw,          // Black & white using half-block characters (▀▄█)
    bw_dot,      // Black & white using Braille patterns (⠿) — higher resolution
    colored,     // True color (24-bit) using half-block characters
    colored_dot  // True color using Braille patterns — highest fidelity
};
```

### When to Use Each Mode

| Mode          | Resolution   | Colors         | Best For                              |
| ------------- | ------------ | -------------- | ------------------------------------- |
| `bw_dot`      | 2×4 per char | 2 (B&W)        | Line art, high detail, slow terminals |
| `bw`          | 1×2 per char | 24 grayscale   | Smooth gradients, photos (B&W)        |
| `colored`     | 1×2 per char | 16.7M (24-bit) | Photos, videos, color accuracy        |
| `colored_dot` | 2×4 per char | 16.7M (24-bit) | Balance of detail + color             |

### Resolution Math

For an 80×24 terminal:

| Mode          | Pixel Resolution |
| ------------- | ---------------- |
| `bw_dot`      | 160 × 96         |
| `bw`          | 80 × 48          |
| `colored`     | 80 × 48          |
| `colored_dot` | 160 × 96         |

Braille modes get 4× resolution (2×4 dots per char vs 1×2 half-blocks).

---

## Part 2: The BrailleCanvas Class

### The Core Data Structure

```cpp
class BrailleCanvas {
private:
    size_t _char_width;    // Width in characters
    size_t _char_height;   // Height in characters
    size_t _pixel_width;   // = char_width * 2
    size_t _pixel_height;  // = char_height * 4

    // One byte per character cell, bits represent dots
    std::vector<std::vector<uint8_t>> _canvas;
};
```

Each byte stores the 8-dot pattern for one Braille character.

### Braille Bit Layout

```cpp
// The 8 dots in a 2×4 Braille cell map to bits like this:
//   Col 0   Col 1
//   [bit0]  [bit3]    Row 0
//   [bit1]  [bit4]    Row 1
//   [bit2]  [bit5]    Row 2
//   [bit6]  [bit7]    Row 3

constexpr uint8_t BRAILLE_DOTS[4][2] = {
    {0x01, 0x08},  // Row 0
    {0x02, 0x10},  // Row 1
    {0x04, 0x20},  // Row 2
    {0x40, 0x80}   // Row 3
};
```

### Setting Individual Pixels

```cpp
void set_pixel(int x, int y, bool on = true) {
    if (x < 0 || x >= _pixel_width || y < 0 || y >= _pixel_height)
        return;

    int char_x = x / 2;   // Which character column
    int char_y = y / 4;   // Which character row
    int local_x = x % 2;  // Position within cell (0 or 1)
    int local_y = y % 4;  // Position within cell (0, 1, 2, or 3)

    uint8_t bit = BRAILLE_DOTS[local_y][local_x];

    if (on)
        _canvas[char_y][char_x] |= bit;
    else
        _canvas[char_y][char_x] &= ~bit;
}
```

### Optimized Block Setting

For video rendering, setting 8 pixels one-by-one is slow. Instead:

```cpp
void set_block_gray(int char_x, int char_y, const uint8_t gray[8], uint8_t threshold) {
    // Compute entire 8-bit pattern with single bitwise OR operations
    uint8_t pattern = 0;
    pattern |= (gray[0] >= threshold) ? 0x01 : 0;  // row 0, col 0
    pattern |= (gray[1] >= threshold) ? 0x08 : 0;  // row 0, col 1
    pattern |= (gray[2] >= threshold) ? 0x02 : 0;  // row 1, col 0
    pattern |= (gray[3] >= threshold) ? 0x10 : 0;  // row 1, col 1
    pattern |= (gray[4] >= threshold) ? 0x04 : 0;  // row 2, col 0
    pattern |= (gray[5] >= threshold) ? 0x20 : 0;  // row 2, col 1
    pattern |= (gray[6] >= threshold) ? 0x40 : 0;  // row 3, col 0
    pattern |= (gray[7] >= threshold) ? 0x80 : 0;  // row 3, col 1

    _canvas[char_y][char_x] = pattern;
}
```

This processes an entire character cell in one function call.

### UTF-8 Lookup Table

Converting pattern → UTF-8 every frame is expensive. Pre-compute it:

```cpp
class BrailleLUT {
private:
    std::array<std::string, 256> _lut;  // All 256 patterns

public:
    BrailleLUT() {
        for (int i = 0; i < 256; ++i) {
            char32_t codepoint = 0x2800 + i;

            // Encode as UTF-8 (3 bytes for Braille)
            _lut[i] += static_cast<char>(0xE0 | (codepoint >> 12));
            _lut[i] += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            _lut[i] += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }

    const std::string& get(uint8_t bits) const { return _lut[bits]; }
};

// Global instance (created once at startup)
inline const BrailleLUT& braille_lut() {
    static BrailleLUT lut;
    return lut;
}
```

### Rendering to String

```cpp
std::string render() const {
    std::string out;
    out.reserve(_char_height * _char_width * 3 + _char_height);  // UTF-8 + newlines

    for (size_t y = 0; y < _char_height; ++y) {
        for (size_t x = 0; x < _char_width; ++x) {
            out += braille_lut().get(_canvas[y][x]);
        }
        out += '\n';
    }

    return out;
}
```

---

## Part 3: The ColorCanvas Class

For true-color rendering using half-block characters.

### Why Half-Blocks?

The character `▀` (U+2580, "upper half block") renders as the top half filled. Combined with:

- **Foreground color**: Colors the top half
- **Background color**: Colors the bottom half

You get **2 vertical pixels per character**.

### Structure

```cpp
class ColorCanvas {
private:
    size_t _char_width;
    size_t _char_height;
    size_t _pixel_width;   // = char_width
    size_t _pixel_height;  // = char_height * 2

    std::vector<std::vector<RGB>> _pixels;  // Full RGB per pixel
};
```

### Loading Frame Data

```cpp
void load_frame_rgb(const uint8_t* data, int width, int height) {
    // Resize if needed
    if (width != _pixel_width || height != _pixel_height) {
        _pixel_width = width;
        _pixel_height = height;
        _char_width = width;
        _char_height = (height + 1) / 2;
        _pixels.assign(_pixel_height, std::vector<RGB>(_pixel_width));
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = (y * width + x) * 3;
            _pixels[y][x] = RGB(data[idx], data[idx+1], data[idx+2]);
        }
    }
}
```

### Rendering with ANSI Colors

```cpp
std::string render() const {
    const char* UPPER_HALF = "\xe2\x96\x80";  // ▀ in UTF-8

    std::string out;
    RGB prev_fg(-1, -1, -1), prev_bg(-1, -1, -1);  // Track previous colors

    for (size_t cy = 0; cy < _char_height; ++cy) {
        size_t py_top = cy * 2;
        size_t py_bot = py_top + 1;

        for (size_t cx = 0; cx < _char_width; ++cx) {
            RGB top = _pixels[py_top][cx];
            RGB bot = (py_bot < _pixel_height) ? _pixels[py_bot][cx] : RGB(0, 0, 0);

            // Only emit escape codes when colors change
            if (top != prev_fg) {
                out += ansi::fg_color(top.r, top.g, top.b);
                prev_fg = top;
            }
            if (bot != prev_bg) {
                out += ansi::bg_color(bot.r, bot.g, bot.b);
                prev_bg = bot;
            }

            out += UPPER_HALF;
        }

        out += ansi::RESET;  // Reset at end of line
        out += '\n';
        prev_fg = prev_bg = RGB(-1, -1, -1);  // Force re-emit next line
    }

    return out;
}
```

### ANSI Escape Code Generation

```cpp
namespace ansi {
    inline std::string fg_color(uint8_t r, uint8_t g, uint8_t b) {
        return "\033[38;2;" + std::to_string(r) + ";"
                           + std::to_string(g) + ";"
                           + std::to_string(b) + "m";
    }

    inline std::string bg_color(uint8_t r, uint8_t g, uint8_t b) {
        return "\033[48;2;" + std::to_string(r) + ";"
                           + std::to_string(g) + ";"
                           + std::to_string(b) + "m";
    }

    constexpr const char* RESET = "\033[0m";
    constexpr const char* CURSOR_HOME = "\033[H";
    constexpr const char* HIDE_CURSOR = "\033[?25l";
    constexpr const char* SHOW_CURSOR = "\033[?25h";
}
```

---

## Part 4: The ColoredBrailleCanvas Class

Combines Braille's high resolution with color support.

### The Trade-off

A terminal cell can only have ONE foreground color. So all 8 dots in a Braille character share the same color. The color is computed as the **average** of all "on" pixel colors.

### Structure

```cpp
class ColoredBrailleCanvas {
private:
    size_t _char_width, _char_height;
    size_t _pixel_width, _pixel_height;

    std::vector<std::vector<uint8_t>> _patterns;  // Braille bit patterns
    std::vector<std::vector<RGB>> _colors;        // One color per cell
};
```

### Loading with Threshold

```cpp
void load_frame_rgb(const uint8_t* data, int width, int height, uint8_t threshold = 128) {
    for (size_t cy = 0; cy < _char_height; ++cy) {
        for (size_t cx = 0; cx < _char_width; ++cx) {
            int px = cx * 2;
            int py = cy * 4;

            uint8_t pattern = 0;
            int r_sum = 0, g_sum = 0, b_sum = 0;
            int on_count = 0;

            // Process 2×4 block
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 2; ++col) {
                    int x = px + col, y = py + row;
                    if (x >= width || y >= height) continue;

                    size_t idx = (y * width + x) * 3;
                    uint8_t r = data[idx], g = data[idx+1], b = data[idx+2];

                    // Grayscale for thresholding
                    uint8_t gray = (299*r + 587*g + 114*b) / 1000;

                    if (gray >= threshold) {
                        pattern |= BRAILLE_DOTS[row][col];
                        r_sum += r; g_sum += g; b_sum += b;
                        on_count++;
                    }
                }
            }

            _patterns[cy][cx] = pattern;

            // Average color of "on" pixels
            if (on_count > 0) {
                _colors[cy][cx] = RGB(r_sum/on_count, g_sum/on_count, b_sum/on_count);
            } else {
                _colors[cy][cx] = RGB(0, 0, 0);
            }
        }
    }
}
```

---

## Part 5: The BWBlockCanvas Class

Grayscale rendering using half-blocks and the 256-color ANSI palette.

### Why Not True Color for B&W?

The ANSI 256-color palette includes 24 grayscale shades (colors 232–255). Using this is:

- More compact output (3-digit codes vs 9-digit RGB)
- Compatible with more terminals

### Grayscale Conversion

```cpp
static int gray_to_ansi256(uint8_t gray) {
    // Map 0-255 to ANSI 232-255 (24 levels)
    return 232 + (gray * 23 / 255);
}
```

### Rendering

```cpp
std::string render() const {
    const char* TOP_HALF = "\xe2\x96\x80";  // ▀

    std::string out;
    for (size_t cy = 0; cy < _char_height; ++cy) {
        for (size_t cx = 0; cx < _char_width; ++cx) {
            auto [gray_top, gray_bot] = _pixels[cy][cx];
            int fg = gray_to_ansi256(gray_top);
            int bg = gray_to_ansi256(gray_bot);

            // ANSI 256-color format: ESC[38;5;Nm (fg) ESC[48;5;Nm (bg)
            out += "\033[38;5;" + std::to_string(fg) + ";48;5;" + std::to_string(bg) + "m";
            out += TOP_HALF;
        }
        out += "\033[0m\n";
    }
    return out;
}
```

---

## Part 6: Drawing Primitives

### Bresenham's Line Algorithm

```cpp
void line(int x0, int y0, int x1, int y1) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        set_pixel(x0, y0);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}
```

### Wu's Anti-Aliased Line

For smoother curves:

```cpp
void line_aa(double x0, double y0, double x1, double y1) {
    auto plot = [this](int x, int y, double intensity) {
        if (intensity > 0.3)  // Threshold for visibility
            set_pixel(x, y, true);
    };

    // Wu's algorithm distributes error to create smoother appearance
    // ... (see full implementation in source)
}
```

### Circle Drawing

```cpp
void circle(int cx, int cy, int radius) {
    // Bresenham's circle algorithm
    int x = radius, y = 0;
    int err = 0;

    while (x >= y) {
        // Plot 8 octants symmetrically
        set_pixel(cx + x, cy + y);
        set_pixel(cx + y, cy + x);
        set_pixel(cx - y, cy + x);
        set_pixel(cx - x, cy + y);
        set_pixel(cx - x, cy - y);
        set_pixel(cx - y, cy - x);
        set_pixel(cx + y, cy - x);
        set_pixel(cx + x, cy - y);

        y++;
        err += 1 + 2*y;
        if (2*(err - x) + 1 > 0) {
            x--;
            err += 1 - 2*x;
        }
    }
}
```

---

## Part 7: Floyd-Steinberg Dithering

### The Problem with Simple Thresholding

Simple threshold (pixel ≥ 128 → on) creates harsh edges and loses gradients.

### Dithering Solution

Distribute quantization error to neighboring pixels:

```
        [ * ]  7/16
   3/16  5/16  1/16
```

```cpp
void load_frame_dithered(const uint8_t* data, int width, int height) {
    // Float buffer for error accumulation
    std::vector<std::vector<float>> buffer(height, std::vector<float>(width));
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            buffer[y][x] = static_cast<float>(data[y * width + x]);

    // Floyd-Steinberg dithering
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float old_pixel = buffer[y][x];
            float new_pixel = (old_pixel >= 128.0f) ? 255.0f : 0.0f;
            buffer[y][x] = new_pixel;

            float error = old_pixel - new_pixel;

            // Distribute error
            if (x + 1 < width)
                buffer[y][x + 1] += error * 7.0f / 16.0f;
            if (y + 1 < height) {
                if (x > 0)
                    buffer[y + 1][x - 1] += error * 3.0f / 16.0f;
                buffer[y + 1][x] += error * 5.0f / 16.0f;
                if (x + 1 < width)
                    buffer[y + 1][x + 1] += error * 1.0f / 16.0f;
            }

            if (new_pixel > 0)
                set_pixel(x, y, true);
        }
    }
}
```

---

## Part 8: Image Loading Pipeline

### ImageMagick Integration

```cpp
inline std::string convert_to_ppm(const std::string& filename, int max_width) {
    std::string output = "/tmp/pythonic_convert_" +
                         std::to_string(std::hash<std::string>{}(filename)) + ".ppm";

    std::string cmd = "convert \"" + filename + "\" -resize " +
                      std::to_string(max_width) + "x -depth 8 -colorspace gray \"" +
                      output + "\" 2>/dev/null";

    if (std::system(cmd.c_str()) != 0)
        return "";

    return output;
}
```

### PPM/PGM Parsing

```cpp
bool load_pgm_ppm(const std::string& filename, uint8_t threshold = 128) {
    std::ifstream file(filename, std::ios::binary);

    std::string magic;
    file >> magic;  // "P5" (grayscale) or "P6" (RGB)

    // Skip comments
    char c;
    file.get(c);
    while (file.peek() == '#') {
        std::string comment;
        std::getline(file, comment);
    }

    int width, height, maxval;
    file >> width >> height >> maxval;
    file.get(c);  // Skip whitespace

    // Read pixel data
    if (magic == "P5") {  // Grayscale
        std::vector<uint8_t> data(width * height);
        file.read(reinterpret_cast<char*>(data.data()), data.size());
        load_frame_fast(data.data(), width, height, threshold);
    } else if (magic == "P6") {  // RGB
        std::vector<uint8_t> data(width * height * 3);
        file.read(reinterpret_cast<char*>(data.data()), data.size());
        load_frame_rgb_fast(data.data(), width, height, threshold);
    }

    return true;
}
```

---

## Part 9: Video Playback Pipeline

### FFmpeg Frame Extraction

```cpp
inline bool play_video(const std::string& filename, int width, int threshold, ...) {
    // 1. Get video info (FPS, duration)
    std::string probe_cmd = "ffprobe -v error -select_streams v:0 "
                            "-show_entries stream=r_frame_rate,duration "
                            "-of csv=p=0 \"" + filename + "\"";

    // 2. Open FFmpeg pipe for raw frame data
    std::string ffmpeg_cmd = "ffmpeg -i \"" + filename + "\" "
                             "-vf scale=" + std::to_string(width) + ":-2 "
                             "-pix_fmt gray "  // or rgb24 for color
                             "-f rawvideo - 2>/dev/null";

    FILE* pipe = popen(ffmpeg_cmd.c_str(), "r");

    // 3. Read and render frames
    int frame_size = width * height;  // or *3 for RGB
    std::vector<uint8_t> frame_data(frame_size);

    while (fread(frame_data.data(), 1, frame_size, pipe) == frame_size) {
        canvas.load_frame_fast(frame_data.data(), width, height, threshold);

        std::cout << "\033[H" << canvas.render();  // Cursor home + render

        // Sleep to match framerate
        std::this_thread::sleep_for(frame_duration);
    }

    pclose(pipe);
}
```

### Frame Rate Control

```cpp
auto frame_time = std::chrono::microseconds(static_cast<int>(1000000.0 / fps));
auto start_time = std::chrono::steady_clock::now();

int frame_number = 0;
while (/* reading frames */) {
    // ... render frame ...

    frame_number++;
    auto target_time = start_time + frame_time * frame_number;
    std::this_thread::sleep_until(target_time);
}
```

---

## Part 10: Signal Handling for Clean Exit

### The Problem

If user presses Ctrl+C during video playback:

- Cursor might be hidden
- Terminal colors might be messed up
- Raw mode might still be active

### Solution: Signal Handler

```cpp
namespace signal_handler {
    inline std::atomic<bool>& playback_active() {
        static std::atomic<bool> active{false};
        return active;
    }

    inline void restore_terminal() {
        // Show cursor, reset colors, clear screen
        const char* restore = "\033[?25h\033[0m\033[H\033[J";
        write(STDOUT_FILENO, restore, strlen(restore));
    }

    inline void signal_handler_func(int signum) {
        restore_terminal();
        std::signal(signum, SIG_DFL);  // Restore default handler
        std::raise(signum);            // Re-raise signal
    }

    inline void install() {
        std::signal(SIGINT, signal_handler_func);
        std::signal(SIGTERM, signal_handler_func);
    }
}
```

### RAII Guard

```cpp
class PlaybackGuard {
public:
    PlaybackGuard() {
        signal_handler::install();
        signal_handler::playback_active() = true;
        std::cout << ansi::HIDE_CURSOR;
    }

    ~PlaybackGuard() {
        signal_handler::playback_active() = false;
        std::cout << ansi::SHOW_CURSOR << ansi::RESET;
    }
};

// Usage:
void play_video(...) {
    PlaybackGuard guard;  // Auto-cleanup on exit
    // ... playback loop ...
}
```

---

## Part 11: Optional GPU Acceleration (OpenCL)

### When Available

If `PYTHONIC_ENABLE_OPENCL` is defined and OpenCL is available:

```cpp
class GPURenderer {
private:
    cl::Context _context;
    cl::CommandQueue _queue;
    cl::Kernel _rgb_to_ansi_kernel;
    // ...

public:
    bool process_frame(const uint8_t* rgb_data, size_t width, size_t height,
                       std::vector<uint8_t>& output)
    {
        // Upload to GPU
        _queue.enqueueWriteBuffer(_input_buffer, CL_TRUE, 0, input_size, rgb_data);

        // Run kernel
        _queue.enqueueNDRangeKernel(_rgb_to_ansi_kernel, ...);

        // Read back
        _queue.enqueueReadBuffer(_output_buffer, CL_TRUE, 0, output_size, output.data());

        return true;
    }
};
```

The GPU kernel processes RGB → ANSI color conversion in parallel.

---

## Part 12: High-Level API Functions

### Image Display

```cpp
// Black & white Braille
inline void print_image(const std::string& filename, int max_width = 80, int threshold = 128);

// True color half-blocks
inline void print_image_colored(const std::string& filename, int max_width = 80);

// Grayscale half-blocks
inline void print_image_bw_block(const std::string& filename, int max_width = 80, int threshold = 128);

// Colored Braille
inline void print_image_colored_dot(const std::string& filename, int max_width = 80, int threshold = 128);

// Mode-based (unified API)
inline void print_image_with_mode(const std::string& filename, int max_width = 80,
                                   Mode mode = Mode::colored, int threshold = 128);
```

### Video Playback

```cpp
// Black & white Braille
inline void play_video(const std::string& filename, int width = 80, int threshold = 128, ...);

// True color half-blocks
inline void play_video_colored(const std::string& filename, int width = 80, ...);

// Grayscale half-blocks
inline void play_video_bw_block(const std::string& filename, int width = 80, int threshold = 128, ...);

// Colored Braille
inline void play_video_colored_dot(const std::string& filename, int width = 80, int threshold = 128, ...);

// Mode-based (unified API)
inline void play_video_with_mode(const std::string& filename, int width = 80,
                                  Mode mode = Mode::colored, int threshold = 128, ...);
```

---

## Design Decisions

### Why Four Separate Canvas Classes?

Each mode has fundamentally different:

- Storage (patterns vs RGB vs grayscale)
- Processing (threshold vs average vs direct copy)
- Output format (Braille vs half-block, B&W vs color)

Separate classes keep each implementation clean.

### Why Pipe FFmpeg Instead of Link libavcodec?

1. **Zero dependencies**: FFmpeg is commonly installed
2. **Format support**: FFmpeg handles everything (h264, h265, vp9, etc.)
3. **Simpler code**: No codec negotiation logic

### Why Pre-computed Braille LUT?

Pattern → UTF-8 conversion runs 80×24 = 1920 times per frame. At 30 FPS, that's 57,600 conversions/second. A lookup table makes this nearly free.

### Why Track Previous Colors in Render?

ANSI escape codes are verbose (`\033[38;2;255;0;0m` = 18 bytes). Tracking previous colors and only emitting when changed reduces output size significantly.

---

## Quick Reference

### Canvas Classes

| Class                  | Mode          | Resolution   | Colors   |
| ---------------------- | ------------- | ------------ | -------- |
| `BrailleCanvas`        | `bw_dot`      | 2×4 per char | B&W      |
| `ColorCanvas`          | `colored`     | 1×2 per char | 24-bit   |
| `ColoredBrailleCanvas` | `colored_dot` | 2×4 per char | 24-bit   |
| `BWBlockCanvas`        | `bw`          | 1×2 per char | 24 grays |

### Key Methods

| Method                     | Purpose                     |
| -------------------------- | --------------------------- |
| `set_pixel(x, y)`          | Set single pixel            |
| `set_block_gray(...)`      | Set 8 pixels from grayscale |
| `load_frame_fast(...)`     | Load grayscale frame        |
| `load_frame_rgb_fast(...)` | Load RGB frame              |
| `load_frame_dithered(...)` | Load with dithering         |
| `render()`                 | Convert to ANSI string      |
| `clear()`                  | Reset canvas                |

### ANSI Helpers

```cpp
ansi::fg_color(r, g, b)    // Foreground color
ansi::bg_color(r, g, b)    // Background color
ansi::RESET                // Reset all attributes
ansi::CURSOR_HOME          // Move cursor to (0,0)
ansi::HIDE_CURSOR          // Hide cursor
ansi::SHOW_CURSOR          // Show cursor
```

---

## Example: Complete Workflow

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;
using namespace py::draw;

int main() {
    // 1. Display static image
    print_image_colored("photo.jpg", 80);

    // 2. Play video
    play_video_with_mode("video.mp4", 80, Mode::colored_dot);

    // 3. Manual canvas drawing
    BrailleCanvas canvas(80, 24);
    canvas.line(0, 0, 159, 95);
    canvas.circle(80, 48, 30);
    std::cout << canvas.render();

    return 0;
}
```
