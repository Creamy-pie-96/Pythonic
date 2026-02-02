# pythonicPlot — Terminal Graphics for Mathematical Functions

## What This Module Does

`pythonicPlot.hpp` turns your terminal into a graphing calculator. It renders mathematical functions using Unicode Braille characters, achieving 8× the resolution of regular terminal characters. Think matplotlib, but for the terminal — and with Desmos-like animation support.

**Core problem it solves**: You want to visualize mathematical functions, animated graphs, or parametric curves without leaving your terminal or setting up a graphical environment.

---

## The Architecture: How It All Fits Together

```
┌──────────────────────────────────────────────────────────┐
│                    pythonicPlot.hpp                       │
├──────────────────────────────────────────────────────────┤
│                                                          │
│   ┌──────────┐      ┌──────────┐      ┌──────────────┐  │
│   │  Figure  │ ──▶  │  Pixel   │ ──▶  │   Braille    │  │
│   │  (Math)  │      │  Buffer  │      │   Renderer   │  │
│   └──────────┘      └──────────┘      └──────────────┘  │
│        │                                     │          │
│        ▼                                     ▼          │
│   ┌──────────┐                        ┌──────────────┐  │
│   │ plot()   │                        │ ANSI Colors  │  │
│   │ scatter()│                        │ + UTF-8      │  │
│   │ parametric()                      └──────────────┘  │
│   └──────────┘                                          │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

**The key insight**: A terminal character cell is a tiny canvas. Braille Unicode characters (U+2800–U+28FF) can represent a 2×4 grid of "dots" in a single cell. So an 80×24 terminal becomes a 160×96 pixel canvas — enough for smooth curves.

---

## Part 1: The Figure Class — Your Plotting Canvas

### Creating a Figure

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;
using namespace py::plot;

// Create an 80×24 character figure (160×96 pixel resolution in Braille)
Figure fig(80, 24);
```

The `Figure` constructor takes:

- `char_width`: Width in terminal characters (default 80)
- `char_height`: Height in terminal characters (default 24)
- `mode`: Rendering mode (default `PlotMode::braille_colored`)

Internally, the Figure maintains:

1. **A pixel buffer** (`_pixels`) — 2D vector of RGBA colors at Braille resolution
2. **Plot data** (`_plots`) — stored x/y data points with colors
3. **Axis ranges** (`_x_range`, `_y_range`) — data coordinate bounds
4. **Margins** — pixel-space padding for labels

### Why margins matter

The Figure reserves space for axis labels:

```cpp
_margin_left(20),   // Space for Y-axis numbers
_margin_right(6),
_margin_top(10),    // Space for title
_margin_bottom(10)  // Space for X-axis numbers
```

This means your actual plot area is slightly smaller than the full canvas. The `update_plot_area()` method calculates the usable plot region.

---

## Part 2: Coordinate Transformation

### The Problem

Mathematical functions use "data coordinates" (e.g., x ∈ [-π, π], y ∈ [-1, 1]).
The screen uses "pixel coordinates" (e.g., x ∈ [0, 159], y ∈ [0, 95]).

### The Solution

Two key methods handle the transformation:

```cpp
int data_to_pixel_x(double x) const {
    double normalized = (x - _x_range.min) / _x_range.span();
    return _plot_x0 + static_cast<int>(normalized * _plot_width);
}

int data_to_pixel_y(double y) const {
    double normalized = (y - _y_range.min) / _y_range.span();
    // Y is INVERTED in screen coordinates (top = 0)
    return _plot_y1 - static_cast<int>(normalized * _plot_height);
}
```

**Important**: Screen Y-coordinates are inverted. In data space, Y increases upward. In pixel space, Y increases downward. That's why `data_to_pixel_y` subtracts from `_plot_y1`.

---

## Part 3: Plotting Functions

### Basic Plotting

```cpp
fig.plot([](double x) { return sin(x); }, -PI, PI, "red", "sin(x)");
```

What happens internally:

1. **Sample the function**: Loop from `x_min` to `x_max` with `num_points` samples
2. **Store the data**: Push (x, y) pairs into `PlotData.x_data` and `PlotData.y_data`
3. **Handle discontinuities**: If `y` is `NaN` or `Inf`, break the line segment
4. **Update ranges**: If auto-scaling, expand the axis ranges to fit the data

```cpp
template <typename Func>
Figure& plot(Func&& f, double x_min, double x_max, ...) {
    PlotData data;
    double step = (x_max - x_min) / num_points;

    for (int i = 0; i <= num_points; ++i) {
        double x = x_min + i * step;
        double y = f(x);  // Call the user's function

        if (std::isfinite(y)) {
            data.x_data.push_back(x);
            data.y_data.push_back(y);
        } else {
            // Discontinuity: save current segment, start new one
            if (!data.x_data.empty()) {
                _plots.push_back(data);
                data.x_data.clear();
                data.y_data.clear();
            }
        }
    }
}
```

### Parametric Plotting

For curves defined as `x(t)`, `y(t)`:

```cpp
fig.parametric(
    [](double t) { return cos(t); },  // x(t)
    [](double t) { return sin(t); },  // y(t)
    0, 2*PI,                          // t range
    "cyan",                           // color
    "circle"                          // label
);
```

This is the same as regular plotting, but both x and y come from functions of t.

### Scatter Plots

```cpp
std::vector<double> x = {1, 2, 3, 4, 5};
std::vector<double> y = {2.3, 4.1, 3.8, 5.2, 4.9};
fig.scatter(x, y, "yellow");
```

Scatter plots set `data.show_points = true`, which draws filled circles at each data point.

---

## Part 4: Rendering Pipeline

When you call `fig.show()` or `fig.render()`, here's the sequence:

```
1. clear_pixels()        → Fill buffer with background color
2. draw_grid()           → Dashed grid lines
3. draw_axes()           → X and Y axis lines + border
4. draw_plot(...)        → Anti-aliased lines for each dataset
5. draw_labels_to_pixels() → Axis labels using pixel font
6. render_header()       → Title + legend as plain text
7. render_braille()      → Convert pixel buffer to Braille string
```

### Anti-Aliased Line Drawing

The `draw_line_aa()` method uses **Wu's algorithm** for smooth lines:

```cpp
void draw_line_aa(int x0, int y0, int x1, int y1, const RGBA& color) {
    // Wu's algorithm adjusts pixel brightness based on sub-pixel position
    // This makes diagonal lines appear smoother

    auto plot_pixel = [&](int x, int y, double brightness) {
        RGBA blended(color.r, color.g, color.b,
                     static_cast<uint8_t>(color.a * brightness));
        set_pixel(x, y, blended);
    };

    // ... Bresenham-style iteration with fractional brightness
}
```

### Braille Rendering

The `render_braille()` method converts the pixel buffer to Braille:

```cpp
// Braille dot positions in each 2×4 cell:
//   [0] [3]    bit 0, bit 3
//   [1] [4]    bit 1, bit 4
//   [2] [5]    bit 2, bit 5
//   [6] [7]    bit 6, bit 7

// For each character cell:
for (int cy = 0; cy < _char_height; ++cy) {
    for (int cx = 0; cx < _char_width; ++cx) {
        uint8_t pattern = 0;
        int r_sum = 0, g_sum = 0, b_sum = 0, count = 0;

        // Check each of the 8 pixels in this cell
        for (int dy = 0; dy < 4; ++dy) {
            for (int dx = 0; dx < 2; ++dx) {
                if (pixel_is_active) {
                    pattern |= (1 << dot_map[dy][dx]);
                    // Accumulate color
                }
            }
        }

        // Output: color escape + Braille character (0x2800 + pattern)
    }
}
```

---

## Part 5: Animation System

### The `animate()` Function

This is the unified animation function that handles:

1. Simple time-varying plots: `f(t, x)`
2. Complex plots with dependencies: `f(t, x, dep1, dep2, ...)`

```cpp
// Simple animation
animate([](double t, double x) {
    return sin(x + t);
}, -PI, PI);

// With dependency functions
animate(
    [](double t, double x, double amplitude, double phase) {
        return amplitude * sin(x + phase);
    },
    -PI, PI,           // x range
    10.0,              // duration (seconds)
    30,                // fps
    80, 24,            // figure size
    [](double t) { return 1.0 + 0.5 * sin(t); },  // amplitude(t)
    [](double t) { return t; }                     // phase(t)
);
```

**How it works**:

```cpp
template <typename MainFunc, typename... DepFuncs>
inline void animate(MainFunc&& f, double x_min, double x_max,
                    double duration, double fps, int width, int height,
                    DepFuncs&&... deps)
{
    // 1. First pass: scan t range to estimate y bounds
    for (double t = 0; t <= duration; t += 0.5) {
        auto dep_values = eval_deps(t);  // Evaluate all dependency functions
        for (double x = x_min; x <= x_max; x += step) {
            double y = std::apply([&](auto... dvals) {
                return f(t, x, dvals...);  // Unpack dependencies
            }, dep_values);
            // Update y_min, y_max
        }
    }

    // 2. Animation loop
    while (true) {
        double t = elapsed_time();
        fig.clear();

        auto dep_values = eval_deps(t);
        auto f_at_t = [&](double t_, double x) {
            return std::apply([&](auto... dvals) {
                return f(t_, x, dvals...);
            }, dep_values);
        };

        fig.plot_animated(f_at_t, x_min, x_max, "cyan");

        std::cout << "\033[H" << fig.render_to_string();  // Cursor home + render
        sleep(1/fps);
    }
}
```

The `std::apply` trick unpacks a tuple of dependency values into function arguments.

### Multiple Plot Animation

```cpp
animate_plots(-PI, PI, 10.0, 30, 80, 24,
    std::make_tuple([](double t, double x) { return sin(x + t); }, "red"),
    std::make_tuple([](double t, double x) { return cos(x - t); }, "blue"),
    std::make_tuple([](double t, double x) { return sin(2*x + t); }, "green")
);
```

This renders multiple functions simultaneously, each with its own color.

---

## Part 6: The Color System

### Named Colors

```cpp
namespace colors {
    inline RGBA red(255, 0, 0);
    inline RGBA blue(0, 0, 255);
    // ...
}
```

### Auto-Cycling Palette

When you don't specify a color, `Figure::next_color()` cycles through a Tableau-inspired palette:

```cpp
const std::vector<RGBA>& palette() {
    static std::vector<RGBA> p = {
        RGBA(31, 119, 180),   // Tableau blue
        RGBA(255, 127, 14),   // Tableau orange
        RGBA(44, 160, 44),    // Tableau green
        RGBA(214, 39, 40),    // Tableau red
        // ...
    };
    return p;
}
```

### RGBA Blending

Colors support alpha compositing:

```cpp
RGBA blend_over(const RGBA& dst) const {
    // Porter-Duff "over" operation
    float src_a = a / 255.0f;
    float out_r = (r * src_a + dst.r * (1.0f - src_a));
    // ...
}
```

---

## Part 7: The Pixel Font

Axis labels are rendered using a tiny 3×5 pixel font:

```cpp
namespace font {
    struct Glyph {
        uint8_t rows[5];  // 5 rows, each 3 bits wide
    };

    // Example: '0' glyph
    // ███  = 0b111
    // █ █  = 0b101
    // █ █  = 0b101
    // █ █  = 0b101
    // ███  = 0b111
    {'0', {0b111, 0b101, 0b101, 0b101, 0b111}},
}
```

The `draw_text()` method iterates through each glyph and sets pixels accordingly.

---

## Part 8: Configuration Methods

### Method Chaining

All configuration methods return `*this` for fluent API:

```cpp
Figure fig(80, 24);
fig.xlim(-PI, PI)
   .ylim(-2, 2)
   .title("Trigonometric Functions")
   .xlabel("x")
   .ylabel("y")
   .grid(true)
   .legend(true);
```

### Text Annotations

```cpp
fig.print("Maximum", 0, 1.0, "cyan");  // Draw text at data coordinates
```

Text annotations are stored and drawn after the axes are finalized, ensuring correct positioning.

---

## Part 9: Integration with pythonicVars

The plot module integrates with `var` lambdas:

```cpp
var f = lambda_(x, sin(x) * x);
plot(f, -10, 10);
```

The `plot()` function detects whether the callable returns `var` and handles conversion:

```cpp
if constexpr (std::is_invocable_v<Func, double>) {
    y = static_cast<double>(f(x));
} else if constexpr (std::is_invocable_v<Func, var>) {
    var result = f(var(x));
    y = result.template get<double>();
}
```

---

## Design Decisions

### Why Braille?

1. **8× resolution**: 2×4 dots per character vs 1×1
2. **Unicode support**: Works in any modern terminal
3. **Color support**: Each cell can have its own foreground color

### Why separate header/graph rendering?

The title and legend are plain text (for readability), while the graph area is Braille. This separation in `render_header()` vs `render_braille()` allows proper text formatting above the graph.

### Why pre-scan Y range in animations?

Animations need stable axis bounds. Scanning the function over the full time range prevents the graph from "jumping" as y-range changes.

---

## Quick Reference

| Method                                 | Purpose                     |
| -------------------------------------- | --------------------------- |
| `Figure(w, h)`                         | Create w×h character canvas |
| `plot(f, x_min, x_max)`                | Plot function y = f(x)      |
| `parametric(fx, fy, t_min, t_max)`     | Plot parametric curve       |
| `scatter(x, y)`                        | Scatter plot from vectors   |
| `xlim(min, max)` / `ylim(min, max)`    | Set axis ranges             |
| `title(s)` / `xlabel(s)` / `ylabel(s)` | Set labels                  |
| `grid(bool)` / `legend(bool)`          | Toggle grid/legend          |
| `print(text, x, y)`                    | Add text annotation         |
| `clear()`                              | Clear all plots             |
| `show()` / `render()`                  | Output to stdout            |
| `render_to_string()`                   | Return as string            |

| Free Function                   | Purpose                     |
| ------------------------------- | --------------------------- |
| `plot(f, x_min, x_max)`         | Quick single-function plot  |
| `animate(f, x_min, x_max, ...)` | Time-varying animation      |
| `animate_plots(...)`            | Multiple animated functions |
| `parametric(fx, fy, ...)`       | Quick parametric plot       |
| `scatter(x, y)`                 | Quick scatter plot          |

---

## Example: Complete Workflow

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;
using namespace py::plot;

int main() {
    // Create figure
    Figure fig(100, 30);

    // Configure
    fig.xlim(-2*PI, 2*PI)
       .ylim(-1.5, 1.5)
       .title("Wave Functions")
       .grid(true);

    // Add plots
    fig.plot([](double x) { return sin(x); }, -2*PI, 2*PI, "red", "sin(x)");
    fig.plot([](double x) { return cos(x); }, -2*PI, 2*PI, "blue", "cos(x)");

    // Annotate
    fig.print("π", PI, 0, "yellow");

    // Render
    fig.show();

    return 0;
}
```

This produces a high-resolution Braille graph with colored sine and cosine curves, axis labels, and a text annotation.
