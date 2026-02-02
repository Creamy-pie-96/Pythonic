[⬅ Back to Table of Contents](../index.md)

# 📊 Plot - matplotlib-style Terminal Plotting

**pythonicPlot.hpp** provides a matplotlib-inspired plotting library that renders beautiful graphs directly in your terminal using Braille Unicode characters for 8× resolution.

---

## 🚀 Quick Start

```cpp
#include "pythonic/pythonicPlot.hpp"
using namespace pythonic::plot;

int main() {
    Figure fig(80, 24);  // 80 chars wide, 24 chars tall

    // Plot a sine wave
    fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");

    std::cout << fig.render();
    return 0;
}
```

Compile:

```bash
g++ -std=c++20 -Iinclude -o plot my_plot.cpp
./plot
```

---

## 📖 Understanding the Module

### 1. The Figure Class

```cpp
class Figure
{
private:
    size_t _char_width;    // Terminal width in characters
    size_t _char_height;   // Terminal height in characters
    size_t _pixel_width;   // char_width × 2 (Braille has 2 dots wide)
    size_t _pixel_height;  // char_height × 4 (Braille has 4 dots tall)

    std::vector<std::vector<RGBA>> _pixels;  // High-res pixel buffer
    std::vector<PlotData> _plots;             // All plotted data

    Range _x_range;   // X axis min/max
    Range _y_range;   // Y axis min/max
```

**Resolution math:**

```
Terminal: 80 × 24 characters
Braille:  Each char = 2 × 4 dots

Pixel buffer: 80×2 = 160 pixels wide
              24×4 = 96 pixels tall

That's 15,360 "pixels" to work with!
```

**Visual:**

```
One Braille character cell:
┌───┐
│⠿⠿│  ← 2 columns of dots
│⠿⠿│
│⠿⠿│
│⠿⠿│  ← 4 rows of dots
└───┘
```

---

### 2. The Range Structure

```cpp
struct Range
{
    double min, max;

    double span() const { return max - min; }
    double center() const { return (min + max) / 2.0; }

    void include(double value)  // Expand to include a value
    {
        min = std::min(min, value);
        max = std::max(max, value);
    }

    static Range nice(double data_min, double data_max)
    {
        // Creates "nice" rounded axis limits
        // e.g., for data 0.3-8.7, returns 0-10
    }
};
```

**Auto-scaling example:**

```
Your data: y values from -0.7 to 3.2
           x values from 0.5 to 15.8

Auto-scaled ranges:
  Y: -1.0 to 4.0 (nice round numbers)
  X:  0.0 to 20.0 (nice round numbers)
```

---

### 3. Coordinate Conversion

```cpp
int data_to_pixel_x(double x) const
{
    double normalized = (x - _x_range.min) / _x_range.span();
    return _plot_x0 + static_cast<int>(normalized * _plot_width);
}

int data_to_pixel_y(double y) const
{
    double normalized = (y - _y_range.min) / _y_range.span();
    // Y is inverted: screen Y increases downward, data Y increases upward
    return _plot_y1 - static_cast<int>(normalized * _plot_height);
}
```

**Coordinate systems:**

```
Data coordinates:          Screen/Pixel coordinates:
      Y ↑                        (0,0) ──────────► X
        │                          │
        │     ●                    │  margin_top
        │    / \                   │  ┌─────────┐
        │   /   \                  │  │ plot    │
   ─────┼───────────► X            │  │ area    │
        │                          ▼  └─────────┘
                                   Y   margin_bottom
```

---

### 4. The Plot Method

```cpp
Figure &plot(std::function<double(double)> func,
             double x_min, double x_max,
             const std::string &color = "",
             const std::string &label = "")
{
    PlotData data;
    data.color = color.empty() ? next_color() : colors::from_name(color);
    data.label = label;

    // Sample the function at many points
    int num_samples = static_cast<int>(_plot_width * 2);  // 2 samples per pixel
    double step = (x_max - x_min) / num_samples;

    for (double x = x_min; x <= x_max; x += step) {
        double y = func(x);
        if (std::isfinite(y)) {
            data.x_data.push_back(x);
            data.y_data.push_back(y);
        }
    }

    _plots.push_back(data);
    return *this;
}
```

**Sampling visualization:**

```
Function: sin(x) from -π to π
Plot width: 100 pixels
Samples: 200 points

x values: -3.14, -3.11, -3.08, ..., 3.08, 3.11, 3.14
y values:  0.00, -0.03, -0.06, ..., 0.06, 0.03,  0.00
```

---

### 5. The Color Palette

```cpp
namespace colors
{
    inline RGBA red(255, 0, 0);
    inline RGBA cyan(0, 255, 255);
    // ... more colors ...

    inline const std::vector<RGBA> &palette()
    {
        static std::vector<RGBA> p = {
            RGBA(31, 119, 180),   // Tableau blue
            RGBA(255, 127, 14),   // Tableau orange
            RGBA(44, 160, 44),    // Tableau green
            RGBA(214, 39, 40),    // Tableau red
            RGBA(148, 103, 189),  // Tableau purple
            // ... more colors
        };
        return p;
    }
}
```

**Auto-cycling colors:**

```cpp
fig.plot(func1);  // Gets first palette color (blue)
fig.plot(func2);  // Gets second (orange)
fig.plot(func3);  // Gets third (green)
// ...and so on, cycling when exhausted
```

---

### 6. The Pixel Font

For axis labels and text annotations, we use a tiny 3×5 pixel font:

```cpp
namespace font
{
    struct Glyph
    {
        uint8_t rows[5];  // 5 rows, each 3 bits wide
    };

    inline const std::map<char, Glyph> &get_font()
    {
        static std::map<char, Glyph> font = {
            {'0', {0b111, 0b101, 0b101, 0b101, 0b111}},
            {'1', {0b010, 0b110, 0b010, 0b010, 0b111}},
            // ... more characters
        };
        return font;
    }
}
```

**Font visualization (digit "5"):**

```cpp
{'5', {0b111, 0b100, 0b111, 0b001, 0b111}}

0b111 → ███
0b100 → █
0b111 → ███
0b001 →   █
0b111 → ███
```

---

### 7. Drawing Text

```cpp
void draw_text(const std::string &text, int x, int y, const RGBA &color)
{
    const auto &font_map = font::get_font();

    for (char c : text) {
        auto it = font_map.find(c);
        if (it == font_map.end()) {
            x += 4;  // Skip unknown characters
            continue;
        }

        const auto &glyph = it->second;
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (glyph.rows[row] & (1 << (2 - col))) {
                    set_pixel(x + col, y + row, color);
                }
            }
        }
        x += 4;  // Move to next character (3 pixels + 1 space)
    }
}
```

---

### 8. The Render Pipeline

```cpp
std::string render()
{
    // 1. Clear pixel buffer
    clear_pixels();

    // 2. Auto-scale if needed
    if (_auto_scale) update_ranges();

    // 3. Draw background elements
    if (_show_grid) draw_grid();
    draw_axes();

    // 4. Draw all plot data
    for (const auto &plot : _plots) {
        draw_plot_data(plot);
    }

    // 5. Draw labels and annotations
    draw_labels_to_pixels();

    // 6. Convert to output
    return render_header() + render_braille();
}
```

---

### 9. Braille Rendering

```cpp
std::string render_braille()
{
    std::ostringstream out;

    // Braille dot positions in each 2×4 cell:
    // [0] [3]    bits 0, 3
    // [1] [4]    bits 1, 4
    // [2] [5]    bits 2, 5
    // [6] [7]    bits 6, 7

    static const int dot_map[4][2] = {
        {0, 3}, {1, 4}, {2, 5}, {6, 7}
    };

    for (int cy = 0; cy < _char_height; ++cy) {
        for (int cx = 0; cx < _char_width; ++cx) {
            uint8_t pattern = 0;
            RGBA dominant_color;

            // Check each of the 8 pixels in this cell
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 2; ++col) {
                    int px = cx * 2 + col;
                    int py = cy * 4 + row;
                    RGBA pixel = _pixels[py][px];

                    if (pixel.a > 0) {
                        pattern |= (1 << dot_map[row][col]);
                        dominant_color = pixel;  // Remember color
                    }
                }
            }

            // Output ANSI color + Braille character
            out << ansi::fg_color(dominant_color.r, dominant_color.g, dominant_color.b);
            out << braille_to_utf8(pattern);  // Unicode 0x2800 + pattern
        }
        out << ansi::RESET << '\n';
    }

    return out.str();
}
```

**Braille pattern example:**

```
Pixels:     Pattern bits:    Unicode:
██          0b00000001 |     0x2800 + 0x49 = 0x2849
  ██        0b00001000 |
██          0b01000000       = ⡉

Result: ⡉ (Braille pattern: dots 1, 4, 7)
```

---

### 10. Drawing Plot Data

```cpp
void draw_plot_data(const PlotData &data)
{
    for (size_t i = 1; i < data.x_data.size(); ++i) {
        int x0 = data_to_pixel_x(data.x_data[i - 1]);
        int y0 = data_to_pixel_y(data.y_data[i - 1]);
        int x1 = data_to_pixel_x(data.x_data[i]);
        int y1 = data_to_pixel_y(data.y_data[i]);

        // Draw line segment with anti-aliasing
        draw_line_aa(x0, y0, x1, y1, data.color);
    }
}
```

---

## 🎨 Complete Example

```cpp
#include "pythonic/pythonicPlot.hpp"
using namespace pythonic::plot;

int main() {
    Figure fig(80, 24);

    fig.title("Trigonometric Functions");
    fig.xlim(-PI, PI);
    fig.ylim(-1.5, 1.5);

    // Plot multiple functions
    fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");
    fig.plot([](double x) { return cos(x); }, -PI, PI, "yellow", "cos(x)");
    fig.plot([](double x) { return sin(2*x)/2; }, -PI, PI, "magenta", "sin(2x)/2");

    // Add text annotations
    fig.print("Maximum", PI/2, 1.0, "green");
    fig.print("Zero", 0, 0, "white");

    std::cout << fig.render();
    return 0;
}
```

**Output (conceptual):**

```
      Trigonometric Functions
      ▄ sin(x)  ▄ cos(x)  ▄ sin(2x)/2
                    Y
   1.5┤        ⢀⣀⡀
   1.0┤Maximum⡴⠁  ⠈⠢⣀
      │     ⡔⠁      ⠈⠢⡀
   0.0├Zero─╋───────────────X
      │   ⡰⠁          ⠈⠢⡀
  -1.0┤ ⡠⠃              ⠈⠢
  -1.5┤⠐⠁
      └──────────────────────
     -3.14              3.14
```

---

## 📚 API Reference

### Figure Configuration

| Method           | Description      |
| ---------------- | ---------------- |
| `xlim(min, max)` | Set X axis range |
| `ylim(min, max)` | Set Y axis range |
| `title(str)`     | Set plot title   |
| `xlabel(str)`    | Set X axis label |
| `ylabel(str)`    | Set Y axis label |
| `legend(bool)`   | Show/hide legend |
| `grid(bool)`     | Show/hide grid   |

### Plotting

| Method                                   | Description         |
| ---------------------------------------- | ------------------- |
| `plot(func, x_min, x_max, color, label)` | Plot a function     |
| `scatter(x_vec, y_vec, color, label)`    | Scatter plot        |
| `print(text, x, y, color)`               | Add text annotation |

### Rendering

| Method     | Description            |
| ---------- | ---------------------- |
| `render()` | Generate output string |
| `show()`   | Print to stdout        |
| `clear()`  | Clear all plots        |

---

## 🎯 Tips & Tricks

### 1. High-DPI Output

```cpp
Figure fig(160, 48);  // Double the resolution
```

### 2. Aspect Ratio

```cpp
// For a square plot area:
Figure fig(40, 20);  // Width:Height ≈ 2:1 accounts for Braille aspect
```

### 3. Animation

```cpp
for (double t = 0; t < 10; t += 0.1) {
    fig.clear();
    fig.plot([t](double x) { return sin(x + t); }, -PI, PI);

    std::cout << "\033[H";  // Cursor home
    std::cout << fig.render();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
```

---

## 📚 Next Steps

- [pythonicDraw Tutorial](../LiveDraw/pythonicDraw_tutorial.md) - Understanding Braille graphics
- [LiveDraw Tutorial](../LiveDraw/livedraw.md) - Interactive drawing
