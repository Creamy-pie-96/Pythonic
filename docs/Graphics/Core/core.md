[⬅ Back to Graphics Index](../index.md)

# Core Module

Fundamental types for time, vectors, rectangles, and colors.

---

## Quick Start

```cpp
#include <pythonic/TerminalGraphics/Core/Core.hpp>
using namespace Pythonic::TG;

// Time and Clock
Clock clock;
// ... game loop ...
Time elapsed = clock.restart();
float dt = elapsed.asSeconds();

// Vectors
Vector2f position(100.0f, 50.0f);
Vector2i tile(3, 7);

// Rectangles
FloatRect bounds(0, 0, 100, 50);  // x, y, width, height
if (bounds.contains(position))
{
    // Point is inside rectangle
}
```

---

## Time

Represents a time duration. Similar to `sf::Time`.

| Method                      | Description               | Example                            |
| --------------------------- | ------------------------- | ---------------------------------- |
| `Time::Zero`                | Zero duration constant    | `Time t = Time::Zero;`             |
| `Time::seconds(float)`      | Create from seconds       | `Time::seconds(1.5f)`              |
| `Time::milliseconds(int)`   | Create from milliseconds  | `Time::milliseconds(16)`           |
| `Time::microseconds(int64)` | Create from microseconds  | `Time::microseconds(1000)`         |
| `asSeconds()`               | Get as float seconds      | `float s = t.asSeconds();`         |
| `asMilliseconds()`          | Get as int milliseconds   | `int ms = t.asMilliseconds();`     |
| `asMicroseconds()`          | Get as int64 microseconds | `int64_t us = t.asMicroseconds();` |
| `sleep(Time)`               | Sleep for duration        | `sleep(Time::milliseconds(16));`   |

```cpp
Time duration = Time::seconds(2.5f);
std::cout << duration.asMilliseconds();  // 2500

// Sleep for 16ms (60 FPS frame time)
sleep(Time::milliseconds(16));
```

---

## Clock

High-resolution timer for measuring elapsed time.

| Method             | Description                  | Example                            |
| ------------------ | ---------------------------- | ---------------------------------- |
| `Clock()`          | Start a new clock            | `Clock clock;`                     |
| `getElapsedTime()` | Get time since start/restart | `Time t = clock.getElapsedTime();` |
| `restart()`        | Restart and return elapsed   | `Time dt = clock.restart();`       |

```cpp
Clock clock;

while (running)
{
    Time deltaTime = clock.restart();
    float dt = deltaTime.asSeconds();

    // Update game with dt
    player.move(speed * dt);
}
```

---

## Vector2

2D vector template. Common specializations: `Vector2i`, `Vector2f`, `Vector2u`.

| Member | Type | Description |
| ------ | ---- | ----------- |
| `x`    | `T`  | X component |
| `y`    | `T`  | Y component |

| Method/Operator    | Description     | Example               |
| ------------------ | --------------- | --------------------- |
| `Vector2<T>()`     | Default (0, 0)  | `Vector2f v;`         |
| `Vector2<T>(x, y)` | From components | `Vector2f v(10, 20);` |
| `+`, `-`, `*`, `/` | Arithmetic      | `v1 + v2`, `v * 2.0f` |
| `==`, `!=`         | Comparison      | `if (v1 == v2)`       |

```cpp
Vector2f position(100.0f, 50.0f);
Vector2f velocity(10.0f, -5.0f);

// Update position
position = position + velocity * dt;

// Or use +=
position += velocity * dt;

// Integer vectors for grid positions
Vector2i tile(3, 7);
Vector2u size(800, 600);
```

---

## Rect

Axis-aligned rectangle template. Specializations: `IntRect`, `FloatRect`.

| Member   | Type | Description            |
| -------- | ---- | ---------------------- |
| `left`   | `T`  | X position (left edge) |
| `top`    | `T`  | Y position (top edge)  |
| `width`  | `T`  | Width                  |
| `height` | `T`  | Height                 |

| Method                  | Description         | Example                           |
| ----------------------- | ------------------- | --------------------------------- |
| `Rect()`                | Default (0,0,0,0)   | `FloatRect r;`                    |
| `Rect(left, top, w, h)` | From components     | `FloatRect r(10, 20, 100, 50);`   |
| `contains(x, y)`        | Point inside check  | `r.contains(50, 30)`              |
| `contains(Vector2)`     | Point inside check  | `r.contains(point)`               |
| `intersects(other)`     | Overlap check       | `r1.intersects(r2)`               |
| `getPosition()`         | Get top-left corner | `Vector2f pos = r.getPosition();` |
| `getSize()`             | Get width/height    | `Vector2f size = r.getSize();`    |

```cpp
FloatRect playerBounds(50, 100, 32, 48);
FloatRect enemyBounds(60, 110, 32, 48);

// Check collision
if (playerBounds.intersects(enemyBounds))
{
    // Collision detected!
}

// Check if point is inside
Vector2f clickPos(70, 120);
if (playerBounds.contains(clickPos))
{
    // Player was clicked
}
```

---

## Color

RGBA color representation.

| Member | Type      | Range | Description        |
| ------ | --------- | ----- | ------------------ |
| `r`    | `uint8_t` | 0-255 | Red component      |
| `g`    | `uint8_t` | 0-255 | Green component    |
| `b`    | `uint8_t` | 0-255 | Blue component     |
| `a`    | `uint8_t` | 0-255 | Alpha (255=opaque) |

| Constructor         | Description             | Example                    |
| ------------------- | ----------------------- | -------------------------- |
| `Color()`           | Default (black, opaque) | `Color c;`                 |
| `Color(r, g, b)`    | RGB (alpha=255)         | `Color c(255, 0, 0);`      |
| `Color(r, g, b, a)` | RGBA                    | `Color c(255, 0, 0, 128);` |

| Predefined Constant  | Color              |
| -------------------- | ------------------ |
| `Color::Black`       | RGB(0, 0, 0)       |
| `Color::White`       | RGB(255, 255, 255) |
| `Color::Red`         | RGB(255, 0, 0)     |
| `Color::Green`       | RGB(0, 255, 0)     |
| `Color::Blue`        | RGB(0, 0, 255)     |
| `Color::Yellow`      | RGB(255, 255, 0)   |
| `Color::Magenta`     | RGB(255, 0, 255)   |
| `Color::Cyan`        | RGB(0, 255, 255)   |
| `Color::Transparent` | RGBA(0, 0, 0, 0)   |

```cpp
// Named colors
Color red = Color::Red;
Color skyBlue(135, 206, 235);

// With transparency
Color semiTransparent(255, 0, 0, 128);  // 50% transparent red

// Clear canvas to sky blue
canvas.clear(Color(135, 206, 235));
```

---

[⬅ Back to Graphics Index](../index.md) | [Next: Window Module →](../Window/window.md)
