/**
 * @file Sprite.hpp
 * @brief Drawable sprite with texture support
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "../Graphics/Drawable.hpp"
#include "../Graphics/Transformable.hpp"
#include "../Graphics/RenderTarget.hpp"
#include "Texture.hpp"

TG_NAMESPACE_BEGIN

/**
 * @brief Drawable sprite for rendering textured images
 *
 * A sprite is a textured rectangle that can be transformed and drawn.
 * Like sf::Sprite, it references a texture and can show a sub-rectangle of it.
 *
 * @code
 * Texture texture;
 * texture.loadFromFile("player.ppm");
 *
 * Sprite player(texture);
 * player.setPosition(50, 50);
 * player.setScale(2, 2);  // Double size
 *
 * canvas.draw(player);
 * @endcode
 */
class Sprite : public Drawable, public Transformable
{
public:
    /**
     * @brief Default constructor
     */
    Sprite() : m_texture(nullptr), m_color(Color::White) {}

    /**
     * @brief Create a sprite from a texture
     */
    explicit Sprite(const Texture &texture)
        : m_texture(&texture), m_color(Color::White)
    {
        setTexture(texture);
    }

    /**
     * @brief Create a sprite from a texture with a specific region
     */
    Sprite(const Texture &texture, const IntRect &rectangle)
        : m_texture(&texture), m_textureRect(rectangle), m_color(Color::White) {}

    /**
     * @brief Set the texture
     * @param texture Source texture
     * @param resetRect Reset texture rectangle to full texture size
     */
    void setTexture(const Texture &texture, bool resetRect = false)
    {
        m_texture = &texture;

        if (resetRect || (m_textureRect.width == 0 && m_textureRect.height == 0))
        {
            Vector2u size = texture.getSize();
            m_textureRect = IntRect(0, 0, size.x, size.y);
        }
    }

    /**
     * @brief Set the texture sub-rectangle
     */
    void setTextureRect(const IntRect &rectangle)
    {
        m_textureRect = rectangle;
    }

    /**
     * @brief Set a color tint
     */
    void setColor(const Color &color)
    {
        m_color = color;
    }

    /**
     * @brief Get the texture
     */
    const Texture *getTexture() const
    {
        return m_texture;
    }

    /**
     * @brief Get the texture rectangle
     */
    const IntRect &getTextureRect() const
    {
        return m_textureRect;
    }

    /**
     * @brief Get the color tint
     */
    const Color &getColor() const
    {
        return m_color;
    }

    /**
     * @brief Get local bounding rectangle
     */
    FloatRect getLocalBounds() const
    {
        return FloatRect(0, 0,
                         static_cast<float>(m_textureRect.width),
                         static_cast<float>(m_textureRect.height));
    }

    /**
     * @brief Get global bounding rectangle
     */
    FloatRect getGlobalBounds() const
    {
        FloatRect local = getLocalBounds();

        Vector2f corners[4] = {
            transformPoint(Vector2f(0, 0)),
            transformPoint(Vector2f(local.width, 0)),
            transformPoint(Vector2f(0, local.height)),
            transformPoint(Vector2f(local.width, local.height))};

        float minX = corners[0].x, maxX = corners[0].x;
        float minY = corners[0].y, maxY = corners[0].y;

        for (int i = 1; i < 4; ++i)
        {
            minX = std::min(minX, corners[i].x);
            maxX = std::max(maxX, corners[i].x);
            minY = std::min(minY, corners[i].y);
            maxY = std::max(maxY, corners[i].y);
        }

        return FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

protected:
    void draw(RenderTarget &target) const override
    {
        if (!m_texture)
            return;

        // Draw each pixel of the texture
        for (int y = 0; y < m_textureRect.height; ++y)
        {
            for (int x = 0; x < m_textureRect.width; ++x)
            {
                // Get texture pixel
                Color texColor = m_texture->getPixel(
                    m_textureRect.left + x,
                    m_textureRect.top + y);

                // Skip transparent pixels
                if (texColor.a < 128)
                    continue;

                // Apply color tint
                Color finalColor = Color(
                    static_cast<uint8_t>((texColor.r * m_color.r) / 255),
                    static_cast<uint8_t>((texColor.g * m_color.g) / 255),
                    static_cast<uint8_t>((texColor.b * m_color.b) / 255),
                    static_cast<uint8_t>((texColor.a * m_color.a) / 255));

                // Transform position
                Vector2f worldPos = transformPoint(Vector2f(
                    static_cast<float>(x), static_cast<float>(y)));

                target.setPixel(
                    static_cast<unsigned int>(worldPos.x),
                    static_cast<unsigned int>(worldPos.y),
                    finalColor);
            }
        }
    }

private:
    const Texture *m_texture;
    IntRect m_textureRect;
    Color m_color;
};

TG_NAMESPACE_END
