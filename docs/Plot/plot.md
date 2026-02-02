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

Pythonic provides two powerful animation functions: `animate()` and `animate_plots()`.

### Simple Animation (`animate`)

Animate a time-varying function with a single call:

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

int main() {
    // animate(f, x_min, x_max, duration, fps, width, height)
    animate(
        [](double t, double x) { return sin(x + t); },  // f(t, x)
        -PI, PI,     // x range
        10.0,        // duration in seconds
        30,          // frames per second
        80, 24       // figure dimensions
    );

    return 0;
}
```

**Parameters:**

- `f` - Function `f(t, x)` where `t` is time and `x` is the variable
- `x_min, x_max` - X axis range
- `duration` - Animation duration in seconds (default: 10.0)
- `fps` - Frames per second (default: 30)
- `width, height` - Figure dimensions in characters (default: 80, 24)

**Features:**

- Auto-scales Y axis by sampling over time
- Loops automatically when duration is exceeded
- Hides cursor during animation for clean display
- Press Ctrl+C to stop

---

### Complex Animation with Dependencies (`animate`)

The same `animate` function also supports functions with time-varying parameters:

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

int main() {
    // f(t, x, a, b) = a * sin(x) + b * cos(t)
    // where 'a' and 'b' are provided by dependency functions

    animate(
        // Main function: receives t, x, and values from dependency functions
        [](double t, double x, double a, double b) {
            return a * sin(x) + b * cos(t);
        },
        -PI, PI,           // x range
        10.0, 30,          // duration, fps
        80, 24,            // width, height
        // Dependency functions - each takes t and returns a value
        [](double t) { return 1.0 + 0.5 * sin(t); },  // -> a
        [](double t) { return cos(2 * t); }           // -> b
    );

    return 0;
}
```

**Use cases:**

- Amplitude modulation: `a(t) * sin(x)` where `a` varies with time
- Frequency modulation: `sin(f(t) * x)` where frequency varies
- Multiple coupled oscillations
- Physics simulations with time-varying parameters

---

### Multi-Plot Animation (`animate_plots`)

Animate multiple functions simultaneously, each with its own color:

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

int main() {
    animate_plots(
        -PI, PI,      // x range
        10.0, 30,     // duration, fps
        80, 24,       // width, height
        // Plot entries: std::make_tuple(function, color)
        std::make_tuple(
            [](double t, double x) { return sin(x + t); },
            "cyan"
        ),
        std::make_tuple(
            [](double t, double x) { return cos(x - t); },
            "yellow"
        ),
        std::make_tuple(
            [](double t, double x) { return sin(2*x + t) * 0.5; },
            "magenta"
        )
    );

    return 0;
}
```

**Features:**

- Variadic - add as many plot functions as you want
- Each function gets its own color
- All share the same Y-axis scale (auto-computed)

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
