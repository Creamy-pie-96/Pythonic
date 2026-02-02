[⬅ Back to Table of Contents](../index.md)

# 🖌️ LiveDraw - Interactive Terminal Drawing

**pythonicLiveDraw.hpp** provides an interactive drawing canvas in your terminal with full mouse support. You can paint, draw shapes, and save your creations to Pythonic's `.pi` image format.

---

## 🚀 Quick Start

```cpp
#include "pythonic/pythonicLiveDraw.hpp"
using namespace pythonic::draw;

int main() {
    // Launch interactive drawing mode
    live_draw();
    return 0;
}
```

Compile and run:

```bash
g++ -std=c++20 -Iinclude -o draw my_draw.cpp
./draw
```

---

## 🎮 Keyboard Controls

| Key       | Action                               |
| --------- | ------------------------------------ |
| `p`       | **Pen tool** - Freehand drawing      |
| `l`       | **Line tool** - Draw straight lines  |
| `c`       | **Circle tool** - Draw circles       |
| `x`       | **Rectangle tool** - Draw rectangles |
| `f`       | **Fill tool** - Flood fill an area   |
| `e`       | **Eraser** - Erase to background     |
| `r`       | Select **R**ed channel               |
| `g`       | Select **G**reen channel             |
| `b`       | Select **B**lue channel              |
| `a`       | Select **A**lpha channel             |
| `0-9`     | Set channel value (0-255)            |
| `+` / `-` | Increase/decrease brush size         |
| `u`       | **Undo** last action                 |
| `y`       | **Redo** undone action               |
| `s`       | **Save** to file                     |
| `q`       | **Quit**                             |

---

## 🖼️ Understanding the Module

Let's walk through the key components of this module:

### 1. The RGBA Color Structure

```cpp
struct RGBA
{
    uint8_t r, g, b, a;

    RGBA() : r(255), g(255), b(255), a(255) {}
    RGBA(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    RGBA blend_over(const RGBA &dst) const
    {
        if (a == 255) return *this;
        if (a == 0) return dst;

        float src_a = a / 255.0f;
        float dst_a = dst.a / 255.0f;
        float out_a = src_a + dst_a * (1.0f - src_a);

        // Porter-Duff "over" compositing
        float out_r = (r * src_a + dst.r * dst_a * (1.0f - src_a)) / out_a;
        float out_g = (g * src_a + dst.g * dst_a * (1.0f - src_a)) / out_a;
        float out_b = (b * src_a + dst.b * dst_a * (1.0f - src_a)) / out_a;

        return RGBA(out_r, out_g, out_b, out_a * 255.0f);
    }
};
```

**What this does:**

- Stores red, green, blue, and alpha (transparency) values
- Default color is opaque white (255, 255, 255, 255)
- The `blend_over()` method combines two colors using alpha compositing

**Visual example of alpha blending:**

```
Semi-transparent red (255, 0, 0, 128) over blue (0, 0, 255, 255)
Result: Purple-ish color where both contribute
```

---

### 2. Drawing Tools Enum

```cpp
enum class Tool
{
    pen,       // Freehand drawing
    line,      // Line from point A to B
    circle,    // Circle (center + radius)
    rectangle, // Rectangle
    fill,      // Flood fill
    eraser     // Eraser (sets to background)
};
```

Each tool behaves differently when you click and drag:

| Tool      | Click           | Drag             | Release              |
| --------- | --------------- | ---------------- | -------------------- |
| Pen       | Start drawing   | Draw along path  | Stop                 |
| Line      | Set start point | Preview line     | Draw final line      |
| Circle    | Set center      | Preview radius   | Draw final circle    |
| Rectangle | Set corner      | Preview size     | Draw final rectangle |
| Fill      | Fill area       | (no effect)      | (no effect)          |
| Eraser    | Start erasing   | Erase along path | Stop                 |

---

### 3. Mouse Event Handling

```cpp
struct MouseEvent
{
    MouseEventType type;    // move, press, release, scroll
    MouseButton button;     // left, middle, right, none
    int cell_x, cell_y;     // Terminal cell position
    int sub_x, sub_y;       // Sub-pixel position (for Braille)
    int pixel_x, pixel_y;   // Actual pixel coordinates
    bool shift_held;
    bool ctrl_held;
};
```

**How mouse tracking works:**

The terminal sends ANSI escape sequences when you move/click:

```
ESC [ < Cb ; Cx ; Cy M    (for press)
ESC [ < Cb ; Cx ; Cy m    (for release)
```

Where:

- `Cb` = button code (with modifier bits)
- `Cx`, `Cy` = 1-based cell coordinates

---

### 4. The LiveCanvas Class

```cpp
class LiveCanvas
{
private:
    size_t _char_width;    // Width in terminal characters
    size_t _char_height;   // Height in terminal characters
    size_t _pixel_width;   // Actual pixel width
    size_t _pixel_height;  // Actual pixel height

    std::vector<std::vector<RGBA>> _pixels;  // The drawing buffer

    Tool _current_tool;
    RGBA _foreground;      // Current drawing color
    RGBA _background;      // Background/eraser color
    uint8_t _brush_size;   // Brush radius

    std::stack<CanvasState> _undo_stack;  // For undo
    std::stack<CanvasState> _redo_stack;  // For redo
```

**Resolution explained:**

```
Terminal: 80 columns × 24 rows
Pixel buffer: 80 × 48 pixels (half-block mode: 2 pixels per row)

Each character cell = 1 pixel wide × 2 pixels tall
┌─┐  ← 1 character cell
│▀│  ← top pixel (foreground color)
│ │  ← bottom pixel (background color)
└─┘
```

---

### 5. Terminal Setup

```cpp
void enable_raw_mode()
{
#ifndef _WIN32
    struct termios new_termios = _old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_termios.c_iflag &= ~(IXON | ICRNL);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 1;  // 100ms timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
#endif
}
```

**What each flag does:**

| Flag     | Effect when disabled                                   |
| -------- | ------------------------------------------------------ |
| `ICANON` | Don't wait for Enter - read each keystroke immediately |
| `ECHO`   | Don't echo typed characters                            |
| `ISIG`   | Don't generate signals on Ctrl+C/Z                     |
| `IXON`   | Don't interpret Ctrl+S/Q as flow control               |
| `ICRNL`  | Don't convert carriage return to newline               |

---

### 6. Mouse Tracking Setup

```cpp
void enable_mouse_tracking()
{
    std::cout << "\033[?1000h"  // Basic mouse reporting
              << "\033[?1002h"  // Button-event tracking
              << "\033[?1003h"  // Any-event tracking (movement)
              << "\033[?1006h"  // SGR extended coordinates
              << std::flush;
}
```

**Mouse tracking modes:**

```
Mode 1000: Report button press/release only
Mode 1002: Report button events while dragging
Mode 1003: Report ALL mouse movement (even without buttons)
Mode 1006: Use SGR format for coordinates (supports large terminals)
```

---

### 7. Drawing Algorithms

#### Bresenham's Line Algorithm

```cpp
void draw_line(int x0, int y0, int x1, int y1, const RGBA &color)
{
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;  // Step direction
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        draw_brush(x0, y0, color);  // Draw at current point

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}
```

**Visual explanation:**

```
Drawing from (0,0) to (8,4):

    0 1 2 3 4 5 6 7 8
  0 █ █
  1     █ █
  2         █ █
  3             █ █
  4                 █
```

The algorithm chooses the best integer pixel approximation for each step.

---

### 8. Midpoint Circle Algorithm

```cpp
void draw_circle(int cx, int cy, int r, const RGBA &color)
{
    int x = 0;
    int y = r;
    int d = 1 - r;  // Decision variable

    while (x <= y) {
        // Draw 8 symmetric points
        draw_brush(cx + x, cy + y, color);
        draw_brush(cx - x, cy + y, color);
        draw_brush(cx + x, cy - y, color);
        draw_brush(cx - x, cy - y, color);
        draw_brush(cx + y, cy + x, color);
        draw_brush(cx - y, cy + x, color);
        draw_brush(cx + y, cy - x, color);
        draw_brush(cx - y, cy - x, color);

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}
```

**Circle symmetry (8-fold):**

```
       (-x,y)  (x,y)
    (-y,x)        (y,x)
         ┌──────┐
         │  ●   │  ← center (cx, cy)
         └──────┘
    (-y,-x)       (y,-x)
       (-x,-y) (x,-y)
```

We only calculate 1/8 of the circle and mirror it 8 ways!

---

### 9. Flood Fill Algorithm

```cpp
void flood_fill(int x, int y, const RGBA &fill_color)
{
    RGBA target_color = get_pixel(x, y);
    if (target_color == fill_color) return;  // Already filled

    std::stack<std::pair<int, int>> stack;
    stack.push({x, y});

    while (!stack.empty()) {
        auto [px, py] = stack.top();
        stack.pop();

        if (get_pixel(px, py) != target_color) continue;

        set_pixel_direct(px, py, fill_color);

        // Add neighbors
        if (px > 0) stack.push({px - 1, py});
        if (px < _pixel_width - 1) stack.push({px + 1, py});
        if (py > 0) stack.push({px, py - 1});
        if (py < _pixel_height - 1) stack.push({px, py + 1});
    }
}
```

**Flood fill visualization:**

```
Before:          After (fill with ▓):
┌─────────┐     ┌─────────┐
│ ███████ │     │ ███████ │
│ █     █ │ →   │ █▓▓▓▓▓█ │
│ █  ●  █ │     │ █▓▓●▓▓█ │  (● = click point)
│ █     █ │     │ █▓▓▓▓▓█ │
│ ███████ │     │ ███████ │
└─────────┘     └─────────┘
```

---

### 10. Undo/Redo System

```cpp
void save_state_for_undo()
{
    _undo_stack.push(CanvasState(_pixels));
    _redo_stack = std::stack<CanvasState>();  // Clear redo

    // Limit undo history
    while (_undo_stack.size() > MAX_UNDO) {
        // Remove oldest...
    }
}

void undo()
{
    if (_undo_stack.empty()) return;
    _redo_stack.push(CanvasState(_pixels));  // Save current for redo
    _pixels = _undo_stack.top().pixels;       // Restore previous
    _undo_stack.pop();
}
```

**State diagram:**

```
Action → Save current → Push to undo stack → Clear redo stack
Undo   → Save current to redo → Pop from undo → Restore
Redo   → Save current to undo → Pop from redo → Restore
```

---

## 💾 Saving Your Drawing

When you press `s`, your drawing is saved to `.pi` format:

```cpp
void save(const std::string &filename)
{
    // Convert RGBA pixels to RGB for saving
    std::vector<uint8_t> rgb_data;
    for (auto &row : _pixels) {
        for (auto &pixel : row) {
            rgb_data.push_back(pixel.r);
            rgb_data.push_back(pixel.g);
            rgb_data.push_back(pixel.b);
        }
    }

    // Save using pythonicMedia's RLE compression
    pythonic::media::save_pi(filename, rgb_data, _pixel_width, _pixel_height);
}
```

The `.pi` format uses:

- Run-Length Encoding (RLE) compression
- XOR encryption for obfuscation
- Header with original dimensions

---

## 🎨 Advanced Usage

### Custom Canvas Size

```cpp
LiveCanvas canvas(100, 50, "my_art.pi");  // 100×50 chars, auto-save file
canvas.run();
```

### Starting with a Color

```cpp
LiveCanvas canvas(80, 40);
canvas.set_foreground(RGBA(255, 0, 128, 255));  // Hot pink
canvas.run();
```

---

## 🔧 Terminal Compatibility

Works on:

- ✅ Linux terminals (gnome-terminal, konsole, xterm)
- ✅ macOS Terminal, iTerm2
- ✅ Windows Terminal
- ✅ VS Code integrated terminal
- ⚠️ Basic Windows cmd.exe (limited mouse support)

---

## 📚 Next Steps

- [pythonicDraw.hpp Tutorial](pythonicDraw_tutorial.md) - Learn the underlying drawing primitives
- [pythonicMedia.hpp Tutorial](../Media/pythonicMedia_tutorial.md) - Understand the file format
- [Plot Tutorial](../Plot/plot.md) - Create data visualizations
