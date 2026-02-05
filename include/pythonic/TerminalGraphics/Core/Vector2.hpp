/**
 * @file Vector2.hpp
 * @brief 2D vector template class
 */

#pragma once

#include "../Config.hpp"
#include <cmath>
#include <type_traits>

TG_NAMESPACE_BEGIN

/**
 * @brief 2D vector template class
 *
 * Represents a point or direction in 2D space.
 * Common specializations: Vector2f (float), Vector2i (int), Vector2u (unsigned)
 */
template <typename T>
class Vector2
{
public:
    T x;
    T y;

    // Constructors
    constexpr Vector2() : x(0), y(0) {}
    constexpr Vector2(T x, T y) : x(x), y(y) {}

    // Copy from different type
    template <typename U>
    constexpr explicit Vector2(const Vector2<U> &other)
        : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}

    // Operators
    constexpr Vector2 operator-() const { return Vector2(-x, -y); }

    constexpr Vector2 &operator+=(const Vector2 &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vector2 &operator-=(const Vector2 &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Vector2 &operator*=(T scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vector2 &operator/=(T scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // Length/magnitude
    T length() const
    {
        return static_cast<T>(std::sqrt(x * x + y * y));
    }

    T lengthSquared() const
    {
        return x * x + y * y;
    }

    // Normalize
    Vector2 normalized() const
    {
        T len = length();
        if (len != 0)
            return Vector2(x / len, y / len);
        return *this;
    }

    // Dot product
    T dot(const Vector2 &other) const
    {
        return x * other.x + y * other.y;
    }

    // Cross product (returns z component of 3D cross product)
    T cross(const Vector2 &other) const
    {
        return x * other.y - y * other.x;
    }

    // Distance to another point
    T distanceTo(const Vector2 &other) const
    {
        return (*this - other).length();
    }
};

// Binary operators
template <typename T>
constexpr Vector2<T> operator+(const Vector2<T> &lhs, const Vector2<T> &rhs)
{
    return Vector2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template <typename T>
constexpr Vector2<T> operator-(const Vector2<T> &lhs, const Vector2<T> &rhs)
{
    return Vector2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}

template <typename T>
constexpr Vector2<T> operator*(const Vector2<T> &vec, T scalar)
{
    return Vector2<T>(vec.x * scalar, vec.y * scalar);
}

template <typename T>
constexpr Vector2<T> operator*(T scalar, const Vector2<T> &vec)
{
    return vec * scalar;
}

template <typename T>
constexpr Vector2<T> operator/(const Vector2<T> &vec, T scalar)
{
    return Vector2<T>(vec.x / scalar, vec.y / scalar);
}

template <typename T>
constexpr bool operator==(const Vector2<T> &lhs, const Vector2<T> &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
constexpr bool operator!=(const Vector2<T> &lhs, const Vector2<T> &rhs)
{
    return !(lhs == rhs);
}

// Common type aliases
using Vector2f = Vector2<float>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned int>;

TG_NAMESPACE_END
