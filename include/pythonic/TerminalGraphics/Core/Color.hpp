/**
 * @file Color.hpp
 * @brief RGBA color class for terminal graphics
 */

#pragma once

#include "../Config.hpp"
#include <cstdint>
#include <string>
#include <algorithm>

TG_NAMESPACE_BEGIN

/**
 * @brief RGBA color representation
 *
 * 24-bit color with 8-bit alpha channel.
 * Used for setting colors of shapes, text, and other drawables.
 */
class Color
{
public:
    uint8_t r; ///< Red component (0-255)
    uint8_t g; ///< Green component (0-255)
    uint8_t b; ///< Blue component (0-255)
    uint8_t a; ///< Alpha component (0-255, 255 = opaque)

    // Constructors
    constexpr Color() : r(0), g(0), b(0), a(255) {}

    constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    // From 32-bit integer (0xRRGGBBAA or 0xRRGGBB)
    constexpr explicit Color(uint32_t color)
        : r((color >> 24) & 0xFF), g((color >> 16) & 0xFF), b((color >> 8) & 0xFF), a(color & 0xFF) {}

    // Convert to 32-bit integer
    constexpr uint32_t toInteger() const
    {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8) |
               static_cast<uint32_t>(a);
    }

    // Generate ANSI foreground escape code
    std::string toAnsiFg() const
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "\033[38;2;%d;%d;%dm", r, g, b);
        return std::string(buf);
    }

    // Generate ANSI background escape code
    std::string toAnsiBg() const
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "\033[48;2;%d;%d;%dm", r, g, b);
        return std::string(buf);
    }

    // Blend with another color using alpha
    Color blend(const Color &background) const
    {
        if (a == 255)
            return *this;
        if (a == 0)
            return background;

        float alpha = a / 255.0f;
        float inv_alpha = 1.0f - alpha;

        return Color(
            static_cast<uint8_t>(r * alpha + background.r * inv_alpha),
            static_cast<uint8_t>(g * alpha + background.g * inv_alpha),
            static_cast<uint8_t>(b * alpha + background.b * inv_alpha),
            255);
    }

    // Interpolate between two colors
    static Color lerp(const Color &a, const Color &b, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return Color(
            static_cast<uint8_t>(a.r + (b.r - a.r) * t),
            static_cast<uint8_t>(a.g + (b.g - a.g) * t),
            static_cast<uint8_t>(a.b + (b.b - a.b) * t),
            static_cast<uint8_t>(a.a + (b.a - a.a) * t));
    }

    // Predefined colors
    static const Color Black;
    static const Color White;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Orange;
    static const Color Purple;
    static const Color Pink;
    static const Color Brown;
    static const Color Gray;
    static const Color DarkGray;
    static const Color LightGray;
    static const Color Transparent;
};

// Comparison operators
constexpr bool operator==(const Color &lhs, const Color &rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

constexpr bool operator!=(const Color &lhs, const Color &rhs)
{
    return !(lhs == rhs);
}

// Addition (clamped to 255)
inline Color operator+(const Color &lhs, const Color &rhs)
{
    return Color(
        static_cast<uint8_t>(std::min(255, lhs.r + rhs.r)),
        static_cast<uint8_t>(std::min(255, lhs.g + rhs.g)),
        static_cast<uint8_t>(std::min(255, lhs.b + rhs.b)),
        static_cast<uint8_t>(std::min(255, lhs.a + rhs.a)));
}

// Subtraction (clamped to 0)
inline Color operator-(const Color &lhs, const Color &rhs)
{
    return Color(
        static_cast<uint8_t>(std::max(0, lhs.r - rhs.r)),
        static_cast<uint8_t>(std::max(0, lhs.g - rhs.g)),
        static_cast<uint8_t>(std::max(0, lhs.b - rhs.b)),
        static_cast<uint8_t>(std::max(0, lhs.a - rhs.a)));
}

// Multiplication (component-wise)
inline Color operator*(const Color &lhs, const Color &rhs)
{
    return Color(
        static_cast<uint8_t>((lhs.r * rhs.r) / 255),
        static_cast<uint8_t>((lhs.g * rhs.g) / 255),
        static_cast<uint8_t>((lhs.b * rhs.b) / 255),
        static_cast<uint8_t>((lhs.a * rhs.a) / 255));
}

// Predefined color definitions
inline const Color Color::Black(0, 0, 0);
inline const Color Color::White(255, 255, 255);
inline const Color Color::Red(255, 0, 0);
inline const Color Color::Green(0, 255, 0);
inline const Color Color::Blue(0, 0, 255);
inline const Color Color::Yellow(255, 255, 0);
inline const Color Color::Cyan(0, 255, 255);
inline const Color Color::Magenta(255, 0, 255);
inline const Color Color::Orange(255, 165, 0);
inline const Color Color::Purple(128, 0, 128);
inline const Color Color::Pink(255, 192, 203);
inline const Color Color::Brown(139, 69, 19);
inline const Color Color::Gray(128, 128, 128);
inline const Color Color::DarkGray(64, 64, 64);
inline const Color Color::LightGray(192, 192, 192);
inline const Color Color::Transparent(0, 0, 0, 0);

TG_NAMESPACE_END
