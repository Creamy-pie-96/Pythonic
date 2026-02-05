/**
 * @file Layer.hpp
 * @brief Z-ordering and layer management for drawable objects
 *
 * Provides SFML-style layering capabilities that SFML itself lacks.
 * Drawables can be assigned z-indices and will be rendered in order.
 */

#pragma once

#include "../Config.hpp"
#include "Drawable.hpp"
#include "RenderTarget.hpp"
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>

TG_NAMESPACE_BEGIN

/**
 * @brief A drawable with an associated z-index for layered rendering
 */
struct LayeredDrawable
{
    const Drawable *drawable = nullptr;
    int zIndex = 0;

    LayeredDrawable() = default;
    LayeredDrawable(const Drawable *d, int z) : drawable(d), zIndex(z) {}

    bool operator<(const LayeredDrawable &other) const
    {
        return zIndex < other.zIndex;
    }
};

/**
 * @brief Manages z-ordered rendering of drawable objects
 *
 * RenderQueue collects drawables with z-indices and renders them
 * in sorted order. Lower z-indices are drawn first (behind higher ones).
 *
 * @code
 * RenderQueue queue;
 *
 * // Add objects with z-indices
 * queue.add(background, 0);   // Drawn first
 * queue.add(player, 50);      // Drawn second
 * queue.add(hud, 100);        // Drawn last (on top)
 *
 * // Render all in sorted order
 * queue.render(canvas);
 * queue.clear();  // Clear for next frame
 * @endcode
 */
class RenderQueue
{
public:
    RenderQueue() = default;

    /**
     * @brief Add a drawable with a z-index
     * @param drawable The drawable to add
     * @param zIndex Z-index (lower = drawn first/behind)
     */
    void add(const Drawable &drawable, int zIndex = 0)
    {
        m_items.emplace_back(&drawable, zIndex);
    }

    /**
     * @brief Add a drawable with a z-index (pointer version)
     */
    void add(const Drawable *drawable, int zIndex = 0)
    {
        if (drawable)
            m_items.emplace_back(drawable, zIndex);
    }

    /**
     * @brief Sort and render all items to the target
     * @param target The render target to draw to
     */
    void render(RenderTarget &target)
    {
        // Sort by z-index (stable sort preserves insertion order for same z)
        std::stable_sort(m_items.begin(), m_items.end());

        // Draw all items in order
        for (const auto &item : m_items)
        {
            if (item.drawable)
                target.draw(*item.drawable);
        }
    }

    /**
     * @brief Clear all queued items
     */
    void clear()
    {
        m_items.clear();
    }

    /**
     * @brief Get the number of queued items
     */
    size_t size() const { return m_items.size(); }

    /**
     * @brief Check if queue is empty
     */
    bool empty() const { return m_items.empty(); }

private:
    std::vector<LayeredDrawable> m_items;
};

/**
 * @brief Named layers for organizing drawables
 */
enum class Layer : int
{
    Background = 0,
    BackgroundDecor = 10,
    GroundEffects = 20,
    Entities = 50,
    Player = 60,
    ForegroundDecor = 70,
    Projectiles = 80,
    Effects = 90,
    UI = 100,
    Overlay = 110,
    Debug = 200
};

/**
 * @brief Convert Layer enum to int for z-index
 */
inline int toZIndex(Layer layer)
{
    return static_cast<int>(layer);
}

/**
 * @brief Extended canvas with built-in z-ordering support
 *
 * LayeredCanvas extends Canvas with automatic z-ordering.
 * Use drawLayered() to add items with z-indices, then display() renders
 * them in sorted order.
 *
 * @code
 * LayeredCanvas canvas(160, 96, RenderMode::Braille);
 *
 * // Draw with z-ordering
 * canvas.drawLayered(background, Layer::Background);
 * canvas.drawLayered(player, Layer::Player);
 * canvas.drawLayered(hud, Layer::UI);
 *
 * // This clears, draws all in sorted order, then displays
 * canvas.displayLayered(Color::Black);
 * @endcode
 */
class LayeredCanvas
{
public:
    LayeredCanvas(unsigned int width, unsigned int height,
                  RenderMode mode = RenderMode::Braille)
        : m_canvas(width, height, mode) {}

    static LayeredCanvas fromTerminalSize(unsigned int termWidth, unsigned int termHeight,
                                          RenderMode mode = RenderMode::Braille)
    {
        unsigned int pixelWidth, pixelHeight;
        switch (mode)
        {
        case RenderMode::Braille:
            pixelWidth = termWidth * 2;
            pixelHeight = termHeight * 4;
            break;
        case RenderMode::Block:
            pixelWidth = termWidth;
            pixelHeight = termHeight * 2;
            break;
        case RenderMode::Quarter:
            pixelWidth = termWidth * 2;
            pixelHeight = termHeight * 2;
            break;
        case RenderMode::ASCII:
        default:
            pixelWidth = termWidth;
            pixelHeight = termHeight;
            break;
        }
        return LayeredCanvas(pixelWidth, pixelHeight, mode);
    }

    /**
     * @brief Queue a drawable for layered rendering
     */
    void drawLayered(const Drawable &drawable, int zIndex)
    {
        m_queue.add(drawable, zIndex);
    }

    /**
     * @brief Queue a drawable using Layer enum
     */
    void drawLayered(const Drawable &drawable, Layer layer)
    {
        m_queue.add(drawable, toZIndex(layer));
    }

    /**
     * @brief Draw immediately without z-ordering (for performance)
     */
    void draw(const Drawable &drawable)
    {
        m_canvas.draw(drawable);
    }

    /**
     * @brief Clear the canvas
     */
    void clear(const Color &color = Color::Black)
    {
        m_canvas.clear(color);
    }

    /**
     * @brief Render all queued items in z-order, then display
     */
    void displayLayered()
    {
        m_queue.render(m_canvas);
        m_queue.clear();
        m_canvas.display();
    }

    /**
     * @brief Clear, render queued items, and display - all in one call
     */
    void displayLayered(const Color &clearColor)
    {
        m_canvas.clear(clearColor);
        m_queue.render(m_canvas);
        m_queue.clear();
        m_canvas.display();
    }

    /**
     * @brief Display without z-ordering (direct to terminal)
     */
    void display()
    {
        m_canvas.display();
    }

    /**
     * @brief Get the underlying canvas for direct access
     */
    Canvas &getCanvas() { return m_canvas; }
    const Canvas &getCanvas() const { return m_canvas; }

    // Forward common canvas methods
    Vector2u getSize() const { return m_canvas.getSize(); }
    Vector2u getTerminalSize() const { return m_canvas.getTerminalSize(); }
    void setPixel(unsigned int x, unsigned int y, const Color &color)
    {
        m_canvas.setPixel(x, y, color);
    }

    void drawLine(int x0, int y0, int x1, int y1, const Color &color)
    {
        m_canvas.drawLine(x0, y0, x1, y1, color);
    }

    void fillRect(int x, int y, int w, int h, const Color &color)
    {
        m_canvas.fillRect(x, y, w, h, color);
    }

    void fillCircle(int cx, int cy, int r, const Color &color)
    {
        m_canvas.fillCircle(cx, cy, r, color);
    }

private:
    Canvas m_canvas;
    RenderQueue m_queue;
};

TG_NAMESPACE_END
