/**
 * @file Time.hpp
 * @brief Time representation and utilities
 */

#pragma once

#include "../Config.hpp"
#include <chrono>
#include <thread>

TG_NAMESPACE_BEGIN

/**
 * @brief Represents a time duration
 *
 * Provides high-precision time measurement for game loops,
 * animations, and timing-sensitive operations.
 */
class Time
{
public:
    constexpr Time() : m_microseconds(0) {}

    // Get time as different units
    constexpr float asSeconds() const
    {
        return static_cast<float>(m_microseconds) / 1000000.0f;
    }

    constexpr int32_t asMilliseconds() const
    {
        return static_cast<int32_t>(m_microseconds / 1000);
    }

    constexpr int64_t asMicroseconds() const
    {
        return m_microseconds;
    }

    // Create from different units
    static constexpr Time seconds(float seconds)
    {
        return Time(static_cast<int64_t>(seconds * 1000000.0f));
    }

    static constexpr Time milliseconds(int32_t milliseconds)
    {
        return Time(static_cast<int64_t>(milliseconds) * 1000);
    }

    static constexpr Time microseconds(int64_t microseconds)
    {
        return Time(microseconds);
    }

    // Zero time constant
    static const Time Zero;

private:
    explicit constexpr Time(int64_t microseconds) : m_microseconds(microseconds) {}

    int64_t m_microseconds;

    friend constexpr Time operator+(Time lhs, Time rhs);
    friend constexpr Time operator-(Time lhs, Time rhs);
    friend constexpr Time operator*(Time lhs, float rhs);
    friend constexpr Time operator*(Time lhs, int64_t rhs);
    friend constexpr Time operator/(Time lhs, float rhs);
    friend constexpr Time operator/(Time lhs, int64_t rhs);
    friend constexpr bool operator==(Time lhs, Time rhs);
    friend constexpr bool operator!=(Time lhs, Time rhs);
    friend constexpr bool operator<(Time lhs, Time rhs);
    friend constexpr bool operator>(Time lhs, Time rhs);
    friend constexpr bool operator<=(Time lhs, Time rhs);
    friend constexpr bool operator>=(Time lhs, Time rhs);
};

// Operators
constexpr Time operator+(Time lhs, Time rhs)
{
    return Time(lhs.m_microseconds + rhs.m_microseconds);
}

constexpr Time operator-(Time lhs, Time rhs)
{
    return Time(lhs.m_microseconds - rhs.m_microseconds);
}

constexpr Time operator*(Time lhs, float rhs)
{
    return Time(static_cast<int64_t>(lhs.m_microseconds * rhs));
}

constexpr Time operator*(Time lhs, int64_t rhs)
{
    return Time(lhs.m_microseconds * rhs);
}

constexpr Time operator*(float lhs, Time rhs)
{
    return rhs * lhs;
}

constexpr Time operator*(int64_t lhs, Time rhs)
{
    return rhs * lhs;
}

constexpr Time operator/(Time lhs, float rhs)
{
    return Time(static_cast<int64_t>(lhs.m_microseconds / rhs));
}

constexpr Time operator/(Time lhs, int64_t rhs)
{
    return Time(lhs.m_microseconds / rhs);
}

constexpr bool operator==(Time lhs, Time rhs)
{
    return lhs.m_microseconds == rhs.m_microseconds;
}

constexpr bool operator!=(Time lhs, Time rhs)
{
    return !(lhs == rhs);
}

constexpr bool operator<(Time lhs, Time rhs)
{
    return lhs.m_microseconds < rhs.m_microseconds;
}

constexpr bool operator>(Time lhs, Time rhs)
{
    return rhs < lhs;
}

constexpr bool operator<=(Time lhs, Time rhs)
{
    return !(rhs < lhs);
}

constexpr bool operator>=(Time lhs, Time rhs)
{
    return !(lhs < rhs);
}

inline const Time Time::Zero;

// Sleep function
inline void sleep(Time duration)
{
    if (duration > Time::Zero)
    {
        std::this_thread::sleep_for(
            std::chrono::microseconds(duration.asMicroseconds()));
    }
}

TG_NAMESPACE_END
