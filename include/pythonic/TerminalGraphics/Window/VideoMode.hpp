/**
 * @file VideoMode.hpp
 * @brief Terminal video mode (dimensions and color depth)
 */

#pragma once

#include "../Config.hpp"
#include <vector>

TG_NAMESPACE_BEGIN

/**
 * @brief Represents terminal video mode (dimensions)
 *
 * Similar to sf::VideoMode but for terminal character cells.
 * Provides methods to query the current terminal size.
 *
 * @code
 * // Get current terminal size
 * VideoMode mode = VideoMode::getDesktopMode();
 * std::cout << "Terminal is " << mode.width << "x" << mode.height << "\n";
 *
 * // Create a specific mode
 * VideoMode mode(80, 24);  // 80 columns, 24 rows
 * @endcode
 */
class VideoMode
{
public:
    /**
     * @brief Width in character columns
     */
    unsigned int width = 80;

    /**
     * @brief Height in character rows
     */
    unsigned int height = 24;

    /**
     * @brief Color depth (bits per pixel for true color)
     *
     * Common values:
     * - 1: Monochrome
     * - 4: 16 colors
     * - 8: 256 colors
     * - 24: True color (16.7M colors)
     */
    unsigned int bitsPerPixel = 24;

    /**
     * @brief Default constructor (80x24, true color)
     */
    VideoMode() = default;

    /**
     * @brief Create a video mode with specific dimensions
     * @param w Width in columns
     * @param h Height in rows
     * @param bpp Bits per pixel (default 24 for true color)
     */
    VideoMode(unsigned int w, unsigned int h, unsigned int bpp = 24)
        : width(w), height(h), bitsPerPixel(bpp) {}

    /**
     * @brief Get the current terminal size
     * @return VideoMode representing current terminal dimensions
     */
    static VideoMode getDesktopMode()
    {
        VideoMode mode;

#ifdef TG_PLATFORM_WINDOWS
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
        {
            mode.width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            mode.height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
#else
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
        {
            mode.width = ws.ws_col;
            mode.height = ws.ws_row;
        }
#endif

        mode.bitsPerPixel = 24; // Assume true color support
        return mode;
    }

    /**
     * @brief Get all available video modes (for terminals, returns common sizes)
     * @return Vector of standard terminal sizes
     */
    static std::vector<VideoMode> getFullscreenModes()
    {
        // Terminals don't have "fullscreen modes" like graphics,
        // but we can return common terminal sizes
        return {
            VideoMode(80, 24),  // Standard VT100
            VideoMode(80, 25),  // DOS standard
            VideoMode(80, 43),  // EGA text mode
            VideoMode(80, 50),  // VGA text mode
            VideoMode(120, 40), // Wide terminal
            VideoMode(132, 43), // Wide + tall
            VideoMode(160, 50), // Ultra-wide
            getDesktopMode()    // Current size
        };
    }

    /**
     * @brief Check if this mode is valid
     * @return true if width and height are non-zero
     */
    bool isValid() const
    {
        return width > 0 && height > 0;
    }

    /**
     * @brief Compare two video modes
     */
    bool operator==(const VideoMode &other) const
    {
        return width == other.width &&
               height == other.height &&
               bitsPerPixel == other.bitsPerPixel;
    }

    bool operator!=(const VideoMode &other) const
    {
        return !(*this == other);
    }

    /**
     * @brief Order video modes by size (area then bpp)
     */
    bool operator<(const VideoMode &other) const
    {
        unsigned int area1 = width * height;
        unsigned int area2 = other.width * other.height;
        if (area1 != area2)
            return area1 < area2;
        return bitsPerPixel < other.bitsPerPixel;
    }
};

TG_NAMESPACE_END
