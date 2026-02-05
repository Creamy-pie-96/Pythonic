[⬅ Back to Graphics Index](../index.md)

# View (Camera) System

The View class provides a 2D camera system for scrolling, zooming, and rotating the game world. Essential for platformers, shooters, RPGs, and any game that needs camera movement.

---

## Overview

| Class          | Description                             | Header              |
| -------------- | --------------------------------------- | ------------------- |
| `View`         | 2D camera with position, size, rotation | `Graphics/View.hpp` |
| `Transform`    | 2D transformation matrix                | `Graphics/View.hpp` |
| `BlendMode`    | Color blending modes                    | `Graphics/View.hpp` |
| `RenderStates` | Drawing state container                 | `Graphics/View.hpp` |

---

## View Class

### Creating a View

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

// Default view (centered at origin)
View camera;

// View with center and size
View camera(Vector2f(100, 50), Vector2f(160, 80));

// View from a rectangle
View camera(FloatRect(0, 0, 160, 80));
```

### Following the Player

```cpp
// Make camera follow player
camera.setCenter(player.x, player.y);
canvas.setView(camera);

// Draw everything relative to camera
canvas.draw(enemy);
canvas.draw(walls);

// Reset to draw HUD in screen coordinates
canvas.setView(canvas.getDefaultView());
canvas.draw(healthBar);
```

### Zooming

```cpp
// Zoom in 2x (smaller view = closer)
camera.zoom(0.5f);

// Zoom out 2x (larger view = further)
camera.zoom(2.0f);

// Set exact size
camera.setSize(80, 40);  // 2x zoom
```

### Rotation

```cpp
// Rotate 45 degrees
camera.setRotation(45);

// Add rotation over time (for screen shake)
camera.rotate(5.0f * dt);
```

### Viewport (Split Screen)

```cpp
// Left half of screen for player 1
View player1View(Vector2f(100, 50), Vector2f(160, 80));
player1View.setViewport(FloatRect(0, 0, 0.5f, 1.0f));

// Right half for player 2
View player2View(Vector2f(200, 50), Vector2f(160, 80));
player2View.setViewport(FloatRect(0.5f, 0, 0.5f, 1.0f));

// Draw each view
canvas.setView(player1View);
renderWorld(canvas, player1);

canvas.setView(player2View);
renderWorld(canvas, player2);
```

### Coordinate Mapping

```cpp
// Convert mouse position to world coordinates
Vector2i mousePos = Mouse::getPosition();
Vector2f worldPos = canvas.mapPixelToCoords(mousePos);

// Convert world position to screen for UI
Vector2f enemyPos(100, 50);
Vector2i screenPos = canvas.mapCoordsToPixel(enemyPos);
```

---

## Transform Class

A 3x3 transformation matrix for 2D transformations.

### Basic Usage

```cpp
Transform t;

// Translation
t.translate(100, 50);

// Rotation (degrees)
t.rotate(45);

// Rotation around a point
t.rotate(45, 50, 50);

// Scaling
t.scale(2.0f, 2.0f);

// Scaling around a center
t.scale(2.0f, 2.0f, 50, 50);
```

### Combining Transforms

```cpp
Transform parent;
parent.translate(100, 100);
parent.rotate(45);

Transform child;
child.translate(20, 0);

// Combine: child is transformed relative to parent
Transform combined = parent * child;

// Transform a point
Vector2f worldPos = combined.transformPoint(0, 0);
```

### Getting Inverse

```cpp
Transform t;
t.translate(100, 50);
t.rotate(30);

// Inverse undoes the transformation
Transform inv = t.getInverse();

Vector2f point(150, 80);
Vector2f transformed = t.transformPoint(point);
Vector2f original = inv.transformPoint(transformed);  // Back to (150, 80)
```

---

## BlendMode Enum

Controls how colors are combined when drawing.

```cpp
enum class BlendMode
{
    Alpha,    // Standard alpha blending (default)
    Add,      // Additive: src + dst (for glows, lights)
    Multiply, // Multiplicative: src * dst (for shadows)
    None      // No blending, direct overwrite
};
```

### Usage Examples

```cpp
// Default alpha blending
sprite.draw(canvas);

// Additive blending for glow effects
RenderStates glowStates(BlendMode::Add);
glowSprite.draw(canvas, glowStates);

// Multiply for shadows
RenderStates shadowStates(BlendMode::Multiply);
shadowSprite.draw(canvas, shadowStates);
```

---

## RenderStates Struct

Bundles rendering state for draw calls.

```cpp
struct RenderStates
{
    Transform transform;
    BlendMode blendMode = BlendMode::Alpha;
    const Texture* texture = nullptr;

    static const RenderStates Default;
};
```

### Usage

```cpp
// Draw with custom transform
RenderStates states;
states.transform.translate(100, 50);
states.transform.rotate(45);
sprite.draw(canvas, states);

// Draw with blend mode
RenderStates states(BlendMode::Add);
particle.draw(canvas, states);

// Combine multiple states
RenderStates states(BlendMode::Alpha, myTransform, &texture);
customDraw(canvas, states);
```

---

## Complete Example: Scrolling Camera

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

int main()
{
    Canvas canvas = Canvas::fullscreen();
    Canvas::initDisplay();

    Keyboard::init();

    View camera(Vector2f(0, 0), Vector2f(canvas.getWidth(), canvas.getHeight()));

    float playerX = 0, playerY = 0;
    Clock clock;

    while (Keyboard::isKeyPressed(Key::Escape) == false)
    {
        float dt = clock.restart().asSeconds();

        // Move player
        if (Keyboard::isKeyPressed(Key::W)) playerY -= 100 * dt;
        if (Keyboard::isKeyPressed(Key::S)) playerY += 100 * dt;
        if (Keyboard::isKeyPressed(Key::A)) playerX -= 100 * dt;
        if (Keyboard::isKeyPressed(Key::D)) playerX += 100 * dt;

        // Camera follows player smoothly
        Vector2f currentCenter = camera.getCenter();
        float lerpFactor = 5.0f * dt;
        camera.setCenter(
            currentCenter.x + (playerX - currentCenter.x) * lerpFactor,
            currentCenter.y + (playerY - currentCenter.y) * lerpFactor
        );

        // Zoom with mouse wheel
        int wheel = Mouse::getWheelDelta();
        if (wheel != 0)
        {
            camera.zoom(wheel > 0 ? 0.9f : 1.1f);
        }

        // Apply camera
        canvas.setView(camera);

        // Draw world
        canvas.clear(Color(20, 30, 40));

        // Draw grid (in world coordinates)
        for (int x = -500; x <= 500; x += 50)
        {
            canvas.drawLine(x, -500, x, 500, Color(50, 50, 60));
        }
        for (int y = -500; y <= 500; y += 50)
        {
            canvas.drawLine(-500, y, 500, y, Color(50, 50, 60));
        }

        // Draw player
        canvas.fillCircle(playerX, playerY, 10, Color::Cyan);

        // Draw HUD in screen coordinates
        canvas.setView(canvas.getDefaultView());
        Text posText;
        posText.setString("Pos: " + std::to_string(int(playerX)) + ", " + std::to_string(int(playerY)));
        posText.draw(canvas, 5, 5, Color::White);

        canvas.display();
    }

    Keyboard::shutdown();
    Canvas::cleanupDisplay();
    return 0;
}
```

---

[⬅ Back to Graphics Module](graphics.md) | [Next: Sprite Module →](../Sprite/sprite.md)
