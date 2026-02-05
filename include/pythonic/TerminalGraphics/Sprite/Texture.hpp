/**
 * @file Texture.hpp
 * @brief Texture for sprite rendering
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "Image.hpp"

TG_NAMESPACE_BEGIN

/**
 * @brief Texture for use with sprites
 *
 * A texture stores image data that can be applied to sprites.
 * In terminal graphics, textures are essentially the same as images,
 * but this class provides the familiar SFML API.
 *
 * @code
 * Texture texture;
 * texture.loadFromFile("character.ppm");
 *
 * Sprite sprite(texture);
 * sprite.setPosition(10, 10);
 * @endcode
 */
class Texture
{
public:
    /**
     * @brief Default constructor (creates empty texture)
     */
    Texture() : m_smooth(false), m_repeated(false) {}

    /**
     * @brief Create an empty texture with the given size
     */
    bool create(unsigned int width, unsigned int height)
    {
        m_image.create(width, height);
        return true;
    }

    /**
     * @brief Load texture from a file
     * @param filename Path to image file (PPM format)
     * @return true if successful
     */
    bool loadFromFile(const std::string &filename)
    {
        return m_image.loadFromFile(filename);
    }

    /**
     * @brief Load texture from an image
     */
    bool loadFromImage(const Image &image)
    {
        m_image = image;
        return true;
    }

    /**
     * @brief Load texture from an image with a specific area
     */
    bool loadFromImage(const Image &image, const IntRect &area)
    {
        m_image.create(area.width, area.height);
        m_image.copy(image, 0, 0, area);
        return true;
    }

    /**
     * @brief Get the texture size
     */
    Vector2u getSize() const
    {
        return m_image.getSize();
    }

    /**
     * @brief Copy the texture to an image
     */
    Image copyToImage() const
    {
        return m_image;
    }

    /**
     * @brief Update a portion of the texture from an image
     */
    void update(const Image &image, unsigned int x = 0, unsigned int y = 0)
    {
        m_image.copy(image, x, y);
    }

    /**
     * @brief Update a portion of the texture from pixel data
     */
    void update(const uint8_t *pixels, unsigned int width, unsigned int height,
                unsigned int x, unsigned int y)
    {
        for (unsigned int py = 0; py < height; ++py)
        {
            for (unsigned int px = 0; px < width; ++px)
            {
                size_t i = (py * width + px) * 4;
                Color c(pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]);
                m_image.setPixel(x + px, y + py, c);
            }
        }
    }

    /**
     * @brief Enable/disable smooth filtering (no effect in terminal)
     */
    void setSmooth(bool smooth)
    {
        m_smooth = smooth;
    }

    /**
     * @brief Check if smooth filtering is enabled
     */
    bool isSmooth() const
    {
        return m_smooth;
    }

    /**
     * @brief Enable/disable texture repeating
     */
    void setRepeated(bool repeated)
    {
        m_repeated = repeated;
    }

    /**
     * @brief Check if texture repeating is enabled
     */
    bool isRepeated() const
    {
        return m_repeated;
    }

    /**
     * @brief Get the underlying image
     */
    const Image &getImage() const
    {
        return m_image;
    }

    /**
     * @brief Get a pixel from the texture
     */
    Color getPixel(unsigned int x, unsigned int y) const
    {
        Vector2u size = m_image.getSize();

        if (m_repeated)
        {
            x = x % size.x;
            y = y % size.y;
        }

        return m_image.getPixel(x, y);
    }

    /**
     * @brief Get the bounding box of opaque pixels (alpha > threshold)
     * @param alphaThreshold Minimum alpha value to consider opaque (default 128)
     * @return IntRect containing the opaque region, or empty rect if all transparent
     *
     * This is useful for accurate collision detection with transparent sprites.
     */
    IntRect getOpaqueBounds(uint8_t alphaThreshold = 128) const
    {
        Vector2u size = m_image.getSize();
        if (size.x == 0 || size.y == 0)
            return IntRect(0, 0, 0, 0);

        int minX = static_cast<int>(size.x);
        int minY = static_cast<int>(size.y);
        int maxX = -1;
        int maxY = -1;

        for (unsigned int y = 0; y < size.y; ++y)
        {
            for (unsigned int x = 0; x < size.x; ++x)
            {
                Color c = m_image.getPixel(x, y);
                if (c.a >= alphaThreshold)
                {
                    if (static_cast<int>(x) < minX)
                        minX = static_cast<int>(x);
                    if (static_cast<int>(y) < minY)
                        minY = static_cast<int>(y);
                    if (static_cast<int>(x) > maxX)
                        maxX = static_cast<int>(x);
                    if (static_cast<int>(y) > maxY)
                        maxY = static_cast<int>(y);
                }
            }
        }

        // No opaque pixels found
        if (maxX < 0 || maxY < 0)
            return IntRect(0, 0, 0, 0);

        return IntRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

private:
    Image m_image;
    bool m_smooth;
    bool m_repeated;
};

TG_NAMESPACE_END
