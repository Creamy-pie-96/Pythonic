/**
 * @file Drawable.hpp
 * @brief Base interface for all drawable objects
 */

#pragma once

#include "../Config.hpp"

TG_NAMESPACE_BEGIN

// Forward declaration
class RenderTarget;

/**
 * @brief Abstract base class for drawable objects
 *
 * All objects that can be drawn to a RenderTarget must inherit from Drawable
 * and implement the draw() method.
 *
 * @code
 * class MySprite : public Drawable
 * {
 * protected:
 *     void draw(RenderTarget& target, RenderStates states) const override
 *     {
 *         // Drawing implementation
 *     }
 * };
 * @endcode
 */
class Drawable
{
public:
    virtual ~Drawable() = default;

protected:
    friend class RenderTarget;

    /**
     * @brief Draw the object to a render target
     * @param target Render target to draw to
     *
     * This is called by RenderTarget::draw(). Override this to implement
     * custom drawing logic.
     */
    virtual void draw(RenderTarget &target) const = 0;
};

TG_NAMESPACE_END
