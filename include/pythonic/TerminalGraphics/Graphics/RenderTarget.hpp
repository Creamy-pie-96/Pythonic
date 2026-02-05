/**
 * @file RenderTarget.hpp
 * @brief Base class for all render targets (windows, textures, etc.)
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "Drawable.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

TG_NAMESPACE_BEGIN

/**
 * @brief Pixel data for a single cell
 */
struct Pixel
{
    Color color = Color::Black;

    Pixel() = default;
    explicit Pixel(const Color &c) : color(c) {}

    bool operator==(const Pixel &other) const { return color == other.color; }
    bool operator!=(const Pixel &other) const { return !(*this == other); }
};

/**
 * @brief Base class for render targets
 *
 * Provides a common interface for drawing to windows, textures, etc.
 * This is the terminal graphics equivalent of sf::RenderTarget.
 */
class RenderTarget
{
public:
    virtual ~RenderTarget() = default;

    /**
     * @brief Get the size of the render target
     */
    virtual Vector2u getSize() const = 0;

    /**
     * @brief Clear the target with a color
     */
    virtual void clear(const Color &color = Color::Black)
    {
        Pixel clearPixel(color);
        std::fill(m_pixels.begin(), m_pixels.end(), clearPixel);
    }

    /**
     * @brief Draw a drawable object
     */
    virtual void draw(const Drawable &drawable)
    {
        drawable.draw(*this);
    }

    /**
     * @brief Set a pixel at the given position
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Pixel color
     */
    void setPixel(unsigned int x, unsigned int y, const Color &color)
    {
        Vector2u size = getSize();
        if (x >= size.x || y >= size.y)
            return;

        m_pixels[y * size.x + x].color = color;
    }

    /**
     * @brief Get a pixel at the given position
     * @param x X coordinate
     * @param y Y coordinate
     * @return The pixel color
     */
    Color getPixel(unsigned int x, unsigned int y) const
    {
        Vector2u size = getSize();
        if (x >= size.x || y >= size.y)
            return Color::Black;

        return m_pixels[y * size.x + x].color;
    }

    /**
     * @brief Draw a line using Bresenham's algorithm
     * @param x0 Start X
     * @param y0 Start Y
     * @param x1 End X
     * @param y1 End Y
     * @param color Line color
     */
    void drawLine(int x0, int y0, int x1, int y1, const Color &color)
    {
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true)
        {
            setPixel(static_cast<unsigned int>(x0),
                     static_cast<unsigned int>(y0), color);

            if (x0 == x1 && y0 == y1)
                break;

            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    /**
     * @brief Draw a rectangle outline
     */
    void drawRect(int x, int y, int width, int height, const Color &color)
    {
        drawLine(x, y, x + width - 1, y, color);                           // Top
        drawLine(x, y + height - 1, x + width - 1, y + height - 1, color); // Bottom
        drawLine(x, y, x, y + height - 1, color);                          // Left
        drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);  // Right
    }

    /**
     * @brief Draw a filled rectangle
     */
    void fillRect(int x, int y, int width, int height, const Color &color)
    {
        for (int py = y; py < y + height; ++py)
        {
            for (int px = x; px < x + width; ++px)
            {
                setPixel(static_cast<unsigned int>(px),
                         static_cast<unsigned int>(py), color);
            }
        }
    }

    /**
     * @brief Draw a circle outline using midpoint algorithm
     */
    void drawCircle(int cx, int cy, int radius, const Color &color)
    {
        int x = radius;
        int y = 0;
        int err = 0;

        while (x >= y)
        {
            setPixel(cx + x, cy + y, color);
            setPixel(cx + y, cy + x, color);
            setPixel(cx - y, cy + x, color);
            setPixel(cx - x, cy + y, color);
            setPixel(cx - x, cy - y, color);
            setPixel(cx - y, cy - x, color);
            setPixel(cx + y, cy - x, color);
            setPixel(cx + x, cy - y, color);

            y++;
            err += 1 + 2 * y;
            if (2 * (err - x) + 1 > 0)
            {
                x--;
                err += 1 - 2 * x;
            }
        }
    }

    /**
     * @brief Draw a filled circle
     */
    void fillCircle(int cx, int cy, int radius, const Color &color)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            for (int x = -radius; x <= radius; ++x)
            {
                if (x * x + y * y <= radius * radius)
                {
                    setPixel(static_cast<unsigned int>(cx + x),
                             static_cast<unsigned int>(cy + y), color);
                }
            }
        }
    }

    /**
     * @brief Draw a quadratic Bezier curve
     * @param p0 Start point
     * @param p1 Control point
     * @param p2 End point
     * @param color Line color
     * @param segments Number of line segments (higher = smoother)
     */
    void drawBezierQuadratic(Vector2f p0, Vector2f p1, Vector2f p2,
                             const Color &color, int segments = 20)
    {
        Vector2f prev = p0;
        for (int i = 1; i <= segments; ++i)
        {
            float t = static_cast<float>(i) / segments;
            float u = 1.0f - t;

            // Quadratic Bezier: B(t) = (1-t)²P0 + 2(1-t)tP1 + t²P2
            Vector2f point(
                u * u * p0.x + 2 * u * t * p1.x + t * t * p2.x,
                u * u * p0.y + 2 * u * t * p1.y + t * t * p2.y);

            drawLine(static_cast<int>(prev.x), static_cast<int>(prev.y),
                     static_cast<int>(point.x), static_cast<int>(point.y), color);
            prev = point;
        }
    }

    /**
     * @brief Draw a cubic Bezier curve
     * @param p0 Start point
     * @param p1 First control point
     * @param p2 Second control point
     * @param p3 End point
     * @param color Line color
     * @param segments Number of line segments (higher = smoother)
     */
    void drawBezierCubic(Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3,
                         const Color &color, int segments = 30)
    {
        Vector2f prev = p0;
        for (int i = 1; i <= segments; ++i)
        {
            float t = static_cast<float>(i) / segments;
            float u = 1.0f - t;

            // Cubic Bezier: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
            Vector2f point(
                u * u * u * p0.x + 3 * u * u * t * p1.x + 3 * u * t * t * p2.x + t * t * t * p3.x,
                u * u * u * p0.y + 3 * u * u * t * p1.y + 3 * u * t * t * p2.y + t * t * t * p3.y);

            drawLine(static_cast<int>(prev.x), static_cast<int>(prev.y),
                     static_cast<int>(point.x), static_cast<int>(point.y), color);
            prev = point;
        }
    }

    /**
     * @brief Draw a Catmull-Rom spline through control points
     * @param points Vector of control points (at least 4)
     * @param color Line color
     * @param segments Segments between each pair of points
     */
    void drawSpline(const std::vector<Vector2f> &points, const Color &color,
                    int segments = 10)
    {
        if (points.size() < 4)
            return;

        for (size_t i = 0; i + 3 < points.size(); ++i)
        {
            const Vector2f &p0 = points[i];
            const Vector2f &p1 = points[i + 1];
            const Vector2f &p2 = points[i + 2];
            const Vector2f &p3 = points[i + 3];

            Vector2f prev = p1;
            for (int j = 1; j <= segments; ++j)
            {
                float t = static_cast<float>(j) / segments;
                float t2 = t * t;
                float t3 = t2 * t;

                // Catmull-Rom matrix
                Vector2f point(
                    0.5f * ((2 * p1.x) + (-p0.x + p2.x) * t +
                            (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * t2 +
                            (-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * t3),
                    0.5f * ((2 * p1.y) + (-p0.y + p2.y) * t +
                            (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * t2 +
                            (-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * t3));

                drawLine(static_cast<int>(prev.x), static_cast<int>(prev.y),
                         static_cast<int>(point.x), static_cast<int>(point.y), color);
                prev = point;
            }
        }
    }

    /**
     * @brief Fill a convex polygon using scanline algorithm
     * @param points Polygon vertices (must be convex)
     * @param color Fill color
     */
    void fillConvexPolygon(const std::vector<Vector2f> &points, const Color &color)
    {
        if (points.size() < 3)
            return;

        // Find bounding box
        float minY = points[0].y, maxY = points[0].y;
        for (const auto &p : points)
        {
            if (p.y < minY)
                minY = p.y;
            if (p.y > maxY)
                maxY = p.y;
        }

        // Scanline fill
        for (int y = static_cast<int>(minY); y <= static_cast<int>(maxY); ++y)
        {
            std::vector<float> intersections;

            // Find edge intersections with this scanline
            for (size_t i = 0; i < points.size(); ++i)
            {
                const Vector2f &p1 = points[i];
                const Vector2f &p2 = points[(i + 1) % points.size()];

                // Check if edge crosses this scanline
                if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y))
                {
                    float x = p1.x + (y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x);
                    intersections.push_back(x);
                }
            }

            // Sort intersections
            std::sort(intersections.begin(), intersections.end());

            // Fill between pairs
            for (size_t i = 0; i + 1 < intersections.size(); i += 2)
            {
                int x1 = static_cast<int>(intersections[i]);
                int x2 = static_cast<int>(intersections[i + 1]);
                for (int x = x1; x <= x2; ++x)
                {
                    setPixel(static_cast<unsigned int>(x),
                             static_cast<unsigned int>(y), color);
                }
            }
        }
    }

    /**
     * @brief Fill any polygon (convex or concave) using even-odd rule
     * @param points Polygon vertices
     * @param color Fill color
     */
    void fillPolygon(const std::vector<Vector2f> &points, const Color &color)
    {
        if (points.size() < 3)
            return;

        // Find bounding box
        float minX = points[0].x, maxX = points[0].x;
        float minY = points[0].y, maxY = points[0].y;
        for (const auto &p : points)
        {
            if (p.x < minX)
                minX = p.x;
            if (p.x > maxX)
                maxX = p.x;
            if (p.y < minY)
                minY = p.y;
            if (p.y > maxY)
                maxY = p.y;
        }

        // Test each pixel in bounding box
        for (int y = static_cast<int>(minY); y <= static_cast<int>(maxY); ++y)
        {
            for (int x = static_cast<int>(minX); x <= static_cast<int>(maxX); ++x)
            {
                // Point-in-polygon test using ray casting (even-odd rule)
                bool inside = false;
                float px = static_cast<float>(x) + 0.5f;
                float py = static_cast<float>(y) + 0.5f;

                for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++)
                {
                    if (((points[i].y > py) != (points[j].y > py)) &&
                        (px < (points[j].x - points[i].x) * (py - points[i].y) /
                                      (points[j].y - points[i].y) +
                                  points[i].x))
                    {
                        inside = !inside;
                    }
                }

                if (inside)
                {
                    setPixel(static_cast<unsigned int>(x),
                             static_cast<unsigned int>(y), color);
                }
            }
        }
    }

    /**
     * @brief Draw a polygon outline
     * @param points Polygon vertices
     * @param color Line color
     */
    void drawPolygon(const std::vector<Vector2f> &points, const Color &color)
    {
        if (points.size() < 2)
            return;

        for (size_t i = 0; i < points.size(); ++i)
        {
            const Vector2f &p1 = points[i];
            const Vector2f &p2 = points[(i + 1) % points.size()];
            drawLine(static_cast<int>(p1.x), static_cast<int>(p1.y),
                     static_cast<int>(p2.x), static_cast<int>(p2.y), color);
        }
    }

    /**
     * @brief Draw a thick line
     * @param x0 Start X
     * @param y0 Start Y
     * @param x1 End X
     * @param y1 End Y
     * @param color Line color
     * @param thickness Line thickness
     */
    void drawThickLine(int x0, int y0, int x1, int y1, const Color &color, int thickness)
    {
        if (thickness <= 1)
        {
            drawLine(x0, y0, x1, y1, color);
            return;
        }

        // Calculate perpendicular vector
        float dx = static_cast<float>(x1 - x0);
        float dy = static_cast<float>(y1 - y0);
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f)
            return;

        float perpX = -dy / len;
        float perpY = dx / len;

        float halfThick = thickness / 2.0f;

        // Create polygon for thick line
        std::vector<Vector2f> quad = {
            {x0 - perpX * halfThick, y0 - perpY * halfThick},
            {x0 + perpX * halfThick, y0 + perpY * halfThick},
            {x1 + perpX * halfThick, y1 + perpY * halfThick},
            {x1 - perpX * halfThick, y1 - perpY * halfThick}};

        fillConvexPolygon(quad, color);
    }

    /**
     * @brief Draw an ellipse outline
     */
    void drawEllipse(int cx, int cy, int rx, int ry, const Color &color)
    {
        // Midpoint ellipse algorithm
        int x = 0;
        int y = ry;

        // Region 1
        int d1 = (ry * ry) - (rx * rx * ry) + (rx * rx / 4);
        int dx = 2 * ry * ry * x;
        int dy = 2 * rx * rx * y;

        while (dx < dy)
        {
            setPixel(cx + x, cy + y, color);
            setPixel(cx - x, cy + y, color);
            setPixel(cx + x, cy - y, color);
            setPixel(cx - x, cy - y, color);

            if (d1 < 0)
            {
                x++;
                dx = dx + (2 * ry * ry);
                d1 = d1 + dx + (ry * ry);
            }
            else
            {
                x++;
                y--;
                dx = dx + (2 * ry * ry);
                dy = dy - (2 * rx * rx);
                d1 = d1 + dx - dy + (ry * ry);
            }
        }

        // Region 2
        int d2 = ((ry * ry) * ((x + 0.5f) * (x + 0.5f))) +
                 ((rx * rx) * ((y - 1) * (y - 1))) - (rx * rx * ry * ry);

        while (y >= 0)
        {
            setPixel(cx + x, cy + y, color);
            setPixel(cx - x, cy + y, color);
            setPixel(cx + x, cy - y, color);
            setPixel(cx - x, cy - y, color);

            if (d2 > 0)
            {
                y--;
                dy = dy - (2 * rx * rx);
                d2 = d2 + (rx * rx) - dy;
            }
            else
            {
                y--;
                x++;
                dx = dx + (2 * ry * ry);
                dy = dy - (2 * rx * rx);
                d2 = d2 + dx - dy + (rx * rx);
            }
        }
    }

    /**
     * @brief Draw a filled ellipse
     */
    void fillEllipse(int cx, int cy, int rx, int ry, const Color &color)
    {
        for (int y = -ry; y <= ry; ++y)
        {
            // Calculate x range for this y using ellipse equation
            // (x/rx)² + (y/ry)² <= 1
            // x² <= rx² * (1 - y²/ry²)
            float ySq = static_cast<float>(y * y);
            float rySq = static_cast<float>(ry * ry);
            float rxSq = static_cast<float>(rx * rx);
            float xMax = std::sqrt(rxSq * (1.0f - ySq / rySq));

            int x1 = static_cast<int>(-xMax);
            int x2 = static_cast<int>(xMax);

            for (int x = x1; x <= x2; ++x)
            {
                setPixel(static_cast<unsigned int>(cx + x),
                         static_cast<unsigned int>(cy + y), color);
            }
        }
    }

    /**
     * @brief Draw an arc
     * @param cx Center X
     * @param cy Center Y
     * @param radius Arc radius
     * @param startAngle Start angle in degrees
     * @param endAngle End angle in degrees
     * @param color Arc color
     */
    void drawArc(int cx, int cy, int radius, float startAngle, float endAngle,
                 const Color &color)
    {
        const float PI = 3.14159265358979f;
        float startRad = startAngle * PI / 180.0f;
        float endRad = endAngle * PI / 180.0f;

        int segments = static_cast<int>(std::abs(endAngle - startAngle) / 5.0f);
        if (segments < 10)
            segments = 10;

        float angleStep = (endRad - startRad) / segments;

        int prevX = cx + static_cast<int>(std::cos(startRad) * radius);
        int prevY = cy + static_cast<int>(std::sin(startRad) * radius);

        for (int i = 1; i <= segments; ++i)
        {
            float angle = startRad + i * angleStep;
            int x = cx + static_cast<int>(std::cos(angle) * radius);
            int y = cy + static_cast<int>(std::sin(angle) * radius);

            drawLine(prevX, prevY, x, y, color);
            prevX = x;
            prevY = y;
        }
    }

    /**
     * @brief Get access to the pixel buffer
     */
    const std::vector<Pixel> &getPixels() const { return m_pixels; }
    std::vector<Pixel> &getPixels() { return m_pixels; }

protected:
    std::vector<Pixel> m_pixels;

    /**
     * @brief Initialize the pixel buffer for the given size
     */
    void initPixelBuffer(unsigned int width, unsigned int height)
    {
        m_pixels.resize(static_cast<size_t>(width) * height);
    }
};

TG_NAMESPACE_END
