/**
 * @file View.hpp
 * @brief Camera/View system for terminal graphics (SFML-compatible API)
 *
 * Provides camera functionality for scrolling, zooming, and rotating views.
 * Essential for platformers, shooters, RPGs, and any scrolling game.
 *
 * Usage:
 *   View camera(Vector2f(0, 0), Vector2f(160, 120)); // Center at origin, 160x120 view
 *   camera.setCenter(player.x, player.y);            // Follow player
 *   camera.zoom(0.5f);                               // Zoom in 2x
 *   canvas.setView(camera);
 *   canvas.draw(sprite);                             // Drawn relative to view
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Vector2.hpp"
#include "../Core/Rect.hpp"
#include <cmath>
#include <array>

TG_NAMESPACE_BEGIN

/**
 * @brief 2D transformation matrix (3x3 stored as 9 floats)
 *
 * Column-major storage for compatibility with graphics APIs.
 * Performs 2D affine transformations (translate, rotate, scale).
 */
class Transform
{
public:
    /**
     * @brief Create identity transform
     */
    Transform()
    {
        // Identity matrix
        m_matrix = {
            1.0f, 0.0f, 0.0f, // column 0
            0.0f, 1.0f, 0.0f, // column 1
            0.0f, 0.0f, 1.0f  // column 2
        };
    }

    /**
     * @brief Create transform from matrix elements
     */
    Transform(float a00, float a01, float a02,
              float a10, float a11, float a12,
              float a20, float a21, float a22)
    {
        m_matrix = {a00, a10, a20, a01, a11, a21, a02, a12, a22};
    }

    /**
     * @brief Get the raw matrix (3x3 column-major)
     */
    const float *getMatrix() const { return m_matrix.data(); }

    /**
     * @brief Get inverse transform
     */
    Transform getInverse() const
    {
        // Calculate determinant
        float det = m_matrix[0] * (m_matrix[4] * m_matrix[8] - m_matrix[5] * m_matrix[7]) - m_matrix[3] * (m_matrix[1] * m_matrix[8] - m_matrix[2] * m_matrix[7]) + m_matrix[6] * (m_matrix[1] * m_matrix[5] - m_matrix[2] * m_matrix[4]);

        if (std::abs(det) < 1e-7f)
            return Transform(); // Return identity if not invertible

        float invDet = 1.0f / det;

        return Transform(
            (m_matrix[4] * m_matrix[8] - m_matrix[5] * m_matrix[7]) * invDet,
            (m_matrix[6] * m_matrix[5] - m_matrix[3] * m_matrix[8]) * invDet,
            (m_matrix[3] * m_matrix[7] - m_matrix[6] * m_matrix[4]) * invDet,
            (m_matrix[7] * m_matrix[2] - m_matrix[1] * m_matrix[8]) * invDet,
            (m_matrix[0] * m_matrix[8] - m_matrix[6] * m_matrix[2]) * invDet,
            (m_matrix[6] * m_matrix[1] - m_matrix[0] * m_matrix[7]) * invDet,
            (m_matrix[1] * m_matrix[5] - m_matrix[4] * m_matrix[2]) * invDet,
            (m_matrix[3] * m_matrix[2] - m_matrix[0] * m_matrix[5]) * invDet,
            (m_matrix[0] * m_matrix[4] - m_matrix[3] * m_matrix[1]) * invDet);
    }

    /**
     * @brief Transform a point
     */
    Vector2f transformPoint(float x, float y) const
    {
        return Vector2f(
            m_matrix[0] * x + m_matrix[3] * y + m_matrix[6],
            m_matrix[1] * x + m_matrix[4] * y + m_matrix[7]);
    }

    Vector2f transformPoint(const Vector2f &point) const
    {
        return transformPoint(point.x, point.y);
    }

    /**
     * @brief Transform a rectangle (returns axis-aligned bounding box)
     */
    FloatRect transformRect(const FloatRect &rect) const
    {
        // Transform all 4 corners
        const Vector2f points[4] = {
            transformPoint(rect.left, rect.top),
            transformPoint(rect.left + rect.width, rect.top),
            transformPoint(rect.left, rect.top + rect.height),
            transformPoint(rect.left + rect.width, rect.top + rect.height)};

        // Find bounding box
        float left = points[0].x;
        float top = points[0].y;
        float right = points[0].x;
        float bottom = points[0].y;

        for (int i = 1; i < 4; ++i)
        {
            if (points[i].x < left)
                left = points[i].x;
            if (points[i].x > right)
                right = points[i].x;
            if (points[i].y < top)
                top = points[i].y;
            if (points[i].y > bottom)
                bottom = points[i].y;
        }

        return FloatRect(left, top, right - left, bottom - top);
    }

    /**
     * @brief Combine with another transform (multiply matrices)
     */
    Transform &combine(const Transform &other)
    {
        const float *a = m_matrix.data();
        const float *b = other.m_matrix.data();

        *this = Transform(
            a[0] * b[0] + a[3] * b[1] + a[6] * b[2],
            a[0] * b[3] + a[3] * b[4] + a[6] * b[5],
            a[0] * b[6] + a[3] * b[7] + a[6] * b[8],
            a[1] * b[0] + a[4] * b[1] + a[7] * b[2],
            a[1] * b[3] + a[4] * b[4] + a[7] * b[5],
            a[1] * b[6] + a[4] * b[7] + a[7] * b[8],
            a[2] * b[0] + a[5] * b[1] + a[8] * b[2],
            a[2] * b[3] + a[5] * b[4] + a[8] * b[5],
            a[2] * b[6] + a[5] * b[7] + a[8] * b[8]);
        return *this;
    }

    /**
     * @brief Apply translation
     */
    Transform &translate(float x, float y)
    {
        Transform translation(1, 0, x,
                              0, 1, y,
                              0, 0, 1);
        return combine(translation);
    }

    Transform &translate(const Vector2f &offset)
    {
        return translate(offset.x, offset.y);
    }

    /**
     * @brief Apply rotation (in degrees)
     */
    Transform &rotate(float angle)
    {
        float rad = angle * 3.14159265359f / 180.0f;
        float cos = std::cos(rad);
        float sin = std::sin(rad);

        Transform rotation(cos, -sin, 0,
                           sin, cos, 0,
                           0, 0, 1);
        return combine(rotation);
    }

    /**
     * @brief Apply rotation around a center point (in degrees)
     */
    Transform &rotate(float angle, float centerX, float centerY)
    {
        translate(centerX, centerY);
        rotate(angle);
        translate(-centerX, -centerY);
        return *this;
    }

    Transform &rotate(float angle, const Vector2f &center)
    {
        return rotate(angle, center.x, center.y);
    }

    /**
     * @brief Apply scaling
     */
    Transform &scale(float scaleX, float scaleY)
    {
        Transform scaling(scaleX, 0, 0,
                          0, scaleY, 0,
                          0, 0, 1);
        return combine(scaling);
    }

    Transform &scale(float scaleX, float scaleY, float centerX, float centerY)
    {
        translate(centerX, centerY);
        scale(scaleX, scaleY);
        translate(-centerX, -centerY);
        return *this;
    }

    Transform &scale(const Vector2f &factors)
    {
        return scale(factors.x, factors.y);
    }

    Transform &scale(const Vector2f &factors, const Vector2f &center)
    {
        return scale(factors.x, factors.y, center.x, center.y);
    }

    /**
     * @brief Multiply transforms
     */
    Transform operator*(const Transform &right) const
    {
        Transform result = *this;
        result.combine(right);
        return result;
    }

    Transform &operator*=(const Transform &right)
    {
        return combine(right);
    }

    /**
     * @brief Transform a point using *
     */
    Vector2f operator*(const Vector2f &point) const
    {
        return transformPoint(point);
    }

    /**
     * @brief Identity transform
     */
    static const Transform Identity;

private:
    std::array<float, 9> m_matrix; // 3x3 column-major
};

inline const Transform Transform::Identity;

/**
 * @brief 2D camera/view for terminal graphics
 *
 * SFML-compatible View class that provides:
 * - Camera position (center)
 * - View size (zoom level)
 * - Rotation
 * - Viewport (screen area to render to)
 * - Coordinate mapping between world and screen
 */
class View
{
public:
    /**
     * @brief Create default view (centered at origin, size matches screen)
     */
    View()
        : m_center(0, 0), m_size(100, 100), m_rotation(0), m_viewport(0, 0, 1, 1), m_transformUpdated(false), m_inverseTransformUpdated(false)
    {
    }

    /**
     * @brief Create view with center and size
     */
    View(const Vector2f &center, const Vector2f &size)
        : m_center(center), m_size(size), m_rotation(0), m_viewport(0, 0, 1, 1), m_transformUpdated(false), m_inverseTransformUpdated(false)
    {
    }

    /**
     * @brief Create view from a rectangle
     */
    explicit View(const FloatRect &rect)
        : m_center(rect.left + rect.width / 2, rect.top + rect.height / 2), m_size(rect.width, rect.height), m_rotation(0), m_viewport(0, 0, 1, 1), m_transformUpdated(false), m_inverseTransformUpdated(false)
    {
    }

    /**
     * @brief Set the center of the view
     */
    void setCenter(float x, float y)
    {
        m_center.x = x;
        m_center.y = y;
        m_transformUpdated = false;
        m_inverseTransformUpdated = false;
    }

    void setCenter(const Vector2f &center)
    {
        setCenter(center.x, center.y);
    }

    /**
     * @brief Set the size of the view
     */
    void setSize(float width, float height)
    {
        m_size.x = width;
        m_size.y = height;
        m_transformUpdated = false;
        m_inverseTransformUpdated = false;
    }

    void setSize(const Vector2f &size)
    {
        setSize(size.x, size.y);
    }

    /**
     * @brief Set rotation in degrees
     */
    void setRotation(float angle)
    {
        m_rotation = std::fmod(angle, 360.0f);
        if (m_rotation < 0)
            m_rotation += 360.0f;
        m_transformUpdated = false;
        m_inverseTransformUpdated = false;
    }

    /**
     * @brief Set viewport (0-1 normalized coordinates)
     *
     * The viewport defines which portion of the screen this view renders to.
     * Default is full screen (0, 0, 1, 1).
     */
    void setViewport(const FloatRect &viewport)
    {
        m_viewport = viewport;
    }

    /**
     * @brief Reset to a rectangular area
     */
    void reset(const FloatRect &rect)
    {
        m_center.x = rect.left + rect.width / 2;
        m_center.y = rect.top + rect.height / 2;
        m_size.x = rect.width;
        m_size.y = rect.height;
        m_rotation = 0;
        m_transformUpdated = false;
        m_inverseTransformUpdated = false;
    }

    /**
     * @brief Get the center
     */
    const Vector2f &getCenter() const { return m_center; }

    /**
     * @brief Get the size
     */
    const Vector2f &getSize() const { return m_size; }

    /**
     * @brief Get the rotation
     */
    float getRotation() const { return m_rotation; }

    /**
     * @brief Get the viewport
     */
    const FloatRect &getViewport() const { return m_viewport; }

    /**
     * @brief Move the view
     */
    void move(float offsetX, float offsetY)
    {
        setCenter(m_center.x + offsetX, m_center.y + offsetY);
    }

    void move(const Vector2f &offset)
    {
        move(offset.x, offset.y);
    }

    /**
     * @brief Rotate the view
     */
    void rotate(float angle)
    {
        setRotation(m_rotation + angle);
    }

    /**
     * @brief Zoom the view (multiply size by factor)
     *
     * @param factor Zoom factor. >1 zooms out, <1 zooms in
     */
    void zoom(float factor)
    {
        setSize(m_size.x * factor, m_size.y * factor);
    }

    /**
     * @brief Get the projection transform (world to view)
     */
    const Transform &getTransform() const
    {
        if (!m_transformUpdated)
        {
            // Translate to center, then rotate, then scale
            float angle = m_rotation * 3.14159265359f / 180.0f;
            float cosine = std::cos(angle);
            float sine = std::sin(angle);
            float a = 2.0f / m_size.x;
            float b = -2.0f / m_size.y;
            float c = -a * m_center.x;
            float d = -b * m_center.y;

            m_transform = Transform(
                a * cosine, a * sine, a * (cosine * (-m_center.x) + sine * (-m_center.y)) + m_center.x,
                -b * sine, b * cosine, b * (-sine * (-m_center.x) + cosine * (-m_center.y)) + m_center.y,
                0.0f, 0.0f, 1.0f);

            // Simplified transform: scale + translate
            m_transform = Transform();
            m_transform.translate(-m_center.x, -m_center.y);
            if (m_rotation != 0)
                m_transform.rotate(-m_rotation);
            // No explicit scale in projection - size handled in viewport

            m_transformUpdated = true;
        }
        return m_transform;
    }

    /**
     * @brief Get the inverse projection transform (view to world)
     */
    const Transform &getInverseTransform() const
    {
        if (!m_inverseTransformUpdated)
        {
            m_inverseTransform = getTransform().getInverse();
            m_inverseTransformUpdated = true;
        }
        return m_inverseTransform;
    }

private:
    Vector2f m_center;
    Vector2f m_size;
    float m_rotation;
    FloatRect m_viewport;

    mutable Transform m_transform;
    mutable Transform m_inverseTransform;
    mutable bool m_transformUpdated;
    mutable bool m_inverseTransformUpdated;
};

/**
 * @brief Blending modes for drawing operations
 *
 * Controls how source and destination colors are combined.
 */
enum class BlendMode
{
    Alpha,    ///< Standard alpha blending: src*srcA + dst*(1-srcA)
    Add,      ///< Additive blending: src + dst (clamped) - for glows, lights
    Multiply, ///< Multiplicative: src * dst - for shadows, tinting
    None      ///< No blending, direct overwrite
};

/**
 * @brief Render states for drawing
 *
 * Bundles transform, blend mode, and other states for draw calls.
 */
struct RenderStates
{
    Transform transform;
    BlendMode blendMode = BlendMode::Alpha;
    const class Texture *texture = nullptr;

    RenderStates() = default;

    explicit RenderStates(const Transform &theTransform)
        : transform(theTransform)
    {
    }

    explicit RenderStates(BlendMode theBlendMode)
        : blendMode(theBlendMode)
    {
    }

    explicit RenderStates(const Texture *theTexture)
        : texture(theTexture)
    {
    }

    RenderStates(BlendMode theBlendMode, const Transform &theTransform, const Texture *theTexture)
        : transform(theTransform), blendMode(theBlendMode), texture(theTexture)
    {
    }

    static const RenderStates Default;
};

inline const RenderStates RenderStates::Default;

TG_NAMESPACE_END
