[⬅ Back to Graphics Index](../index.md) | [⬅ Back to Window](../Window/window.md)

# Graphics Module

Canvas rendering, colors, and drawing primitives for terminal graphics.

---

## Quick Start

```cpp
#include <pythonic/TerminalGraphics/Graphics/Graphics.hpp>
using namespace Pythonic::TG;

// Create a canvas (pixel buffer)
Canvas canvas(160, 80, RenderMode::Braille);

// Clear with background color
canvas.clear(Color(30, 30, 50));

// Draw shapes
canvas.setPixel(10, 10, Color::White);
canvas.drawLine(0, 0, 100, 50, Color::Red);
canvas.fillRect(50, 20, 30, 20, Color::Green);
canvas.fillCircle(80, 40, 15, Color::Yellow);

// Display to terminal
canvas.display();
```

---

## Canvas

The main drawing surface. Pixels are rendered to terminal using Unicode characters.

### Constructor

| Constructor                   | Description                      |
| ----------------------------- | -------------------------------- |
| `Canvas(width, height)`       | Create with Block render mode    |
| `Canvas(width, height, mode)` | Create with specific render mode |

### Render Modes

| Mode                  | Resolution per Cell | Characters | Description                  |
| --------------------- | ------------------- | ---------- | ---------------------------- |
| `RenderMode::Braille` | 2×4 pixels          | ⡀⣿         | Highest detail, 24-bit color |
| `RenderMode::Block`   | 1×2 pixels          | ▀▄█        | Good for solid shapes        |
| `RenderMode::Quarter` | 2×2 pixels          | ▖▗▘▝       | Medium detail                |
| `RenderMode::ASCII`   | 1×1 pixel           | .:-=+\*#@  | Most compatible              |

```cpp
// High detail for graphics
Canvas hd(320, 160, RenderMode::Braille);

// Solid colors
Canvas solid(80, 40, RenderMode::Block);

// Maximum compatibility
Canvas ascii(80, 40, RenderMode::ASCII);
```

---

### Drawing Methods

| Method                  | Description       | Example                                |
| ----------------------- | ----------------- | -------------------------------------- |
| `clear()`               | Clear to black    | `canvas.clear();`                      |
| `clear(color)`          | Clear to color    | `canvas.clear(Color::Blue);`           |
| `setPixel(x, y, color)` | Set single pixel  | `canvas.setPixel(10, 20, Color::Red);` |
| `getPixel(x, y)`        | Get pixel color   | `Color c = canvas.getPixel(10, 20);`   |
| `getWidth()`            | Get canvas width  | `unsigned w = canvas.getWidth();`      |
| `getHeight()`           | Get canvas height | `unsigned h = canvas.getHeight();`     |

### Line Drawing

| Method                                       | Description              |
| -------------------------------------------- | ------------------------ |
| `drawLine(x1, y1, x2, y2, color)`            | Draw line between points |
| `drawLine(x1, y1, x2, y2, color, thickness)` | Draw thick line          |

```cpp
canvas.drawLine(0, 0, 100, 50, Color::White);
canvas.drawLine(10, 10, 90, 10, Color::Red, 3);  // 3px thick
```

### Rectangle Drawing

| Method                        | Description            |
| ----------------------------- | ---------------------- |
| `drawRect(x, y, w, h, color)` | Draw rectangle outline |
| `fillRect(x, y, w, h, color)` | Draw filled rectangle  |

```cpp
canvas.drawRect(10, 10, 50, 30, Color::White);      // Outline
canvas.fillRect(20, 20, 30, 20, Color(0, 100, 200)); // Filled
```

### Circle Drawing

| Method                         | Description         |
| ------------------------------ | ------------------- |
| `drawCircle(cx, cy, r, color)` | Draw circle outline |
| `fillCircle(cx, cy, r, color)` | Draw filled circle  |

```cpp
canvas.drawCircle(80, 40, 20, Color::White);   // Outline
canvas.fillCircle(80, 40, 15, Color::Yellow);  // Filled
```

---

### Display Methods

| Method      | Description                                |
| ----------- | ------------------------------------------ |
| `display()` | Render to terminal (with cleanup handlers) |
| `render()`  | Get render string without displaying       |

```cpp
// Normal usage - displays and handles terminal cleanup
canvas.display();

// Get the rendered string (for custom output)
std::string output = canvas.render();
```

---

### Terminal Utilities (Static)

| Method                    | Description                   |
| ------------------------- | ----------------------------- |
| `Canvas::wasResized()`    | Check if terminal was resized |
| `Canvas::getTermWidth()`  | Get terminal width in columns |
| `Canvas::getTermHeight()` | Get terminal height in rows   |

```cpp
if (Canvas::wasResized())
{
    // Terminal was resized, recreate canvas
    unsigned int cols = Canvas::getTermWidth();
    unsigned int rows = Canvas::getTermHeight();

    // Braille: 2 pixels per column, 4 pixels per row
    canvas = Canvas(cols * 2, rows * 4, RenderMode::Braille);
}
```

---

## Color

RGBA color type. See [Core Module](../Core/core.md#color) for full documentation.

| Predefined           | RGB Value       |
| -------------------- | --------------- |
| `Color::Black`       | (0, 0, 0)       |
| `Color::White`       | (255, 255, 255) |
| `Color::Red`         | (255, 0, 0)     |
| `Color::Green`       | (0, 255, 0)     |
| `Color::Blue`        | (0, 0, 255)     |
| `Color::Yellow`      | (255, 255, 0)   |
| `Color::Magenta`     | (255, 0, 255)   |
| `Color::Cyan`        | (0, 255, 255)   |
| `Color::Transparent` | (0, 0, 0, 0)    |

---

## RenderTarget

Abstract base for drawable targets. Canvas inherits from RenderTarget.

| Method           | Description            |
| ---------------- | ---------------------- |
| `draw(drawable)` | Draw a Drawable object |
| `clear(color)`   | Clear to color         |
| `getSize()`      | Get target size        |

```cpp
void drawEntity(RenderTarget& target, const Entity& entity)
{
    Sprite sprite(entity.texture);
    sprite.setPosition(entity.x, entity.y);
    target.draw(sprite);
}
```

---

## Complete Example

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

int main()
{
    Canvas canvas(200, 100, RenderMode::Braille);
    Clock clock;

    float angle = 0;

    while (!Keyboard::isKeyPressed(Key::Escape))
    {
        float dt = clock.restart().asSeconds();
        angle += dt * 2;  // Rotate 2 radians/sec

        canvas.clear(Color(20, 20, 40));

        // Draw rotating pattern
        int cx = 100, cy = 50;
        for (int i = 0; i < 8; ++i)
        {
            float a = angle + i * 3.14159f / 4;
            int x = cx + static_cast<int>(40 * std::cos(a));
            int y = cy + static_cast<int>(30 * std::sin(a));

            canvas.drawLine(cx, cy, x, y, Color(255, 128, i * 32));
            canvas.fillCircle(x, y, 5, Color(i * 32, 255, 128));
        }

        canvas.display();
        sleep(Time::milliseconds(16));
    }

    return 0;
}
```

---

[⬅ Back to Window](../Window/window.md) | [Next: Sprite Module →](../Sprite/sprite.md)
