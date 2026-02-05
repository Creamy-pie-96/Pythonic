/**
 * @file Canvas.hpp
 * @brief High-resolution rendering canvas using braille/block characters
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "RenderTarget.hpp"
#include "View.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>
#include <csignal>
#include <mutex>

#ifndef _WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#endif

TG_NAMESPACE_BEGIN

// Forward declaration of signal handler helper
namespace detail
{
    // Global cleanup registry for signal-safe terminal restoration
    // Components can register cleanup callbacks that will be called on Ctrl+C
    inline std::vector<void (*)()> &getCleanupCallbacks()
    {
        static std::vector<void (*)()> callbacks;
        return callbacks;
    }

    inline void registerCleanupCallback(void (*callback)())
    {
        getCleanupCallbacks().push_back(callback);
    }

    inline void restoreTerminal()
    {
        // Call all registered cleanup callbacks
        for (auto &callback : getCleanupCallbacks())
        {
            if (callback)
                callback();
        }

        // Disable mouse tracking (in case terminal mouse mode was enabled)
        std::fputs("\033[?1000l\033[?1002l\033[?1003l\033[?1006l", stdout);
        std::fputs("\033[?25h", stdout);   // Show cursor
        std::fputs("\033[0m", stdout);     // Reset colors
        std::fputs("\033[?1049l", stdout); // Leave alternate screen
        std::fflush(stdout);
    }

    inline void signalHandler(int sig)
    {
        restoreTerminal();
        // Re-raise signal with default handler
        std::signal(sig, SIG_DFL);
        std::raise(sig);
    }

    inline unsigned int terminalWidth = 80;
    inline unsigned int terminalHeight = 24;
    inline bool terminalSizeChanged = false;

#ifndef _WIN32
    inline void resizeHandler(int)
    {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
        {
            terminalWidth = ws.ws_col;
            terminalHeight = ws.ws_row;
            terminalSizeChanged = true;
        }
    }
#endif

    inline void installSignalHandlers()
    {
        static bool installed = false;
        if (installed)
            return;
        installed = true;

        // Install cleanup handlers for various signals
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
#ifndef _WIN32
        std::signal(SIGHUP, signalHandler);
        std::signal(SIGQUIT, signalHandler);
        std::signal(SIGWINCH, resizeHandler);

        // Get initial terminal size
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
        {
            terminalWidth = ws.ws_col;
            terminalHeight = ws.ws_row;
        }
#endif
    }
}

/**
 * @brief Rendering mode for the canvas
 */
enum class RenderMode
{
    Block,   ///< Block characters (▀▄█ ) - 1x2 resolution
    Braille, ///< Braille characters (⠀-⣿) - 2x4 resolution
    ASCII,   ///< ASCII characters (.:-=+*#@) for compatibility
    Quarter  ///< Quarter block characters (▖▗▘▝▌▐▀▄█) - 2x2 resolution
};

/**
 * @brief High-resolution rendering canvas for terminal graphics
 *
 * The Canvas provides a pixel-level drawing surface that is rendered
 * to the terminal using Unicode characters. Different render modes
 * offer various resolution/compatibility tradeoffs:
 *
 * - Block: Each terminal cell = 1x2 pixels (half blocks)
 * - Braille: Each terminal cell = 2x4 pixels (braille patterns)
 * - Quarter: Each terminal cell = 2x2 pixels (quarter blocks)
 * - ASCII: Each terminal cell = 1x1 pixels (for compatibility)
 *
 * @code
 * Canvas canvas(160, 48, RenderMode::Braille);  // 80x24 terminal = 160x96 pixels
 *
 * // Draw shapes
 * canvas.setPixel(10, 10, Color::Red);
 * canvas.fillCircle(40, 24, 10, Color::Blue);
 *
 * // Render to terminal
 * canvas.display();
 * @endcode
 */
class Canvas : public RenderTarget
{
public:
    /**
     * @brief Create a canvas with the given pixel dimensions
     * @param width Width in pixels
     * @param height Height in pixels
     * @param mode Render mode
     */
    Canvas(unsigned int width, unsigned int height,
           RenderMode mode = RenderMode::Braille)
        : m_width(width), m_height(height), m_mode(mode)
    {
        initPixelBuffer(width, height);
        calculateTerminalSize();
    }

    /**
     * @brief Create a canvas from terminal dimensions
     * @param termWidth Terminal width in characters
     * @param termHeight Terminal height in characters
     * @param mode Render mode
     * @return Canvas with appropriate pixel resolution
     */
    static Canvas fromTerminalSize(unsigned int termWidth, unsigned int termHeight,
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

        return Canvas(pixelWidth, pixelHeight, mode);
    }

    /**
     * @brief Create a canvas that fills the entire terminal
     * @param mode Render mode (default: Braille for highest resolution)
     * @return Canvas sized to fill the current terminal
     *
     * Uses the current terminal size (auto-detected). Call this again
     * if the terminal is resized and you want to resize the canvas.
     */
    static Canvas createFullscreen(RenderMode mode = RenderMode::Braille)
    {
        // Ensure signal handlers are installed to track terminal size
        detail::installSignalHandlers();

        // Leave 1 row for status/input to prevent scroll
        unsigned int termH = detail::terminalHeight > 1 ? detail::terminalHeight - 1 : detail::terminalHeight;

        return fromTerminalSize(detail::terminalWidth, termH, mode);
    }

    Vector2u getSize() const override
    {
        return Vector2u(m_width, m_height);
    }

    /**
     * @brief Get the terminal size needed to display this canvas
     */
    Vector2u getTerminalSize() const
    {
        return Vector2u(m_termWidth, m_termHeight);
    }

    /**
     * @brief Get the current render mode
     */
    RenderMode getRenderMode() const { return m_mode; }

    /**
     * @brief Set the render mode
     */
    void setRenderMode(RenderMode mode)
    {
        m_mode = mode;
        calculateTerminalSize();
    }

    /**
     * @brief Render the canvas to a string
     * @return Terminal-ready string with ANSI colors
     */
    std::string render() const
    {
        switch (m_mode)
        {
        case RenderMode::Braille:
            return renderBraille();
        case RenderMode::Block:
            return renderBlock();
        case RenderMode::Quarter:
            return renderQuarter();
        case RenderMode::ASCII:
        default:
            return renderASCII();
        }
    }

    /**
     * @brief Display the canvas to stdout with minimal flicker
     *
     * Uses cursor positioning to update in-place.
     * Automatically sets up alternate screen buffer on first call.
     * Installs signal handlers for proper cleanup on exit.
     */
    void display()
    {
        static std::once_flag initFlag;
        std::call_once(initFlag, []()
                       {
            // Install signal handlers for proper cleanup
            detail::installSignalHandlers();

            // Enter alternate screen buffer, hide cursor
            std::fputs("\033[?1049h", stdout); // Alternate screen buffer
            std::fputs("\033[?25l", stdout);   // Hide cursor
            std::fflush(stdout);

            // Register cleanup on normal exit
            std::atexit([]()
                        { detail::restoreTerminal(); }); });

        std::string output = render();

        // Use synchronized output to prevent tearing (DEC private mode 2026)
        // This tells the terminal to batch all output until we're done
        std::fputs("\033[?2026h", stdout); // Begin synchronized update
        std::fputs("\033[H", stdout);      // Move cursor home
        std::fwrite(output.data(), 1, output.size(), stdout);
        std::fputs("\033[?2026l", stdout); // End synchronized update
        std::fflush(stdout);
    }

    /**
     * @brief Check if terminal was resized
     * @return true if terminal size changed since last check
     */
    static bool wasResized()
    {
        if (detail::terminalSizeChanged)
        {
            detail::terminalSizeChanged = false;
            return true;
        }
        return false;
    }

    /**
     * @brief Get current terminal width
     */
    static unsigned int getTermWidth()
    {
        return detail::terminalWidth;
    }

    /**
     * @brief Get current terminal height
     */
    static unsigned int getTermHeight()
    {
        return detail::terminalHeight;
    }

    /**
     * @brief Initialize the display (alternate screen, hide cursor)
     */
    static void initDisplay()
    {
        detail::installSignalHandlers();
        std::fputs("\033[?1049h", stdout); // Alternate screen buffer
        std::fputs("\033[?25l", stdout);   // Hide cursor
        std::fputs("\033[2J", stdout);     // Clear screen
        std::fflush(stdout);
    }

    /**
     * @brief Cleanup display (restore terminal)
     */
    static void cleanupDisplay()
    {
        detail::restoreTerminal();
    }

    // ==================== View (Camera) Support ====================

    // ==================== View (Camera) Support ====================
    // See inline implementations at end of class

private:
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_termWidth;
    unsigned int m_termHeight;
    RenderMode m_mode;

    // View (camera) system
    mutable View m_view;
    mutable View m_defaultView;
    mutable bool m_viewInitialized = false;

    void initDefaultView() const
    {
        if (!m_viewInitialized)
        {
            m_defaultView = View(Vector2f(m_width / 2.0f, m_height / 2.0f),
                                 Vector2f(static_cast<float>(m_width), static_cast<float>(m_height)));
            m_view = m_defaultView;
            m_viewInitialized = true;
        }
    }

    void calculateTerminalSize()
    {
        switch (m_mode)
        {
        case RenderMode::Braille:
            m_termWidth = (m_width + 1) / 2;
            m_termHeight = (m_height + 3) / 4;
            break;
        case RenderMode::Block:
            m_termWidth = m_width;
            m_termHeight = (m_height + 1) / 2;
            break;
        case RenderMode::Quarter:
            m_termWidth = (m_width + 1) / 2;
            m_termHeight = (m_height + 1) / 2;
            break;
        case RenderMode::ASCII:
        default:
            m_termWidth = m_width;
            m_termHeight = m_height;
            break;
        }
    }

    /**
     * @brief Check if a pixel is "on" (foreground) vs "off" (background)
     *
     * Uses perceived luminance with a low threshold for crisp braille rendering.
     * Pixels with very low luminance are considered "off" (background).
     */
    bool isPixelOn(unsigned int x, unsigned int y) const
    {
        if (x >= m_width || y >= m_height)
            return false;

        const Color &c = m_pixels[y * m_width + x].color;

        // Skip transparent pixels
        if (c.a < 32)
            return false;

        // Calculate perceived luminance (human eye sensitivity)
        // Using fast integer approximation: Y = (R*2 + G*5 + B) / 8
        int luminance = (c.r * 2 + c.g * 5 + c.b) >> 3;

        // Low threshold (25) for crisp graphics - most dark colors become background
        return luminance > 25;
    }

    /**
     * @brief Get pixel color, returning black for out-of-bounds
     */
    Color getPixelSafe(unsigned int x, unsigned int y) const
    {
        if (x >= m_width || y >= m_height)
            return Color::Black;
        return m_pixels[y * m_width + x].color;
    }

    /**
     * @brief Render using braille characters (2x4 pixels per cell)
     *
     * Braille mode uses foreground color for "on" dots and background
     * for empty areas. Computes average colors from pixel groups.
     */
    std::string renderBraille() const
    {
        std::string result;
        result.reserve(m_termWidth * m_termHeight * 40);

        // Braille pattern bit positions:
        // 0 3
        // 1 4
        // 2 5
        // 6 7
        static const int brailleMap[4][2] = {
            {0, 3},
            {1, 4},
            {2, 5},
            {6, 7}};

        Color lastFg(255, 255, 255);
        Color lastBg(0, 0, 0);
        bool colorSet = false;

        for (unsigned int ty = 0; ty < m_termHeight; ++ty)
        {
            for (unsigned int tx = 0; tx < m_termWidth; ++tx)
            {
                unsigned int px = tx * 2;
                unsigned int py = ty * 4;

                // Calculate braille pattern and collect colors
                // Use BRIGHTEST color instead of averaging for sharper contrast
                unsigned char pattern = 0;
                Color fgColor = Color::Black;
                Color bgColor = Color::Black;
                int fgBrightness = 0;
                int bgBrightness = 0;

                for (int dy = 0; dy < 4; ++dy)
                {
                    for (int dx = 0; dx < 2; ++dx)
                    {
                        Color c = getPixelSafe(px + dx, py + dy);
                        int brightness = c.r + c.g + c.b;

                        if (isPixelOn(px + dx, py + dy))
                        {
                            pattern |= (1 << brailleMap[dy][dx]);
                            // Use the brightest "on" pixel as foreground
                            if (brightness > fgBrightness)
                            {
                                fgBrightness = brightness;
                                fgColor = c;
                            }
                        }
                        else
                        {
                            // Use the brightest "off" pixel as background
                            if (brightness > bgBrightness)
                            {
                                bgBrightness = brightness;
                                bgColor = c;
                            }
                        }
                    }
                }

                // Default colors if none found
                if (fgBrightness == 0)
                    fgColor = Color::White;

                // Output color codes if changed
                if (!colorSet || fgColor != lastFg || bgColor != lastBg)
                {
                    result += fgColor.toAnsiFg();
                    result += bgColor.toAnsiBg();
                    lastFg = fgColor;
                    lastBg = bgColor;
                    colorSet = true;
                }

                // Braille Unicode: U+2800 + pattern
                // UTF-8: E2 A0 80 + pattern
                char braille[4];
                braille[0] = static_cast<char>(0xE2);
                braille[1] = static_cast<char>(0xA0 | (pattern >> 6));
                braille[2] = static_cast<char>(0x80 | (pattern & 0x3F));
                braille[3] = '\0';
                result += braille;
            }

            result += "\033[0m\n";
            colorSet = false;
        }

        return result;
    }

    /**
     * @brief Render using half-block characters (1x2 pixels per cell)
     */
    std::string renderBlock() const
    {
        std::string result;
        result.reserve(m_termWidth * m_termHeight * 30);

        for (unsigned int ty = 0; ty < m_termHeight; ++ty)
        {
            for (unsigned int tx = 0; tx < m_termWidth; ++tx)
            {
                unsigned int py = ty * 2;

                Color top = getPixelSafe(tx, py);
                Color bottom = getPixelSafe(tx, py + 1);

                // Use upper half block (▀) with top as fg, bottom as bg
                result += top.toAnsiFg();
                result += bottom.toAnsiBg();
                result += "▀"; // U+2580
            }

            result += "\033[0m\n";
        }

        return result;
    }

    /**
     * @brief Render using quarter block characters (2x2 pixels per cell)
     */
    std::string renderQuarter() const
    {
        std::string result;
        result.reserve(m_termWidth * m_termHeight * 30);

        // Quarter block patterns based on which pixels are "on"
        // TL TR BL BR -> character
        static const char *quarterChars[16] = {
            " ", // 0000
            "▖", // 0001 - BL
            "▗", // 0010 - BR
            "▄", // 0011 - BL+BR
            "▘", // 0100 - TL
            "▌", // 0101 - TL+BL
            "▚", // 0110 - TL+BR
            "▙", // 0111 - TL+BL+BR
            "▝", // 1000 - TR
            "▞", // 1001 - TR+BL
            "▐", // 1010 - TR+BR
            "▟", // 1011 - TR+BL+BR
            "▀", // 1100 - TL+TR
            "▛", // 1101 - TL+TR+BL
            "▜", // 1110 - TL+TR+BR
            "█"  // 1111 - all
        };

        for (unsigned int ty = 0; ty < m_termHeight; ++ty)
        {
            for (unsigned int tx = 0; tx < m_termWidth; ++tx)
            {
                unsigned int px = tx * 2;
                unsigned int py = ty * 2;

                // Determine which quadrants are filled
                int pattern = 0;
                if (isPixelOn(px, py))
                    pattern |= 4; // TL
                if (isPixelOn(px + 1, py))
                    pattern |= 8; // TR
                if (isPixelOn(px, py + 1))
                    pattern |= 1; // BL
                if (isPixelOn(px + 1, py + 1))
                    pattern |= 2; // BR

                // Average color of filled pixels
                Color avgColor = Color::White;
                int count = 0;
                for (int dy = 0; dy < 2; ++dy)
                {
                    for (int dx = 0; dx < 2; ++dx)
                    {
                        if (isPixelOn(px + dx, py + dy))
                        {
                            Color c = getPixelSafe(px + dx, py + dy);
                            avgColor.r = (avgColor.r * count + c.r) / (count + 1);
                            avgColor.g = (avgColor.g * count + c.g) / (count + 1);
                            avgColor.b = (avgColor.b * count + c.b) / (count + 1);
                            count++;
                        }
                    }
                }

                result += avgColor.toAnsiFg();
                result += quarterChars[pattern];
            }

            result += "\033[0m\n";
        }

        return result;
    }

    /**
     * @brief Render using ASCII characters
     */
    std::string renderASCII() const
    {
        std::string result;
        result.reserve(m_termWidth * m_termHeight * 20);

        // ASCII grayscale ramp
        static const char *asciiRamp = " .:-=+*#%@";
        static const int rampLen = 10;

        for (unsigned int y = 0; y < m_termHeight; ++y)
        {
            for (unsigned int x = 0; x < m_termWidth; ++x)
            {
                Color c = getPixelSafe(x, y);

                // Calculate luminance
                int lum = (c.r * 299 + c.g * 587 + c.b * 114) / 1000;
                int idx = (lum * (rampLen - 1)) / 255;

                result += c.toAnsiFg();
                result += asciiRamp[idx];
            }

            result += "\033[0m\n";
        }

        return result;
    }

    // ==================== View Implementation ====================
public:
    inline void setView(const View &view)
    {
        initDefaultView();
        m_view = view;
    }

    inline const View &getView() const
    {
        initDefaultView();
        return m_view;
    }

    inline const View &getDefaultView() const
    {
        initDefaultView();
        return m_defaultView;
    }

    inline void resetView()
    {
        initDefaultView();
        m_view = m_defaultView;
    }

    inline Vector2f mapPixelToCoords(const Vector2i &pixel) const
    {
        return mapPixelToCoords(pixel, getView());
    }

    inline Vector2f mapPixelToCoords(const Vector2i &pixel, const View &view) const
    {
        // Get viewport in pixels
        FloatRect viewport = view.getViewport();
        float viewLeft = viewport.left * m_width;
        float viewTop = viewport.top * m_height;
        float viewWidth = viewport.width * m_width;
        float viewHeight = viewport.height * m_height;

        // Normalize to -1..1
        Vector2f normalized;
        normalized.x = -1.f + 2.f * (pixel.x - viewLeft) / viewWidth;
        normalized.y = 1.f - 2.f * (pixel.y - viewTop) / viewHeight;

        // Apply inverse view transform
        const Vector2f &viewCenter = view.getCenter();
        const Vector2f &viewSize = view.getSize();

        Vector2f world;
        world.x = viewCenter.x + normalized.x * viewSize.x / 2.f;
        world.y = viewCenter.y - normalized.y * viewSize.y / 2.f;

        // Handle rotation if any
        if (view.getRotation() != 0)
        {
            float angle = -view.getRotation() * 3.14159265359f / 180.0f;
            float cos = std::cos(angle);
            float sin = std::sin(angle);
            float dx = world.x - viewCenter.x;
            float dy = world.y - viewCenter.y;
            world.x = viewCenter.x + dx * cos - dy * sin;
            world.y = viewCenter.y + dx * sin + dy * cos;
        }

        return world;
    }

    inline Vector2i mapCoordsToPixel(const Vector2f &point) const
    {
        return mapCoordsToPixel(point, getView());
    }

    inline Vector2i mapCoordsToPixel(const Vector2f &point, const View &view) const
    {
        const Vector2f &viewCenter = view.getCenter();
        const Vector2f &viewSize = view.getSize();

        Vector2f transformed = point;

        // Handle rotation
        if (view.getRotation() != 0)
        {
            float angle = view.getRotation() * 3.14159265359f / 180.0f;
            float cos = std::cos(angle);
            float sin = std::sin(angle);
            float dx = point.x - viewCenter.x;
            float dy = point.y - viewCenter.y;
            transformed.x = viewCenter.x + dx * cos - dy * sin;
            transformed.y = viewCenter.y + dx * sin + dy * cos;
        }

        // Normalize to -1..1
        Vector2f normalized;
        normalized.x = (transformed.x - viewCenter.x) / (viewSize.x / 2.f);
        normalized.y = (viewCenter.y - transformed.y) / (viewSize.y / 2.f);

        // Get viewport in pixels
        FloatRect viewport = view.getViewport();
        float viewLeft = viewport.left * m_width;
        float viewTop = viewport.top * m_height;
        float viewWidth = viewport.width * m_width;
        float viewHeight = viewport.height * m_height;

        // Convert to screen coordinates
        Vector2i pixel;
        pixel.x = static_cast<int>((normalized.x + 1.f) / 2.f * viewWidth + viewLeft);
        pixel.y = static_cast<int>((1.f - normalized.y) / 2.f * viewHeight + viewTop);

        return pixel;
    }
};

TG_NAMESPACE_END
