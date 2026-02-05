/**
 * @file Text.hpp
 * @brief Text drawing utilities for terminal graphics
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Color.hpp"
#include "Font.hpp"
#include <string>

TG_NAMESPACE_BEGIN

/**
 * @brief Text drawing class for pixel-based rendering
 *
 * Draws text to any RenderTarget using the pixel font.
 *
 * @code
 * Canvas canvas(160, 96, RenderMode::Braille);
 *
 * // Draw centered text
 * Text::drawCentered(canvas, "GAME OVER", canvas.getSize().x / 2, 40, Color::Red);
 *
 * // Draw score
 * Text::draw(canvas, "Score: " + std::to_string(score), 10, 5, Color::White);
 * @endcode
 */
class Text
{
public:
    /**
     * @brief Draw text at a position on a render target
     * @tparam RenderTarget Any class with setPixel(x, y, Color)
     * @param target Render target to draw on
     * @param text Text to draw
     * @param x Left edge X coordinate
     * @param y Top edge Y coordinate
     * @param color Text color
     */
    template <typename RenderTarget>
    static void draw(RenderTarget &target, const std::string &text,
                     int x, int y, const Color &color)
    {
        const auto &glyphs = font::getDefaultFont();
        int cursorX = x;

        for (char c : text)
        {
            auto it = glyphs.find(c);
            if (it == glyphs.end())
            {
                // Unknown char, use space
                cursorX += 4;
                continue;
            }

            const font::Glyph &glyph = it->second;
            for (int row = 0; row < 5; ++row)
            {
                uint8_t bits = glyph.rows[row];
                for (int col = 0; col < 3; ++col)
                {
                    // Bit 2 = leftmost, bit 0 = rightmost
                    if (bits & (1 << (2 - col)))
                    {
                        target.setPixel(cursorX + col, y + row, color);
                    }
                }
            }
            cursorX += 4; // 3 pixels wide + 1 pixel spacing
        }
    }

    /**
     * @brief Draw text centered horizontally at a position
     * @tparam RenderTarget Any class with setPixel(x, y, Color)
     * @param target Render target to draw on
     * @param text Text to draw
     * @param centerX Center X coordinate
     * @param y Top edge Y coordinate
     * @param color Text color
     */
    template <typename RenderTarget>
    static void drawCentered(RenderTarget &target, const std::string &text,
                             int centerX, int y, const Color &color)
    {
        int width = font::textWidth(text);
        int x = centerX - width / 2;
        draw(target, text, x, y, color);
    }

    /**
     * @brief Draw text right-aligned at a position
     * @tparam RenderTarget Any class with setPixel(x, y, Color)
     * @param target Render target to draw on
     * @param text Text to draw
     * @param rightX Right edge X coordinate
     * @param y Top edge Y coordinate
     * @param color Text color
     */
    template <typename RenderTarget>
    static void drawRight(RenderTarget &target, const std::string &text,
                          int rightX, int y, const Color &color)
    {
        int width = font::textWidth(text);
        int x = rightX - width;
        draw(target, text, x, y, color);
    }

    /**
     * @brief Draw text with a background box
     * @tparam RenderTarget Any class with setPixel and fillRect
     * @param target Render target to draw on
     * @param text Text to draw
     * @param x Left edge X coordinate
     * @param y Top edge Y coordinate
     * @param fgColor Text color
     * @param bgColor Background color
     * @param padding Padding around text
     */
    template <typename RenderTarget>
    static void drawWithBackground(RenderTarget &target, const std::string &text,
                                   int x, int y, const Color &fgColor,
                                   const Color &bgColor, int padding = 1)
    {
        int width = font::textWidth(text);
        int height = font::textHeight();

        // Draw background rectangle
        for (int py = y - padding; py < y + height + padding; ++py)
        {
            for (int px = x - padding; px < x + width + padding; ++px)
            {
                target.setPixel(px, py, bgColor);
            }
        }

        // Draw text
        draw(target, text, x, y, fgColor);
    }

    /**
     * @brief Draw text centered horizontally with background
     */
    template <typename RenderTarget>
    static void drawCenteredWithBackground(RenderTarget &target, const std::string &text,
                                           int centerX, int y, const Color &fgColor,
                                           const Color &bgColor, int padding = 1)
    {
        int width = font::textWidth(text);
        int x = centerX - width / 2;
        drawWithBackground(target, text, x, y, fgColor, bgColor, padding);
    }

    /**
     * @brief Draw an outlined/shadow text effect
     * @tparam RenderTarget Any class with setPixel
     * @param target Render target to draw on
     * @param text Text to draw
     * @param x Left edge X coordinate
     * @param y Top edge Y coordinate
     * @param fgColor Text color
     * @param shadowColor Shadow/outline color
     */
    template <typename RenderTarget>
    static void drawWithShadow(RenderTarget &target, const std::string &text,
                               int x, int y, const Color &fgColor,
                               const Color &shadowColor)
    {
        // Draw shadow offset by 1 pixel
        draw(target, text, x + 1, y + 1, shadowColor);
        // Draw foreground
        draw(target, text, x, y, fgColor);
    }

    /**
     * @brief Get the width of text in pixels
     */
    static int width(const std::string &text)
    {
        return font::textWidth(text);
    }

    /**
     * @brief Get the height of text in pixels
     */
    static int height()
    {
        return font::textHeight();
    }

    // ========== LARGE TEXT (5x7 font) ==========

    /**
     * @brief Draw large text at a position (5x7 font for better readability)
     * @tparam RenderTarget Any class with setPixel(x, y, Color)
     * @param target Render target to draw on
     * @param text Text to draw
     * @param x Left edge X coordinate
     * @param y Top edge Y coordinate
     * @param color Text color
     */
    template <typename RenderTarget>
    static void drawLarge(RenderTarget &target, const std::string &text,
                          int x, int y, const Color &color)
    {
        const auto &glyphs = font::getLargeFont();
        int cursorX = x;

        for (char c : text)
        {
            char uc = (c >= 'a' && c <= 'z') ? (c - 32) : c; // uppercase
            auto it = glyphs.find(uc);
            if (it == glyphs.end())
            {
                cursorX += 6; // space width
                continue;
            }

            const font::LargeGlyph &glyph = it->second;
            for (int row = 0; row < 7; ++row)
            {
                uint8_t bits = glyph.rows[row];
                for (int col = 0; col < 5; ++col)
                {
                    if (bits & (1 << (4 - col)))
                    {
                        target.setPixel(cursorX + col, y + row, color);
                    }
                }
            }
            cursorX += 6; // 5 pixels wide + 1 pixel spacing
        }
    }

    /**
     * @brief Get width of large text in pixels
     */
    static int widthLarge(const std::string &text)
    {
        return static_cast<int>(text.size()) * 6 - (text.empty() ? 0 : 1);
    }

    /**
     * @brief Get height of large text (always 7)
     */
    static int heightLarge()
    {
        return 7;
    }

    /**
     * @brief Draw large text centered horizontally
     */
    template <typename RenderTarget>
    static void drawLargeCentered(RenderTarget &target, const std::string &text,
                                  int centerX, int y, const Color &color)
    {
        int w = widthLarge(text);
        int x = centerX - w / 2;
        drawLarge(target, text, x, y, color);
    }

    /**
     * @brief Draw large text with shadow for better visibility
     */
    template <typename RenderTarget>
    static void drawLargeWithShadow(RenderTarget &target, const std::string &text,
                                    int x, int y, const Color &fgColor,
                                    const Color &shadowColor)
    {
        drawLarge(target, text, x + 1, y + 1, shadowColor);
        drawLarge(target, text, x, y, fgColor);
    }

    /**
     * @brief Draw large text centered with shadow
     */
    template <typename RenderTarget>
    static void drawLargeCenteredWithShadow(RenderTarget &target, const std::string &text,
                                            int centerX, int y, const Color &fgColor,
                                            const Color &shadowColor)
    {
        int w = widthLarge(text);
        int x = centerX - w / 2;
        drawLargeWithShadow(target, text, x, y, fgColor, shadowColor);
    }

    /**
     * @brief Draw large text with background box
     */
    template <typename RenderTarget>
    static void drawLargeWithBackground(RenderTarget &target, const std::string &text,
                                        int x, int y, const Color &fgColor,
                                        const Color &bgColor, int padding = 2)
    {
        int w = widthLarge(text);
        int h = heightLarge();

        for (int py = y - padding; py < y + h + padding; ++py)
        {
            for (int px = x - padding; px < x + w + padding; ++px)
            {
                target.setPixel(px, py, bgColor);
            }
        }
        drawLarge(target, text, x, y, fgColor);
    }

    /**
     * @brief Draw large text centered with background
     */
    template <typename RenderTarget>
    static void drawLargeCenteredWithBackground(RenderTarget &target, const std::string &text,
                                                int centerX, int y, const Color &fgColor,
                                                const Color &bgColor, int padding = 2)
    {
        int w = widthLarge(text);
        int x = centerX - w / 2;
        drawLargeWithBackground(target, text, x, y, fgColor, bgColor, padding);
    }
};

TG_NAMESPACE_END
