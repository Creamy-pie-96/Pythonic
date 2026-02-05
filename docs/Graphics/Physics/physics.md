[⬅ Back to Graphics Index](../index.md) | [⬅ Back to Text](../Text/text.md)

# Physics Module

Collision detection and response utilities.

---

## Quick Start

```cpp
#include <pythonic/TerminalGraphics/Core/Rect.hpp>
using namespace Pythonic::TG;

FloatRect player(100, 50, 20, 30);
FloatRect enemy(110, 55, 25, 25);

if (player.intersects(enemy))
{
    // Collision detected!
}
```

---

## Rect-Based Collision

### Intersection Check

| Method                            | Return | Description            |
| --------------------------------- | ------ | ---------------------- |
| `rect.intersects(other)`          | `bool` | Check if rects overlap |
| `rect.intersects(other, overlap)` | `bool` | Get overlap region     |

```cpp
FloatRect a(0, 0, 50, 50);
FloatRect b(25, 25, 50, 50);

if (a.intersects(b))
{
    // Overlapping
}

// Get overlap region
FloatRect overlap;
if (a.intersects(b, overlap))
{
    // overlap contains the intersection rectangle
    float overlapWidth = overlap.width;
    float overlapHeight = overlap.height;
}
```

### Point Containment

| Method                    | Return | Description              |
| ------------------------- | ------ | ------------------------ |
| `rect.contains(x, y)`     | `bool` | Check if point is inside |
| `rect.contains(Vector2f)` | `bool` | Check if point is inside |

```cpp
FloatRect button(50, 20, 100, 30);
float mouseX = 75, mouseY = 35;

if (button.contains(mouseX, mouseY))
{
    // Mouse is over button
}

Vector2f point(75, 35);
if (button.contains(point))
{
    // Same check with vector
}
```

---

## Sprite Collision

### Using Global Bounds

```cpp
Sprite player(playerTexture);
Sprite enemy(enemyTexture);

FloatRect playerBounds = player.getGlobalBounds();
FloatRect enemyBounds = enemy.getGlobalBounds();

if (playerBounds.intersects(enemyBounds))
{
    // Collision!
}
```

### Using Opaque Bounds (Accurate)

For sprites with transparent padding, use opaque bounds:

```cpp
Texture playerTex;
playerTex.loadFromFile("player.png");

// Get tight bounds (ignoring transparent pixels)
IntRect opaqueBounds = playerTex.getOpaqueBounds(128);  // Alpha threshold

// Calculate world-space opaque bounds
Sprite player(playerTex);
player.setPosition(100, 50);
Vector2f scale = player.getScale();
Vector2f pos = player.getPosition();

FloatRect accurateBounds(
    pos.x + opaqueBounds.left * scale.x,
    pos.y + opaqueBounds.top * scale.y,
    opaqueBounds.width * scale.x,
    opaqueBounds.height * scale.y
);
```

---

## Common Collision Patterns

### AABB vs AABB

Axis-Aligned Bounding Box collision (most common):

```cpp
bool checkCollision(const FloatRect& a, const FloatRect& b)
{
    return a.intersects(b);
}
```

### Circle vs Circle

```cpp
bool circleCollision(Vector2f p1, float r1, Vector2f p2, float r2)
{
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float distSq = dx * dx + dy * dy;
    float radiusSum = r1 + r2;
    return distSq <= radiusSum * radiusSum;
}
```

### Circle vs Rect

```cpp
bool circleRectCollision(Vector2f center, float radius, const FloatRect& rect)
{
    // Find closest point on rect to circle center
    float closestX = std::max(rect.left, std::min(center.x, rect.left + rect.width));
    float closestY = std::max(rect.top, std::min(center.y, rect.top + rect.height));

    float dx = center.x - closestX;
    float dy = center.y - closestY;

    return (dx * dx + dy * dy) <= (radius * radius);
}
```

---

## Collision Response

### Push Out (Separation)

```cpp
void separateRects(FloatRect& a, const FloatRect& b)
{
    FloatRect overlap;
    if (!a.intersects(b, overlap))
        return;

    // Push out in direction of minimum overlap
    if (overlap.width < overlap.height)
    {
        // Horizontal separation
        if (a.left < b.left)
            a.left -= overlap.width;  // Push left
        else
            a.left += overlap.width;  // Push right
    }
    else
    {
        // Vertical separation
        if (a.top < b.top)
            a.top -= overlap.height;  // Push up
        else
            a.top += overlap.height;  // Push down
    }
}
```

### Bounce (Reflection)

```cpp
void bounce(Vector2f& velocity, const FloatRect& a, const FloatRect& b)
{
    FloatRect overlap;
    if (!a.intersects(b, overlap))
        return;

    if (overlap.width < overlap.height)
    {
        velocity.x = -velocity.x;  // Horizontal bounce
    }
    else
    {
        velocity.y = -velocity.y;  // Vertical bounce
    }
}
```

---

## Multi-Object Collision

### Against Walls

```cpp
void checkWallCollision(FloatRect& player, float canvasWidth, float canvasHeight)
{
    // Left wall
    if (player.left < 0)
        player.left = 0;

    // Right wall
    if (player.left + player.width > canvasWidth)
        player.left = canvasWidth - player.width;

    // Top wall
    if (player.top < 0)
        player.top = 0;

    // Bottom wall
    if (player.top + player.height > canvasHeight)
        player.top = canvasHeight - player.height;
}
```

### Against Multiple Objects

```cpp
bool checkEnemyCollisions(const FloatRect& player,
                          const std::vector<Sprite>& enemies)
{
    for (const auto& enemy : enemies)
    {
        if (player.intersects(enemy.getGlobalBounds()))
        {
            return true;
        }
    }
    return false;
}
```

---

## Complete Example: Platformer Collision

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

struct Player
{
    Vector2f position{80, 10};
    Vector2f velocity{0, 0};
    float width = 10;
    float height = 15;
    bool onGround = false;

    FloatRect getBounds() const
    {
        return FloatRect(position.x, position.y, width, height);
    }
};

struct Platform
{
    FloatRect rect;
    Color color;
};

int main()
{
    Canvas canvas(160, 80, RenderMode::Braille);

    Player player;

    std::vector<Platform> platforms = {
        {{0, 70, 160, 10}, Color::Green},
        {{50, 50, 40, 5}, Color(100, 200, 100)},
        {{90, 35, 40, 5}, Color(100, 200, 100)},
        {{20, 25, 30, 5}, Color(100, 200, 100)}
    };

    Clock clock;

    while (!Keyboard::isKeyPressed(Key::Escape))
    {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;  // Cap delta time

        // Input
        if (Keyboard::isKeyPressed(Key::Left))
            player.velocity.x = -80;
        else if (Keyboard::isKeyPressed(Key::Right))
            player.velocity.x = 80;
        else
            player.velocity.x = 0;

        if (Keyboard::isKeyPressed(Key::Space) && player.onGround)
        {
            player.velocity.y = -150;
            player.onGround = false;
        }

        // Gravity
        player.velocity.y += 400 * dt;

        // Move
        player.position.x += player.velocity.x * dt;
        player.position.y += player.velocity.y * dt;

        // Collision with platforms
        player.onGround = false;
        FloatRect playerRect = player.getBounds();

        for (const auto& platform : platforms)
        {
            FloatRect overlap;
            if (playerRect.intersects(platform.rect, overlap))
            {
                // Vertical collision
                if (overlap.width > overlap.height)
                {
                    if (player.velocity.y > 0)  // Falling
                    {
                        player.position.y -= overlap.height;
                        player.onGround = true;
                    }
                    else  // Rising
                    {
                        player.position.y += overlap.height;
                    }
                    player.velocity.y = 0;
                }
                // Horizontal collision
                else
                {
                    if (player.velocity.x > 0)
                        player.position.x -= overlap.width;
                    else
                        player.position.x += overlap.width;
                }

                // Recalculate bounds after adjustment
                playerRect = player.getBounds();
            }
        }

        // Draw
        canvas.clear(Color(30, 30, 50));

        for (const auto& platform : platforms)
        {
            canvas.fillRect(platform.rect.left, platform.rect.top,
                           platform.rect.width, platform.rect.height,
                           platform.color);
        }

        canvas.fillRect(player.position.x, player.position.y,
                       player.width, player.height, Color::Cyan);

        Text::draw(canvas, 5, 2, player.onGround ? "On Ground" : "In Air");

        canvas.display();
        sleep(Time::milliseconds(16));
    }

    return 0;
}
```

---

## Tips

### Broad Phase / Narrow Phase

For many objects, use spatial partitioning:

1. **Broad phase**: Quickly reject distant objects (grid, quadtree)
2. **Narrow phase**: Precise collision on nearby objects

### Continuous Collision

For fast-moving objects, use ray casting or swept tests to prevent tunneling.

### Debug Visualization

Draw collision bounds during development:

```cpp
// Debug draw
FloatRect bounds = sprite.getGlobalBounds();
canvas.drawRect(bounds.left, bounds.top, bounds.width, bounds.height, Color::Red);
```

---

[⬅ Back to Text](../Text/text.md) | [⬅ Back to Graphics Index](../index.md)
