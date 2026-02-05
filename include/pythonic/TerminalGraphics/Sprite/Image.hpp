/**
 * @file Image.hpp
 * @brief In-memory image for pixel manipulation
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <functional>

TG_NAMESPACE_BEGIN

/**
 * @brief In-memory image for pixel manipulation
 *
 * An image that can be loaded, modified, and used to create textures.
 * Supports loading from raw pixel data and simple image formats.
 *
 * @code
 * Image img;
 * img.create(100, 100, Color::Blue);  // Create solid blue image
 * img.setPixel(50, 50, Color::Red);   // Set center pixel
 *
 * // Or load from file (PPM format supported)
 * img.loadFromFile("sprite.ppm");
 * @endcode
 */
class Image
{
public:
    /**
     * @brief Default constructor (creates empty image)
     */
    Image() : m_width(0), m_height(0) {}

    /**
     * @brief Create an image with the given size and fill color
     * @param width Image width
     * @param height Image height
     * @param color Fill color (default: black)
     */
    void create(unsigned int width, unsigned int height,
                const Color &color = Color::Black)
    {
        m_width = width;
        m_height = height;
        m_pixels.resize(static_cast<size_t>(width) * height, color);
    }

    /**
     * @brief Create an image from a pixel array
     * @param width Image width
     * @param height Image height
     * @param pixels Pointer to RGBA pixel data (4 bytes per pixel)
     */
    void create(unsigned int width, unsigned int height, const uint8_t *pixels)
    {
        m_width = width;
        m_height = height;
        m_pixels.resize(static_cast<size_t>(width) * height);

        for (size_t i = 0; i < m_pixels.size(); ++i)
        {
            m_pixels[i] = Color(
                pixels[i * 4 + 0],
                pixels[i * 4 + 1],
                pixels[i * 4 + 2],
                pixels[i * 4 + 3]);
        }
    }

    /**
     * @brief Load image from a file
     * @param filename Path to image file (PPM, PNG, JPG supported)
     * @return true if successful
     *
     * Supports PPM/PAM natively, and PNG/JPG via ImageMagick if installed.
     * PNG files with transparency are properly loaded with alpha channel.
     */
    bool loadFromFile(const std::string &filename)
    {
        // Get file extension
        std::string ext = "";
        size_t dotPos = filename.rfind('.');
        if (dotPos != std::string::npos)
        {
            ext = filename.substr(dotPos);
            for (char &c : ext)
                c = std::tolower(c);
        }

        // Try loading PPM/PAM directly
        if (ext == ".ppm" || ext == ".pgm")
        {
            return loadPPM(filename);
        }
        if (ext == ".pam")
        {
            return loadPAM(filename);
        }

        // For PNG, use PAM conversion to preserve alpha channel
        if (ext == ".png")
        {
            std::string tempPam = convertToPAM(filename);
            if (!tempPam.empty())
            {
                bool result = loadPAM(tempPam);
                std::remove(tempPam.c_str());
                return result;
            }
        }

        // For other formats (JPG, etc.), use PPM conversion
        std::string tempPpm = convertToPPM(filename);
        if (!tempPpm.empty())
        {
            bool result = loadPPM(tempPpm);
            std::remove(tempPpm.c_str());
            return result;
        }

        // Fall back to trying PPM anyway
        return loadPPM(filename);
    }

private:
    /**
     * @brief Convert any image to PAM (with alpha) using ImageMagick
     * @return Path to temp file, or empty string on failure
     *
     * PAM format supports RGBA, unlike PPM which is RGB only
     */
    static std::string convertToPAM(const std::string &inputFile)
    {
        std::string tempPam = "/tmp/tg_img_" +
                              std::to_string(std::hash<std::string>{}(inputFile)) + ".pam";

        // Convert to PAM format which supports RGBA
        // -background none: preserve transparency
        // -type TrueColorAlpha: force RGBA output
        std::string cmd = "convert \"" + inputFile + "\" -background none -type TrueColorAlpha -depth 8 \"" +
                          tempPam + "\" 2>/dev/null";

        int result = std::system(cmd.c_str());
        if (result != 0)
            return "";

        return tempPam;
    }

    /**
     * @brief Convert any image to PPM using ImageMagick (fallback for non-alpha)
     */
    static std::string convertToPPM(const std::string &inputFile)
    {
        std::string tempPpm = "/tmp/tg_img_" +
                              std::to_string(std::hash<std::string>{}(inputFile)) + ".ppm";

        std::string cmd = "convert \"" + inputFile + "\" -depth 8 \"" +
                          tempPpm + "\" 2>/dev/null";

        int result = std::system(cmd.c_str());
        if (result != 0)
            return "";

        return tempPpm;
    }

    /**
     * @brief Load a PAM file (with alpha support)
     */
    bool loadPAM(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
            return false;

        std::string magic;
        file >> magic;

        if (magic != "P7")
            return false;

        unsigned int width = 0, height = 0, depth = 0, maxval = 255;
        std::string tupltype;

        // Parse PAM header
        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            if (line.find("WIDTH") == 0)
                width = std::stoul(line.substr(6));
            else if (line.find("HEIGHT") == 0)
                height = std::stoul(line.substr(7));
            else if (line.find("DEPTH") == 0)
                depth = std::stoul(line.substr(6));
            else if (line.find("MAXVAL") == 0)
                maxval = std::stoul(line.substr(7));
            else if (line.find("TUPLTYPE") == 0)
                tupltype = line.substr(9);
            else if (line.find("ENDHDR") == 0)
                break;
        }

        if (width == 0 || height == 0 || depth == 0)
            return false;

        m_width = width;
        m_height = height;
        m_pixels.resize(static_cast<size_t>(width) * height);

        // Read pixel data
        for (size_t i = 0; i < m_pixels.size(); ++i)
        {
            if (depth == 4)
            {
                // RGBA
                uint8_t rgba[4];
                file.read(reinterpret_cast<char *>(rgba), 4);
                m_pixels[i] = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
            }
            else if (depth == 3)
            {
                // RGB
                uint8_t rgb[3];
                file.read(reinterpret_cast<char *>(rgb), 3);
                m_pixels[i] = Color(rgb[0], rgb[1], rgb[2]);
            }
            else if (depth == 2)
            {
                // Grayscale + Alpha
                uint8_t ga[2];
                file.read(reinterpret_cast<char *>(ga), 2);
                m_pixels[i] = Color(ga[0], ga[0], ga[0], ga[1]);
            }
            else if (depth == 1)
            {
                // Grayscale
                uint8_t g;
                file.read(reinterpret_cast<char *>(&g), 1);
                m_pixels[i] = Color(g, g, g);
            }
        }

        return true;
    }

    /**
     * @brief Load a PPM file
     */
    bool loadPPM(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
            return false;

        std::string magic;
        file >> magic;

        if (magic == "P6" || magic == "P3")
        {
            // Skip comments
            char c;
            file.get(c);
            while (file.peek() == '#')
            {
                std::string comment;
                std::getline(file, comment);
            }

            unsigned int width, height, maxVal;
            file >> width >> height >> maxVal;
            file.get(); // Skip whitespace

            m_width = width;
            m_height = height;
            m_pixels.resize(static_cast<size_t>(width) * height);

            if (magic == "P6")
            {
                // Binary PPM
                for (size_t i = 0; i < m_pixels.size(); ++i)
                {
                    uint8_t rgb[3];
                    file.read(reinterpret_cast<char *>(rgb), 3);
                    m_pixels[i] = Color(rgb[0], rgb[1], rgb[2]);
                }
            }
            else
            {
                // ASCII PPM
                for (size_t i = 0; i < m_pixels.size(); ++i)
                {
                    int r, g, b;
                    file >> r >> g >> b;
                    m_pixels[i] = Color(
                        static_cast<uint8_t>(r),
                        static_cast<uint8_t>(g),
                        static_cast<uint8_t>(b));
                }
            }

            return true;
        }

        return false;
    }

public:
    /**
     * @brief Save image to a PPM file
     * @param filename Output file path
     * @return true if successful
     */
    bool saveToFile(const std::string &filename) const
    {
        std::ofstream file(filename, std::ios::binary);
        if (!file)
            return false;

        // Write PPM header
        file << "P6\n"
             << m_width << " " << m_height << "\n255\n";

        // Write pixel data
        for (const Color &c : m_pixels)
        {
            char rgb[3] = {
                static_cast<char>(c.r),
                static_cast<char>(c.g),
                static_cast<char>(c.b)};
            file.write(rgb, 3);
        }

        return true;
    }

    /**
     * @brief Get image size
     */
    Vector2u getSize() const
    {
        return Vector2u(m_width, m_height);
    }

    /**
     * @brief Set a pixel color
     */
    void setPixel(unsigned int x, unsigned int y, const Color &color)
    {
        if (x < m_width && y < m_height)
            m_pixels[y * m_width + x] = color;
    }

    /**
     * @brief Get a pixel color
     */
    Color getPixel(unsigned int x, unsigned int y) const
    {
        if (x < m_width && y < m_height)
            return m_pixels[y * m_width + x];
        return Color::Black;
    }

    /**
     * @brief Get pointer to pixel data
     */
    const Color *getPixelsPtr() const
    {
        return m_pixels.empty() ? nullptr : m_pixels.data();
    }

    /**
     * @brief Copy a rectangular region from another image
     */
    void copy(const Image &source, unsigned int destX, unsigned int destY,
              const IntRect &sourceRect = IntRect(0, 0, 0, 0))
    {
        IntRect srcRect = sourceRect;
        if (srcRect.width == 0 || srcRect.height == 0)
        {
            srcRect.left = 0;
            srcRect.top = 0;
            srcRect.width = source.m_width;
            srcRect.height = source.m_height;
        }

        for (int y = 0; y < srcRect.height; ++y)
        {
            for (int x = 0; x < srcRect.width; ++x)
            {
                unsigned int sx = srcRect.left + x;
                unsigned int sy = srcRect.top + y;
                unsigned int dx = destX + x;
                unsigned int dy = destY + y;

                if (sx < source.m_width && sy < source.m_height &&
                    dx < m_width && dy < m_height)
                {
                    m_pixels[dy * m_width + dx] = source.m_pixels[sy * source.m_width + sx];
                }
            }
        }
    }

    /**
     * @brief Flip the image horizontally
     */
    void flipHorizontally()
    {
        for (unsigned int y = 0; y < m_height; ++y)
        {
            for (unsigned int x = 0; x < m_width / 2; ++x)
            {
                std::swap(m_pixels[y * m_width + x],
                          m_pixels[y * m_width + (m_width - 1 - x)]);
            }
        }
    }

    /**
     * @brief Flip the image vertically
     */
    void flipVertically()
    {
        for (unsigned int y = 0; y < m_height / 2; ++y)
        {
            for (unsigned int x = 0; x < m_width; ++x)
            {
                std::swap(m_pixels[y * m_width + x],
                          m_pixels[(m_height - 1 - y) * m_width + x]);
            }
        }
    }

private:
    unsigned int m_width;
    unsigned int m_height;
    std::vector<Color> m_pixels;
};

TG_NAMESPACE_END
