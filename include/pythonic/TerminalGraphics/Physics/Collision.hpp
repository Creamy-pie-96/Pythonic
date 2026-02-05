/**
 * @file Collision.hpp
 * @brief Collision detection utilities
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "../Graphics/RenderTarget.hpp"
#include "../Sprite/Sprite.hpp"
#include "../Shapes/Shape.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

TG_NAMESPACE_BEGIN

/**
 * @brief Collision detection and bounding box utilities
 *
 * Provides various collision detection methods including:
 * - AABB (Axis-Aligned Bounding Box) collision
 * - Pixel-perfect collision for sprites
 * - Bounding box calculation from active pixels
 *
 * @code
 * Sprite bird(birdTexture);
 * RectangleShape pipe(Vector2f(50, 200));
 *
 * if (Collision::intersects(bird.getGlobalBounds(), pipe.getGlobalBounds()))
 * {
 *     // Collision detected!
 *     gameOver = true;
 * }
 * @endcode
 */
class Collision
{
public:
    /**
     * @brief Check if two rectangles intersect (AABB collision)
     * @param a First rectangle
     * @param b Second rectangle
     * @return true if rectangles overlap
     */
    static bool intersects(const FloatRect &a, const FloatRect &b)
    {
        return a.intersects(b);
    }

    /**
     * @brief Check if a point is inside a rectangle
     */
    static bool contains(const FloatRect &rect, const Vector2f &point)
    {
        return rect.contains(point.x, point.y);
    }

    /**
     * @brief Check if a point is inside a rectangle
     */
    static bool contains(const FloatRect &rect, float x, float y)
    {
        return rect.contains(x, y);
    }

    /**
     * @brief Get the intersection rectangle of two rectangles
     * @param a First rectangle
     * @param b Second rectangle
     * @return The overlapping region, or empty rect if no intersection
     */
    static FloatRect getIntersection(const FloatRect &a, const FloatRect &b)
    {
        FloatRect result;
        if (a.findIntersection(b, result))
            return result;
        return FloatRect(0, 0, 0, 0);
    }

    /**
     * @brief Calculate bounding box from active pixels in a texture
     * @param texture The texture to analyze
     * @param alphaThreshold Minimum alpha value to consider pixel "active"
     * @return Bounding box containing all active pixels
     *
     * This is useful for creating tight bounding boxes around
     * irregularly shaped sprites.
     */
    static IntRect calculateTextureBounds(const Texture &texture,
                                          uint8_t alphaThreshold = 128)
    {
        Vector2u size = texture.getSize();
        if (size.x == 0 || size.y == 0)
            return IntRect(0, 0, 0, 0);

        int minX = static_cast<int>(size.x);
        int minY = static_cast<int>(size.y);
        int maxX = -1;
        int maxY = -1;

        const Image &img = texture.getImage();

        for (unsigned int y = 0; y < size.y; ++y)
        {
            for (unsigned int x = 0; x < size.x; ++x)
            {
                Color c = img.getPixel(x, y);
                if (c.a >= alphaThreshold && (c.r > 32 || c.g > 32 || c.b > 32))
                {
                    minX = std::min(minX, static_cast<int>(x));
                    minY = std::min(minY, static_cast<int>(y));
                    maxX = std::max(maxX, static_cast<int>(x));
                    maxY = std::max(maxY, static_cast<int>(y));
                }
            }
        }

        if (maxX < 0 || maxY < 0)
            return IntRect(0, 0, 0, 0);

        return IntRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

    /**
     * @brief Get tight bounding box for a sprite
     * @param sprite The sprite to analyze
     * @return Global bounding box containing only active pixels
     */
    static FloatRect getTightBounds(const Sprite &sprite)
    {
        const Texture *tex = sprite.getTexture();
        if (!tex)
            return sprite.getGlobalBounds();

        IntRect localBounds = calculateTextureBounds(*tex);
        if (localBounds.width == 0 || localBounds.height == 0)
            return sprite.getGlobalBounds();

        // Transform the tight bounds to world space
        Vector2f tl = sprite.transformPoint(Vector2f(
            static_cast<float>(localBounds.left),
            static_cast<float>(localBounds.top)));
        Vector2f br = sprite.transformPoint(Vector2f(
            static_cast<float>(localBounds.left + localBounds.width),
            static_cast<float>(localBounds.top + localBounds.height)));

        float minX = std::min(tl.x, br.x);
        float minY = std::min(tl.y, br.y);
        float maxX = std::max(tl.x, br.x);
        float maxY = std::max(tl.y, br.y);

        return FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

    /**
     * @brief Check pixel-perfect collision between two sprites
     * @param a First sprite
     * @param b Second sprite
     * @param alphaThreshold Minimum alpha to consider pixel solid
     * @return true if any solid pixels overlap
     *
     * This is more accurate but slower than bounding box collision.
     * First checks bounding boxes, then checks individual pixels.
     */
    static bool pixelPerfect(const Sprite &a, const Sprite &b,
                             uint8_t alphaThreshold = 128)
    {
        // First do fast AABB check
        FloatRect boundsA = a.getGlobalBounds();
        FloatRect boundsB = b.getGlobalBounds();

        if (!boundsA.intersects(boundsB))
            return false;

        // Get intersection region
        FloatRect intersection = getIntersection(boundsA, boundsB);
        if (intersection.width <= 0 || intersection.height <= 0)
            return false;

        const Texture *texA = a.getTexture();
        const Texture *texB = b.getTexture();

        if (!texA || !texB)
            return true; // Assume collision if no texture

        // Check each pixel in the intersection region
        for (float y = intersection.top; y < intersection.top + intersection.height; ++y)
        {
            for (float x = intersection.left; x < intersection.left + intersection.width; ++x)
            {
                // Transform to local coordinates for each sprite
                Vector2f localA = a.inverseTransformPoint(Vector2f(x, y));
                Vector2f localB = b.inverseTransformPoint(Vector2f(x, y));

                // Get pixel from each sprite
                Color colorA = texA->getPixel(
                    static_cast<unsigned int>(localA.x),
                    static_cast<unsigned int>(localA.y));
                Color colorB = texB->getPixel(
                    static_cast<unsigned int>(localB.x),
                    static_cast<unsigned int>(localB.y));

                // Check if both pixels are solid
                if (colorA.a >= alphaThreshold && colorB.a >= alphaThreshold)
                    return true;
            }
        }

        return false;
    }

    /**
     * @brief Check collision between a sprite and a shape
     */
    static bool spriteVsShape(const Sprite &sprite, const Shape &shape)
    {
        return intersects(sprite.getGlobalBounds(), shape.getGlobalBounds());
    }

    /**
     * @brief Check collision between two shapes
     */
    static bool shapeVsShape(const Shape &a, const Shape &b)
    {
        return intersects(a.getGlobalBounds(), b.getGlobalBounds());
    }

    /**
     * @brief Check if a rectangle is within screen bounds
     * @param rect The rectangle to check
     * @param screenWidth Screen width
     * @param screenHeight Screen height
     * @return true if fully within bounds
     */
    static bool isWithinScreen(const FloatRect &rect,
                               float screenWidth, float screenHeight)
    {
        return rect.left >= 0 &&
               rect.top >= 0 &&
               rect.left + rect.width <= screenWidth &&
               rect.top + rect.height <= screenHeight;
    }

    /**
     * @brief Check if a rectangle is touching or outside screen bounds
     */
    static bool isTouchingScreenBorder(const FloatRect &rect,
                                       float screenWidth, float screenHeight)
    {
        return rect.left <= 0 ||
               rect.top <= 0 ||
               rect.left + rect.width >= screenWidth ||
               rect.top + rect.height >= screenHeight;
    }

    /**
     * @brief Calculate distance between two points
     */
    static float distance(const Vector2f &a, const Vector2f &b)
    {
        return (b - a).length();
    }

    /**
     * @brief Calculate distance between rectangle centers
     */
    static float distance(const FloatRect &a, const FloatRect &b)
    {
        Vector2f centerA(a.left + a.width / 2, a.top + a.height / 2);
        Vector2f centerB(b.left + b.width / 2, b.top + b.height / 2);
        return distance(centerA, centerB);
    }

    /**
     * @brief Check circle-circle collision
     */
    static bool circleVsCircle(const Vector2f &centerA, float radiusA,
                               const Vector2f &centerB, float radiusB)
    {
        float d = distance(centerA, centerB);
        return d <= (radiusA + radiusB);
    }

    /**
     * @brief Check circle-rectangle collision
     */
    static bool circleVsRect(const Vector2f &center, float radius,
                             const FloatRect &rect)
    {
        // Find closest point on rectangle to circle center
        float closestX = std::max(rect.left,
                                  std::min(center.x, rect.left + rect.width));
        float closestY = std::max(rect.top,
                                  std::min(center.y, rect.top + rect.height));

        // Check if closest point is within circle
        float dx = center.x - closestX;
        float dy = center.y - closestY;
        return (dx * dx + dy * dy) <= (radius * radius);
    }
};

TG_NAMESPACE_END
