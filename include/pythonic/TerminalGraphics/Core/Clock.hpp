/**
 * @file Clock.hpp
 * @brief High-precision clock for timing game loops and animations
 */

#pragma once

#include "../Config.hpp"
#include "Time.hpp"
#include <chrono>

TG_NAMESPACE_BEGIN

/**
 * @brief High-precision clock for measuring elapsed time
 *
 * Used for game loops, delta time calculation, and profiling.
 *
 * @code
 * Clock clock;
 * while (running) {
 *     Time dt = clock.restart();
 *     // Update game with dt...
 * }
 * @endcode
 */
class Clock
{
public:
    /**
     * @brief Create a clock and start it
     */
    Clock() : m_startTime(std::chrono::steady_clock::now()) {}

    /**
     * @brief Get elapsed time since clock was started or restarted
     * @return Time elapsed
     */
    Time getElapsedTime() const
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            now - m_startTime);
        return Time::microseconds(duration.count());
    }

    /**
     * @brief Restart the clock and return elapsed time since last restart
     * @return Time elapsed since last restart
     *
     * This is useful for calculating delta time in game loops.
     */
    Time restart()
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            now - m_startTime);
        m_startTime = now;
        return Time::microseconds(duration.count());
    }

private:
    std::chrono::steady_clock::time_point m_startTime;
};

TG_NAMESPACE_END
