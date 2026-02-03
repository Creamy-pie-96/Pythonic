/**
 * @file pythonicExport.hpp
 * @brief High-quality export functions for ASCII/Braille art to image and video formats
 *
 * This module provides proper rendering of Braille and ASCII art to image files,
 * handling the 2×4 dot pattern of Braille characters and ANSI color codes.
 *
 * Braille dot layout (Unicode 0x2800 + pattern):
 *   Col 0   Col 1
 *   [1]     [4]     Row 0  (bits 0, 3)
 *   [2]     [5]     Row 1  (bits 1, 4)
 *   [3]     [6]     Row 2  (bits 2, 5)
 *   [7]     [8]     Row 3  (bits 6, 7)
 *
 * Example:
 *   export::braille_to_png("⣿⢸⣸", 8, "output.png");  // Renders braille to actual dots
 */

#ifndef PYTHONIC_EXPORT_HPP
#define PYTHONIC_EXPORT_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <regex>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

namespace pythonic
{
    namespace ex
    {
        // ==================== Constants ====================

        // Braille Unicode range starts at 0x2800
        constexpr char32_t BRAILLE_BASE = 0x2800;

        // Braille dot bit positions (for a 2×4 grid in Unicode encoding)
        // Dot positions in the 2×4 grid:
        //   [0] [3]
        //   [1] [4]
        //   [2] [5]
        //   [6] [7]
        constexpr int BRAILLE_DOT_X[8] = {0, 0, 0, 1, 1, 1, 0, 1};
        constexpr int BRAILLE_DOT_Y[8] = {0, 1, 2, 0, 1, 2, 3, 3};

        // Default rendering parameters
        constexpr int DEFAULT_DOT_RADIUS = 2;   // Pixels per dot radius
        constexpr int DEFAULT_DOT_SPACING = 2;  // Pixels between dot centers
        constexpr int DEFAULT_CELL_PADDING = 1; // Pixels padding around cell

        // ==================== Color Structures ====================

        struct RGB
        {
            uint8_t r, g, b;
            RGB() : r(0), g(0), b(0) {}
            RGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
        };

        struct RGBA
        {
            uint8_t r, g, b, a;
            RGBA() : r(0), g(0), b(0), a(255) {}
            RGBA(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}
        };

        // ==================== ANSI Color Parsing ====================

        /**
         * @brief Parse ANSI 24-bit color escape sequence
         * @param ansi ANSI escape sequence like "\033[38;2;R;G;Bm"
         * @param r Output red component
         * @param g Output green component
         * @param b Output blue component
         * @return true if parsed successfully
         */
        inline bool parse_ansi_rgb(const std::string &ansi, uint8_t &r, uint8_t &g, uint8_t &b)
        {
            // Match ESC[38;2;R;G;Bm (foreground) or ESC[48;2;R;G;Bm (background)
            // Use \x1b for ESC character (0x1B = 27 decimal)
            std::regex rgb_regex(R"(\x1b\[([34])8;2;(\d+);(\d+);(\d+)m)");
            std::smatch match;
            if (std::regex_search(ansi, match, rgb_regex))
            {
                r = static_cast<uint8_t>(std::stoi(match[2].str()));
                g = static_cast<uint8_t>(std::stoi(match[3].str()));
                b = static_cast<uint8_t>(std::stoi(match[4].str()));
                return true;
            }
            return false;
        }

        /**
         * @brief Parse ANSI 256-color escape sequence
         * @param ansi ANSI escape sequence like "\033[38;5;XXXm" or combined "\033[38;5;FG;48;5;BGm"
         * @param r Output red component
         * @param g Output green component
         * @param b Output blue component
         * @param is_foreground Output: true if this is a foreground color
         * @return true if parsed successfully
         */
        inline bool parse_ansi_256(const std::string &ansi, uint8_t &r, uint8_t &g, uint8_t &b, bool &is_foreground)
        {
            // Convert color index to RGB
            auto color_to_rgb = [](int color_index, uint8_t &r, uint8_t &g, uint8_t &b) -> bool
            {
                // Standard colors (0-15): Use basic color mapping
                if (color_index < 16)
                {
                    static const uint8_t basic_colors[16][3] = {
                        {0, 0, 0},       // 0 Black
                        {128, 0, 0},     // 1 Red
                        {0, 128, 0},     // 2 Green
                        {128, 128, 0},   // 3 Yellow
                        {0, 0, 128},     // 4 Blue
                        {128, 0, 128},   // 5 Magenta
                        {0, 128, 128},   // 6 Cyan
                        {192, 192, 192}, // 7 White
                        {128, 128, 128}, // 8 Bright Black
                        {255, 0, 0},     // 9 Bright Red
                        {0, 255, 0},     // 10 Bright Green
                        {255, 255, 0},   // 11 Bright Yellow
                        {0, 0, 255},     // 12 Bright Blue
                        {255, 0, 255},   // 13 Bright Magenta
                        {0, 255, 255},   // 14 Bright Cyan
                        {255, 255, 255}  // 15 Bright White
                    };
                    r = basic_colors[color_index][0];
                    g = basic_colors[color_index][1];
                    b = basic_colors[color_index][2];
                    return true;
                }
                // Color cube (16-231): 6x6x6 RGB
                else if (color_index < 232)
                {
                    int idx = color_index - 16;
                    int ri = idx / 36;
                    int gi = (idx % 36) / 6;
                    int bi = idx % 6;
                    r = ri ? (ri * 40 + 55) : 0;
                    g = gi ? (gi * 40 + 55) : 0;
                    b = bi ? (bi * 40 + 55) : 0;
                    return true;
                }
                // Grayscale (232-255): 24 shades
                else if (color_index < 256)
                {
                    int gray = (color_index - 232) * 10 + 8;
                    r = g = b = static_cast<uint8_t>(gray);
                    return true;
                }
                return false;
            };

            // Match ESC[38;5;Nm (foreground only)
            std::regex fg_only_regex(R"(\x1b\[38;5;(\d+)m)");
            std::smatch match;
            if (std::regex_search(ansi, match, fg_only_regex) && match.prefix().str().empty())
            {
                int color_index = std::stoi(match[1].str());
                is_foreground = true;
                return color_to_rgb(color_index, r, g, b);
            }

            // Match ESC[48;5;Nm (background only)
            std::regex bg_only_regex(R"(\x1b\[48;5;(\d+)m)");
            if (std::regex_search(ansi, match, bg_only_regex) && match.prefix().str().empty())
            {
                int color_index = std::stoi(match[1].str());
                is_foreground = false;
                return color_to_rgb(color_index, r, g, b);
            }

            return false;
        }

        /**
         * @brief Parse combined ANSI 256-color escape sequence with both fg and bg
         * @param ansi ANSI escape sequence like "\033[38;5;FG;48;5;BGm"
         * @param fg_r, fg_g, fg_b Output foreground RGB
         * @param bg_r, bg_g, bg_b Output background RGB
         * @param has_fg, has_bg Output: which colors were found
         * @return true if parsed successfully
         */
        inline bool parse_ansi_256_combined(const std::string &ansi,
                                            uint8_t &fg_r, uint8_t &fg_g, uint8_t &fg_b,
                                            uint8_t &bg_r, uint8_t &bg_g, uint8_t &bg_b,
                                            bool &has_fg, bool &has_bg)
        {
            has_fg = has_bg = false;

            // Convert color index to RGB
            auto color_to_rgb = [](int color_index, uint8_t &r, uint8_t &g, uint8_t &b) -> bool
            {
                if (color_index < 16)
                {
                    static const uint8_t basic_colors[16][3] = {
                        {0, 0, 0}, {128, 0, 0}, {0, 128, 0}, {128, 128, 0}, {0, 0, 128}, {128, 0, 128}, {0, 128, 128}, {192, 192, 192}, {128, 128, 128}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0}, {0, 0, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
                    r = basic_colors[color_index][0];
                    g = basic_colors[color_index][1];
                    b = basic_colors[color_index][2];
                    return true;
                }
                else if (color_index < 232)
                {
                    int idx = color_index - 16;
                    r = (idx / 36) ? ((idx / 36) * 40 + 55) : 0;
                    g = ((idx % 36) / 6) ? (((idx % 36) / 6) * 40 + 55) : 0;
                    b = (idx % 6) ? ((idx % 6) * 40 + 55) : 0;
                    return true;
                }
                else if (color_index < 256)
                {
                    int gray = (color_index - 232) * 10 + 8;
                    r = g = b = static_cast<uint8_t>(gray);
                    return true;
                }
                return false;
            };

            // Match combined fg+bg: ESC[38;5;FG;48;5;BGm
            std::regex combined_regex(R"(\x1b\[38;5;(\d+);48;5;(\d+)m)");
            std::smatch match;
            if (std::regex_search(ansi, match, combined_regex))
            {
                int fg_idx = std::stoi(match[1].str());
                int bg_idx = std::stoi(match[2].str());
                has_fg = color_to_rgb(fg_idx, fg_r, fg_g, fg_b);
                has_bg = color_to_rgb(bg_idx, bg_r, bg_g, bg_b);
                return has_fg || has_bg;
            }

            // Also match reverse order: ESC[48;5;BG;38;5;FGm
            std::regex combined_rev_regex(R"(\x1b\[48;5;(\d+);38;5;(\d+)m)");
            if (std::regex_search(ansi, match, combined_rev_regex))
            {
                int bg_idx = std::stoi(match[1].str());
                int fg_idx = std::stoi(match[2].str());
                has_fg = color_to_rgb(fg_idx, fg_r, fg_g, fg_b);
                has_bg = color_to_rgb(bg_idx, bg_r, bg_g, bg_b);
                return has_fg || has_bg;
            }

            return false;
        }

        /**
         * @brief Strip all ANSI escape codes from a string
         */
        inline std::string strip_ansi(const std::string &input)
        {
            std::regex ansi_regex(R"(\x1b\[[0-9;]*m)");
            return std::regex_replace(input, ansi_regex, "");
        }

        // ==================== UTF-8 Decoding ====================

        /**
         * @brief Decode UTF-8 character to Unicode codepoint
         * @param str UTF-8 string
         * @param pos Position in string (updated to next character)
         * @return Unicode codepoint
         */
        inline char32_t decode_utf8(const std::string &str, size_t &pos)
        {
            if (pos >= str.size())
                return 0;

            unsigned char c = static_cast<unsigned char>(str[pos]);

            // ASCII
            if ((c & 0x80) == 0)
            {
                pos++;
                return c;
            }

            // 2-byte UTF-8
            if ((c & 0xE0) == 0xC0 && pos + 1 < str.size())
            {
                char32_t cp = (c & 0x1F) << 6;
                cp |= (static_cast<unsigned char>(str[pos + 1]) & 0x3F);
                pos += 2;
                return cp;
            }

            // 3-byte UTF-8 (Braille is here: U+2800–U+28FF)
            if ((c & 0xF0) == 0xE0 && pos + 2 < str.size())
            {
                char32_t cp = (c & 0x0F) << 12;
                cp |= (static_cast<unsigned char>(str[pos + 1]) & 0x3F) << 6;
                cp |= (static_cast<unsigned char>(str[pos + 2]) & 0x3F);
                pos += 3;
                return cp;
            }

            // 4-byte UTF-8
            if ((c & 0xF8) == 0xF0 && pos + 3 < str.size())
            {
                char32_t cp = (c & 0x07) << 18;
                cp |= (static_cast<unsigned char>(str[pos + 1]) & 0x3F) << 12;
                cp |= (static_cast<unsigned char>(str[pos + 2]) & 0x3F) << 6;
                cp |= (static_cast<unsigned char>(str[pos + 3]) & 0x3F);
                pos += 4;
                return cp;
            }

            // Invalid, skip
            pos++;
            return 0xFFFD; // Replacement character
        }

        /**
         * @brief Check if codepoint is a Braille pattern character
         */
        inline bool is_braille(char32_t cp)
        {
            return cp >= BRAILLE_BASE && cp < BRAILLE_BASE + 256;
        }

        /**
         * @brief Check if codepoint is a block character (half blocks, full blocks, etc.)
         */
        inline bool is_block_char(char32_t cp)
        {
            // Half blocks, quarter blocks, etc.: U+2580–U+259F
            // Full block: U+2588
            return (cp >= 0x2580 && cp <= 0x259F);
        }

        // ==================== Image Buffer ====================

        /**
         * @brief Simple image buffer for rendering
         */
        class ImageBuffer
        {
        public:
            int width, height;
            std::vector<RGBA> pixels;

            ImageBuffer() : width(0), height(0) {}

            ImageBuffer(int w, int h, RGBA fill = RGBA(0, 0, 0, 255))
                : width(w), height(h), pixels(w * h, fill) {}

            void resize(int w, int h, RGBA fill = RGBA(0, 0, 0, 255))
            {
                width = w;
                height = h;
                pixels.assign(w * h, fill);
            }

            RGBA &at(int x, int y)
            {
                return pixels[y * width + x];
            }

            const RGBA &at(int x, int y) const
            {
                return pixels[y * width + x];
            }

            void set_pixel(int x, int y, const RGBA &color)
            {
                if (x >= 0 && x < width && y >= 0 && y < height)
                {
                    pixels[y * width + x] = color;
                }
            }

            /**
             * @brief Draw a filled circle (anti-aliased)
             */
            void fill_circle(int cx, int cy, int radius, const RGBA &color)
            {
                int r2 = radius * radius;
                for (int dy = -radius; dy <= radius; dy++)
                {
                    for (int dx = -radius; dx <= radius; dx++)
                    {
                        int dist2 = dx * dx + dy * dy;
                        if (dist2 <= r2)
                        {
                            set_pixel(cx + dx, cy + dy, color);
                        }
                    }
                }
            }

            /**
             * @brief Fill a rectangle
             */
            void fill_rect(int x1, int y1, int x2, int y2, const RGBA &color)
            {
                for (int y = y1; y <= y2; y++)
                {
                    for (int x = x1; x <= x2; x++)
                    {
                        set_pixel(x, y, color);
                    }
                }
            }
        };

        // ==================== Fast Direct Rendering for Half-Block ====================

        /**
         * @brief Render grayscale half-block pixel data directly to ImageBuffer
         *
         * This bypasses ANSI string generation/parsing for much faster export.
         * Each input cell has (top_gray, bottom_gray) values.
         *
         * @param pixels Vector of (top_gray, bottom_gray) pairs, row-major by cell
         * @param char_width Number of character cells wide
         * @param char_height Number of character cells tall
         * @param pixel_size Size of each half-block pixel in output image
         * @return ImageBuffer with rendered grayscale image
         */
        inline ImageBuffer render_half_block_direct(
            const std::vector<std::vector<std::pair<uint8_t, uint8_t>>> &pixels,
            size_t char_width, size_t char_height, int pixel_size = 2)
        {
            int img_width = static_cast<int>(char_width * pixel_size);
            int img_height = static_cast<int>(char_height * pixel_size * 2); // 2 pixels per cell

            ImageBuffer img(img_width, img_height, RGBA(0, 0, 0, 255));

            for (size_t cy = 0; cy < char_height && cy < pixels.size(); ++cy)
            {
                for (size_t cx = 0; cx < char_width && cx < pixels[cy].size(); ++cx)
                {
                    auto [gray_top, gray_bot] = pixels[cy][cx];

                    int x = static_cast<int>(cx * pixel_size);
                    int y_top = static_cast<int>(cy * pixel_size * 2);
                    int y_bot = y_top + pixel_size;

                    // Fill top half
                    for (int py = y_top; py < y_top + pixel_size && py < img_height; ++py)
                    {
                        for (int px = x; px < x + pixel_size && px < img_width; ++px)
                        {
                            img.set_pixel(px, py, RGBA(gray_top, gray_top, gray_top, 255));
                        }
                    }

                    // Fill bottom half
                    for (int py = y_bot; py < y_bot + pixel_size && py < img_height; ++py)
                    {
                        for (int px = x; px < x + pixel_size && px < img_width; ++px)
                        {
                            img.set_pixel(px, py, RGBA(gray_bot, gray_bot, gray_bot, 255));
                        }
                    }
                }
            }

            return img;
        }

        // ==================== PPM Writer (Simple, No Dependencies) ====================

        /**
         * @brief Write image buffer to PPM file (P6 binary format)
         */
        inline bool write_ppm(const ImageBuffer &img, const std::string &filename)
        {
            std::ofstream out(filename, std::ios::binary);
            if (!out)
                return false;

            out << "P6\n"
                << img.width << " " << img.height << "\n255\n";

            for (const auto &pixel : img.pixels)
            {
                out.put(static_cast<char>(pixel.r));
                out.put(static_cast<char>(pixel.g));
                out.put(static_cast<char>(pixel.b));
            }

            return out.good();
        }

        /**
         * @brief Write image buffer to PNG file using ImageMagick conversion
         */
        inline bool write_png(const ImageBuffer &img, const std::string &filename)
        {
            // Extract the directory from the output filename and use it for temp PPM
            // This ensures temp file is on the same filesystem as output
            std::string temp_dir;
            size_t last_slash = filename.rfind('/');
            if (last_slash != std::string::npos)
            {
                temp_dir = filename.substr(0, last_slash);
            }
            else
            {
#ifdef _WIN32
                last_slash = filename.rfind('\\');
                if (last_slash != std::string::npos)
                    temp_dir = filename.substr(0, last_slash);
                else
                    temp_dir = ".";
#else
                temp_dir = ".";
#endif
            }

            // Generate unique temp filename using thread id and timestamp
            std::hash<std::thread::id> hasher;
            auto thread_hash = hasher(std::this_thread::get_id());
            auto time_hash = std::chrono::steady_clock::now().time_since_epoch().count();

            std::string temp_ppm = temp_dir + "/pythonic_temp_" +
                                   std::to_string(thread_hash) + "_" +
                                   std::to_string(time_hash) + ".ppm";

            if (!write_ppm(img, temp_ppm))
                return false;

            // Convert to PNG using ImageMagick
            std::string cmd = "convert \"" + temp_ppm + "\" \"" + filename + "\" 2>/dev/null";
            int result = std::system(cmd.c_str());

            // Cleanup
            std::remove(temp_ppm.c_str());

            return (result == 0);
        }

        // ==================== Braille Rendering ====================

        /**
         * @brief Parsed character with position and color information
         */
        struct ParsedChar
        {
            char32_t codepoint;
            RGB fg_color;
            RGB bg_color;
            bool has_fg_color;
            bool has_bg_color;

            ParsedChar()
                : codepoint(0), fg_color(255, 255, 255), bg_color(0, 0, 0),
                  has_fg_color(false), has_bg_color(false) {}
        };

        /**
         * @brief Parse a line of ASCII/Braille art with ANSI colors
         * @param line Input line with ANSI escape codes
         * @return Vector of parsed characters with colors
         */
        inline std::vector<ParsedChar> parse_line(const std::string &line)
        {
            std::vector<ParsedChar> result;

            RGB current_fg(255, 255, 255);
            RGB current_bg(0, 0, 0);
            bool has_fg = false, has_bg = false;

            size_t i = 0;
            while (i < line.size())
            {
                // Check for ANSI escape sequence
                if (i + 1 < line.size() && line[i] == '\033' && line[i + 1] == '[')
                {
                    // Find the end of the escape sequence
                    size_t start = i;
                    i += 2;
                    while (i < line.size() && line[i] != 'm')
                        i++;
                    if (i < line.size())
                        i++; // Skip 'm'

                    std::string escape = line.substr(start, i - start);

                    // Try parsing 24-bit RGB color first
                    uint8_t r, g, b;
                    if (parse_ansi_rgb(escape, r, g, b))
                    {
                        if (escape.find("[38;2;") != std::string::npos)
                        {
                            current_fg = RGB(r, g, b);
                            has_fg = true;
                        }
                        else if (escape.find("[48;2;") != std::string::npos)
                        {
                            current_bg = RGB(r, g, b);
                            has_bg = true;
                        }
                    }
                    // Try parsing combined 256-color (fg+bg in one escape)
                    else
                    {
                        uint8_t fg_r, fg_g, fg_b, bg_r, bg_g, bg_b;
                        bool got_fg, got_bg;
                        if (parse_ansi_256_combined(escape, fg_r, fg_g, fg_b, bg_r, bg_g, bg_b, got_fg, got_bg))
                        {
                            if (got_fg)
                            {
                                current_fg = RGB(fg_r, fg_g, fg_b);
                                has_fg = true;
                            }
                            if (got_bg)
                            {
                                current_bg = RGB(bg_r, bg_g, bg_b);
                                has_bg = true;
                            }
                        }
                        // Try parsing single 256-color
                        else
                        {
                            bool is_foreground;
                            if (parse_ansi_256(escape, r, g, b, is_foreground))
                            {
                                if (is_foreground)
                                {
                                    current_fg = RGB(r, g, b);
                                    has_fg = true;
                                }
                                else
                                {
                                    current_bg = RGB(r, g, b);
                                    has_bg = true;
                                }
                            }
                            // Reset code
                            else if (escape.find("[0m") != std::string::npos ||
                                     escape.find("[m") != std::string::npos)
                            {
                                current_fg = RGB(255, 255, 255);
                                current_bg = RGB(0, 0, 0);
                                has_fg = false;
                                has_bg = false;
                            }
                        }
                    }
                    continue;
                }

                // Decode UTF-8 character
                char32_t cp = decode_utf8(line, i);
                if (cp == 0 || cp == '\n' || cp == '\r')
                    continue;

                ParsedChar pc;
                pc.codepoint = cp;
                pc.fg_color = current_fg;
                pc.bg_color = current_bg;
                pc.has_fg_color = has_fg;
                pc.has_bg_color = has_bg;
                result.push_back(pc);
            }

            return result;
        }

        /**
         * @brief Render a single Braille character to image buffer
         * @param img Image buffer to render to
         * @param x Base X position (top-left of cell)
         * @param y Base Y position (top-left of cell)
         * @param pattern Braille pattern (0x00-0xFF)
         * @param fg Foreground (dot) color
         * @param bg Background color
         * @param dot_radius Radius of each dot in pixels
         * @param cell_width Width of the braille cell in pixels
         * @param cell_height Height of the braille cell in pixels
         */
        inline void render_braille_char(ImageBuffer &img, int x, int y, uint8_t pattern,
                                        const RGB &fg, const RGB &bg,
                                        int dot_radius, int cell_width, int cell_height)
        {
            // Fill background
            RGBA bg_color(bg.r, bg.g, bg.b, 255);
            img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1, bg_color);

            // Calculate dot spacing
            int dot_spacing_x = cell_width / 2;
            int dot_spacing_y = cell_height / 4;

            // Render each dot
            RGBA dot_color(fg.r, fg.g, fg.b, 255);
            for (int bit = 0; bit < 8; bit++)
            {
                if (pattern & (1 << bit))
                {
                    int dot_x = BRAILLE_DOT_X[bit];
                    int dot_y = BRAILLE_DOT_Y[bit];

                    int px = x + dot_spacing_x / 2 + dot_x * dot_spacing_x;
                    int py = y + dot_spacing_y / 2 + dot_y * dot_spacing_y;

                    img.fill_circle(px, py, dot_radius, dot_color);
                }
            }
        }

        /**
         * @brief Render a block character (▀, ▄, █, etc.) to image buffer
         */
        inline void render_block_char(ImageBuffer &img, int x, int y, char32_t cp,
                                      const RGB &fg, const RGB &bg,
                                      int cell_width, int cell_height)
        {
            RGBA fg_color(fg.r, fg.g, fg.b, 255);
            RGBA bg_color(bg.r, bg.g, bg.b, 255);

            // Fill with background first
            img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1, bg_color);

            int half = cell_height / 2;

            switch (cp)
            {
            case 0x2588: // █ Full block
                img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1, fg_color);
                break;
            case 0x2580: // ▀ Upper half
                img.fill_rect(x, y, x + cell_width - 1, y + half - 1, fg_color);
                break;
            case 0x2584: // ▄ Lower half
                img.fill_rect(x, y + half, x + cell_width - 1, y + cell_height - 1, fg_color);
                break;
            case 0x258C: // ▌ Left half
                img.fill_rect(x, y, x + cell_width / 2 - 1, y + cell_height - 1, fg_color);
                break;
            case 0x2590: // ▐ Right half
                img.fill_rect(x + cell_width / 2, y, x + cell_width - 1, y + cell_height - 1, fg_color);
                break;
            case 0x2591: // ░ Light shade (25%)
            case 0x2592: // ▒ Medium shade (50%)
            case 0x2593: // ▓ Dark shade (75%)
            {
                // Simple dithering based on shade level
                int shade = (cp == 0x2591) ? 64 : ((cp == 0x2592) ? 128 : 192);
                RGBA blended((fg.r * shade + bg.r * (255 - shade)) / 255,
                             (fg.g * shade + bg.g * (255 - shade)) / 255,
                             (fg.b * shade + bg.b * (255 - shade)) / 255, 255);
                img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1, blended);
                break;
            }
            default:
                // Unknown block, just fill with fg
                img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1, fg_color);
                break;
            }
        }

        // ==================== Export Configuration ====================

        /**
         * @brief Configuration options for ASCII/Braille export
         */
        struct ExportConfig
        {
            int dot_size = 2;                    // Dot radius in pixels
            int dot_density = 3;                 // Spacing multiplier (higher = more spaced out)
            RGB bg_color = RGB(0, 0, 0);         // Background color (where no dots)
            RGB default_fg = RGB(255, 255, 255); // Default foreground (dot) color when no ANSI
            bool preserve_colors = true;         // Use ANSI colors if present

            ExportConfig() = default;

            // Builder pattern methods for easy configuration
            ExportConfig &set_dot_size(int size)
            {
                dot_size = size;
                return *this;
            }

            ExportConfig &set_density(int d)
            {
                dot_density = d;
                return *this;
            }

            ExportConfig &set_background(uint8_t r, uint8_t g, uint8_t b)
            {
                bg_color = RGB(r, g, b);
                return *this;
            }

            ExportConfig &set_foreground(uint8_t r, uint8_t g, uint8_t b)
            {
                default_fg = RGB(r, g, b);
                return *this;
            }

            ExportConfig &set_preserve_colors(bool p)
            {
                preserve_colors = p;
                return *this;
            }
        };

        // ==================== Main Export Functions ====================

        /**
         * @brief Render Braille/ASCII art string to an image
         * @param content ASCII/Braille art with optional ANSI colors
         * @param config Export configuration options
         * @return ImageBuffer with rendered art
         */
        inline ImageBuffer render_art_to_image(const std::string &content, const ExportConfig &config)
        {
            // Parse into lines
            std::vector<std::string> lines;
            std::istringstream ss(content);
            std::string line;
            while (std::getline(ss, line))
            {
                lines.push_back(line);
            }

            if (lines.empty())
                return ImageBuffer();

            // Parse all lines to determine dimensions and content
            std::vector<std::vector<ParsedChar>> parsed_lines;
            size_t max_chars = 0;

            for (const auto &l : lines)
            {
                auto parsed = parse_line(l);
                max_chars = std::max(max_chars, parsed.size());
                parsed_lines.push_back(parsed);
            }

            if (max_chars == 0)
                return ImageBuffer();

            // Detect content type - braille vs block
            bool has_braille = false;
            bool has_blocks = false;
            for (const auto &pl : parsed_lines)
            {
                for (const auto &pc : pl)
                {
                    if (is_braille(pc.codepoint))
                        has_braille = true;
                    if (is_block_char(pc.codepoint))
                        has_blocks = true;
                }
            }

            // Determine cell sizes based on content type
            int cell_width, cell_height;
            if (has_braille)
            {
                // Braille: 2 dots wide × 4 dots tall
                cell_width = config.dot_size * config.dot_density * 2;
                cell_height = config.dot_size * config.dot_density * 4;
            }
            else if (has_blocks)
            {
                // Block: 1 char wide × 2 pixels tall (half-block = 2 rows per char)
                int pixel_size = config.dot_size * config.dot_density;
                cell_width = pixel_size;
                cell_height = pixel_size * 2; // Upper half + lower half
            }
            else
            {
                // Default: square cells
                cell_width = config.dot_size * config.dot_density * 2;
                cell_height = config.dot_size * config.dot_density * 2;
            }

            // Create image
            int img_width = static_cast<int>(max_chars) * cell_width;
            int img_height = static_cast<int>(parsed_lines.size()) * cell_height;

            ImageBuffer img(img_width, img_height, RGBA(config.bg_color.r, config.bg_color.g, config.bg_color.b, 255));

            // Render each character
            for (size_t row = 0; row < parsed_lines.size(); row++)
            {
                const auto &chars = parsed_lines[row];
                for (size_t col = 0; col < chars.size(); col++)
                {
                    const ParsedChar &pc = chars[col];
                    int x = static_cast<int>(col) * cell_width;
                    int y = static_cast<int>(row) * cell_height;

                    // Determine colors
                    RGB fg = config.default_fg;
                    RGB bg = config.bg_color;
                    if (config.preserve_colors && pc.has_fg_color)
                        fg = pc.fg_color;
                    if (config.preserve_colors && pc.has_bg_color)
                        bg = pc.bg_color;

                    if (is_braille(pc.codepoint))
                    {
                        uint8_t pattern = static_cast<uint8_t>(pc.codepoint - BRAILLE_BASE);
                        render_braille_char(img, x, y, pattern, fg, bg, config.dot_size, cell_width, cell_height);
                    }
                    else if (is_block_char(pc.codepoint))
                    {
                        render_block_char(img, x, y, pc.codepoint, fg, bg, cell_width, cell_height);
                    }
                    else if (pc.codepoint == ' ')
                    {
                        // Space - just background
                        img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1,
                                      RGBA(bg.r, bg.g, bg.b, 255));
                    }
                    else
                    {
                        // Other character - render as filled block with color
                        img.fill_rect(x, y, x + cell_width - 1, y + cell_height - 1,
                                      RGBA(fg.r, fg.g, fg.b, 255));
                    }
                }
            }

            return img;
        }

        /**
         * @brief Render Braille/ASCII art string to an image (simple overload)
         * @param content ASCII/Braille art with optional ANSI colors
         * @param dot_size Size of each braille dot in pixels (default 2)
         * @param bg_color Background color (default black)
         * @return ImageBuffer with rendered art
         */
        inline ImageBuffer render_art_to_image(const std::string &content,
                                               int dot_size = 2,
                                               RGB bg_color = RGB(0, 0, 0))
        {
            ExportConfig config;
            config.dot_size = dot_size;
            config.bg_color = bg_color;
            return render_art_to_image(content, config);
        }

        /**
         * @brief Export ASCII/Braille art to PNG file with full config
         * @param content ASCII/Braille art with optional ANSI colors
         * @param filename Output PNG filename
         * @param config Export configuration
         * @return true on success
         */
        inline bool export_art_to_png(const std::string &content, const std::string &filename,
                                      const ExportConfig &config)
        {
            ImageBuffer img = render_art_to_image(content, config);
            if (img.width == 0 || img.height == 0)
                return false;

            return write_png(img, filename);
        }

        /**
         * @brief Export ASCII/Braille art to PNG file (simple overload)
         * @param content ASCII/Braille art with optional ANSI colors
         * @param filename Output PNG filename
         * @param dot_size Size of each braille dot in pixels
         * @param bg_color Background color
         * @return true on success
         */
        inline bool export_art_to_png(const std::string &content, const std::string &filename,
                                      int dot_size, RGB bg_color = RGB(0, 0, 0))
        {
            ExportConfig config;
            config.dot_size = dot_size;
            config.bg_color = bg_color;
            return export_art_to_png(content, filename, config);
        }

        /**
         * @brief Export ASCII/Braille art to PPM file with full config
         * @param content ASCII/Braille art with optional ANSI colors
         * @param filename Output PPM filename
         * @param config Export configuration
         * @return true on success
         */
        inline bool export_art_to_ppm(const std::string &content, const std::string &filename,
                                      const ExportConfig &config)
        {
            ImageBuffer img = render_art_to_image(content, config);
            if (img.width == 0 || img.height == 0)
                return false;

            return write_ppm(img, filename);
        }

        /**
         * @brief Export ASCII/Braille art to PPM file (simple overload)
         * @param content ASCII/Braille art with optional ANSI colors
         * @param filename Output PPM filename
         * @param dot_size Size of each braille dot in pixels
         * @param bg_color Background color
         * @return true on success
         */
        inline bool export_art_to_ppm(const std::string &content, const std::string &filename,
                                      int dot_size = 2, RGB bg_color = RGB(0, 0, 0))
        {
            ImageBuffer img = render_art_to_image(content, dot_size, bg_color);
            if (img.width == 0 || img.height == 0)
                return false;

            return write_ppm(img, filename);
        }

        // ==================== Video Export ====================

        /**
         * @brief Export ASCII art frames to video file with full config
         *
         * Takes a vector of frame strings (ASCII/Braille art) and creates a video.
         *
         * @param frames Vector of ASCII art strings (one per frame)
         * @param output_path Output video path (e.g., "output.mp4")
         * @param fps Frames per second
         * @param config Export configuration
         * @param audio_path Optional audio file to mux with video
         * @return true on success
         */
        inline bool export_frames_to_video(const std::vector<std::string> &frames,
                                           const std::string &output_path,
                                           int fps, const ExportConfig &config,
                                           const std::string &audio_path = "")
        {
            if (frames.empty())
                return false;

            // Create temp directory for frames
            std::string temp_dir = "/tmp/pythonic_video_export_" +
                                   std::to_string(std::hash<std::string>{}(output_path));
            std::string mkdir_cmd = "mkdir -p \"" + temp_dir + "\"";
            std::system(mkdir_cmd.c_str());

            // Render each frame
            int frame_num = 1;
            for (const auto &frame : frames)
            {
                ImageBuffer img = render_art_to_image(frame, config);
                if (img.width == 0 || img.height == 0)
                    continue;

                char frame_path[256];
                snprintf(frame_path, sizeof(frame_path), "%s/frame_%05d.ppm",
                         temp_dir.c_str(), frame_num);

                write_ppm(img, frame_path);
                frame_num++;
            }

            // Combine into video using FFmpeg
            std::string video_cmd;
            if (!audio_path.empty())
            {
                video_cmd = "ffmpeg -y -framerate " + std::to_string(fps) +
                            " -i \"" + temp_dir + "/frame_%05d.ppm\" " +
                            "-i \"" + audio_path + "\" " +
                            "-c:v libx264 -c:a aac -pix_fmt yuv420p -shortest \"" +
                            output_path + "\" 2>/dev/null";
            }
            else
            {
                video_cmd = "ffmpeg -y -framerate " + std::to_string(fps) +
                            " -i \"" + temp_dir + "/frame_%05d.ppm\" " +
                            "-c:v libx264 -pix_fmt yuv420p \"" +
                            output_path + "\" 2>/dev/null";
            }

            int result = std::system(video_cmd.c_str());

            // Cleanup
            std::string rm_cmd = "rm -rf \"" + temp_dir + "\"";
            std::system(rm_cmd.c_str());

            return (result == 0);
        }

        /**
         * @brief Export ASCII art frames to video file (simple overload)
         *
         * @param frames Vector of ASCII art strings (one per frame)
         * @param output_path Output video path (e.g., "output.mp4")
         * @param fps Frames per second (default 24)
         * @param dot_size Size of braille dots (default 2)
         * @param bg_color Background color (default black)
         * @param audio_path Optional audio file to mux with video
         * @return true on success
         */
        inline bool export_frames_to_video(const std::vector<std::string> &frames,
                                           const std::string &output_path,
                                           int fps = 24, int dot_size = 2,
                                           RGB bg_color = RGB(0, 0, 0),
                                           const std::string &audio_path = "")
        {
            ExportConfig config;
            config.dot_size = dot_size;
            config.bg_color = bg_color;
            return export_frames_to_video(frames, output_path, fps, config, audio_path);
        }

    } // namespace ex
} // namespace pythonic

#endif // PYTHONIC_EXPORT_HPP