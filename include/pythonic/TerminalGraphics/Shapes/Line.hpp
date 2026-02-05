/**
 * @file Line.hpp
 * @brief Line shape drawable
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "../Graphics/Drawable.hpp"
#include "../Graphics/Transformable.hpp"
#include "../Graphics/RenderTarget.hpp"

TG_NAMESPACE_BEGIN

/**
 * @brief Drawable line segment
 *
 * A simple line from point A to point B.
 *
 * @code
 * Line line(Vector2f(0, 0), Vector2f(50, 30));
 * line.setColor(Color::White);
 * line.setThickness(1);
 *
 * canvas.draw(line);
 * @endcode
 */
class Line : public Drawable, public Transformable
{
public:
    /**
     * @brief Default constructor
     */
    Line() : m_start(0, 0), m_end(0, 0), m_color(Color::White), m_thickness(1) {}

    /**
     * @brief Create a line from start to end
     */
    Line(const Vector2f &start, const Vector2f &end, const Color &color = Color::White)
        : m_start(start), m_end(end), m_color(color), m_thickness(1) {}

    /**
     * @brief Set the start point
     */
    void setStart(const Vector2f &point) { m_start = point; }

    /**
     * @brief Get the start point
     */
    const Vector2f &getStart() const { return m_start; }

    /**
     * @brief Set the end point
     */
    void setEnd(const Vector2f &point) { m_end = point; }

    /**
     * @brief Get the end point
     */
    const Vector2f &getEnd() const { return m_end; }

    /**
     * @brief Set the line color
     */
    void setColor(const Color &color) { m_color = color; }

    /**
     * @brief Get the line color
     */
    const Color &getColor() const { return m_color; }

    /**
     * @brief Set the line thickness
     * @param thickness Thickness in pixels
     */
    void setThickness(float thickness) { m_thickness = thickness; }

    /**
     * @brief Get the line thickness
     */
    float getThickness() const { return m_thickness; }

    /**
     * @brief Get the length of the line
     */
    float getLength() const
    {
        return (m_end - m_start).length();
    }

    /**
     * @brief Get the local bounding rectangle
     */
    FloatRect getLocalBounds() const
    {
        float minX = std::min(m_start.x, m_end.x);
        float minY = std::min(m_start.y, m_end.y);
        float maxX = std::max(m_start.x, m_end.x);
        float maxY = std::max(m_start.y, m_end.y);
        return FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

    /**
     * @brief Get the global bounding rectangle
     */
    FloatRect getGlobalBounds() const
    {
        Vector2f ts = transformPoint(m_start);
        Vector2f te = transformPoint(m_end);

        float minX = std::min(ts.x, te.x);
        float minY = std::min(ts.y, te.y);
        float maxX = std::max(ts.x, te.x);
        float maxY = std::max(ts.y, te.y);

        return FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

protected:
    void draw(RenderTarget &target) const override
    {
        Vector2f ts = transformPoint(m_start);
        Vector2f te = transformPoint(m_end);

        target.drawLine(
            static_cast<int>(ts.x), static_cast<int>(ts.y),
            static_cast<int>(te.x), static_cast<int>(te.y),
            m_color);
    }

private:
    Vector2f m_start;
    Vector2f m_end;
    Color m_color;
    float m_thickness;
};

TG_NAMESPACE_END
