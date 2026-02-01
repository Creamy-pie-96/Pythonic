#pragma once
/**
 * @file pythonicDraw.hpp
 * @brief Terminal graphics using Braille Unicode characters
 *
 * This module provides a high-resolution drawing system for terminal output
 * using Unicode Braille patterns (U+2800-U+28FF). Each character cell represents
 * a 2×4 pixel grid, allowing for smooth curves and detailed graphics.
 *
 * Features:
 * - High-resolution pixel graphics (8x terminal resolution)
 * - Optimized block-based rendering for real-time video
 * - FFmpeg integration for video streaming
 * - Double-buffering with ANSI escape codes to avoid flickering
 *
 * Braille dot layout per character:
 *   Col 0   Col 1
 *   [1]     [4]     Row 0  (bits 0, 3)
 *   [2]     [5]     Row 1  (bits 1, 4)
 *   [3]     [6]     Row 2  (bits 2, 5)
 *   [7]     [8]     Row 3  (bits 6, 7)
 *
 * Unicode codepoint = 0x2800 + bit_pattern
 *
 * Example usage:
 *   BrailleCanvas canvas(80, 40);  // 160×160 pixel resolution
 *   canvas.line(0, 0, 159, 159);   // Draw diagonal
 *   canvas.circle(80, 80, 40);     // Draw circle
 *   std::cout << canvas.render();
 *
 *   // Video streaming:
 *   VideoPlayer player("video.mp4", 80);
 *   player.play();
 */

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <functional>
#include <array>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <iomanip>
#include <tuple>

namespace pythonic
{
    namespace draw
    {

        /**
         * Braille dot bit values for each position in a 2×4 cell
         * Index: [row][col]
         */
        constexpr uint8_t BRAILLE_DOTS[4][2] = {
            {0x01, 0x08}, // Row 0: bit 0, bit 3
            {0x02, 0x10}, // Row 1: bit 1, bit 4
            {0x04, 0x20}, // Row 2: bit 2, bit 5
            {0x40, 0x80}  // Row 3: bit 6, bit 7
        };

        /**
         * @brief Precomputed lookup table for all 256 braille patterns
         * Maps byte value directly to UTF-8 string for maximum speed
         */
        class BrailleLUT
        {
        private:
            std::array<std::string, 256> _lut;

        public:
            BrailleLUT()
            {
                for (int i = 0; i < 256; ++i)
                {
                    char32_t codepoint = 0x2800 + i;
                    _lut[i].reserve(3);

                    if (codepoint < 0x80)
                    {
                        _lut[i] += static_cast<char>(codepoint);
                    }
                    else if (codepoint < 0x800)
                    {
                        _lut[i] += static_cast<char>(0xC0 | (codepoint >> 6));
                        _lut[i] += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else
                    {
                        _lut[i] += static_cast<char>(0xE0 | (codepoint >> 12));
                        _lut[i] += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        _lut[i] += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                }
            }

            const std::string &get(uint8_t bits) const { return _lut[bits]; }
        };

        // Global lookup table instance (created once at startup)
        inline const BrailleLUT &braille_lut()
        {
            static BrailleLUT lut;
            return lut;
        }

        /**
         * @brief Convert a braille bit pattern to UTF-8 string
         */
        inline std::string braille_to_utf8(uint8_t bits)
        {
            return braille_lut().get(bits);
        }

        /**
         * @brief ANSI escape codes for terminal control
         */
        namespace ansi
        {
            constexpr const char *CURSOR_HOME = "\033[H";    // Move cursor to top-left
            constexpr const char *CLEAR_SCREEN = "\033[2J";  // Clear entire screen
            constexpr const char *HIDE_CURSOR = "\033[?25l"; // Hide cursor
            constexpr const char *SHOW_CURSOR = "\033[?25h"; // Show cursor
            constexpr const char *RESET = "\033[0m";         // Reset all attributes

            inline std::string cursor_to(int row, int col)
            {
                return "\033[" + std::to_string(row + 1) + ";" + std::to_string(col + 1) + "H";
            }
        } // namespace ansi

        /**
         * @brief High-resolution terminal canvas using Braille characters
         *
         * Provides a pixel-addressable canvas where each character cell contains
         * a 2×4 pixel grid. Drawing operations work in pixel coordinates.
         *
         * Optimized for real-time rendering with:
         * - Block-based pixel setting (set entire 2×4 block at once)
         * - Precomputed UTF-8 lookup table
         * - Memory-efficient storage
         */
        class BrailleCanvas
        {
        private:
            size_t _char_width;   // Width in characters
            size_t _char_height;  // Height in characters
            size_t _pixel_width;  // Width in pixels (char_width * 2)
            size_t _pixel_height; // Height in pixels (char_height * 4)

            // Storage: one byte per character cell, bits represent dots
            std::vector<std::vector<uint8_t>> _canvas;

        public:
            /**
             * @brief Create a canvas with given character dimensions
             * @param char_width Width in terminal characters
             * @param char_height Height in terminal characters
             */
            BrailleCanvas(size_t char_width = 80, size_t char_height = 24)
                : _char_width(char_width), _char_height(char_height), _pixel_width(char_width * 2), _pixel_height(char_height * 4), _canvas(char_height, std::vector<uint8_t>(char_width, 0))
            {
            }

            /**
             * @brief Create canvas from pixel dimensions
             */
            static BrailleCanvas from_pixels(size_t pixel_width, size_t pixel_height)
            {
                size_t cw = (pixel_width + 1) / 2;
                size_t ch = (pixel_height + 3) / 4;
                return BrailleCanvas(cw, ch);
            }

            // Dimensions
            size_t char_width() const { return _char_width; }
            size_t char_height() const { return _char_height; }
            size_t pixel_width() const { return _pixel_width; }
            size_t pixel_height() const { return _pixel_height; }

            /**
             * @brief Clear the canvas
             */
            void clear()
            {
                for (auto &row : _canvas)
                    std::fill(row.begin(), row.end(), 0);
            }

            /**
             * @brief Set a single pixel
             * @param x X coordinate (0 to pixel_width-1)
             * @param y Y coordinate (0 to pixel_height-1)
             * @param on True to set dot, false to clear
             */
            void set_pixel(int x, int y, bool on = true)
            {
                if (x < 0 || x >= (int)_pixel_width || y < 0 || y >= (int)_pixel_height)
                    return;

                int char_x = x / 2;
                int char_y = y / 4;
                int local_x = x % 2;
                int local_y = y % 4;

                uint8_t bit = BRAILLE_DOTS[local_y][local_x];

                if (on)
                    _canvas[char_y][char_x] |= bit;
                else
                    _canvas[char_y][char_x] &= ~bit;
            }

            /**
             * @brief OPTIMIZED: Set an entire 2×4 pixel block at once using a single OR
             *
             * This is 8x faster than calling set_pixel 8 times.
             * The pixel block is a 2×4 array of boolean values where:
             *   pixels[row][col] for row 0-3, col 0-1
             *
             * @param char_x Character cell X coordinate
             * @param char_y Character cell Y coordinate
             * @param pixels 4×2 array of pixel values (row-major: pixels[row][col])
             */
            void set_block(int char_x, int char_y, const bool pixels[4][2])
            {
                if (char_x < 0 || char_x >= (int)_char_width ||
                    char_y < 0 || char_y >= (int)_char_height)
                    return;

                // Compute entire 8-bit pattern with single bitwise OR operations
                uint8_t pattern = 0;
                if (pixels[0][0])
                    pattern |= 0x01;
                if (pixels[0][1])
                    pattern |= 0x08;
                if (pixels[1][0])
                    pattern |= 0x02;
                if (pixels[1][1])
                    pattern |= 0x10;
                if (pixels[2][0])
                    pattern |= 0x04;
                if (pixels[2][1])
                    pattern |= 0x20;
                if (pixels[3][0])
                    pattern |= 0x40;
                if (pixels[3][1])
                    pattern |= 0x80;

                _canvas[char_y][char_x] = pattern;
            }

            /**
             * @brief OPTIMIZED: Set block from 8 grayscale values with threshold
             *
             * Perfect for video frame rendering - processes a 2×4 pixel block
             * by comparing each grayscale value against a threshold.
             *
             * @param char_x Character cell X coordinate
             * @param char_y Character cell Y coordinate
             * @param gray Array of 8 grayscale values [row0_col0, row0_col1, row1_col0, ...]
             * @param threshold Threshold value (pixels >= threshold are "on")
             */
            void set_block_gray(int char_x, int char_y, const uint8_t gray[8], uint8_t threshold)
            {
                if (char_x < 0 || char_x >= (int)_char_width ||
                    char_y < 0 || char_y >= (int)_char_height)
                    return;

                // Compute pattern in one go - unrolled for maximum speed
                uint8_t pattern = 0;
                pattern |= (gray[0] >= threshold) ? 0x01 : 0; // row 0, col 0
                pattern |= (gray[1] >= threshold) ? 0x08 : 0; // row 0, col 1
                pattern |= (gray[2] >= threshold) ? 0x02 : 0; // row 1, col 0
                pattern |= (gray[3] >= threshold) ? 0x10 : 0; // row 1, col 1
                pattern |= (gray[4] >= threshold) ? 0x04 : 0; // row 2, col 0
                pattern |= (gray[5] >= threshold) ? 0x20 : 0; // row 2, col 1
                pattern |= (gray[6] >= threshold) ? 0x40 : 0; // row 3, col 0
                pattern |= (gray[7] >= threshold) ? 0x80 : 0; // row 3, col 1

                _canvas[char_y][char_x] = pattern;
            }

            /**
             * @brief Set entire character cell directly with bit pattern
             */
            void set_cell(int char_x, int char_y, uint8_t pattern)
            {
                if (char_x >= 0 && char_x < (int)_char_width &&
                    char_y >= 0 && char_y < (int)_char_height)
                {
                    _canvas[char_y][char_x] = pattern;
                }
            }

            /**
             * @brief Get pixel state
             */
            bool get_pixel(int x, int y) const
            {
                if (x < 0 || x >= (int)_pixel_width || y < 0 || y >= (int)_pixel_height)
                    return false;

                int char_x = x / 2;
                int char_y = y / 4;
                int local_x = x % 2;
                int local_y = y % 4;

                return (_canvas[char_y][char_x] & BRAILLE_DOTS[local_y][local_x]) != 0;
            }

            /**
             * @brief OPTIMIZED: Load grayscale frame data using block operations
             *
             * This is optimized for video streaming - loads an entire frame
             * using block-based operations instead of per-pixel set_pixel calls.
             *
             * @param data Grayscale pixel data (row-major)
             * @param width Image width in pixels
             * @param height Image height in pixels
             * @param threshold Binarization threshold
             */
            void load_frame_fast(const uint8_t *data, int width, int height, uint8_t threshold = 128)
            {
                // Resize canvas if needed
                size_t new_cw = (width + 1) / 2;
                size_t new_ch = (height + 3) / 4;

                if (new_cw != _char_width || new_ch != _char_height)
                {
                    _char_width = new_cw;
                    _char_height = new_ch;
                    _pixel_width = _char_width * 2;
                    _pixel_height = _char_height * 4;
                    _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                }

                // Process each character cell using block operations
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        uint8_t gray[8] = {0, 0, 0, 0, 0, 0, 0, 0};

                        // Extract 2×4 pixel block from source image
                        int px = cx * 2;
                        int py = cy * 4;

                        for (int row = 0; row < 4; ++row)
                        {
                            int y = py + row;
                            if (y >= height)
                                continue;

                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col;
                                if (x >= width)
                                    continue;

                                gray[row * 2 + col] = data[y * width + x];
                            }
                        }

                        set_block_gray(cx, cy, gray, threshold);
                    }
                }
            }

            /**
             * @brief Load RGB frame data using block operations
             */
            void load_frame_rgb_fast(const uint8_t *data, int width, int height, uint8_t threshold = 128)
            {
                // Resize canvas if needed
                size_t new_cw = (width + 1) / 2;
                size_t new_ch = (height + 3) / 4;

                if (new_cw != _char_width || new_ch != _char_height)
                {
                    _char_width = new_cw;
                    _char_height = new_ch;
                    _pixel_width = _char_width * 2;
                    _pixel_height = _char_height * 4;
                    _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                }

                // Process each character cell
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        uint8_t gray[8] = {0, 0, 0, 0, 0, 0, 0, 0};

                        int px = cx * 2;
                        int py = cy * 4;

                        for (int row = 0; row < 4; ++row)
                        {
                            int y = py + row;
                            if (y >= height)
                                continue;

                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col;
                                if (x >= width)
                                    continue;

                                size_t idx = (y * width + x) * 3;
                                // Convert RGB to grayscale: 0.299R + 0.587G + 0.114B
                                gray[row * 2 + col] = (uint8_t)((299 * data[idx] +
                                                                 587 * data[idx + 1] +
                                                                 114 * data[idx + 2]) /
                                                                1000);
                            }
                        }

                        set_block_gray(cx, cy, gray, threshold);
                    }
                }
            }

            // ==================== Drawing Primitives ====================

            /**
             * @brief Draw a line using Bresenham's algorithm
             */
            void line(int x0, int y0, int x1, int y1)
            {
                int dx = std::abs(x1 - x0);
                int dy = std::abs(y1 - y0);
                int sx = (x0 < x1) ? 1 : -1;
                int sy = (y0 < y1) ? 1 : -1;
                int err = dx - dy;

                while (true)
                {
                    set_pixel(x0, y0);

                    if (x0 == x1 && y0 == y1)
                        break;

                    int e2 = 2 * err;
                    if (e2 > -dy)
                    {
                        err -= dy;
                        x0 += sx;
                    }
                    if (e2 < dx)
                    {
                        err += dx;
                        y0 += sy;
                    }
                }
            }

            /**
             * @brief Draw a rectangle outline
             */
            void rect(int x0, int y0, int x1, int y1)
            {
                line(x0, y0, x1, y0); // Top
                line(x1, y0, x1, y1); // Right
                line(x1, y1, x0, y1); // Bottom
                line(x0, y1, x0, y0); // Left
            }

            /**
             * @brief Draw a filled rectangle
             */
            void fill_rect(int x0, int y0, int x1, int y1)
            {
                if (x0 > x1)
                    std::swap(x0, x1);
                if (y0 > y1)
                    std::swap(y0, y1);

                for (int y = y0; y <= y1; ++y)
                    for (int x = x0; x <= x1; ++x)
                        set_pixel(x, y);
            }

            /**
             * @brief Draw a circle using midpoint algorithm
             */
            void circle(int cx, int cy, int radius)
            {
                int x = radius;
                int y = 0;
                int err = 0;

                while (x >= y)
                {
                    set_pixel(cx + x, cy + y);
                    set_pixel(cx + y, cy + x);
                    set_pixel(cx - y, cy + x);
                    set_pixel(cx - x, cy + y);
                    set_pixel(cx - x, cy - y);
                    set_pixel(cx - y, cy - x);
                    set_pixel(cx + y, cy - x);
                    set_pixel(cx + x, cy - y);

                    y++;
                    err += 1 + 2 * y;
                    if (2 * (err - x) + 1 > 0)
                    {
                        x--;
                        err += 1 - 2 * x;
                    }
                }
            }

            /**
             * @brief Draw a filled circle
             */
            void fill_circle(int cx, int cy, int radius)
            {
                for (int y = -radius; y <= radius; ++y)
                {
                    int width = (int)std::sqrt(radius * radius - y * y);
                    for (int x = -width; x <= width; ++x)
                        set_pixel(cx + x, cy + y);
                }
            }

            /**
             * @brief Draw an ellipse
             */
            void ellipse(int cx, int cy, int rx, int ry)
            {
                int rx2 = rx * rx;
                int ry2 = ry * ry;
                int two_rx2 = 2 * rx2;
                int two_ry2 = 2 * ry2;

                int x = 0;
                int y = ry;
                int px = 0;
                int py = two_rx2 * y;

                // Plot initial points
                set_pixel(cx + x, cy + y);
                set_pixel(cx - x, cy + y);
                set_pixel(cx + x, cy - y);
                set_pixel(cx - x, cy - y);

                // Region 1
                int p = (int)(ry2 - rx2 * ry + 0.25 * rx2);
                while (px < py)
                {
                    x++;
                    px += two_ry2;
                    if (p < 0)
                        p += ry2 + px;
                    else
                    {
                        y--;
                        py -= two_rx2;
                        p += ry2 + px - py;
                    }
                    set_pixel(cx + x, cy + y);
                    set_pixel(cx - x, cy + y);
                    set_pixel(cx + x, cy - y);
                    set_pixel(cx - x, cy - y);
                }

                // Region 2
                p = (int)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
                while (y > 0)
                {
                    y--;
                    py -= two_rx2;
                    if (p > 0)
                        p += rx2 - py;
                    else
                    {
                        x++;
                        px += two_ry2;
                        p += rx2 - py + px;
                    }
                    set_pixel(cx + x, cy + y);
                    set_pixel(cx - x, cy + y);
                    set_pixel(cx + x, cy - y);
                    set_pixel(cx - x, cy - y);
                }
            }

            /**
             * @brief Draw text at pixel position
             * Simple 3x5 pixel font
             */
            void text(int x, int y, const std::string &str)
            {
                // Simple 3x5 bitmap font for digits and uppercase letters
                static const std::array<std::array<uint8_t, 5>, 36> font = {{
                    // 0-9
                    {{0b111, 0b101, 0b101, 0b101, 0b111}}, // 0
                    {{0b010, 0b110, 0b010, 0b010, 0b111}}, // 1
                    {{0b111, 0b001, 0b111, 0b100, 0b111}}, // 2
                    {{0b111, 0b001, 0b111, 0b001, 0b111}}, // 3
                    {{0b101, 0b101, 0b111, 0b001, 0b001}}, // 4
                    {{0b111, 0b100, 0b111, 0b001, 0b111}}, // 5
                    {{0b111, 0b100, 0b111, 0b101, 0b111}}, // 6
                    {{0b111, 0b001, 0b001, 0b001, 0b001}}, // 7
                    {{0b111, 0b101, 0b111, 0b101, 0b111}}, // 8
                    {{0b111, 0b101, 0b111, 0b001, 0b111}}, // 9
                    // A-Z
                    {{0b111, 0b101, 0b111, 0b101, 0b101}}, // A
                    {{0b110, 0b101, 0b110, 0b101, 0b110}}, // B
                    {{0b111, 0b100, 0b100, 0b100, 0b111}}, // C
                    {{0b110, 0b101, 0b101, 0b101, 0b110}}, // D
                    {{0b111, 0b100, 0b110, 0b100, 0b111}}, // E
                    {{0b111, 0b100, 0b110, 0b100, 0b100}}, // F
                    {{0b111, 0b100, 0b101, 0b101, 0b111}}, // G
                    {{0b101, 0b101, 0b111, 0b101, 0b101}}, // H
                    {{0b111, 0b010, 0b010, 0b010, 0b111}}, // I
                    {{0b001, 0b001, 0b001, 0b101, 0b111}}, // J
                    {{0b101, 0b110, 0b100, 0b110, 0b101}}, // K
                    {{0b100, 0b100, 0b100, 0b100, 0b111}}, // L
                    {{0b101, 0b111, 0b111, 0b101, 0b101}}, // M
                    {{0b101, 0b111, 0b111, 0b111, 0b101}}, // N
                    {{0b111, 0b101, 0b101, 0b101, 0b111}}, // O
                    {{0b111, 0b101, 0b111, 0b100, 0b100}}, // P
                    {{0b111, 0b101, 0b101, 0b111, 0b001}}, // Q
                    {{0b111, 0b101, 0b111, 0b110, 0b101}}, // R
                    {{0b111, 0b100, 0b111, 0b001, 0b111}}, // S
                    {{0b111, 0b010, 0b010, 0b010, 0b010}}, // T
                    {{0b101, 0b101, 0b101, 0b101, 0b111}}, // U
                    {{0b101, 0b101, 0b101, 0b101, 0b010}}, // V
                    {{0b101, 0b101, 0b111, 0b111, 0b101}}, // W
                    {{0b101, 0b101, 0b010, 0b101, 0b101}}, // X
                    {{0b101, 0b101, 0b010, 0b010, 0b010}}, // Y
                    {{0b111, 0b001, 0b010, 0b100, 0b111}}, // Z
                }};

                int px = x;
                for (char c : str)
                {
                    int idx = -1;
                    if (c >= '0' && c <= '9')
                        idx = c - '0';
                    else if (c >= 'A' && c <= 'Z')
                        idx = c - 'A' + 10;
                    else if (c >= 'a' && c <= 'z')
                        idx = c - 'a' + 10;
                    else if (c == ' ')
                    {
                        px += 4;
                        continue;
                    }

                    if (idx >= 0 && idx < 36)
                    {
                        for (int row = 0; row < 5; ++row)
                        {
                            for (int col = 0; col < 3; ++col)
                            {
                                if (font[idx][row] & (1 << (2 - col)))
                                    set_pixel(px + col, y + row);
                            }
                        }
                    }
                    px += 4; // Character width + spacing
                }
            }

            /**
             * @brief Draw an arrow from (x0,y0) to (x1,y1)
             */
            void arrow(int x0, int y0, int x1, int y1, int head_size = 4)
            {
                // Draw the line
                line(x0, y0, x1, y1);

                // Calculate arrow head
                double angle = std::atan2(y1 - y0, x1 - x0);
                double angle_offset = 2.5; // ~143 degrees

                int ax1 = x1 - (int)(head_size * std::cos(angle - angle_offset));
                int ay1 = y1 - (int)(head_size * std::sin(angle - angle_offset));
                int ax2 = x1 - (int)(head_size * std::cos(angle + angle_offset));
                int ay2 = y1 - (int)(head_size * std::sin(angle + angle_offset));

                line(x1, y1, ax1, ay1);
                line(x1, y1, ax2, ay2);
            }

            // ==================== Image Loading ====================

            /**
             * @brief Load a PGM (P5 binary) or PPM (P6 binary) image
             * Converts to grayscale if color, then thresholds
             */
            bool load_pgm_ppm(const std::string &filename, int threshold = 128)
            {
                std::ifstream file(filename, std::ios::binary);
                if (!file)
                    return false;

                std::string magic;
                file >> magic;

                if (magic != "P5" && magic != "P6")
                    return false;

                bool is_color = (magic == "P6");

                // Skip comments
                char c;
                file.get(c);
                while (file.peek() == '#')
                {
                    std::string comment;
                    std::getline(file, comment);
                }

                int width, height, maxval;
                file >> width >> height >> maxval;
                file.get(c); // Skip single whitespace

                if (maxval > 255)
                    return false;

                // Resize canvas to fit image
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0));

                // Read and threshold pixels
                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        int gray;
                        if (is_color)
                        {
                            unsigned char rgb[3];
                            file.read(reinterpret_cast<char *>(rgb), 3);
                            // Convert to grayscale: 0.299R + 0.587G + 0.114B
                            gray = (299 * rgb[0] + 587 * rgb[1] + 114 * rgb[2]) / 1000;
                        }
                        else
                        {
                            unsigned char g;
                            file.read(reinterpret_cast<char *>(&g), 1);
                            gray = g;
                        }

                        if (gray >= threshold)
                            set_pixel(x, y);
                    }
                }

                return true;
            }

            /**
             * @brief Load raw pixel data (grayscale)
             * @param data Pixel values (row-major, 0-255)
             * @param width Image width in pixels
             * @param height Image height in pixels
             * @param threshold Binarization threshold
             */
            void load_pixels(const std::vector<uint8_t> &data, int width, int height, int threshold = 128)
            {
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0));

                for (int y = 0; y < height && y < (int)data.size() / width; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        if (data[y * width + x] >= threshold)
                            set_pixel(x, y);
                    }
                }
            }

            /**
             * @brief Load raw pixel data (RGB)
             */
            void load_rgb(const std::vector<uint8_t> &data, int width, int height, int threshold = 128)
            {
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0));

                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        size_t idx = (y * width + x) * 3;
                        if (idx + 2 < data.size())
                        {
                            int gray = (299 * data[idx] + 587 * data[idx + 1] + 114 * data[idx + 2]) / 1000;
                            if (gray >= threshold)
                                set_pixel(x, y);
                        }
                    }
                }
            }

            // ==================== Rendering ====================

            /**
             * @brief Render canvas to string
             */
            std::string render() const
            {
                std::ostringstream out;

                for (size_t y = 0; y < _char_height; ++y)
                {
                    for (size_t x = 0; x < _char_width; ++x)
                    {
                        out << braille_to_utf8(_canvas[y][x]);
                    }
                    if (y < _char_height - 1)
                        out << "\n";
                }

                return out.str();
            }

            /**
             * @brief Render with a border
             */
            std::string render_bordered(const std::string &title = "") const
            {
                std::ostringstream out;

                // Top border
                out << "┌";
                if (!title.empty())
                {
                    out << "─ " << title << " ";
                    for (size_t i = title.length() + 4; i < _char_width; ++i)
                        out << "─";
                }
                else
                {
                    for (size_t i = 0; i < _char_width; ++i)
                        out << "─";
                }
                out << "┐\n";

                // Content
                for (size_t y = 0; y < _char_height; ++y)
                {
                    out << "│";
                    for (size_t x = 0; x < _char_width; ++x)
                        out << braille_to_utf8(_canvas[y][x]);
                    out << "│\n";
                }

                // Bottom border
                out << "└";
                for (size_t i = 0; i < _char_width; ++i)
                    out << "─";
                out << "┘";

                return out.str();
            }

            /**
             * @brief Apply a transformation function to each pixel
             */
            void transform(std::function<bool(int, int, bool)> func)
            {
                for (int y = 0; y < (int)_pixel_height; ++y)
                {
                    for (int x = 0; x < (int)_pixel_width; ++x)
                    {
                        bool current = get_pixel(x, y);
                        set_pixel(x, y, func(x, y, current));
                    }
                }
            }

            /**
             * @brief Invert all pixels
             */
            void invert()
            {
                for (auto &row : _canvas)
                    for (auto &cell : row)
                        cell = ~cell;
            }

            /**
             * @brief Flood fill from a point
             */
            void flood_fill(int x, int y, bool fill_value = true)
            {
                if (x < 0 || x >= (int)_pixel_width || y < 0 || y >= (int)_pixel_height)
                    return;
                if (get_pixel(x, y) == fill_value)
                    return;

                std::vector<std::pair<int, int>> stack;
                stack.push_back({x, y});

                while (!stack.empty())
                {
                    auto [cx, cy] = stack.back();
                    stack.pop_back();

                    if (cx < 0 || cx >= (int)_pixel_width || cy < 0 || cy >= (int)_pixel_height)
                        continue;
                    if (get_pixel(cx, cy) == fill_value)
                        continue;

                    set_pixel(cx, cy, fill_value);

                    stack.push_back({cx + 1, cy});
                    stack.push_back({cx - 1, cy});
                    stack.push_back({cx, cy + 1});
                    stack.push_back({cx, cy - 1});
                }
            }

            /**
             * @brief Draw a polygon
             */
            void polygon(const std::vector<std::pair<int, int>> &points)
            {
                if (points.size() < 2)
                    return;

                for (size_t i = 0; i < points.size(); ++i)
                {
                    size_t j = (i + 1) % points.size();
                    line(points[i].first, points[i].second, points[j].first, points[j].second);
                }
            }

            /**
             * @brief Draw a bezier curve
             */
            void bezier(int x0, int y0, int x1, int y1, int x2, int y2, int segments = 20)
            {
                int prev_x = x0, prev_y = y0;

                for (int i = 1; i <= segments; ++i)
                {
                    double t = (double)i / segments;
                    double t2 = t * t;
                    double mt = 1 - t;
                    double mt2 = mt * mt;

                    int x = (int)(mt2 * x0 + 2 * mt * t * x1 + t2 * x2);
                    int y = (int)(mt2 * y0 + 2 * mt * t * y1 + t2 * y2);

                    line(prev_x, prev_y, x, y);
                    prev_x = x;
                    prev_y = y;
                }
            }

            /**
             * @brief Draw a cubic bezier curve
             */
            void bezier_cubic(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int segments = 30)
            {
                int prev_x = x0, prev_y = y0;

                for (int i = 1; i <= segments; ++i)
                {
                    double t = (double)i / segments;
                    double t2 = t * t;
                    double t3 = t2 * t;
                    double mt = 1 - t;
                    double mt2 = mt * mt;
                    double mt3 = mt2 * mt;

                    int x = (int)(mt3 * x0 + 3 * mt2 * t * x1 + 3 * mt * t2 * x2 + t3 * x3);
                    int y = (int)(mt3 * y0 + 3 * mt2 * t * y1 + 3 * mt * t2 * y2 + t3 * y3);

                    line(prev_x, prev_y, x, y);
                    prev_x = x;
                    prev_y = y;
                }
            }
        };

        // ==================== Image Format Support ====================

        /**
         * @brief Check if a file is an image based on extension
         */
        inline bool is_image_file(const std::string &filename)
        {
            std::string ext = filename;
            size_t dot = ext.rfind('.');
            if (dot == std::string::npos)
                return false;
            ext = ext.substr(dot);

            // Convert to lowercase
            for (auto &c : ext)
                c = std::tolower(c);

            return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
                   ext == ".gif" || ext == ".bmp" || ext == ".ppm" ||
                   ext == ".pgm" || ext == ".pbm";
        }

        /**
         * @brief Convert any image to PPM using ImageMagick's convert command
         * @return Path to temporary PPM file, or empty string on failure
         */
        inline std::string convert_to_ppm(const std::string &input_file, int max_width = 160)
        {
            // Create temp file path
            std::string temp_ppm = "/tmp/pythonic_img_" + std::to_string(std::hash<std::string>{}(input_file)) + ".ppm";

            // Use ImageMagick convert: resize and convert to PPM
            std::string cmd = "convert \"" + input_file + "\" -resize " +
                              std::to_string(max_width) + "x -depth 8 \"" + temp_ppm + "\" 2>/dev/null";

            int result = std::system(cmd.c_str());
            if (result != 0)
                return "";

            return temp_ppm;
        }

        /**
         * @brief Render a DOT graph string to PPM image using Graphviz
         * @return Path to temporary PPM file, or empty string on failure
         */
        inline std::string dot_to_ppm(const std::string &dot_content, int width = 800)
        {
            // Create temp files
            std::string hash = std::to_string(std::hash<std::string>{}(dot_content));
            std::string temp_dot = "/tmp/pythonic_graph_" + hash + ".dot";
            std::string temp_ppm = "/tmp/pythonic_graph_" + hash + ".ppm";

            // Write DOT content
            std::ofstream dot_file(temp_dot);
            if (!dot_file)
                return "";
            dot_file << dot_content;
            dot_file.close();

            // Render with Graphviz (using dot command)
            // First to PNG, then convert to PPM for easier parsing
            std::string temp_png = "/tmp/pythonic_graph_" + hash + ".png";
            std::string cmd = "dot -Tpng -Gsize=\"" + std::to_string(width / 72) + "," +
                              std::to_string(width / 72) + "\" -Gdpi=72 \"" + temp_dot +
                              "\" -o \"" + temp_png + "\" 2>/dev/null";

            int result = std::system(cmd.c_str());
            if (result != 0)
            {
                // Try neato as fallback
                cmd = "neato -Tpng -Gsize=\"" + std::to_string(width / 72) + "," +
                      std::to_string(width / 72) + "\" -Gdpi=72 \"" + temp_dot +
                      "\" -o \"" + temp_png + "\" 2>/dev/null";
                result = std::system(cmd.c_str());
                if (result != 0)
                    return "";
            }

            // Convert PNG to PPM
            cmd = "convert \"" + temp_png + "\" -depth 8 \"" + temp_ppm + "\" 2>/dev/null";
            result = std::system(cmd.c_str());
            if (result != 0)
                return "";

            // Cleanup intermediate files
            std::remove(temp_dot.c_str());
            std::remove(temp_png.c_str());

            return temp_ppm;
        }

        /**
         * @brief Render an image file to terminal string with auto-format detection
         */
        inline std::string render_image(const std::string &filename, int max_width = 80, int threshold = 128)
        {
            // Check file exists
            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            // Check extension
            std::string ext = filename;
            size_t dot = ext.rfind('.');
            if (dot != std::string::npos)
            {
                ext = ext.substr(dot);
                for (auto &c : ext)
                    c = std::tolower(c);
            }
            else
            {
                ext = "";
            }

            BrailleCanvas canvas;

            // Try direct PPM/PGM loading first
            if (ext == ".ppm" || ext == ".pgm" || ext == ".pbm")
            {
                if (canvas.load_pgm_ppm(filename, threshold))
                    return canvas.render();
            }

            // For other formats, try ImageMagick conversion
            std::string ppm_file = convert_to_ppm(filename, max_width * 2); // 2 pixels per braille char width
            if (!ppm_file.empty())
            {
                if (canvas.load_pgm_ppm(ppm_file, threshold))
                {
                    std::remove(ppm_file.c_str());
                    return canvas.render();
                }
                std::remove(ppm_file.c_str());
            }

            return "Error: Could not load image. Install ImageMagick for PNG/JPG support.\n";
        }

        /**
         * @brief Render a DOT graph to terminal string
         */
        inline std::string render_dot(const std::string &dot_content, int max_width = 80, int threshold = 128)
        {
            std::string ppm_file = dot_to_ppm(dot_content, max_width * 8); // Higher resolution for better quality
            if (ppm_file.empty())
                return "Error: Could not render graph. Install Graphviz (dot) and ImageMagick.\n";

            BrailleCanvas canvas;
            std::string result;

            if (canvas.load_pgm_ppm(ppm_file, threshold))
                result = canvas.render();
            else
                result = "Error: Could not load rendered graph.\n";

            std::remove(ppm_file.c_str());
            return result;
        }

        /**
         * @brief Print an image file to stdout with auto-format detection
         */
        inline void print_image(const std::string &filename, int max_width = 80, int threshold = 128)
        {
            std::cout << render_image(filename, max_width, threshold) << std::endl;
        }

        /**
         * @brief Print a DOT graph to stdout
         */
        inline void print_dot(const std::string &dot_content, int max_width = 80, int threshold = 128)
        {
            std::cout << render_dot(dot_content, max_width, threshold) << std::endl;
        }

        // ==================== Video Streaming Support ====================

        /**
         * @brief Check if a file is a video based on extension
         */
        inline bool is_video_file(const std::string &filename)
        {
            std::string ext = filename;
            size_t dot = ext.rfind('.');
            if (dot == std::string::npos)
                return false;
            ext = ext.substr(dot);

            for (auto &c : ext)
                c = std::tolower(c);

            return ext == ".mp4" || ext == ".avi" || ext == ".mkv" ||
                   ext == ".mov" || ext == ".webm" || ext == ".flv" ||
                   ext == ".wmv" || ext == ".m4v" || ext == ".gif";
        }

        /**
         * @brief RAII helper to manage terminal state during video playback
         *
         * Ensures the cursor is restored and terminal state is reset even if
         * an exception is thrown or the program exits unexpectedly.
         */
        class TerminalStateGuard
        {
        private:
            bool _active;

        public:
            TerminalStateGuard() : _active(true)
            {
                // Save terminal state and hide cursor
                std::cout << ansi::HIDE_CURSOR << std::flush;
            }

            ~TerminalStateGuard()
            {
                restore();
            }

            void restore()
            {
                if (_active)
                {
                    _active = false;
                    std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
                }
            }

            // Prevent copying
            TerminalStateGuard(const TerminalStateGuard &) = delete;
            TerminalStateGuard &operator=(const TerminalStateGuard &) = delete;
        };

        /**
         * @brief Video player for terminal using braille graphics
         *
         * Uses FFmpeg to decode video frames and renders them in real-time
         * using braille characters. Implements double-buffering with ANSI
         * escape codes to avoid flickering.
         *
         * Example:
         *   VideoPlayer player("video.mp4", 80);
         *   player.play();  // Blocking playback
         *
         *   // Or async:
         *   player.play_async();
         *   // ... do other work ...
         *   player.stop();
         */
        class VideoPlayer
        {
        private:
            std::string _filename;
            int _width;     // Output width in terminal characters
            int _threshold; // Binarization threshold
            double _fps;    // Target FPS (0 = use video's native FPS)
            std::atomic<bool> _running;
            std::thread _playback_thread;

            BrailleCanvas _canvas;

        public:
            /**
             * @brief Create a video player
             * @param filename Path to video file
             * @param width Width in terminal characters (height auto-calculated)
             * @param threshold Brightness threshold for binary conversion (default: 128)
             * @param target_fps Target FPS, 0 = use video's native FPS
             */
            VideoPlayer(const std::string &filename, int width = 80, int threshold = 128, double target_fps = 0)
                : _filename(filename), _width(width), _threshold(threshold), _fps(target_fps), _running(false)
            {
            }

            ~VideoPlayer()
            {
                stop();
                // Ensure terminal state is restored on destruction
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            /**
             * @brief Play video (blocking)
             * @return true if playback completed successfully
             */
            bool play()
            {
                if (_running.exchange(true))
                    return false; // Already running

                bool result = _play_internal();
                _running = false;
                return result;
            }

            /**
             * @brief Start async playback in background thread
             */
            void play_async()
            {
                if (_running.exchange(true))
                    return; // Already running

                _playback_thread = std::thread([this]()
                                               {
                    _play_internal();
                    _running = false; });
            }

            /**
             * @brief Stop playback
             */
            void stop()
            {
                _running = false;
                if (_playback_thread.joinable())
                    _playback_thread.join();
            }

            /**
             * @brief Check if video is playing
             */
            bool is_playing() const { return _running; }

            /**
             * @brief Get video information using ffprobe
             * @return Tuple of (width, height, fps, duration_seconds)
             */
            std::tuple<int, int, double, double> get_info() const
            {
                // Use ffprobe to get video info
                std::string cmd = "ffprobe -v quiet -select_streams v:0 "
                                  "-show_entries stream=width,height,r_frame_rate,duration "
                                  "-of csv=p=0 \"" +
                                  _filename + "\" 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                    return {0, 0, 0, 0};

                char buffer[256];
                std::string result;
                while (fgets(buffer, sizeof(buffer), pipe))
                    result += buffer;
                pclose(pipe);

                // Parse: width,height,fps_num/fps_den,duration
                int w = 0, h = 0;
                double fps = 0, duration = 0;

                size_t pos = 0;
                size_t comma = result.find(',');
                if (comma != std::string::npos)
                {
                    w = std::stoi(result.substr(0, comma));
                    pos = comma + 1;
                }
                comma = result.find(',', pos);
                if (comma != std::string::npos)
                {
                    h = std::stoi(result.substr(pos, comma - pos));
                    pos = comma + 1;
                }
                comma = result.find(',', pos);
                if (comma != std::string::npos)
                {
                    std::string fps_str = result.substr(pos, comma - pos);
                    size_t slash = fps_str.find('/');
                    if (slash != std::string::npos)
                    {
                        double num = std::stod(fps_str.substr(0, slash));
                        double den = std::stod(fps_str.substr(slash + 1));
                        if (den > 0)
                            fps = num / den;
                    }
                    pos = comma + 1;
                }
                try
                {
                    duration = std::stod(result.substr(pos));
                }
                catch (...)
                {
                }

                return {w, h, fps, duration};
            }

        private:
            bool _play_internal()
            {
                // Get video dimensions
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info. Is FFmpeg installed?\n";
                    return false;
                }

                // Calculate output dimensions (preserve aspect ratio)
                // Each braille char is 2 pixels wide, 4 pixels tall
                int pixel_w = _width * 2;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);

                // Ensure height is multiple of 4 for braille
                pixel_h = (pixel_h + 3) / 4 * 4;

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30; // Default

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                // Build FFmpeg command to pipe raw grayscale frames
                std::string cmd = "ffmpeg -i \"" + _filename + "\" "
                                                               "-vf scale=" +
                                  std::to_string(pixel_w) + ":" + std::to_string(pixel_h) +
                                  " -pix_fmt gray -f rawvideo -v quiet - 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg. Is it installed?\n";
                    return false;
                }

                // Allocate frame buffer
                size_t frame_size = pixel_w * pixel_h;
                std::vector<uint8_t> frame_buffer(frame_size);

                // Initialize canvas
                int char_height = pixel_h / 4;
                _canvas = BrailleCanvas(_width, char_height);

                // Use RAII guard for terminal state management
                TerminalStateGuard term_guard;

                // Clear screen and position cursor at top
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();

                while (_running)
                {
                    auto frame_start = std::chrono::steady_clock::now();

                    // Read one frame
                    size_t bytes_read = fread(frame_buffer.data(), 1, frame_size, pipe);
                    if (bytes_read < frame_size)
                        break; // End of video

                    // Convert frame to braille using optimized block operations
                    _canvas.load_frame_fast(frame_buffer.data(), pixel_w, pixel_h, _threshold);

                    // Render with double-buffering (move cursor to top-left without clearing)
                    std::cout << ansi::CURSOR_HOME;
                    std::cout << _canvas.render() << std::flush;

                    ++frame_num;

                    // Frame rate limiting
                    auto frame_end = std::chrono::steady_clock::now();
                    auto elapsed = frame_end - frame_start;

                    if (elapsed < frame_duration)
                    {
                        std::this_thread::sleep_for(frame_duration - elapsed);
                    }
                }

                pclose(pipe);

                // Restore terminal state (guard destructor handles cursor)
                term_guard.restore();

                // Clear screen and show statistics
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME;

                auto total_time = std::chrono::steady_clock::now() - start_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback finished: " << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return true;
            }
        };

        /**
         * @brief Play a video file in the terminal
         * @param filename Path to video file
         * @param width Terminal width in characters
         * @param threshold Brightness threshold (0-255)
         */
        inline void play_video(const std::string &filename, int width = 80, int threshold = 128)
        {
            VideoPlayer player(filename, width, threshold);
            player.play();
        }

        /**
         * @brief Print video info
         */
        inline void print_video_info(const std::string &filename)
        {
            VideoPlayer player(filename);
            auto [w, h, fps, duration] = player.get_info();
            std::cout << "Video: " << filename << "\n"
                      << "  Resolution: " << w << "x" << h << "\n"
                      << "  FPS: " << fps << "\n"
                      << "  Duration: " << duration << " seconds\n";
        }

        /**
         * @brief Extended print function that handles images and videos
         *
         * Detects file type by extension and renders appropriately:
         * - Images: render as static braille graphics
         * - Videos: play with real-time braille rendering
         */
        inline void print_media(const std::string &filename, int max_width = 80, int threshold = 128)
        {
            if (is_video_file(filename))
            {
                play_video(filename, max_width, threshold);
            }
            else if (is_image_file(filename))
            {
                print_image(filename, max_width, threshold);
            }
            else
            {
                // Not a media file, just print as text
                std::cout << filename << std::endl;
            }
        }

    } // namespace draw
} // namespace pythonic
