/**
 * @file Rect.hpp
 * @brief Rectangle template class for bounds and positioning
 */

#pragma once

#include "../Config.hpp"
#include "Vector2.hpp"
#include <algorithm>

TG_NAMESPACE_BEGIN

/**
 * @brief Axis-aligned rectangle template
 *
 * Useful for bounds checking, collision detection, and defining regions.
 */
template <typename T>
class Rect
{
public:
    T left;   ///< Left coordinate
    T top;    ///< Top coordinate
    T width;  ///< Width of rectangle
    T height; ///< Height of rectangle

    // Constructors
    constexpr Rect() : left(0), top(0), width(0), height(0) {}

    constexpr Rect(T left, T top, T width, T height)
        : left(left), top(top), width(width), height(height) {}

    constexpr Rect(const Vector2<T> &position, const Vector2<T> &size)
        : left(position.x), top(position.y), width(size.x), height(size.y) {}

    // Copy from different type
    template <typename U>
    constexpr explicit Rect(const Rect<U> &other)
        : left(static_cast<T>(other.left)), top(static_cast<T>(other.top)), width(static_cast<T>(other.width)), height(static_cast<T>(other.height)) {}

    // Get position and size as vectors
    constexpr Vector2<T> getPosition() const { return Vector2<T>(left, top); }
    constexpr Vector2<T> getSize() const { return Vector2<T>(width, height); }

    // Get corners
    constexpr T right() const { return left + width; }
    constexpr T bottom() const { return top + height; }
    constexpr Vector2<T> getCenter() const
    {
        return Vector2<T>(left + width / 2, top + height / 2);
    }

    /**
     * @brief Check if a point is inside the rectangle
     * @param x X coordinate
     * @param y Y coordinate
     * @return true if point is inside
     */
    constexpr bool contains(T x, T y) const
    {
        T minX = std::min(left, left + width);
        T maxX = std::max(left, left + width);
        T minY = std::min(top, top + height);
        T maxY = std::max(top, top + height);

        return x >= minX && x < maxX && y >= minY && y < maxY;
    }

    constexpr bool contains(const Vector2<T> &point) const
    {
        return contains(point.x, point.y);
    }

    /**
     * @brief Check intersection with another rectangle
     * @param other Rectangle to check against
     * @return true if rectangles overlap
     */
    constexpr bool intersects(const Rect<T> &other) const
    {
        T thisMinX = std::min(left, left + width);
        T thisMaxX = std::max(left, left + width);
        T thisMinY = std::min(top, top + height);
        T thisMaxY = std::max(top, top + height);

        T otherMinX = std::min(other.left, other.left + other.width);
        T otherMaxX = std::max(other.left, other.left + other.width);
        T otherMinY = std::min(other.top, other.top + other.height);
        T otherMaxY = std::max(other.top, other.top + other.height);

        return !(thisMaxX <= otherMinX || thisMinX >= otherMaxX ||
                 thisMaxY <= otherMinY || thisMinY >= otherMaxY);
    }

    /**
     * @brief Get intersection rectangle
     * @param other Rectangle to intersect with
     * @param intersection Output rectangle
     * @return true if intersection exists
     */
    bool findIntersection(const Rect<T> &other, Rect<T> &intersection) const
    {
        T thisMinX = std::min(left, left + width);
        T thisMaxX = std::max(left, left + width);
        T thisMinY = std::min(top, top + height);
        T thisMaxY = std::max(top, top + height);

        T otherMinX = std::min(other.left, other.left + other.width);
        T otherMaxX = std::max(other.left, other.left + other.width);
        T otherMinY = std::min(other.top, other.top + other.height);
        T otherMaxY = std::max(other.top, other.top + other.height);

        T intLeft = std::max(thisMinX, otherMinX);
        T intTop = std::max(thisMinY, otherMinY);
        T intRight = std::min(thisMaxX, otherMaxX);
        T intBottom = std::min(thisMaxY, otherMaxY);

        if (intLeft < intRight && intTop < intBottom)
        {
            intersection = Rect<T>(intLeft, intTop, intRight - intLeft, intBottom - intTop);
            return true;
        }

        return false;
    }
};

// Comparison
template <typename T>
constexpr bool operator==(const Rect<T> &lhs, const Rect<T> &rhs)
{
    return lhs.left == rhs.left && lhs.top == rhs.top &&
           lhs.width == rhs.width && lhs.height == rhs.height;
}

template <typename T>
constexpr bool operator!=(const Rect<T> &lhs, const Rect<T> &rhs)
{
    return !(lhs == rhs);
}

// Common type aliases
using FloatRect = Rect<float>;
using IntRect = Rect<int>;

TG_NAMESPACE_END
