[⬅ Back to Table of Contents](../index.md)

# 📊 Plot - matplotlib-style Terminal Plotting

**pythonicPlot.hpp** provides a matplotlib-inspired plotting library that renders beautiful graphs directly in your terminal using Braille Unicode characters for 8× resolution.

---

## 🚀 Quick Start

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

int main() {
    Figure fig(80, 24);  // 80 chars wide, 24 chars tall

    // Plot a sine wave
    fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");

    fig.show();  // or fig.render() or std::cout << fig.render_to_string();
    return 0;
}
```

**Compile:**

```bash
g++ -std=c++20 -Iinclude -o plot my_plot.cpp
./plot
```

---

## 📐 Resolution & Dimensions

### Braille Resolution

Each terminal character cell maps to a 2×4 Braille dot grid, providing **8× the resolution** of standard ASCII art:

```
Terminal: 80 × 24 characters
Braille:  Each char = 2 × 4 dots

Pixel buffer: 80 × 2 = 160 pixels wide
              24 × 4 = 96 pixels tall

Total: 15,360 "pixels" to work with!
```

**Visual representation:**

```
One Braille character cell:
┌───┐
│⠿⠿│  ← 2 columns of dots
│⠿⠿│
│⠿⠿│
│⠿⠿│  ← 4 rows of dots
└───┘
```

### Supported Render Modes

```cpp
enum class PlotMode {
    braille_colored,  // Default - colored Braille dots
    braille_bw,       // Black and white Braille
    block_colored,    // Colored half-blocks (▀)
    block_bw          // BW half-blocks
};
```

---

## 🎨 API Reference

### Figure Class

```cpp
class Figure {
    // Constructor
    Figure(size_t char_width = 80, size_t char_height = 24,
           PlotMode mode = PlotMode::braille_colored);

    // Configuration (chainable methods)
    Figure& xlim(double min, double max);      // Set X axis range
    Figure& ylim(double min, double max);      // Set Y axis range
    Figure& title(const std::string& t);       // Set plot title
    Figure& xlabel(const std::string& l);      // Set X axis label
    Figure& ylabel(const std::string& l);      // Set Y axis label
    Figure& grid(bool show);                   // Show/hide grid
    Figure& legend(bool show);                 // Show/hide legend

    // Plotting
    Figure& plot(func, x_min, x_max, color, label);  // Plot function
    Figure& scatter(x_vec, y_vec, color, label);     // Scatter plot
    Figure& parametric(fx, fy, t_min, t_max, color); // Parametric plot

    // Annotations
    Figure& print(text, x, y, color);         // Add text annotation

    // Variables (Desmos-style)
    Figure& add_variable(name, value, min, max, step);
    Figure& set_var(name, value);

    // Rendering
    void render();                             // Print to stdout
    void show();                               // Alias for render()
    std::string render_to_string();            // Get as string
    Figure& clear();                           // Clear all plots
};
```

---

## 📈 Plotting Functions

### Basic Function Plot

```cpp
Figure fig(80, 24);

// Using lambda
fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");

// Using std::function
std::function<double(double)> f = [](double x) { return x * x; };
fig.plot(f, -5, 5, "yellow", "x²");

// Multiple functions on same axes
fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin");
fig.plot([](double x) { return cos(x); }, -PI, PI, "yellow", "cos");
fig.plot([](double x) { return tan(x); }, -PI/2.5, PI/2.5, "magenta", "tan");

fig.show();
```

### Scatter Plot

```cpp
Figure fig(80, 24);

std::vector<double> x = {1, 2, 3, 4, 5};
std::vector<double> y = {2.3, 4.1, 3.7, 5.2, 4.8};

fig.scatter(x, y, "cyan", "data points");
fig.show();

// Using pythonic var
var x_var = {1, 2, 3, 4, 5};
var y_var = {2.3, 4.1, 3.7, 5.2, 4.8};
fig.scatter(x_var, y_var, "green");
```

### Parametric Plot

```cpp
Figure fig(80, 24);

// Circle
fig.parametric(
    [](double t) { return cos(t); },  // x(t)
    [](double t) { return sin(t); },  // y(t)
    0, 2*PI,                          // t range
    "cyan",                           // color
    "circle"                          // label
);

// Lissajous curve
fig.parametric(
    [](double t) { return sin(3*t); },
    [](double t) { return sin(4*t); },
    0, 2*PI, "magenta", "lissajous"
);

fig.show();
```

---

## 🎯 Axis Configuration

### Setting Axis Ranges

```cpp
Figure fig(80, 24);

// Fixed ranges
fig.xlim(-10, 10);
fig.ylim(-5, 5);

// Auto-scale (default behavior)
// Ranges are automatically calculated from data
```

### Labels and Titles

```cpp
Figure fig(80, 24);

fig.title("Trigonometric Functions")
   .xlabel("angle (radians)")
   .ylabel("amplitude");

fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");
fig.show();
```

### Grid and Legend

```cpp
Figure fig(80, 24);

fig.grid(true);    // Show grid (default)
fig.grid(false);   // Hide grid

fig.legend(true);  // Show legend (default)
fig.legend(false); // Hide legend
```

---

## 🎨 Colors

### Named Colors

```cpp
// Available named colors:
"red", "green", "blue", "yellow", "cyan", "magenta",
"orange", "purple", "white", "black", "gray" (or "grey")
```

### Auto-Cycling Colors

When no color is specified, colors are automatically assigned from the Tableau palette:

```cpp
fig.plot(f1);  // Gets first palette color (blue)
fig.plot(f2);  // Gets second (orange)
fig.plot(f3);  // Gets third (green)
// ...cycles when exhausted
```

### Custom RGBA Colors

```cpp
RGBA custom_color(255, 128, 64, 255);  // RGBA
fig.plot([](double x) { return sin(x); }, -PI, PI, custom_color);
```

---

## ✏️ Text Annotations

### Adding Text to Plots

```cpp
Figure fig(80, 24);

fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");

// Add annotations at data coordinates
fig.print("Maximum", PI/2, 1.0, "green");
fig.print("Minimum", -PI/2, -1.0, "red");
fig.print("Zero", 0, 0, "yellow");

fig.show();
```

### Method Chaining

```cpp
fig.title("Annotated Graph")
   .xlim(-PI, PI)
   .ylim(-1.5, 1.5)
   .plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)")
   .print("Peak", PI/2, 1.0, "green")
   .print("Valley", -PI/2, -1.0, "red");
```

---

## 🎬 Animation

Pythonic provides a unified `animate()` function for creating beautiful terminal animations.

### AnimateConfig - Configuration Struct

The `AnimateConfig` struct lets you configure all animation parameters:

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

// Create config with fluent builder pattern
auto cfg = AnimateConfig()
    .x_range(-PI, PI)       // X axis range
    .time(20.0)             // Duration in seconds
    .framerate(30)          // FPS
    .size(120, 35)          // Size in characters (width, height)
    // OR
    .size_px(240, 140)      // Size in pixels (auto-converts to chars)
    .labels("cyan")         // Color for X/Y axis labels
    .ranges("magenta")      // Color for min/max range values
    .set_title("My Animation")
    .loop(false);           // Stop after duration (don't loop)
```

**Available configuration methods:**

| Method               | Description                                          | Default   |
| -------------------- | ---------------------------------------------------- | --------- |
| `.x_range(min, max)` | X axis range                                         | -10 to 10 |
| `.time(seconds)`     | Animation duration                                   | 10.0      |
| `.framerate(fps)`    | Frames per second                                    | 30        |
| `.size(w, h)`        | Size in terminal characters                          | 80×24     |
| `.size_px(w, h)`     | Size in pixels (Braille converts to chars)           | -         |
| `.labels(color)`     | Color for X/Y axis labels                            | "cyan"    |
| `.ranges(color)`     | Color for min/max values                             | "magenta" |
| `.set_title(text)`   | Plot title                                           | ""        |
| `.loop(bool)`        | Loop animation (true) or stop after duration (false) | true      |

**Signal Handling:**

- Press **Ctrl+C** to stop the animation at any time
- Terminal state (cursor visibility) is automatically restored via RAII
- Signal handlers are properly restored after animation ends

---

### Simple Single-Plot Animation

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

int main() {
    // Simplest form: function, x_range
    animate(
        [](double t, double x) { return sin(x + t); },
        -PI, PI  // x range
    );

    // With more options (positional arguments)
    animate(
        [](double t, double x) { return sin(x + t); },
        -PI, PI,   // x range
        10.0,      // duration
        30,        // fps
        80, 24,    // width, height
        "cyan",    // color
        "sin(x+t)" // legend label
    );

    return 0;
}
```

---

### Multi-Plot Animation with Config

For multiple plots with full control:

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

int main() {
    // Configure the animation
    auto cfg = AnimateConfig()
        .x_range(-2 * M_PI, 2 * M_PI)
        .time(20.0)
        .framerate(24)
        .size_px(240, 140)  // Pixel dimensions (converted to chars)
        .labels("cyan")
        .ranges("magenta")
        .set_title("Frequency Modulation Demo");

    // Animate with multiple plots
    animate(cfg,
        // Each plot is a tuple: (function, color, label)
        std::make_tuple(
            [](double t, double x) {
                double w = 1.0 + 0.5 * std::sin(0.5 * t);  // Varying frequency
                double amp = 1.0 + 0.3 * std::cos(0.8 * t); // Varying amplitude
                return amp * std::sin(w * t + x);
            },
            "red",
            "sin(w*t+x)*A"
        ),
        std::make_tuple(
            [](double t, double x) {
                double w = 1.0 + 0.5 * std::sin(0.5 * t);
                return std::cos(w * t + x);
            },
            "cyan",
            "cos(w*t+x)"
        ),
        std::make_tuple(
            [](double t, double x) {
                (void)x;  // Envelope - doesn't depend on x
                return 1.0 + 0.3 * std::cos(0.8 * t);
            },
            "yellow",
            "envelope"
        )
    );

    return 0;
}
```

**Plot tuple format:**

- 2 elements: `(function, color)`
- 3 elements: `(function, color, label)` - label appears in legend

---

### Non-Looping Animation

Use `.loop(false)` to make the animation stop after the specified duration:

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

int main() {
    auto cfg = AnimateConfig()
        .x_range(-PI, PI)
        .time(5.0)          // Run for exactly 5 seconds
        .framerate(30)
        .size(80, 24)
        .loop(false);       // Stop after duration (don't repeat)

    animate(cfg,
        std::make_tuple(
            [](double t, double x) { return std::sin(x + t); },
            "green",
            "wave"
        )
    );

    std::cout << "Animation completed after 5 seconds!\n";
    return 0;
}
```

> **Note:** When `.loop(true)` (default), the animation repeats until you press Ctrl+C.

---

### Complete Animation Example

Here's a comprehensive example demonstrating all features:

```cpp
#include <pythonic/pythonic.hpp>
using namespace Pythonic;
using namespace pythonic::plot;

int main() {
    print("=== Animated Plot Demo ===");
    print("Press Ctrl+C to stop");

    // Configure with fluent API
    auto cfg = AnimateConfig()
        .x_range(-2 * M_PI, 2 * M_PI)
        .time(20.0)
        .framerate(24)
        .size_px(240, 140)
        .labels("cyan")
        .ranges("magenta")
        .set_title("Frequency Modulated Waves");

    animate(cfg,
        // Amplitude-modulated sine wave
        std::make_tuple(
            [](double t, double x) -> double {
                double w = 1.0 + 0.5 * std::sin(0.5 * t);
                double amp = 1.0 + 0.3 * std::cos(0.8 * t);
                double phi = 0.1 * x * x;  // Position-dependent phase
                return amp * std::sin(w * t + x + phi);
            },
            "red", "sin(w*t+x+φ)*A"
        ),
        // Cosine wave
        std::make_tuple(
            [](double t, double x) -> double {
                double w = 1.0 + 0.5 * std::sin(0.5 * t);
                return std::cos(w * t + x);
            },
            "cyan", "cos(w*t+x)"
        ),
        // Upper envelope
        std::make_tuple(
            [](double t, double x) -> double {
                (void)x;
                return 1.0 + 0.3 * std::cos(0.8 * t);
            },
            "yellow", "±Amplitude"
        ),
        // Lower envelope (no label)
        std::make_tuple(
            [](double t, double x) -> double {
                (void)x;
                return -(1.0 + 0.3 * std::cos(0.8 * t));
            },
            "yellow", ""
        )
    );

    print("Animation finished!");
    return 0;
}
```

---

### Figure Class - Pixel Dimensions

You can also create figures with pixel dimensions:

```cpp
// Method 1: Factory function
auto fig = Figure::from_pixels(320, 160);  // 160×40 characters

// Method 2: Standard constructor (character dimensions)
Figure fig(120, 35);  // 120×35 characters = 240×140 pixels
```

---

### Animation Features

- **Auto Y-scaling**: Y axis is automatically computed from function range
- **Looping**: Animation loops when duration is exceeded
- **Clean display**: Cursor is hidden during animation
- **Multi-color**: Each plot can have its own color and legend
- **Legends**: Labels appear in the legend area above the plot
- **Configurable colors**: Axis labels and range values can be customized

---

### Legacy Functions (Backward Compatibility)

For backward compatibility, these functions are still available:

```cpp
// Legacy single-plot with dependency functions
animate_legacy(f, x_min, x_max, duration, fps, width, height, deps...);

// Legacy multi-plot (still works)
animate_plots(x_min, x_max, duration, fps, width, height, plots...);
```

However, we recommend using the unified `animate()` function with `AnimateConfig` for new code.

---

### Manual Animation Control

For more control over animation, use the Figure class directly:

```cpp
Figure fig(80, 24);

// Hide cursor for smooth animation
std::cout << "\033[?25l" << std::flush;

for (double t = 0; t < 20; t += 0.05) {
    fig.clear();
    fig.set_time(t);

    fig.plot_animated(
        [](double t, double x) { return sin(x + t); },
        -PI, PI, "cyan"
    );

    std::cout << "\033[H" << fig.render_to_string();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// Show cursor
std::cout << "\033[?25h" << std::flush;
```

---

## 🔧 Desmos-style Variables

### Adding Sliders

```cpp
Figure fig(80, 24);

// Add a variable with range and step
fig.add_variable("a", 1.0, 0.1, 5.0, 0.1);  // name, value, min, max, step
fig.add_variable("b", 0.5, 0.0, 2.0, 0.1);

// Use variables in plot function
fig.plot([&](double x) {
    double a = fig.get_var("a");
    double b = fig.get_var("b");
    return a * sin(b * x);
}, -PI, PI, "cyan");

// Update variable dynamically
fig.set_var("a", 2.0);
fig.render();
```

---

## 🐍 Pythonic var Integration

### Using var Lambdas

```cpp
using namespace Pythonic;
using namespace pythonic::plot;

var f = lambda_(x, sin(x));

plot(f, -PI, PI, "cyan");  // Quick plot with var lambda
```

### Using var Data

```cpp
var x_data = {1.0, 2.0, 3.0, 4.0, 5.0};
var y_data = {1.1, 4.0, 8.9, 16.1, 25.0};

Figure fig(80, 24);
fig.scatter(x_data, y_data, "cyan", "measurements");
fig.show();
```

---

## 💡 Quick Plotting Functions

### One-Liner Plots

```cpp
using namespace pythonic::plot;

// Quick function plot
plot([](double x) { return sin(x); }, -PI, PI, "cyan");

// Quick parametric plot
parametric(
    [](double t) { return cos(t); },
    [](double t) { return sin(t); },
    0, 2*PI, "cyan"
);

// Quick scatter plot
scatter({1,2,3,4,5}, {2.3,4.1,3.7,5.2,4.8}, "cyan");
```

---

## 📊 Complete Examples

### Trigonometric Functions

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

int main() {
    Figure fig(80, 24);

    fig.title("Trigonometric Functions")
       .xlim(-PI, PI)
       .ylim(-1.5, 1.5);

    fig.plot([](double x) { return sin(x); }, -PI, PI, "cyan", "sin(x)");
    fig.plot([](double x) { return cos(x); }, -PI, PI, "yellow", "cos(x)");
    fig.plot([](double x) { return sin(2*x)/2; }, -PI, PI, "magenta", "sin(2x)/2");

    fig.print("Maximum", PI/2, 1.0, "green");
    fig.print("Zero", 0, 0, "white");

    fig.show();
    return 0;
}
```

### Polynomial with Roots

```cpp
Figure fig(80, 24);

// f(x) = (x-1)(x+2)(x-3) = x³ - 2x² - 5x + 6
fig.plot([](double x) { return (x-1)*(x+2)*(x-3); }, -3, 4, "cyan", "polynomial");

// Mark the roots
fig.print("root", 1, 0, "red");
fig.print("root", -2, 0, "red");
fig.print("root", 3, 0, "red");

fig.show();
```

### Real-time Data Visualization

```cpp
Figure fig(80, 24);
std::vector<double> x_data, y_data;

std::cout << "\033[?25l";  // Hide cursor

for (int i = 0; i < 100; ++i) {
    x_data.push_back(i);
    y_data.push_back(sin(i * 0.1) + 0.1 * (rand() % 10 - 5));

    fig.clear();
    fig.scatter(x_data, y_data, "cyan", "live data");

    std::cout << "\033[H" << fig.render_to_string();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

std::cout << "\033[?25h";  // Show cursor
```

---

## 🎯 Tips & Best Practices

### High-DPI Output

```cpp
Figure fig(160, 48);  // Double the resolution
```

### Aspect Ratio for Square Plots

```cpp
// For visually square plot area:
Figure fig(40, 20);  // Width:Height ≈ 2:1 accounts for Braille aspect ratio
```

### Performance Tips

- Use `render_to_string()` and print once rather than multiple renders
- For animation, use `\033[H` (cursor home) instead of clearing terminal
- Reduce number of samples for faster plotting

---

## 🔗 Related Documentation

- [pythonicDraw Tutorial](../LiveDraw/pythonicDraw_tutorial.md) - Understanding Braille graphics
- [LiveDraw Tutorial](../LiveDraw/livedraw.md) - Interactive drawing
- [Print Documentation](../Print/print.md) - Output functions

---

## 📚 Mathematical Constants

Available in `pythonic::plot` namespace:

```cpp
constexpr double PI = 3.14159265358979323846;
constexpr double E = 2.71828182845904523536;
constexpr double TAU = 2.0 * PI;  // 2π
```
