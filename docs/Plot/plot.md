[⬅ Back to Table of Contents](../index.md)
[⬅ Back to File I/O](../File_io/File_io.md)

# Plotting

This page documents all user-facing plotting functions in Pythonic for creating terminal-based mathematical plots and animations, using a clear tabular format with concise examples.

---

## Quick Start

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

// Simple function plot
plot([](double x) { return sin(x); }, -PI, PI);

// Multiple plots with Figure
Figure fig(80, 40);
fig.plot([](double x) { return sin(x); }, -PI, PI, "red", "sin(x)");
fig.plot([](double x) { return cos(x); }, -PI, PI, "blue", "cos(x)");
fig.show();
```

---

## Simple Plot Functions

| Function                                                 | Description                       | Example                                                                                  |
| -------------------------------------------------------- | --------------------------------- | ---------------------------------------------------------------------------------------- |
| `plot(f, x_min, x_max, color, width, height)`            | Plot a function over a range      | `plot([](double x) { return x*x; }, -10, 10);`                                           |
| `parametric(fx, fy, t_min, t_max, color, width, height)` | Parametric plot (x(t), y(t))      | `parametric([](double t) { return cos(t); }, [](double t) { return sin(t); }, 0, 2*PI);` |
| `scatter(x_data, y_data, color, width, height)`          | Scatter plot from data points     | `scatter({1,2,3}, {1,4,9}, "green");`                                                    |
| `animate(f, x_min, x_max, config)`                       | Animated plot with time parameter | `animate([](double t, double x) { return sin(x+t); }, -PI, PI);`                         |

**Parameters:**

- `f` - Lambda function `[](double x) -> double`
- `fx, fy` - Lambda functions for parametric plots
- `x_min, x_max` - Range of x values
- `t_min, t_max` - Range of parameter t
- `color` - Color name (e.g., "red", "blue", "green") or empty for default
- `width, height` - Terminal width/height in characters (default: 80, 24)

---

## Figure Class

The `Figure` class provides matplotlib-style plotting with multiple plots, legends, and customization.

### Construction

| Constructor                   | Description                      | Example                                          |
| ----------------------------- | -------------------------------- | ------------------------------------------------ |
| `Figure(width, height)`       | Create figure with dimensions    | `Figure fig(80, 40);`                            |
| `Figure(width, height, mode)` | Create with specific render mode | `Figure fig(80, 40, PlotMode::braille_colored);` |

### Adding Plots

| Method                                           | Description            | Example                                                                                              |
| ------------------------------------------------ | ---------------------- | ---------------------------------------------------------------------------------------------------- |
| `plot(f, x_min, x_max, color, label)`            | Add function plot      | `fig.plot([](double x) { return x*x; }, -10, 10, "red", "x²");`                                      |
| `parametric(fx, fy, t_min, t_max, color, label)` | Add parametric plot    | `fig.parametric([](double t) { return cos(t); }, [](double t) { return sin(t); }, 0, 2*PI, "blue");` |
| `scatter(x_data, y_data, color, label)`          | Add scatter plot       | `fig.scatter({1,2,3}, {1,4,9}, "green", "data");`                                                    |
| `plot_animated(f, x_min, x_max, color, label)`   | Add time-animated plot | Used internally by `animate()`                                                                       |

### Display & Configuration

| Method                  | Description                   | Example                                        |
| ----------------------- | ----------------------------- | ---------------------------------------------- |
| `show()`                | Render and display the figure | `fig.show();`                                  |
| `render_to_string()`    | Get rendered output as string | `std::string output = fig.render_to_string();` |
| `clear()`               | Clear all plots               | `fig.clear();`                                 |
| `set_title(title)`      | Set plot title                | `fig.set_title("My Plot");`                    |
| `set_xlabel(label)`     | Set x-axis label              | `fig.set_xlabel("Time (s)");`                  |
| `set_ylabel(label)`     | Set y-axis label              | `fig.set_ylabel("Value");`                     |
| `set_xlim(min, max)`    | Set x-axis limits             | `fig.set_xlim(-5, 5);`                         |
| `set_ylim(min, max)`    | Set y-axis limits             | `fig.set_ylim(0, 100);`                        |
| `enable_legend(enable)` | Show/hide legend              | `fig.enable_legend(true);`                     |
| `enable_grid(enable)`   | Show/hide grid                | `fig.enable_grid(true);`                       |

---

## Animation

Animations use a time parameter `t` that changes over time. The function signature is `f(t, x)` where `t` is time and `x` is the x-coordinate.

### Simple Animation

```cpp
using namespace pythonic::plot;

// Animate a traveling wave
animate([](double t, double x) {
    return sin(x + t);
}, -PI, PI);
```

### AnimateConfig

| Field           | Description                  | Default  | Example               |
| --------------- | ---------------------------- | -------- | --------------------- |
| `x_min, x_max`  | X-axis range                 | Required | `cfg.x_min = -10;`    |
| `duration`      | Animation duration (seconds) | 10.0     | `cfg.duration = 5.0;` |
| `fps`           | Frames per second            | 30       | `cfg.fps = 60;`       |
| `width, height` | Figure dimensions            | 80, 24   | `cfg.width = 120;`    |
| `loop`          | Loop animation               | true     | `cfg.loop = false;`   |

```cpp
AnimateConfig cfg;
cfg.x_min = -PI;
cfg.x_max = PI;
cfg.duration = 5.0;
cfg.fps = 60;

animate(cfg,
    std::make_tuple([](double t, double x) { return sin(x + t); }, "red", "wave1"),
    std::make_tuple([](double t, double x) { return cos(x - t); }, "blue", "wave2")
);
```

---

## Render Modes

| Mode                        | Description                | Resolution  | Colors     |
| --------------------------- | -------------------------- | ----------- | ---------- |
| `PlotMode::braille_bw`      | Braille patterns (default) | 8× terminal | B&W        |
| `PlotMode::braille_colored` | Colored braille            | 8× terminal | True color |
| `PlotMode::block_colored`   | Colored blocks             | 2× terminal | True color |

```cpp
Figure fig(80, 40, PlotMode::braille_colored);
fig.plot([](double x) { return sin(x); }, -PI, PI, "red");
fig.show();
```

---

## Color Names

Supported color names: `"black"`, `"red"`, `"green"`, `"yellow"`, `"blue"`, `"magenta"`, `"cyan"`, `"white"`, `"bright_red"`, `"bright_green"`, `"bright_blue"`, `"orange"`, `"purple"`, `"pink"`

Or use empty string `""` for automatic color selection.

---

## Complete Examples

### Example 1: Multiple Functions

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

Figure fig(100, 30);
fig.set_title("Trigonometric Functions");
fig.set_xlabel("x");
fig.set_ylabel("y");

fig.plot([](double x) { return sin(x); }, -PI, PI, "red", "sin(x)");
fig.plot([](double x) { return cos(x); }, -PI, PI, "blue", "cos(x)");
fig.plot([](double x) { return tan(x); }, -PI, PI, "green", "tan(x)");

fig.enable_legend(true);
fig.enable_grid(true);
fig.show();
```

### Example 2: Lissajous Curve (Parametric)

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

parametric(
    [](double t) { return sin(3*t); },  // x(t)
    [](double t) { return sin(2*t); },  // y(t)
    0, 2*PI,
    "magenta"
);
```

### Example 3: Animated Interference Pattern

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

AnimateConfig cfg;
cfg.x_min = -10;
cfg.x_max = 10;
cfg.duration = 8.0;
cfg.fps = 30;
cfg.width = 120;
cfg.height = 40;

animate(cfg,
    std::make_tuple(
        [](double t, double x) { return sin(x - t) + cos(2*x + t); },
        "cyan",
        "interference"
    )
);
```

### Example 4: Using with `var` Lambda

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::vars;
using namespace pythonic::plot;

var f = lambda_(x, pow(x, 2));  // Pythonic var lambda
plot(f, -10, 10, "blue");
```

---

# Next Check

- [Print](../Print/print.md)
