[⬅ Back to Graphics Index](../index.md) | [⬅ Back to Core](../Core/core.md)

# Window & Event Module

Keyboard input and event handling for terminal-based applications.

---

## Quick Start

```cpp
#include <pythonic/TerminalGraphics/Window/Window.hpp>
using namespace Pythonic::TG;

while (running)
{
    // Check if specific key is pressed
    if (Keyboard::isKeyPressed(Key::Escape))
        running = false;

    if (Keyboard::isKeyPressed(Key::Space))
        player.jump();

    if (Keyboard::isKeyPressed(Key::Left))
        player.moveLeft();

    if (Keyboard::isKeyPressed(Key::Right))
        player.moveRight();
}
```

---

## Keyboard

Static class for polling keyboard state. Uses non-blocking terminal input.

| Method               | Description                | Example                              |
| -------------------- | -------------------------- | ------------------------------------ |
| `isKeyPressed(Key)`  | Check if key is pressed    | `Keyboard::isKeyPressed(Key::Space)` |
| `isKeyPressed(char)` | Check if character pressed | `Keyboard::isKeyPressed('a')`        |

```cpp
// Check arrow keys
if (Keyboard::isKeyPressed(Key::Up))
    velocity.y = -100;
if (Keyboard::isKeyPressed(Key::Down))
    velocity.y = 100;

// Check character keys
if (Keyboard::isKeyPressed('p'))
    togglePause();

// Multiple keys at once
if (Keyboard::isKeyPressed(Key::Left) && Keyboard::isKeyPressed(Key::LShift))
    player.sprint(-1);
```

---

## Key Enum

Keyboard key constants.

### Arrow Keys

| Key          | Description |
| ------------ | ----------- |
| `Key::Up`    | Up arrow    |
| `Key::Down`  | Down arrow  |
| `Key::Left`  | Left arrow  |
| `Key::Right` | Right arrow |

### Special Keys

| Key              | Description  |
| ---------------- | ------------ |
| `Key::Escape`    | Escape key   |
| `Key::Space`     | Spacebar     |
| `Key::Enter`     | Enter/Return |
| `Key::Backspace` | Backspace    |
| `Key::Tab`       | Tab key      |
| `Key::Delete`    | Delete key   |
| `Key::Insert`    | Insert key   |
| `Key::Home`      | Home key     |
| `Key::End`       | End key      |
| `Key::PageUp`    | Page Up      |
| `Key::PageDown`  | Page Down    |

### Modifier Keys

| Key             | Description   |
| --------------- | ------------- |
| `Key::LShift`   | Left Shift    |
| `Key::RShift`   | Right Shift   |
| `Key::LControl` | Left Control  |
| `Key::RControl` | Right Control |
| `Key::LAlt`     | Left Alt      |
| `Key::RAlt`     | Right Alt     |

### Letter Keys

| Key                 | Description         |
| ------------------- | ------------------- |
| `Key::A` - `Key::Z` | Letters A through Z |

### Number Keys

| Key                       | Description     |
| ------------------------- | --------------- |
| `Key::Num0` - `Key::Num9` | Top row numbers |

### Function Keys

| Key                    | Description   |
| ---------------------- | ------------- |
| `Key::F1` - `Key::F12` | Function keys |

---

## One-Shot Input Pattern

For actions that should trigger once per key press (not repeat while held):

```cpp
class Game
{
    bool m_spaceWasPressed = false;  // Track previous state

    void handleInput()
    {
        bool spaceNow = Keyboard::isKeyPressed(Key::Space);

        // Only jump on initial press (not hold)
        if (spaceNow && !m_spaceWasPressed)
        {
            player.jump();
        }

        m_spaceWasPressed = spaceNow;
    }
};
```

---

## Terminal Resize Detection

The Canvas module provides terminal resize detection:

```cpp
Canvas canvas(160, 80);

while (running)
{
    // Check if terminal was resized
    if (Canvas::wasResized())
    {
        unsigned int newWidth = Canvas::getTermWidth();
        unsigned int newHeight = Canvas::getTermHeight();

        // Recreate canvas at new size
        canvas = Canvas(newWidth * 2, newHeight * 4, RenderMode::Braille);
    }

    // ... rest of game loop
}
```

---

## Complete Input Example

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

int main()
{
    Canvas canvas(160, 80, RenderMode::Braille);
    Clock clock;

    float x = 80, y = 40;
    float speed = 100.0f;
    bool running = true;

    while (running)
    {
        float dt = clock.restart().asSeconds();

        // Handle input
        if (Keyboard::isKeyPressed(Key::Escape) || Keyboard::isKeyPressed(Key::Q))
            running = false;

        if (Keyboard::isKeyPressed(Key::Up) || Keyboard::isKeyPressed(Key::W))
            y -= speed * dt;
        if (Keyboard::isKeyPressed(Key::Down) || Keyboard::isKeyPressed(Key::S))
            y += speed * dt;
        if (Keyboard::isKeyPressed(Key::Left) || Keyboard::isKeyPressed(Key::A))
            x -= speed * dt;
        if (Keyboard::isKeyPressed(Key::Right) || Keyboard::isKeyPressed(Key::D))
            x += speed * dt;

        // Sprint with shift
        if (Keyboard::isKeyPressed(Key::LShift))
            speed = 200.0f;
        else
            speed = 100.0f;

        // Keep in bounds
        x = std::clamp(x, 5.0f, 155.0f);
        y = std::clamp(y, 5.0f, 75.0f);

        // Draw
        canvas.clear(Color(20, 20, 40));
        canvas.fillCircle(static_cast<int>(x), static_cast<int>(y), 5, Color::Cyan);
        canvas.display();

        sleep(Time::milliseconds(16));
    }

    return 0;
}
```

---

[⬅ Back to Core](../Core/core.md) | [Next: Graphics Module →](../Graphics/graphics.md)
