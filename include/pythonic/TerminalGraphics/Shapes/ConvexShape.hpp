/**
 * @file ConvexShape.hpp
 * @brief Convex polygon shape drawable
 */

#pragma once

#include "../Config.hpp"
#include "Shape.hpp"
#include <vector>

TG_NAMESPACE_BEGIN

/**
 * @brief Drawable convex polygon shape
 *
 * A shape defined by a set of points forming a convex polygon.
 * Points should be defined in order (clockwise or counter-clockwise).
 *
 * @code
 * ConvexShape triangle;
 * triangle.setPointCount(3);
 * triangle.setPoint(0, Vector2f(0, 20));    // Bottom-left
 * triangle.setPoint(1, Vector2f(10, 0));    // Top
 * triangle.setPoint(2, Vector2f(20, 20));   // Bottom-right
 * triangle.setFillColor(Color::Green);
 *
 * canvas.draw(triangle);
 * @endcode
 */
class ConvexShape : public Shape
{
public:
    /**
     * @brief Default constructor
     * @param pointCount Initial number of points
     */
    explicit ConvexShape(size_t pointCount = 0)
    {
        setPointCount(pointCount);
    }

    /**
     * @brief Set the number of points
     * @param count New point count
     */
    void setPointCount(size_t count)
    {
        m_points.resize(count);
        update();
    }

    /**
     * @brief Get the number of points
     */
    size_t getPointCount() const override
    {
        return m_points.size();
    }

    /**
     * @brief Set a point's position
     * @param index Point index
     * @param point New position
     */
    void setPoint(size_t index, const Vector2f &point)
    {
        if (index < m_points.size())
        {
            m_points[index] = point;
            update();
        }
    }

    /**
     * @brief Get a point's position
     * @param index Point index
     */
    Vector2f getPoint(size_t index) const override
    {
        if (index < m_points.size())
            return m_points[index];
        return Vector2f(0, 0);
    }

    /**
     * @brief Add a point to the shape
     * @param point Position of the new point
     */
    void addPoint(const Vector2f &point)
    {
        m_points.push_back(point);
        update();
    }

    /**
     * @brief Clear all points
     */
    void clear()
    {
        m_points.clear();
        update();
    }

private:
    std::vector<Vector2f> m_points;
};

TG_NAMESPACE_END
