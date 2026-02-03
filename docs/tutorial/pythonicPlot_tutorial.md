[⬅ Back to Table of Contents](../index.md)

# 📊 pythonicPlot Tutorial - Terminal Graphing from Scratch

**pythonicPlot.hpp** provides matplotlib-style plotting for terminal graphics. This tutorial explains how it works, from basic concepts to advanced animations.

---

## 📚 Table of Contents

1. [What is Braille Plotting?](#1-what-is-braille-plotting)
2. [The Figure Class](#2-the-figure-class)
3. [Rendering Modes](#3-rendering-modes)
4. [Animation System](#4-animation-system)
5. [Color System](#5-color-system)
6. [Practical Examples](#6-practical-examples)

---

## 1. What is Braille Plotting?

Terminal plotting uses Unicode Braille characters (U+2800–U+28FF) for high-resolution graphics. Each terminal character represents a 2×4 pixel grid.

```
Terminal: 80 columns × 24 rows
Plot Area: 160 pixels × 96 pixels (8× resolution!)

Single Braille character: ⣿
Represents 8 pixels in 2×4 grid:
  ●●
  ●●
  ●●
  ●●
```

**Why Braille for plots?**

| Method            | Resolution       | Best For             |
| ----------------- | ---------------- | -------------------- |
| ASCII (`*`, `.`)  | 1×1 per char     | Simple scatter plots |
| Blocks (`▀`, `▄`) | 1×2 per char     | Bar charts           |
| **Braille** (`⠿`) | **2×4 per char** | **Smooth curves**    |

---

## 2. The Figure Class

The `Figure` class is like matplotlib's `Figure` - it manages multiple plots, axes, and rendering.

### Creating a Figure

| Code                   | Dimensions        | Resolution     |
| ---------------------- | ----------------- | -------------- |
| `Figure fig(80, 24);`  | 80×24 characters  | 160×96 pixels  |
| `Figure fig(120, 40);` | 120×40 characters | 240×160 pixels |

### Adding Plots

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

Figure fig(100, 30);

// Plot a function
fig.plot([](double x) { return sin(x); }, -PI, PI, "red", "sin(x)");

// Add another function
fig.plot([](double x) { return cos(x); }, -PI, PI, "blue", "cos(x)");

// Configure and show
fig.set_title("Trigonometric Functions");
fig.enable_legend(true);
fig.show();
```

**Visual Output:**

```
┌────────────────────────────────────────────────┐
│ Trigonometric Functions                        │
├────────────────────────────────────────────────┤
│                                                │
│  1 ┤     ⡠⠊⠉⠉⢉⣉⣉⠉⠑⠢⡀                          │
│    │   ⢀⠎        ⢀⡠⠊⠑⠢⡀  ⠈⢢                        │
│  0 ┤  ⢰            ⢸      ⠱⡀ ⡇                       │
│    │ ⡜              ⡇      ⠈⢆⠸                       │
│ -1 ┤⡎                ⢧       ⠈⣇                      │
│    └────────────────────────────────────────────│
│    -3.14        0         3.14                 │
│                                                │
│  ⬛ sin(x)   ⬛ cos(x)                           │
└────────────────────────────────────────────────┘
```

### Key Figure Methods

| Method                                         | Purpose              | Example                                                                                      |
| ---------------------------------------------- | -------------------- | -------------------------------------------------------------------------------------------- |
| `plot(f, xmin, xmax, color, label)`            | Add function plot    | `fig.plot([](double x) { return x*x; }, -10, 10);`                                           |
| `scatter(x_data, y_data, color, label)`        | Add scatter plot     | `fig.scatter({1,2,3}, {1,4,9}, "green");`                                                    |
| `parametric(fx, fy, tmin, tmax, color, label)` | Add parametric curve | `fig.parametric([](double t) { return cos(t); }, [](double t) { return sin(t); }, 0, 2*PI);` |
| `show()`                                       | Display the figure   | `fig.show();`                                                                                |
| `clear()`                                      | Remove all plots     | `fig.clear();`                                                                               |
| `set_xlim(min, max)`                           | Set x-axis range     | `fig.set_xlim(-5, 5);`                                                                       |
| `set_ylim(min, max)`                           | Set y-axis range     | `fig.set_ylim(0, 100);`                                                                      |

---

## 3. Rendering Modes

### PlotMode Enum

| Mode                        | Resolution | Colors     | Visual Quality             |
| --------------------------- | ---------- | ---------- | -------------------------- |
| `PlotMode::braille_bw`      | 8× (2×4)   | B&W        | High detail, crisp lines   |
| `PlotMode::braille_colored` | 8× (2×4)   | True color | High detail + color        |
| `PlotMode::block_colored`   | 2× (1×2)   | True color | Lower detail, better color |

**Example:**

```cpp
// Black & white braille (default, best detail)
Figure fig1(80, 30, PlotMode::braille_bw);
fig1.plot([](double x) { return sin(x); }, -PI, PI);
fig1.show();

// Colored braille (detail + color)
Figure fig2(80, 30, PlotMode::braille_colored);
fig2.plot([](double x) { return sin(x); }, -PI, PI, "red");
fig2.plot([](double x) { return cos(x); }, -PI, PI, "blue");
fig2.show();

// Colored blocks (best color clarity)
Figure fig3(80, 30, PlotMode::block_colored);
fig3.plot([](double x) { return sin(x); }, -PI, PI, "magenta");
fig3.show();
```

**Mode Comparison:**

```
braille_bw:        braille_colored:     block_colored:
⣿⣿⡟⠛⠛⠛⠻⣿⣿⣿          ⣿⣿⡟⠛⠛⠛⠻⣿⣿⣿          ▓▓█▄▄▄██▓▓
⡟⠁      ⠈⢻⣇         ⡟⠁      ⠈⢻⣇         █       ██
⡇        ⢸⡇         ⡇        ⢸⡇         █      ▐█
⠁        ⠈⠃         ⠁        ⠈⠃         ▀      ▀▘

B&W, crisp      Color, crisp      Color, smooth
```

---

## 4. Animation System

Animations use a time variable `t` that changes over time. The function signature is `f(t, x)` where `t` is time and `x` is the x-coordinate.

### Simple Animation

```cpp
using namespace pythonic::plot;

// Create a traveling sine wave
animate([](double t, double x) {
    return sin(x + t);  // Wave moves with time
}, -PI, PI);
```

**What happens:**

```
t = 0.0s:    t = 0.5s:    t = 1.0s:
  ⠤⠤⢤⠤⠤          ⢤⠤⠤⠤⠤         ⠤⠤⠤⠤⢤
 ⡎    ⢱        ⡎    ⢱       ⡎    ⢱
⡎      ⢱      ⡎      ⢱     ⡎      ⢱
```

### AnimateConfig

Control animation parameters:

| Field           | Default  | Description                | Example               |
| --------------- | -------- | -------------------------- | --------------------- |
| `x_min, x_max`  | Required | Plot x-axis range          | `cfg.x_min = -10;`    |
| `duration`      | 10.0     | Animation length (seconds) | `cfg.duration = 5.0;` |
| `fps`           | 30       | Frames per second          | `cfg.fps = 60;`       |
| `width, height` | 80, 24   | Terminal dimensions        | `cfg.width = 120;`    |
| `loop`          | true     | Loop animation             | `cfg.loop = false;`   |

**Example:**

```cpp
AnimateConfig cfg;
cfg.x_min = -2*PI;
cfg.x_max = 2*PI;
cfg.duration = 8.0;  // 8-second loop
cfg.fps = 60;        // Smooth 60 FPS
cfg.width = 120;     // Wide plot
cfg.height = 40;

animate(cfg,
    std::make_tuple(
        [](double t, double x) { return sin(x - t); },
        "cyan",
        "Wave 1"
    ),
    std::make_tuple(
        [](double t, double x) { return cos(x + t); },
        "magenta",
        "Wave 2"
    )
);
```

### How Animation Works

1. **Time Loop** - `t` increments based on `fps` and real time
2. **Function Evaluation** - For each `t`, compute `y = f(t, x)` for all x values
3. **Render Frame** - Draw the curve using Braille/blocks
4. **Display** - Clear screen with `\033[H`, print new frame
5. **Repeat** - Continue until duration elapsed or Ctrl+C

```cpp
// Pseudo-code of animation loop
for (t = 0; t < duration; t += 1.0/fps) {
    fig.clear();
    fig.set_time(t);

    for (x = x_min; x <= x_max; x += step) {
        y = f(t, x);
        plot_point(x, y);
    }

    render_to_terminal();
    sleep(1.0/fps);
}
```

---

## 5. Color System

### Supported Colors

| Color Name  | RGB             | Visual                                |
| ----------- | --------------- | ------------------------------------- |
| `"red"`     | (255, 0, 0)     | <span style="color:red">⬛</span>     |
| `"green"`   | (0, 255, 0)     | <span style="color:green">⬛</span>   |
| `"blue"`    | (0, 0, 255)     | <span style="color:blue">⬛</span>    |
| `"yellow"`  | (255, 255, 0)   | <span style="color:yellow">⬛</span>  |
| `"cyan"`    | (0, 255, 255)   | <span style="color:cyan">⬛</span>    |
| `"magenta"` | (255, 0, 255)   | <span style="color:magenta">⬛</span> |
| `"white"`   | (255, 255, 255) | ⬛                                    |
| `"black"`   | (0, 0, 0)       | ⬜                                    |
| `"orange"`  | (255, 165, 0)   | <span style="color:orange">⬛</span>  |
| `"purple"`  | (128, 0, 128)   | <span style="color:purple">⬛</span>  |
| `"pink"`    | (255, 192, 203) | <span style="color:pink">⬛</span>    |

### How Colors are Applied

In **colored modes**, each Braille cell or block gets a foreground color:

```cpp
// Colored braille mode
Figure fig(80, 30, PlotMode::braille_colored);
fig.plot([](double x) { return sin(x); }, -PI, PI, "red");

// Internally:
// For each pixel at (x, y):
//   1. Compute which Braille cell it belongs to
//   2. Set that cell's bit pattern
//   3. Set that cell's foreground color to red
//   4. Render: ANSI_COLOR(red) + BRAILLE_CHAR + ANSI_RESET
```

**ANSI Color Codes:**

```
Red:     \033[38;2;255;0;0m
Green:   \033[38;2;0;255;0m
Blue:    \033[38;2;0;0;255m
Reset:   \033[0m

Output: "\033[38;2;255;0;0m⣿\033[0m"  → red braille character
```

---

## 6. Practical Examples

### Example 1: Comparing Functions

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

Figure fig(120, 35);
fig.set_title("Polynomial Comparison");
fig.set_xlabel("x");
fig.set_ylabel("y");

// Plot different polynomials
fig.plot([](double x) { return x; }, -5, 5, "blue", "y = x");
fig.plot([](double x) { return x*x; }, -5, 5, "green", "y = x²");
fig.plot([](double x) { return x*x*x; }, -5, 5, "red", "y = x³");

fig.enable_legend(true);
fig.enable_grid(true);
fig.show();
```

### Example 2: Lissajous Curves (Parametric)

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

Figure fig(80, 40);
fig.set_title("Lissajous Curve: a=3, b=2");

// Parametric: x(t) = sin(3t), y(t) = sin(2t)
fig.parametric(
    [](double t) { return sin(3*t); },  // x(t)
    [](double t) { return sin(2*t); },  // y(t)
    0, 2*PI,
    "magenta",
    "3:2"
);

fig.show();
```

**Visual Output:**

```
    ⢀⣠⠴⠒⠒⠒⠒⠤⣄
  ⢀⡞⠁        ⠈⢳⡀
 ⢀⠎            ⠈⢣
⢰⠁              ⠈⢧
⡇                ⢸
⢧                ⡼
 ⠱⡀            ⢀⠜
  ⠈⢦⡀        ⢀⡴⠁
    ⠉⠒⠤⠤⠤⠤⠒⠉
```

### Example 3: Wave Interference Animation

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

AnimateConfig cfg;
cfg.x_min = -10;
cfg.x_max = 10;
cfg.duration = 10.0;
cfg.fps = 30;
cfg.width = 120;
cfg.height = 40;

// Two waves interfering
animate(cfg,
    std::make_tuple(
        [](double t, double x) {
            return sin(x - t) + sin(2*x + t);
        },
        "cyan",
        "Interference Pattern"
    )
);
```

**What you see:**

```
Frame 1:         Frame 2:         Frame 3:
⠤⢤⡤⢤⡤⢤⡤⢤⡤     ⢤⡤⢤⡤⢤⡤⢤⡤⢤     ⡤⢤⡤⢤⡤⢤⡤⢤⡤⢤
 ⡇ ⡇ ⡇ ⡇ ⡇      ⡇ ⡇ ⡇ ⡇ ⡇       ⡇ ⡇ ⡇ ⡇ ⡇
⡇ ⢸⡇ ⢸⡇ ⢸⡇ ⢸    ⡇ ⢸⡇ ⢸⡇ ⢸⡇     ⢸⡇ ⢸⡇ ⢸⡇ ⢸⡇
(Wave moves →)   (Continues →)   (Pattern shifts →)
```

### Example 4: Scatter Plot from Data

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::plot;

// Generate some data
std::vector<double> x_data, y_data;
for (int i = 0; i < 50; i++) {
    double x = i * 0.1;
    double y = sin(x) + (rand() % 20 - 10) * 0.05;  // Noisy sine
    x_data.push_back(x);
    y_data.push_back(y);
}

Figure fig(100, 30);
fig.set_title("Noisy Data");
fig.scatter(x_data, y_data, "green", "measurements");
fig.enable_grid(true);
fig.show();
```

### Example 5: Using `var` Lambdas

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic::vars;
using namespace pythonic::plot;

// Define function as var lambda
var f = lambda_(x, pow(x, 2));

// Plot it
plot(f, -10, 10, "blue", 120, 40);
```

---

## 📊 Performance Tips

| Scenario                   | Recommendation              | Reason                              |
| -------------------------- | --------------------------- | ----------------------------------- |
| Many data points (>1000)   | Use `braille_bw` mode       | Faster rendering, no color overhead |
| Animation at high FPS      | Reduce `width` and `height` | Fewer pixels to compute             |
| Complex functions          | Pre-compute values in array | Avoid repeated function calls       |
| Multiple overlapping plots | Use `braille_colored`       | Colors help distinguish curves      |

---

## 🎨 Best Practices

1. **Choose appropriate ranges** - Make sure x/y limits show interesting part of function
2. **Use descriptive labels** - Helps readers understand the plot
3. **Enable grid for data plots** - Grid lines help read values
4. **Limit animation duration** - Long animations can be tiresome, 5-10s is good
5. **Test different modes** - Different rendering modes suit different data

---

## 🔧 Troubleshooting

| Problem             | Solution                                |
| ------------------- | --------------------------------------- |
| Plot looks squashed | Increase `height` parameter             |
| Curves not smooth   | Use Braille mode instead of blocks      |
| Animation too slow  | Reduce FPS or plot dimensions           |
| Colors not showing  | Check terminal supports 24-bit color    |
| Out of range errors | Check x_min < x_max and sensible limits |

---

[⬅ Back to Table of Contents](../index.md)
