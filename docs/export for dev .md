# pythonicExport — Rendering ASCII/Braille Art to Images and Video

## What This Module Does

`pythonicExport.hpp` bridges the gap between terminal output and real image/video files. When you render a Braille graph or colored ASCII art to the terminal, you're outputting strings with Unicode characters and ANSI escape codes. This module takes that same output and converts it into actual PNG/PPM images and MP4 videos.

**Core problem it solves**: You want to save your terminal graphics as shareable image files or videos, not just ephemeral terminal output.

---

## The Architecture: How It All Fits Together

```
┌────────────────────────────────────────────────────────────────┐
│                     pythonicExport.hpp                          │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  Input: ASCII/Braille string with ANSI colors                  │
│         "⣿⣿⣿\033[38;2;255;0;0m⣿⣿\033[0m"                      │
│                                                                │
│         ┌─────────────────┐                                    │
│         │   parse_line()  │  Extract codepoints + colors       │
│         └────────┬────────┘                                    │
│                  │                                             │
│                  ▼                                             │
│         ┌─────────────────┐                                    │
│         │  ParsedChar[]   │  {codepoint, fg_color, bg_color}   │
│         └────────┬────────┘                                    │
│                  │                                             │
│                  ▼                                             │
│         ┌─────────────────┐                                    │
│         │ render_art_to_  │  Convert to pixel buffer           │
│         │     image()     │                                    │
│         └────────┬────────┘                                    │
│                  │                                             │
│                  ▼                                             │
│         ┌─────────────────┐                                    │
│         │  ImageBuffer    │  Raw RGBA pixels                   │
│         └────────┬────────┘                                    │
│                  │                                             │
│         ┌───────┴───────┐                                      │
│         ▼               ▼                                      │
│  ┌────────────┐  ┌────────────┐                               │
│  │ write_ppm()│  │ write_png()│                               │
│  │   (raw)    │  │(ImageMagick)                               │
│  └────────────┘  └────────────┘                               │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

---

## Part 1: Understanding Braille Encoding

### The Braille Unicode Block

Unicode Braille characters occupy U+2800 to U+28FF (256 codepoints). Each character represents a 2×4 grid of dots:

```
  Col 0   Col 1
  [bit0]  [bit3]    Row 0
  [bit1]  [bit4]    Row 1
  [bit2]  [bit5]    Row 2
  [bit6]  [bit7]    Row 3
```

The codepoint is `0x2800 + bit_pattern`. For example:
- `⠀` (U+2800) = no dots = pattern 0x00
- `⣿` (U+28FF) = all dots = pattern 0xFF
- `⠁` (U+2801) = top-left dot = pattern 0x01
- `⠈` (U+2808) = top-right dot = pattern 0x08

### Extracting the Pattern

```cpp
// If codepoint is 0x2847 (⡇), then:
uint8_t pattern = static_cast<uint8_t>(codepoint - BRAILLE_BASE);  // = 0x47
// Now pattern tells us which dots are "on"
```

### Dot Position Lookup

```cpp
// BRAILLE_DOT_X[bit] = which column (0 or 1)
// BRAILLE_DOT_Y[bit] = which row (0, 1, 2, or 3)
constexpr int BRAILLE_DOT_X[8] = {0, 0, 0, 1, 1, 1, 0, 1};
constexpr int BRAILLE_DOT_Y[8] = {0, 1, 2, 0, 1, 2, 3, 3};
```

So bit 0 is at (0, 0), bit 3 is at (1, 0), bit 6 is at (0, 3), etc.

---

## Part 2: Parsing ANSI-Colored Strings

### The Problem

Terminal output looks like this:
```
\033[38;2;255;0;0m⣿⣿⣿\033[0m⠀⣿
```

We need to:
1. Parse the escape codes to extract RGB colors
2. Associate colors with the characters that follow
3. Handle resets (`\033[0m`)

### The ParsedChar Structure

```cpp
struct ParsedChar {
    char32_t codepoint;    // The Unicode character
    RGB fg_color;          // Foreground color (default white)
    RGB bg_color;          // Background color (default black)
    bool has_fg_color;     // Was color explicitly set?
    bool has_bg_color;
};
```

### Parsing Logic

```cpp
inline std::vector<ParsedChar> parse_line(const std::string& line) {
    std::vector<ParsedChar> result;
    RGB current_fg(255, 255, 255);  // Default white
    RGB current_bg(0, 0, 0);        // Default black
    bool has_fg = false, has_bg = false;
    
    size_t i = 0;
    while (i < line.size()) {
        // Check for ANSI escape sequence
        if (line[i] == '\033' && line[i+1] == '[') {
            // Find the 'm' that ends the sequence
            size_t start = i;
            i += 2;
            while (i < line.size() && line[i] != 'm') i++;
            i++;  // Skip 'm'
            
            std::string escape = line.substr(start, i - start);
            
            // Parse RGB color: ESC[38;2;R;G;Bm (foreground)
            //                  ESC[48;2;R;G;Bm (background)
            uint8_t r, g, b;
            if (parse_ansi_rgb(escape, r, g, b)) {
                if (escape.find("[38;2;") != std::string::npos) {
                    current_fg = RGB(r, g, b);
                    has_fg = true;
                } else if (escape.find("[48;2;") != std::string::npos) {
                    current_bg = RGB(r, g, b);
                    has_bg = true;
                }
            }
            // Check for reset code
            else if (escape.find("[0m") != std::string::npos) {
                current_fg = RGB(255, 255, 255);
                current_bg = RGB(0, 0, 0);
                has_fg = has_bg = false;
            }
            continue;
        }
        
        // Decode UTF-8 character
        char32_t cp = decode_utf8(line, i);  // i is advanced
        
        ParsedChar pc;
        pc.codepoint = cp;
        pc.fg_color = current_fg;
        pc.bg_color = current_bg;
        pc.has_fg_color = has_fg;
        pc.has_bg_color = has_bg;
        result.push_back(pc);
    }
    
    return result;
}
```

### UTF-8 Decoding

Braille characters are 3-byte UTF-8 sequences:

```cpp
inline char32_t decode_utf8(const std::string& str, size_t& pos) {
    unsigned char c = str[pos];
    
    // ASCII (1 byte)
    if ((c & 0x80) == 0) {
        pos++;
        return c;
    }
    
    // 2-byte UTF-8
    if ((c & 0xE0) == 0xC0) {
        char32_t cp = (c & 0x1F) << 6;
        cp |= (str[pos+1] & 0x3F);
        pos += 2;
        return cp;
    }
    
    // 3-byte UTF-8 (Braille lives here: U+2800–U+28FF)
    if ((c & 0xF0) == 0xE0) {
        char32_t cp = (c & 0x0F) << 12;
        cp |= (str[pos+1] & 0x3F) << 6;
        cp |= (str[pos+2] & 0x3F);
        pos += 3;
        return cp;
    }
    
    // 4-byte UTF-8 (emoji, rare symbols)
    // ... similar pattern
}
```

---

## Part 3: The ImageBuffer Class

### Structure

```cpp
class ImageBuffer {
public:
    int width, height;
    std::vector<RGBA> pixels;  // Row-major storage
    
    ImageBuffer(int w, int h, RGBA fill = RGBA(0, 0, 0, 255))
        : width(w), height(h), pixels(w * h, fill) {}
    
    RGBA& at(int x, int y) {
        return pixels[y * width + x];
    }
    
    void set_pixel(int x, int y, const RGBA& color) {
        if (x >= 0 && x < width && y >= 0 && y < height)
            pixels[y * width + x] = color;
    }
};
```

### Drawing Primitives

The ImageBuffer provides basic drawing operations:

```cpp
// Filled circle (for Braille dots)
void fill_circle(int cx, int cy, int radius, const RGBA& color) {
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= r2)
                set_pixel(cx + dx, cy + dy, color);
        }
    }
}

// Filled rectangle (for block characters)
void fill_rect(int x1, int y1, int x2, int y2, const RGBA& color) {
    for (int y = y1; y <= y2; y++)
        for (int x = x1; x <= x2; x++)
            set_pixel(x, y, color);
}
```

---

## Part 4: Rendering Braille to Pixels

### The Core Algorithm

```cpp
inline void render_braille_char(ImageBuffer& img, int x, int y,
                                 uint8_t pattern, const RGB& fg, const RGB& bg,
                                 int dot_radius, int cell_width, int cell_height)
{
    // 1. Fill background
    RGBA bg_color(bg.r, bg.g, bg.b, 255);
    img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1, bg_color);
    
    // 2. Calculate dot spacing within the cell
    int dot_spacing_x = cell_width / 2;
    int dot_spacing_y = cell_height / 4;
    
    // 3. Render each active dot
    RGBA dot_color(fg.r, fg.g, fg.b, 255);
    for (int bit = 0; bit < 8; bit++) {
        if (pattern & (1 << bit)) {
            // Determine dot position
            int dot_x = BRAILLE_DOT_X[bit];
            int dot_y = BRAILLE_DOT_Y[bit];
            
            // Calculate pixel center
            int px = x + dot_spacing_x/2 + dot_x * dot_spacing_x;
            int py = y + dot_spacing_y/2 + dot_y * dot_spacing_y;
            
            // Draw the dot
            img.fill_circle(px, py, dot_radius, dot_color);
        }
    }
}
```

### Block Character Rendering

For half-block characters (▀, ▄, █):

```cpp
inline void render_block_char(ImageBuffer& img, int x, int y, char32_t cp,
                               const RGB& fg, const RGB& bg,
                               int cell_width, int cell_height)
{
    RGBA fg_color(fg.r, fg.g, fg.b, 255);
    RGBA bg_color(bg.r, bg.g, bg.b, 255);
    
    // Fill background
    img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1, bg_color);
    
    int half = cell_height / 2;
    
    switch (cp) {
        case 0x2588:  // █ Full block
            img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1, fg_color);
            break;
        case 0x2580:  // ▀ Upper half
            img.fill_rect(x, y, x + cell_width - 1, y + half - 1, fg_color);
            break;
        case 0x2584:  // ▄ Lower half
            img.fill_rect(x, y + half, x + cell_width - 1, y + cell_height - 1, fg_color);
            break;
        case 0x258C:  // ▌ Left half
            img.fill_rect(x, y, x + cell_width/2 - 1, y + cell_height - 1, fg_color);
            break;
        case 0x2590:  // ▐ Right half
            img.fill_rect(x + cell_width/2, y, x + cell_width - 1, y + cell_height - 1, fg_color);
            break;
        case 0x2591:  // ░ Light shade
        case 0x2592:  // ▒ Medium shade
        case 0x2593:  // ▓ Dark shade
            // Blend colors based on shade level
            break;
    }
}
```

---

## Part 5: The ExportConfig Structure

### Configuration Options

```cpp
struct ExportConfig {
    int dot_size = 2;                     // Dot radius in pixels
    int dot_density = 3;                  // Spacing multiplier
    RGB bg_color = RGB(0, 0, 0);          // Background (default black)
    RGB default_fg = RGB(255, 255, 255);  // Default foreground (white)
    bool preserve_colors = true;          // Use ANSI colors if present
};
```

### Cell Size Calculation

The cell dimensions depend on content type and config:

```cpp
// For Braille: 2 dots wide × 4 dots tall
cell_width = config.dot_size * config.dot_density * 2;
cell_height = config.dot_size * config.dot_density * 4;

// For block characters: 1 char wide × 2 pixels tall
cell_width = config.dot_size * config.dot_density;
cell_height = config.dot_size * config.dot_density * 2;
```

With default values (`dot_size=2`, `dot_density=3`):
- Braille cell: 12×24 pixels
- Block cell: 6×12 pixels

### Builder Pattern

ExportConfig uses a fluent builder pattern:

```cpp
ExportConfig config;
config.set_dot_size(3)
      .set_density(4)
      .set_background(30, 30, 40)
      .set_foreground(200, 200, 255)
      .set_preserve_colors(true);
```

---

## Part 6: The Main Rendering Pipeline

### `render_art_to_image()`

```cpp
inline ImageBuffer render_art_to_image(const std::string& content,
                                        const ExportConfig& config)
{
    // 1. Split into lines
    std::vector<std::string> lines;
    std::istringstream ss(content);
    std::string line;
    while (std::getline(ss, line))
        lines.push_back(line);
    
    // 2. Parse all lines to determine dimensions
    std::vector<std::vector<ParsedChar>> parsed_lines;
    size_t max_chars = 0;
    
    for (const auto& l : lines) {
        auto parsed = parse_line(l);
        max_chars = std::max(max_chars, parsed.size());
        parsed_lines.push_back(parsed);
    }
    
    // 3. Detect content type (braille vs blocks)
    bool has_braille = false, has_blocks = false;
    for (const auto& pl : parsed_lines) {
        for (const auto& pc : pl) {
            if (is_braille(pc.codepoint)) has_braille = true;
            if (is_block_char(pc.codepoint)) has_blocks = true;
        }
    }
    
    // 4. Calculate cell sizes
    int cell_width, cell_height;
    if (has_braille) {
        cell_width = config.dot_size * config.dot_density * 2;
        cell_height = config.dot_size * config.dot_density * 4;
    } else if (has_blocks) {
        cell_width = config.dot_size * config.dot_density;
        cell_height = config.dot_size * config.dot_density * 2;
    }
    
    // 5. Create image buffer
    int img_width = max_chars * cell_width;
    int img_height = parsed_lines.size() * cell_height;
    ImageBuffer img(img_width, img_height,
                    RGBA(config.bg_color.r, config.bg_color.g, config.bg_color.b, 255));
    
    // 6. Render each character
    for (size_t row = 0; row < parsed_lines.size(); row++) {
        for (size_t col = 0; col < parsed_lines[row].size(); col++) {
            const ParsedChar& pc = parsed_lines[row][col];
            int x = col * cell_width;
            int y = row * cell_height;
            
            // Determine colors
            RGB fg = config.preserve_colors && pc.has_fg_color
                   ? pc.fg_color : config.default_fg;
            RGB bg = config.preserve_colors && pc.has_bg_color
                   ? pc.bg_color : config.bg_color;
            
            if (is_braille(pc.codepoint)) {
                uint8_t pattern = pc.codepoint - BRAILLE_BASE;
                render_braille_char(img, x, y, pattern, fg, bg,
                                    config.dot_size, cell_width, cell_height);
            } else if (is_block_char(pc.codepoint)) {
                render_block_char(img, x, y, pc.codepoint, fg, bg,
                                  cell_width, cell_height);
            } else if (pc.codepoint == ' ') {
                // Space: just background
                img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1,
                              RGBA(bg.r, bg.g, bg.b, 255));
            } else {
                // Unknown character: render as solid block
                img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1,
                              RGBA(fg.r, fg.g, fg.b, 255));
            }
        }
    }
    
    return img;
}
```

---

## Part 7: File Output

### PPM Format (Native)

PPM is a dead-simple image format that requires no external libraries:

```cpp
inline bool write_ppm(const ImageBuffer& img, const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;
    
    // PPM header
    out << "P6\n"
        << img.width << " " << img.height << "\n"
        << "255\n";
    
    // Pixel data (RGB, no alpha)
    for (const auto& pixel : img.pixels) {
        out.put(static_cast<char>(pixel.r));
        out.put(static_cast<char>(pixel.g));
        out.put(static_cast<char>(pixel.b));
    }
    
    return out.good();
}
```

### PNG Format (via ImageMagick)

```cpp
inline bool write_png(const ImageBuffer& img, const std::string& filename) {
    // 1. Write temporary PPM
    std::string temp_ppm = "/tmp/pythonic_export_" +
                           std::to_string(std::hash<std::string>{}(filename)) + ".ppm";
    
    if (!write_ppm(img, temp_ppm))
        return false;
    
    // 2. Convert to PNG using ImageMagick
    std::string cmd = "convert \"" + temp_ppm + "\" \"" + filename + "\" 2>/dev/null";
    int result = std::system(cmd.c_str());
    
    // 3. Cleanup
    std::remove(temp_ppm.c_str());
    
    return (result == 0);
}
```

**Note**: PNG export requires ImageMagick to be installed.

---

## Part 8: Video Export

### Frame-by-Frame Rendering

```cpp
inline bool export_frames_to_video(const std::vector<std::string>& frames,
                                    const std::string& output_path,
                                    int fps, const ExportConfig& config,
                                    const std::string& audio_path = "")
{
    // 1. Create temp directory
    std::string temp_dir = "/tmp/pythonic_video_export_" +
                           std::to_string(std::hash<std::string>{}(output_path));
    std::system(("mkdir -p \"" + temp_dir + "\"").c_str());
    
    // 2. Render each frame to PPM
    int frame_num = 1;
    for (const auto& frame : frames) {
        ImageBuffer img = render_art_to_image(frame, config);
        
        char frame_path[256];
        snprintf(frame_path, sizeof(frame_path), "%s/frame_%05d.ppm",
                 temp_dir.c_str(), frame_num);
        
        write_ppm(img, frame_path);
        frame_num++;
    }
    
    // 3. Combine with FFmpeg
    std::string cmd;
    if (!audio_path.empty()) {
        cmd = "ffmpeg -y -framerate " + std::to_string(fps) +
              " -i \"" + temp_dir + "/frame_%05d.ppm\" " +
              "-i \"" + audio_path + "\" " +
              "-c:v libx264 -c:a aac -pix_fmt yuv420p -shortest \"" +
              output_path + "\" 2>/dev/null";
    } else {
        cmd = "ffmpeg -y -framerate " + std::to_string(fps) +
              " -i \"" + temp_dir + "/frame_%05d.ppm\" " +
              "-c:v libx264 -pix_fmt yuv420p \"" +
              output_path + "\" 2>/dev/null";
    }
    
    int result = std::system(cmd.c_str());
    
    // 4. Cleanup temp files
    std::system(("rm -rf \"" + temp_dir + "\"").c_str());
    
    return (result == 0);
}
```

---

## Part 9: The Export API Functions

### Single Image Export

```cpp
// Full config version
inline bool export_art_to_png(const std::string& content,
                               const std::string& filename,
                               const ExportConfig& config)
{
    ImageBuffer img = render_art_to_image(content, config);
    if (img.width == 0 || img.height == 0) return false;
    return write_png(img, filename);
}

// Simple version
inline bool export_art_to_png(const std::string& content,
                               const std::string& filename,
                               int dot_size,
                               RGB bg_color = RGB(0, 0, 0))
{
    ExportConfig config;
    config.dot_size = dot_size;
    config.bg_color = bg_color;
    return export_art_to_png(content, filename, config);
}
```

### PPM Export (no external dependencies)

```cpp
inline bool export_art_to_ppm(const std::string& content,
                               const std::string& filename,
                               const ExportConfig& config)
{
    ImageBuffer img = render_art_to_image(content, config);
    return write_ppm(img, filename);
}
```

---

## Design Decisions

### Why PPM as intermediate format?

1. **No dependencies**: PPM is trivial to write — just header + raw pixels
2. **FFmpeg and ImageMagick both read it**: Universal intermediate format
3. **Fast**: No compression overhead during rendering

### Why delegate to ImageMagick/FFmpeg?

1. **PNG compression is complex**: libpng is a heavy dependency
2. **Video encoding is complex**: H.264 encoding is best left to FFmpeg
3. **Flexibility**: Users can install what they need

### Why separate `dot_size` and `dot_density`?

- `dot_size`: Controls the actual rendered dot radius
- `dot_density`: Controls spacing between dots

This allows independent control over "how fat are dots" vs "how packed are dots".

---

## Quick Reference

### Types

| Type | Purpose |
|------|---------|
| `RGB` | 8-bit RGB color |
| `RGBA` | 8-bit RGBA color |
| `ParsedChar` | Parsed character with colors |
| `ImageBuffer` | Raw pixel buffer |
| `ExportConfig` | Rendering configuration |

### Functions

| Function | Purpose |
|----------|---------|
| `parse_line(str)` | Parse ANSI-colored string |
| `render_art_to_image(str, config)` | Convert to ImageBuffer |
| `write_ppm(img, path)` | Write PPM file |
| `write_png(img, path)` | Write PNG via ImageMagick |
| `export_art_to_png(str, path, config)` | Full PNG export |
| `export_art_to_ppm(str, path, config)` | Full PPM export |
| `export_frames_to_video(frames, path, fps, config)` | Video export |

### ExportConfig Builder

```cpp
ExportConfig config;
config.set_dot_size(3)         // Dot radius
      .set_density(4)          // Spacing multiplier
      .set_background(r, g, b) // Background color
      .set_foreground(r, g, b) // Default foreground
      .set_preserve_colors(true);  // Use ANSI colors
```

---

## Example: Complete Workflow

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;
using namespace py::ex;

int main() {
    // 1. Create some ASCII art with colors
    std::string art = "\033[38;2;255;0;0m⣿⣿⣿\033[0m "
                      "\033[38;2;0;255;0m⣿⣿⣿\033[0m "
                      "\033[38;2;0;0;255m⣿⣿⣿\033[0m";
    
    // 2. Configure export
    ExportConfig config;
    config.set_dot_size(4)
          .set_density(3)
          .set_background(20, 20, 30);
    
    // 3. Export to PNG
    if (export_art_to_png(art, "output.png", config)) {
        std::cout << "Exported to output.png" << std::endl;
    }
    
    // 4. For video: collect frames
    std::vector<std::string> frames;
    for (int i = 0; i < 60; i++) {
        std::string frame = generate_frame(i);  // Your frame generator
        frames.push_back(frame);
    }
    
    // 5. Export to video
    if (export_frames_to_video(frames, "output.mp4", 30, config)) {
        std::cout << "Exported to output.mp4" << std::endl;
    }
    
    return 0;
}
```

---

## Dependencies

| Feature | Dependency |
|---------|------------|
| PPM export | None |
| PNG export | ImageMagick (`convert` command) |
| Video export | FFmpeg (`ffmpeg` command) |

Install on Ubuntu/Debian:
```bash
sudo apt install imagemagick ffmpeg
```
