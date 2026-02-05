/**
 * @file Shape.hpp
 * @brief Base class for all shapes
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "../Graphics/Drawable.hpp"
#include "../Graphics/Transformable.hpp"
#include "../Graphics/RenderTarget.hpp"
#include <vector>

TG_NAMESPACE_BEGIN

/**
 * @brief Base class for all shape types
 *
 * Provides common functionality for shapes including fill color,
 * outline color, outline thickness, and vertex management.
 */
class Shape : public Drawable, public Transformable
{
public:
    virtual ~Shape() = default;

    /**
     * @brief Set the fill color
     */
    void setFillColor(const Color &color)
    {
        m_fillColor = color;
    }

    /**
     * @brief Get the fill color
     */
    const Color &getFillColor() const
    {
        return m_fillColor;
    }

    /**
     * @brief Set the outline color
     */
    void setOutlineColor(const Color &color)
    {
        m_outlineColor = color;
    }

    /**
     * @brief Get the outline color
     */
    const Color &getOutlineColor() const
    {
        return m_outlineColor;
    }

    /**
     * @brief Set the outline thickness
     * @param thickness Outline thickness in pixels
     */
    void setOutlineThickness(float thickness)
    {
        m_outlineThickness = thickness;
    }

    /**
     * @brief Get the outline thickness
     */
    float getOutlineThickness() const
    {
        return m_outlineThickness;
    }

    /**
     * @brief Get the number of points defining the shape
     */
    virtual size_t getPointCount() const = 0;

    /**
     * @brief Get a point of the shape in local coordinates
     * @param index Index of the point
     */
    virtual Vector2f getPoint(size_t index) const = 0;

    /**
     * @brief Get the local bounding rectangle
     */
    FloatRect getLocalBounds() const
    {
        return m_bounds;
    }

    /**
     * @brief Get the global bounding rectangle
     */
    FloatRect getGlobalBounds() const
    {
        Vector2f tl = transformPoint(Vector2f(m_bounds.left, m_bounds.top));
        Vector2f br = transformPoint(Vector2f(m_bounds.left + m_bounds.width,
                                              m_bounds.top + m_bounds.height));

        float minX = std::min(tl.x, br.x);
        float minY = std::min(tl.y, br.y);
        float maxX = std::max(tl.x, br.x);
        float maxY = std::max(tl.y, br.y);

        return FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

protected:
    /**
     * @brief Update the shape's geometry
     *
     * Call this after modifying points to recalculate bounds.
     */
    void update()
    {
        size_t count = getPointCount();
        if (count < 1)
        {
            m_bounds = FloatRect();
            return;
        }

        Vector2f point = getPoint(0);
        float minX = point.x, maxX = point.x;
        float minY = point.y, maxY = point.y;

        for (size_t i = 1; i < count; ++i)
        {
            point = getPoint(i);
            minX = std::min(minX, point.x);
            maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
        }

        m_bounds = FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

    void draw(RenderTarget &target) const override
    {
        // Get bounds in world coordinates
        FloatRect bounds = getGlobalBounds();

        // Fill the shape
        if (m_fillColor.a > 0)
        {
            // Use scanline fill for the polygon
            fillPolygon(target);
        }

        // Draw outline
        if (m_outlineThickness > 0 && m_outlineColor.a > 0)
        {
            size_t count = getPointCount();
            for (size_t i = 0; i < count; ++i)
            {
                Vector2f p1 = transformPoint(getPoint(i));
                Vector2f p2 = transformPoint(getPoint((i + 1) % count));

                target.drawLine(
                    static_cast<int>(p1.x), static_cast<int>(p1.y),
                    static_cast<int>(p2.x), static_cast<int>(p2.y),
                    m_outlineColor);
            }
        }
    }

private:
    Color m_fillColor = Color::White;
    Color m_outlineColor = Color::Transparent;
    float m_outlineThickness = 0;
    FloatRect m_bounds;

    /**
     * @brief Fill the polygon using scanline algorithm
     */
    void fillPolygon(RenderTarget &target) const
    {
        size_t count = getPointCount();
        if (count < 3)
            return;

        // Get all transformed points
        std::vector<Vector2f> points(count);
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();

        for (size_t i = 0; i < count; ++i)
        {
            points[i] = transformPoint(getPoint(i));
            minY = std::min(minY, points[i].y);
            maxY = std::max(maxY, points[i].y);
        }

        // Scanline fill
        for (int y = static_cast<int>(minY); y <= static_cast<int>(maxY); ++y)
        {
            std::vector<float> intersections;

            for (size_t i = 0; i < count; ++i)
            {
                const Vector2f &p1 = points[i];
                const Vector2f &p2 = points[(i + 1) % count];

                // Check if scanline intersects this edge
                if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y))
                {
                    float x = p1.x + (y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x);
                    intersections.push_back(x);
                }
            }

            std::sort(intersections.begin(), intersections.end());

            // Fill between pairs of intersections
            for (size_t i = 0; i + 1 < intersections.size(); i += 2)
            {
                int x1 = static_cast<int>(intersections[i]);
                int x2 = static_cast<int>(intersections[i + 1]);
                for (int x = x1; x <= x2; ++x)
                {
                    target.setPixel(static_cast<unsigned int>(x),
                                    static_cast<unsigned int>(y),
                                    m_fillColor);
                }
            }
        }
    }
};

TG_NAMESPACE_END
