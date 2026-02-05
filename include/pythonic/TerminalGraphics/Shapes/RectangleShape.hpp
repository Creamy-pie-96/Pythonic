/**
 * @file RectangleShape.hpp
 * @brief Rectangle shape drawable
 */

#pragma once

#include "../Config.hpp"
#include "Shape.hpp"

TG_NAMESPACE_BEGIN

/**
 * @brief Drawable rectangle shape
 *
 * A specialized shape representing a rectangle. Like sf::RectangleShape,
 * it provides a simple way to draw filled/outlined rectangles.
 *
 * @code
 * RectangleShape rect(Vector2f(50, 30));
 * rect.setPosition(10, 10);
 * rect.setFillColor(Color::Blue);
 * rect.setOutlineColor(Color::White);
 * rect.setOutlineThickness(1);
 *
 * canvas.draw(rect);
 * @endcode
 */
class RectangleShape : public Shape
{
public:
    /**
     * @brief Default constructor (creates empty rectangle)
     */
    RectangleShape() : m_size(0, 0)
    {
        update();
    }

    /**
     * @brief Create a rectangle with the given size
     * @param size Width and height
     */
    explicit RectangleShape(const Vector2f &size) : m_size(size)
    {
        update();
    }

    /**
     * @brief Create a rectangle with width and height
     */
    RectangleShape(float width, float height) : m_size(width, height)
    {
        update();
    }

    /**
     * @brief Set the size of the rectangle
     * @param size New size
     */
    void setSize(const Vector2f &size)
    {
        m_size = size;
        update();
    }

    /**
     * @brief Set the size of the rectangle
     */
    void setSize(float width, float height)
    {
        m_size = Vector2f(width, height);
        update();
    }

    /**
     * @brief Get the size of the rectangle
     */
    const Vector2f &getSize() const
    {
        return m_size;
    }

    /**
     * @brief Get the number of points (always 4 for rectangle)
     */
    size_t getPointCount() const override
    {
        return 4;
    }

    /**
     * @brief Get a corner point
     * @param index Corner index (0=top-left, clockwise)
     */
    Vector2f getPoint(size_t index) const override
    {
        switch (index)
        {
        case 0:
            return Vector2f(0, 0);
        case 1:
            return Vector2f(m_size.x, 0);
        case 2:
            return Vector2f(m_size.x, m_size.y);
        case 3:
            return Vector2f(0, m_size.y);
        default:
            return Vector2f(0, 0);
        }
    }

private:
    Vector2f m_size;
};

TG_NAMESPACE_END
