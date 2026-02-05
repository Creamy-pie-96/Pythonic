[⬅ Back to Table of Contents](../index.md)

# Terminal Graphics Engine

A modular SFML-like graphics engine for terminal-based games and applications.

---

## Quick Start

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

int main()
{
    // Create a 160x80 pixel canvas with braille rendering
    Canvas canvas(160, 80, RenderMode::Braille);

    Clock clock;
    while (true)
    {
        // Handle input
        if (Keyboard::isKeyPressed(Key::Escape))
            break;

        // Clear and draw
        canvas.clear(Color::Black);
        canvas.fillCircle(80, 40, 20, Color::Red);
        canvas.display();

        // Control frame rate
        sleep(Time::milliseconds(16));
    }
    return 0;
}
```

### Compilation

```bash
# Without SDL2 audio (uses system commands for audio)
g++ -std=c++20 -Iinclude -o game game.cpp -pthread

# With SDL2 audio (low-latency sound)
g++ -std=c++20 -Iinclude -DPYTHONIC_ENABLE_SDL2_AUDIO -o game game.cpp -lSDL2 -pthread
```

---

## Module Overview

| Module                           | Description                       | Header                  |
| -------------------------------- | --------------------------------- | ----------------------- |
| [Core](Core/core.md)             | Time, Clock, Vector, Rect types   | `Core/Core.hpp`         |
| [Window](Window/window.md)       | Window and event handling         | `Window/Window.hpp`     |
| [Graphics](Graphics/graphics.md) | Canvas, Color, RenderModes        | `Graphics/Graphics.hpp` |
| [View](Graphics/view.md)         | Camera/View, Transform, BlendMode | `Graphics/View.hpp`     |
| [Sprite](Sprite/sprite.md)       | Texture, Sprite, Image loading    | `Sprite/Sprite.hpp`     |
| [Audio](Audio/audio.md)          | Sound, SoundBuffer playback       | `Audio/Audio.hpp`       |
| [Text](Text/text.md)             | Pixel font text rendering         | `Text/Text.hpp`         |
| [Physics](Physics/physics.md)    | Collision detection               | `Physics/Physics.hpp`   |
| [Shapes](Shapes/shapes.md)       | Shape drawing primitives          | `Shapes/Shapes.hpp`     |

---

## Namespace Structure

All TerminalGraphics types are in the `Pythonic::TG` namespace, with `TG` as an alias:

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>

// Either of these works:
using namespace Pythonic::TG;
using namespace TG;

// Now use types directly:
Canvas canvas(160, 80);
Color red(255, 0, 0);
```

---

## Rendering Modes

| Mode                  | Resolution   | Characters | Best For              |
| --------------------- | ------------ | ---------- | --------------------- |
| `RenderMode::Braille` | 2×4 per cell | ⡀⣿         | High detail graphics  |
| `RenderMode::Block`   | 1×2 per cell | ▀▄█        | Solid colors          |
| `RenderMode::Quarter` | 2×2 per cell | ▖▗▘▝       | Medium detail         |
| `RenderMode::ASCII`   | 1×1 per cell | .:-=+\*#@  | Maximum compatibility |

---

## Complete Example: Simple Game Loop

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

int main()
{
    Canvas canvas(200, 100, RenderMode::Braille);
    Clock clock;

    float x = 100, y = 50;
    float vx = 100, vy = 80;

    while (true)
    {
        float dt = clock.restart().asSeconds();

        // Exit on Q
        if (Keyboard::isKeyPressed(Key::Q))
            break;

        // Update position
        x += vx * dt;
        y += vy * dt;

        // Bounce off walls
        if (x < 5 || x > 195) vx = -vx;
        if (y < 5 || y > 95) vy = -vy;

        // Draw
        canvas.clear(Color(20, 20, 40));
        canvas.fillCircle(static_cast<int>(x), static_cast<int>(y), 5, Color::Yellow);
        canvas.display();

        sleep(Time::milliseconds(16));
    }

    return 0;
}
```

---

[Next: Core Module →](Core/core.md)
