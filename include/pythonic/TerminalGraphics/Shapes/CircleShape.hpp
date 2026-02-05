/**
 * @file CircleShape.hpp
 * @brief Circle/ellipse shape drawable
 */

#pragma once

#include "../Config.hpp"
#include "Shape.hpp"
#include <cmath>

TG_NAMESPACE_BEGIN

/**
 * @brief Drawable circle shape
 *
 * Represents a circle (or ellipse) that can be drawn to a render target.
 * The circle is approximated using a polygon with configurable point count.
 *
 * @code
 * CircleShape circle(20);  // radius 20
 * circle.setPosition(50, 50);
 * circle.setFillColor(Color::Red);
 * circle.setPointCount(32);  // smoother circle
 *
 * canvas.draw(circle);
 * @endcode
 */
class CircleShape : public Shape
{
public:
    /**
     * @brief Default constructor (creates empty circle)
     */
    CircleShape() : m_radius(0), m_pointCount(30)
    {
        update();
    }

    /**
     * @brief Create a circle with the given radius
     * @param radius Circle radius
     * @param pointCount Number of points for approximation (default: 30)
     */
    explicit CircleShape(float radius, size_t pointCount = 30)
        : m_radius(radius), m_pointCount(pointCount)
    {
        update();
    }

    /**
     * @brief Set the radius
     */
    void setRadius(float radius)
    {
        m_radius = radius;
        update();
    }

    /**
     * @brief Get the radius
     */
    float getRadius() const
    {
        return m_radius;
    }

    /**
     * @brief Set the number of points for polygon approximation
     * @param count Number of points (minimum 3)
     */
    void setPointCount(size_t count)
    {
        m_pointCount = count < 3 ? 3 : count;
        update();
    }

    /**
     * @brief Get the number of points
     */
    size_t getPointCount() const override
    {
        return m_pointCount;
    }

    /**
     * @brief Get a point on the circle
     * @param index Point index
     */
    Vector2f getPoint(size_t index) const override
    {
        float angle = static_cast<float>(index) / static_cast<float>(m_pointCount) * 2.0f * 3.14159265359f - 3.14159265359f / 2.0f;

        float x = m_radius + std::cos(angle) * m_radius;
        float y = m_radius + std::sin(angle) * m_radius;

        return Vector2f(x, y);
    }

private:
    float m_radius;
    size_t m_pointCount;
};

TG_NAMESPACE_END
