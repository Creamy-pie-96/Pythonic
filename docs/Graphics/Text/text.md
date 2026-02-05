[⬅ Back to Graphics Index](../index.md) | [⬅ Back to Audio](../Audio/audio.md)

# Text Module

Text rendering for terminal graphics using a built-in 3x5 pixel font.

---

## Quick Start

```cpp
#include <pythonic/TerminalGraphics/Text/Text.hpp>
using namespace Pythonic::TG;

Canvas canvas(160, 80, RenderMode::Braille);

// Simple text
Text::draw(canvas, "Hello World", 10, 5, Color::White);

// Styled text
Text::draw(canvas, "Score: 100", 10, 10, Color::Yellow);

// Centered text
Text::drawCentered(canvas, "GAME OVER", canvas.getSize().x/2, 40, Color::Red);

canvas.display();
```

---

## Text Class

Static functions for drawing text to any RenderTarget.

### Basic Drawing

| Method                            | Description           |
| --------------------------------- | --------------------- |
| `draw(target, text, x, y, color)` | Draw text at position |

```cpp
Text::draw(canvas, "Top Left", 0, 0, Color::White);
Text::draw(canvas, "Red Text", 0, 5, Color::Red);

// With variables
int score = 42;
Text::draw(canvas, "Score: " + std::to_string(score), 0, 10, Color::White);
```

### Centered Drawing

| Method                                          | Description          |
| ----------------------------------------------- | -------------------- |
| `drawCentered(target, text, centerX, y, color)` | Draw centered on x,y |

```cpp
int centerX = canvas.getSize().x / 2;
Text::drawCentered(canvas, "TITLE", centerX, 10, Color::Cyan);
```

### Right-Aligned Drawing

| Method                                      | Description        |
| ------------------------------------------- | ------------------ |
| `drawRight(target, text, rightX, y, color)` | Draw right-aligned |

```cpp
Text::drawRight(canvas, "9999", canvas.getSize().x - 5, 5, Color::Yellow);
```

### With Effects

| Method                                                                  | Description              |
| ----------------------------------------------------------------------- | ------------------------ |
| `drawWithShadow(target, text, x, y, fg, shadow)`                        | Text with drop shadow    |
| `drawWithBackground(target, text, x, y, fg, bg, padding)`               | Text with background     |
| `drawCenteredWithBackground(target, text, centerX, y, fg, bg, padding)` | Centered with background |

```cpp
// Drop shadow effect
Text::drawWithShadow(canvas, "SCORE", 10, 10, Color::White, Color::Black);

// Text on background (like a button)
Text::drawWithBackground(canvas, " PLAY ", 50, 30, Color::Black, Color::Green);

// Centered with background
Text::drawCenteredWithBackground(canvas, "PAUSED", 80, 40, Color::White, Color::Blue, 2);
```

### Size Queries

| Method              | Return | Description                |
| ------------------- | ------ | -------------------------- |
| `Text::width(text)` | `int`  | Get text width in pixels   |
| `Text::height()`    | `int`  | Get text height (5 pixels) |

```cpp
int w = Text::width("Hello");  // Width in pixels
int h = Text::height();        // 5 pixels
```

---

## Font Namespace

Low-level access to the built-in pixel font.

| Function                 | Return                         | Description            |
| ------------------------ | ------------------------------ | ---------------------- |
| `font::getDefaultFont()` | `const map<char, Glyph>&`      | Get 3x5 glyph map      |
| `font::textWidth(text)`  | `int`                          | Text width in pixels   |
| `font::textHeight()`     | `int`                          | Text height (5 pixels) |
| `font::getLargeFont()`   | `const map<char, LargeGlyph>&` | Get 5x7 font           |

```cpp
// Character dimensions: 3 pixels wide, 5 pixels tall, 1 pixel spacing
int width = font::textWidth("Hello");  // (5 chars * 4) - 1 = 19 pixels
int height = font::textHeight();       // 5 pixels
```

---

## Text Positioning

Each character takes 4 pixels width (3 + 1 spacing).

```cpp
// Draw multiple items aligned
Text::draw(canvas, "HP:", 10, 10, Color::White);
int afterHP = 10 + font::textWidth("HP: ");
Text::draw(canvas, "100/100", afterHP, 10, Color::Green);

// Right-align using drawRight
Text::drawRight(canvas, "9999", canvas.getSize().x - 5, 5, Color::Yellow);
```

---

## Multi-line Text

For multi-line text, calculate y offset:

```cpp
int lineHeight = font::textHeight() + 2;  // 5 + 2 = 7 pixel spacing

std::vector<std::string> lines = {
    "Line 1",
    "Line 2",
    "Line 3"
};

for (size_t i = 0; i < lines.size(); ++i)
{
    Text::draw(canvas, lines[i], 10, 10 + i * lineHeight, Color::White);
}
```

---

## Complete Example: HUD Display

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

void drawHUD(Canvas& canvas, int score, int health, float time)
{
    // Top bar
    Text::draw(canvas, "SCORE:", 5, 2, Color::White);
    Text::draw(canvas, std::to_string(score), 40, 2, Color::Yellow);

    // Health bar
    Text::draw(canvas, "HP:", 5, 10, Color::White);
    for (int i = 0; i < 10; ++i)
    {
        Color c = (i < health) ? Color::Green : Color(50, 50, 50);
        canvas.fillRect(25 + i * 6, 9, 5, 6, c);
    }

    // Timer (right-aligned)
    std::string timeStr = std::to_string(static_cast<int>(time)) + "s";
    Text::drawRight(canvas, timeStr, canvas.getSize().x - 5, 2, Color::Cyan);

    // Center message
    if (health <= 0)
    {
        int centerX = canvas.getSize().x / 2;
        int centerY = canvas.getSize().y / 2;

        Text::drawCenteredWithBackground(canvas, " GAME OVER ",
            centerX, centerY, Color::White, Color::Red, 2);
        Text::drawCentered(canvas, "Press R to Restart",
            centerX, centerY + 15, Color::White);
    }
}

int main()
{
    Canvas canvas(160, 80, RenderMode::Braille);

    int score = 0;
    int health = 10;
    Clock clock;

    while (!Keyboard::isKeyPressed(Key::Escape))
    {
        float time = clock.getElapsedTime().asSeconds();

        // Demo: decrease health over time
        health = 10 - static_cast<int>(time / 2);
        if (health < 0) health = 0;

        // Demo: increase score
        score = static_cast<int>(time * 10);

        canvas.clear(Color(20, 20, 30));
        drawHUD(canvas, score, health, time);
        canvas.display();

        sleep(Time::milliseconds(50));
    }

    return 0;
}
```

---

## Tips

### Performance

- Text drawing is fast for typical game UIs
- For static text, consider drawing once and caching

### Supported Characters

- ASCII printable characters (32-126)
- Letters A-Z, a-z
- Numbers 0-9
- Common punctuation

### Colors

- Text uses the same Color class as graphics
- All predefined colors work (Color::Red, Color::White, etc.)

---

[⬅ Back to Audio](../Audio/audio.md) | [Next: Physics Module →](../Physics/physics.md)
