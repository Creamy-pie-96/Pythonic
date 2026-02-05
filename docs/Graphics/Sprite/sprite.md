[⬅ Back to Graphics Index](../index.md) | [⬅ Back to Graphics](../Graphics/graphics.md)

# Sprite Module

Textures, sprites, and image loading for terminal graphics.

---

## Quick Start

```cpp
#include <pythonic/TerminalGraphics/Sprite/Sprite.hpp>
using namespace Pythonic::TG;

// Load a texture from file
Texture texture;
if (texture.loadFromFile("player.png"))
{
    // Create sprite from texture
    Sprite sprite(texture);
    sprite.setPosition(100, 50);
    sprite.setScale(2.0f, 2.0f);  // Double size

    // Draw to canvas
    canvas.draw(sprite);
}
```

---

## Image

Raw image data container. Supports loading from PPM, PNG (via ImageMagick), and direct pixel manipulation.

### Constructor

| Constructor                   | Description               |
| ----------------------------- | ------------------------- |
| `Image()`                     | Create empty image        |
| `Image(width, height)`        | Create with size (black)  |
| `Image(width, height, color)` | Create with size and fill |

### Loading/Saving

| Method                | Return | Description                     |
| --------------------- | ------ | ------------------------------- |
| `loadFromFile(path)`  | `bool` | Load image file (PNG, PPM, JPG) |
| `saveToFile(path)`    | `bool` | Save as PPM file                |
| `create(w, h)`        | —      | Create empty image              |
| `create(w, h, color)` | —      | Create filled image             |

```cpp
Image img;
img.loadFromFile("sprite.png");  // Loads PNG with transparency

// Save as PPM
img.saveToFile("output.ppm");
```

### Pixel Access

| Method                  | Return     | Description     |
| ----------------------- | ---------- | --------------- |
| `getPixel(x, y)`        | `Color`    | Get pixel color |
| `setPixel(x, y, color)` | —          | Set pixel color |
| `getSize()`             | `Vector2u` | Get dimensions  |

```cpp
Color c = img.getPixel(10, 20);
img.setPixel(10, 20, Color::Red);

Vector2u size = img.getSize();  // {width, height}
```

### Image Operations

| Method                                | Description                      |
| ------------------------------------- | -------------------------------- |
| `copy(source, destX, destY)`          | Copy another image onto this one |
| `copy(source, destX, destY, srcRect)` | Copy portion of image            |
| `flipHorizontally()`                  | Flip left-right                  |
| `flipVertically()`                    | Flip top-bottom                  |

```cpp
Image combined;
combined.create(200, 100);
combined.copy(sprite1, 0, 0);
combined.copy(sprite2, 100, 0);
```

---

## Texture

Image wrapper for sprite rendering. Like `sf::Texture`.

### Loading

| Method                       | Return | Description            |
| ---------------------------- | ------ | ---------------------- |
| `create(w, h)`               | `bool` | Create empty texture   |
| `loadFromFile(path)`         | `bool` | Load from image file   |
| `loadFromImage(image)`       | `bool` | Load from Image object |
| `loadFromImage(image, rect)` | `bool` | Load portion of Image  |

```cpp
Texture tex;
tex.loadFromFile("character.png");

// Load from Image
Image img;
img.loadFromFile("spritesheet.png");
tex.loadFromImage(img, IntRect(0, 0, 32, 32));  // First frame
```

### Properties

| Method                       | Return     | Description                              |
| ---------------------------- | ---------- | ---------------------------------------- |
| `getSize()`                  | `Vector2u` | Get texture dimensions                   |
| `getPixel(x, y)`             | `Color`    | Get pixel color                          |
| `getOpaqueBounds(threshold)` | `IntRect`  | Get bounds of opaque pixels              |
| `setSmooth(bool)`            | —          | Enable smoothing (no effect in terminal) |
| `setRepeated(bool)`          | —          | Enable texture repeating                 |

```cpp
Vector2u size = texture.getSize();

// For accurate collision detection with transparent sprites
IntRect opaque = texture.getOpaqueBounds(128);  // Alpha > 128
```

---

## Sprite

Drawable textured rectangle. Like `sf::Sprite`.

### Constructor

| Constructor             | Description                       |
| ----------------------- | --------------------------------- |
| `Sprite()`              | Create without texture            |
| `Sprite(texture)`       | Create with texture               |
| `Sprite(texture, rect)` | Create with texture sub-rectangle |

### Texture

| Method                           | Description                        |
| -------------------------------- | ---------------------------------- |
| `setTexture(texture)`            | Set texture (keep rect)            |
| `setTexture(texture, resetRect)` | Set texture, optionally reset rect |
| `getTexture()`                   | Get texture pointer                |
| `setTextureRect(rect)`           | Set which part of texture to show  |
| `getTextureRect()`               | Get current texture rect           |

```cpp
Texture spritesheet;
spritesheet.loadFromFile("characters.png");

Sprite player(spritesheet);
player.setTextureRect(IntRect(0, 0, 32, 48));  // First frame

// Animation: change texture rect
player.setTextureRect(IntRect(32, 0, 32, 48));  // Second frame
```

### Transform (inherited from Transformable)

| Method                  | Description            |
| ----------------------- | ---------------------- |
| `setPosition(x, y)`     | Set position           |
| `setPosition(Vector2f)` | Set position           |
| `getPosition()`         | Get position           |
| `move(dx, dy)`          | Move relative          |
| `setRotation(angle)`    | Set rotation (degrees) |
| `getRotation()`         | Get rotation           |
| `rotate(angle)`         | Rotate relative        |
| `setScale(sx, sy)`      | Set scale factors      |
| `setScale(Vector2f)`    | Set scale factors      |
| `getScale()`            | Get scale              |
| `scale(sx, sy)`         | Scale relative         |
| `setOrigin(x, y)`       | Set transform origin   |
| `getOrigin()`           | Get origin             |

```cpp
Sprite sprite(texture);

sprite.setPosition(100, 50);
sprite.setScale(2.0f, 2.0f);     // Double size
sprite.setRotation(45.0f);       // 45 degrees
sprite.setOrigin(16, 16);        // Center origin (for 32x32 sprite)

sprite.move(10, 0);              // Move right
sprite.rotate(5);                // Rotate 5 more degrees
```

### Bounds

| Method              | Return      | Description             |
| ------------------- | ----------- | ----------------------- |
| `getLocalBounds()`  | `FloatRect` | Bounds before transform |
| `getGlobalBounds()` | `FloatRect` | Bounds after transform  |

```cpp
FloatRect bounds = sprite.getGlobalBounds();
if (bounds.intersects(enemyBounds))
{
    // Collision!
}
```

### Color

| Method            | Description               |
| ----------------- | ------------------------- |
| `setColor(color)` | Set color tint/modulation |
| `getColor()`      | Get color tint            |

```cpp
sprite.setColor(Color(255, 128, 128));  // Reddish tint
sprite.setColor(Color(255, 255, 255, 128));  // 50% transparent
```

---

## Drawing Sprites to Canvas

```cpp
Canvas canvas(200, 100, RenderMode::Braille);
Texture texture;
texture.loadFromFile("player.png");
Sprite sprite(texture);
sprite.setPosition(50, 25);

// Draw sprite to canvas
canvas.draw(sprite);

// Or manually (for pixel-perfect control)
const IntRect& rect = sprite.getTextureRect();
for (int y = 0; y < rect.height; ++y)
{
    for (int x = 0; x < rect.width; ++x)
    {
        Color c = texture.getPixel(rect.left + x, rect.top + y);
        if (c.a > 128)  // Only draw opaque pixels
        {
            Vector2f pos = sprite.transformPoint(Vector2f(x, y));
            canvas.setPixel(pos.x, pos.y, c);
        }
    }
}
```

---

## Complete Example: Sprite Animation

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

int main()
{
    Canvas canvas(160, 80, RenderMode::Braille);

    Texture spritesheet;
    spritesheet.loadFromFile("character.png");

    Sprite player(spritesheet);
    player.setTextureRect(IntRect(0, 0, 32, 32));  // Frame 0
    player.setPosition(80, 40);

    int frame = 0;
    Clock clock;
    float animTimer = 0;

    while (!Keyboard::isKeyPressed(Key::Escape))
    {
        float dt = clock.restart().asSeconds();

        // Animate
        animTimer += dt;
        if (animTimer >= 0.1f)  // 10 FPS animation
        {
            animTimer = 0;
            frame = (frame + 1) % 4;  // 4 frames
            player.setTextureRect(IntRect(frame * 32, 0, 32, 32));
        }

        // Move with arrow keys
        if (Keyboard::isKeyPressed(Key::Left))
            player.move(-100 * dt, 0);
        if (Keyboard::isKeyPressed(Key::Right))
            player.move(100 * dt, 0);

        // Draw
        canvas.clear(Color(20, 20, 40));
        canvas.draw(player);
        canvas.display();

        sleep(Time::milliseconds(16));
    }

    return 0;
}
```

---

[⬅ Back to Graphics](../Graphics/graphics.md) | [Next: Audio Module →](../Audio/audio.md)
