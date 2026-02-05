/**
 * @file Transformable.hpp
 * @brief Position, rotation, scale, and origin for objects
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Vector2.hpp"
#include <cmath>

TG_NAMESPACE_BEGIN

/**
 * @brief Provides position, rotation, scale, and origin
 *
 * Base class for objects that can be transformed in 2D space.
 * Note: Terminal rendering has limited rotation support - rotation
 * is primarily useful for game logic rather than visual rotation.
 *
 * @code
 * class Player : public Transformable, public Drawable
 * {
 *     void update()
 *     {
 *         move(1, 0);  // Move right
 *         rotate(5);   // Rotate 5 degrees
 *     }
 * };
 * @endcode
 */
class Transformable
{
public:
    Transformable() = default;
    virtual ~Transformable() = default;

    /**
     * @brief Set the position
     * @param x X coordinate
     * @param y Y coordinate
     */
    void setPosition(float x, float y)
    {
        m_position.x = x;
        m_position.y = y;
        m_transformNeedsUpdate = true;
    }

    /**
     * @brief Set the position
     * @param position New position
     */
    void setPosition(const Vector2f &position)
    {
        m_position = position;
        m_transformNeedsUpdate = true;
    }

    /**
     * @brief Set the rotation angle
     * @param angle Rotation in degrees
     */
    void setRotation(float angle)
    {
        m_rotation = std::fmod(angle, 360.0f);
        if (m_rotation < 0)
            m_rotation += 360.0f;
        m_transformNeedsUpdate = true;
    }

    /**
     * @brief Set the scale factors
     * @param factorX Horizontal scale
     * @param factorY Vertical scale
     */
    void setScale(float factorX, float factorY)
    {
        m_scale.x = factorX;
        m_scale.y = factorY;
        m_transformNeedsUpdate = true;
    }

    /**
     * @brief Set the scale factors
     * @param factors Scale vector
     */
    void setScale(const Vector2f &factors)
    {
        m_scale = factors;
        m_transformNeedsUpdate = true;
    }

    /**
     * @brief Set the local origin point
     * @param x X coordinate of origin
     * @param y Y coordinate of origin
     *
     * The origin is the center point for rotation and scaling.
     */
    void setOrigin(float x, float y)
    {
        m_origin.x = x;
        m_origin.y = y;
        m_transformNeedsUpdate = true;
    }

    /**
     * @brief Set the local origin point
     * @param origin New origin
     */
    void setOrigin(const Vector2f &origin)
    {
        m_origin = origin;
        m_transformNeedsUpdate = true;
    }

    /**
     * @brief Get the position
     */
    const Vector2f &getPosition() const
    {
        return m_position;
    }

    /**
     * @brief Get the rotation angle
     */
    float getRotation() const
    {
        return m_rotation;
    }

    /**
     * @brief Get the scale factors
     */
    const Vector2f &getScale() const
    {
        return m_scale;
    }

    /**
     * @brief Get the origin
     */
    const Vector2f &getOrigin() const
    {
        return m_origin;
    }

    /**
     * @brief Move by an offset
     * @param offsetX X offset
     * @param offsetY Y offset
     */
    void move(float offsetX, float offsetY)
    {
        setPosition(m_position.x + offsetX, m_position.y + offsetY);
    }

    /**
     * @brief Move by an offset
     * @param offset Movement vector
     */
    void move(const Vector2f &offset)
    {
        setPosition(m_position + offset);
    }

    /**
     * @brief Rotate by an angle
     * @param angle Rotation offset in degrees
     */
    void rotate(float angle)
    {
        setRotation(m_rotation + angle);
    }

    /**
     * @brief Scale by factors
     * @param factorX Horizontal scale factor
     * @param factorY Vertical scale factor
     */
    void scale(float factorX, float factorY)
    {
        setScale(m_scale.x * factorX, m_scale.y * factorY);
    }

    /**
     * @brief Scale by factors
     * @param factor Scale vector
     */
    void scale(const Vector2f &factor)
    {
        setScale(m_scale.x * factor.x, m_scale.y * factor.y);
    }

    /**
     * @brief Transform a point from local to world coordinates
     * @param point Point in local coordinates
     * @return Point in world coordinates
     */
    Vector2f transformPoint(const Vector2f &point) const
    {
        // Apply origin
        Vector2f p = point - m_origin;

        // Apply scale
        p.x *= m_scale.x;
        p.y *= m_scale.y;

        // Apply rotation
        if (m_rotation != 0)
        {
            float rad = m_rotation * 3.14159265359f / 180.0f;
            float cos = std::cos(rad);
            float sin = std::sin(rad);
            Vector2f rotated;
            rotated.x = p.x * cos - p.y * sin;
            rotated.y = p.x * sin + p.y * cos;
            p = rotated;
        }

        // Apply translation
        return p + m_position;
    }

    /**
     * @brief Transform a point from world to local coordinates
     * @param point Point in world coordinates
     * @return Point in local coordinates
     */
    Vector2f inverseTransformPoint(const Vector2f &point) const
    {
        // Reverse translation
        Vector2f p = point - m_position;

        // Reverse rotation
        if (m_rotation != 0)
        {
            float rad = -m_rotation * 3.14159265359f / 180.0f;
            float cos = std::cos(rad);
            float sin = std::sin(rad);
            Vector2f rotated;
            rotated.x = p.x * cos - p.y * sin;
            rotated.y = p.x * sin + p.y * cos;
            p = rotated;
        }

        // Reverse scale
        if (m_scale.x != 0)
            p.x /= m_scale.x;
        if (m_scale.y != 0)
            p.y /= m_scale.y;

        // Reverse origin
        return p + m_origin;
    }

protected:
    Vector2f m_position{0, 0};
    float m_rotation = 0;
    Vector2f m_scale{1, 1};
    Vector2f m_origin{0, 0};
    mutable bool m_transformNeedsUpdate = true;
};

TG_NAMESPACE_END
