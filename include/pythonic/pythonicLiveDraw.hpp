#pragma once
/**
 * @file pythonicLiveDraw.hpp
 * @brief Interactive terminal drawing with mouse support
 *
 * This module provides a live drawing canvas that captures mouse and keyboard
 * inputs to let users "paint" in the terminal using Braille characters.
 * Drawings can be saved to Pythonic's .pi image format.
 *
 * Features:
 * - Mouse tracking with ANSI escape sequences (\e[?1003h, \e[?1006h)
 * - Sub-pixel precision using Braille's 2×4 grid (8x resolution)
 * - RGB color selection with keyboard controls
 * - Multiple drawing tools (pen, line, circle, rectangle, fill)
 * - Undo/Redo support
 * - Save to .pi format with RLE compression
 * - Alpha blending for overlapping colors
 *
 * Mouse tracking:
 * - Uses \e[?1003h (any-event tracking) and \e[?1006h (SGR extended)
 * - Works on modern terminals: xterm, iTerm2, Windows Terminal, VS Code
 *
 * Keyboard controls:
 * - r/g/b: Toggle RGB channel selection
 * - 0-9: Set selected channel value (press twice for two digits)
 * - n: Select none (finish color input)
 * - p: Pen tool (freehand)
 * - l: Line tool
 * - c: Circle tool
 * - x: Rectangle tool
 * - f: Fill tool
 * - u: Undo
 * - y: Redo
 * - s: Save to file
 * - q: Quit
 *
 * Example usage:
 *   // Simple entry point
 *   pythonic::draw::live_draw();  // Opens interactive canvas
 *
 *   // With options
 *   pythonic::draw::live_draw(80, 40, "my_drawing.pi");
 */

#include <string>
#include <vector>
#include <stack>
#include <functional>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <memory>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#endif

#include "pythonicDraw.hpp"
#include "pythonicMedia.hpp"

namespace pythonic
{
    namespace draw
    {

        // ==================== RGBA Color with Alpha Blending ====================

        /**
         * @brief RGBA color with alpha channel for blending
         */
        struct RGBA
        {
            uint8_t r, g, b, a;

            RGBA() : r(255), g(255), b(255), a(255) {}
            RGBA(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
                : r(r_), g(g_), b(b_), a(a_) {}

            explicit RGBA(const RGB &rgb, uint8_t alpha = 255)
                : r(rgb.r), g(rgb.g), b(rgb.b), a(alpha) {}

            RGB to_rgb() const { return RGB(r, g, b); }

            bool operator==(const RGBA &other) const
            {
                return r == other.r && g == other.g && b == other.b && a == other.a;
            }

            bool operator!=(const RGBA &other) const { return !(*this == other); }

            /**
             * @brief Blend this color over another using alpha compositing
             *
             * Uses "over" operator: result = src * src_alpha + dst * (1 - src_alpha)
             * This is the standard Porter-Duff "over" operation.
             *
             * @param dst Background color
             * @return Blended color
             */
            RGBA blend_over(const RGBA &dst) const
            {
                if (a == 255)
                    return *this;
                if (a == 0)
                    return dst;

                float src_a = a / 255.0f;
                float dst_a = dst.a / 255.0f;
                float out_a = src_a + dst_a * (1.0f - src_a);

                if (out_a < 0.001f)
                    return RGBA(0, 0, 0, 0);

                float out_r = (r * src_a + dst.r * dst_a * (1.0f - src_a)) / out_a;
                float out_g = (g * src_a + dst.g * dst_a * (1.0f - src_a)) / out_a;
                float out_b = (b * src_a + dst.b * dst_a * (1.0f - src_a)) / out_a;

                return RGBA(
                    static_cast<uint8_t>(std::clamp(out_r, 0.0f, 255.0f)),
                    static_cast<uint8_t>(std::clamp(out_g, 0.0f, 255.0f)),
                    static_cast<uint8_t>(std::clamp(out_b, 0.0f, 255.0f)),
                    static_cast<uint8_t>(std::clamp(out_a * 255.0f, 0.0f, 255.0f)));
            }

            /**
             * @brief Linear interpolation between two colors
             */
            static RGBA lerp(const RGBA &a, const RGBA &b, float t)
            {
                t = std::clamp(t, 0.0f, 1.0f);
                return RGBA(
                    static_cast<uint8_t>(a.r + (b.r - a.r) * t),
                    static_cast<uint8_t>(a.g + (b.g - a.g) * t),
                    static_cast<uint8_t>(a.b + (b.b - a.b) * t),
                    static_cast<uint8_t>(a.a + (b.a - a.a) * t));
            }
        };

        // ==================== Drawing Tools ====================

        /**
         * @brief Available drawing tools
         */
        enum class Tool
        {
            pen,       // Freehand drawing
            line,      // Line from point A to B
            circle,    // Circle (center + radius)
            rectangle, // Rectangle
            fill,      // Flood fill
            eraser     // Eraser (sets to background)
        };

        /**
         * @brief Color channel being edited
         */
        enum class ColorChannel
        {
            none,
            red,
            green,
            blue,
            alpha
        };

        // ==================== Mouse Event ====================

        /**
         * @brief Mouse event types
         */
        enum class MouseEventType
        {
            move,
            press,
            release,
            scroll_up,
            scroll_down
        };

        /**
         * @brief Mouse button
         */
        enum class MouseButton
        {
            none,
            left,
            middle,
            right
        };

        /**
         * @brief Mouse event data
         */
        struct MouseEvent
        {
            MouseEventType type;
            MouseButton button;
            int cell_x;  // Terminal cell X
            int cell_y;  // Terminal cell Y
            int sub_x;   // Sub-pixel X within cell (0-1 for Braille)
            int sub_y;   // Sub-pixel Y within cell (0-3 for Braille)
            int pixel_x; // Actual pixel X (cell_x * 2 + sub_x)
            int pixel_y; // Actual pixel Y (cell_y * 4 + sub_y)
            bool shift_held;
            bool ctrl_held;
        };

        // ==================== Undo/Redo History ====================

        /**
         * @brief Canvas state for undo/redo
         */
        struct CanvasState
        {
            std::vector<std::vector<RGBA>> pixels;

            CanvasState() = default;
            CanvasState(const std::vector<std::vector<RGBA>> &p) : pixels(p) {}
        };

        // ==================== Render Mode for LiveDraw ====================

        /**
         * @brief Render mode for the live drawing canvas
         */
        enum class DrawMode
        {
            block,  // Half-block characters (▀) - 1x2 resolution per char
            braille // Braille characters (⠿) - 2x4 resolution per char
        };

        // ==================== Live Canvas ====================

        /**
         * @brief Interactive drawing canvas with mouse and keyboard input
         *
         * The canvas uses Braille or block characters for display and stores full
         * RGBA pixel data internally for color support and alpha blending.
         */
        class LiveCanvas
        {
        private:
            // Canvas dimensions
            size_t _char_width;   // Width in terminal characters
            size_t _char_height;  // Height in terminal characters
            size_t _pixel_width;  // Width in pixels
            size_t _pixel_height; // Height in pixels

            // Draw mode (block or braille)
            DrawMode _draw_mode;

            // Pixel storage (RGBA for full color with alpha)
            std::vector<std::vector<RGBA>> _pixels;

            // Preview layer for shape tools (line, rect, circle)
            std::vector<std::vector<RGBA>> _preview;
            bool _preview_active;

            // Current state
            Tool _current_tool;
            RGBA _foreground;
            RGBA _background;
            ColorChannel _active_channel;
            std::string _input_buffer; // For multi-digit color input (Enter to confirm)
            uint8_t _brush_size;

            // Tool state
            bool _drawing;
            int _start_x, _start_y; // For line/rect/circle tools
            int _last_x, _last_y;   // Last mouse position
            int _mouse_x, _mouse_y; // Current mouse position for brush preview

            // Undo/Redo
            std::stack<CanvasState> _undo_stack;
            std::stack<CanvasState> _redo_stack;
            static constexpr size_t MAX_UNDO = 50;

            // Terminal state
#ifndef _WIN32
            struct termios _old_termios;
            bool _raw_mode;
#endif
            bool _mouse_enabled;
            std::atomic<bool> _running;

            // Output filename
            std::string _output_file;

            // UI dimensions
            static constexpr int UI_PANEL_WIDTH = 20; // Width of side panel
            static constexpr int STATUS_HEIGHT = 2;   // Height of status bar

        public:
            /**
             * @brief Get proper output filename (remove existing extension, ensure .pi)
             */
            static std::string sanitize_output_filename(const std::string &filename)
            {
                std::string result = filename;
                // Keep removing known extensions until none remain
                bool removed = true;
                while (removed)
                {
                    removed = false;
                    size_t dot_pos = result.rfind('.');
                    if (dot_pos != std::string::npos && dot_pos > 0)
                    {
                        std::string ext = result.substr(dot_pos);
                        // Convert to lowercase for comparison
                        std::string lower_ext = ext;
                        for (auto &c : lower_ext)
                            c = std::tolower(c);
                        // Remove if it's a known extension
                        if (lower_ext == ".pi" || lower_ext == ".png" || lower_ext == ".jpg" ||
                            lower_ext == ".jpeg" || lower_ext == ".ppm" || lower_ext == ".bmp")
                        {
                            result = result.substr(0, dot_pos);
                            removed = true;
                        }
                    }
                }
                // Add .pi extension
                return result + ".pi";
            }

            /**
             * @brief Create a live drawing canvas
             * @param char_width Width in terminal characters
             * @param char_height Height in terminal characters
             * @param output_file Default filename for saving (optional)
             * @param mode Draw mode: block (default) or braille
             */
            LiveCanvas(size_t char_width = 60, size_t char_height = 30,
                       const std::string &output_file = "drawing",
                       DrawMode mode = DrawMode::block)
                : _char_width(char_width), _char_height(char_height),
                  _draw_mode(mode),
                  // Block mode: 1x2 pixels per char. Braille mode: 2x4 pixels per char.
                  _pixel_width(mode == DrawMode::braille ? char_width * 2 : char_width),
                  _pixel_height(mode == DrawMode::braille ? char_height * 4 : char_height * 2),
                  _pixels(_pixel_height, std::vector<RGBA>(_pixel_width, RGBA(0, 0, 0, 255))),
                  _preview(_pixel_height, std::vector<RGBA>(_pixel_width, RGBA(0, 0, 0, 0))),
                  _preview_active(false),
                  _current_tool(Tool::pen), _foreground(255, 255, 255, 255), _background(0, 0, 0, 255),
                  _active_channel(ColorChannel::none), _input_buffer(""), _brush_size(1),
                  _drawing(false), _start_x(0), _start_y(0), _last_x(0), _last_y(0),
                  _mouse_x(-1), _mouse_y(-1),
#ifndef _WIN32
                  _raw_mode(false),
#endif
                  _mouse_enabled(false), _running(false),
                  _output_file(sanitize_output_filename(output_file))
            {
            }

            ~LiveCanvas()
            {
                disable_raw_mode();
                disable_mouse_tracking();
            }

            // ==================== Terminal Setup ====================

            /**
             * @brief Enable raw terminal mode for immediate input
             */
            void enable_raw_mode()
            {
#ifndef _WIN32
                if (_raw_mode)
                    return;
                if (!isatty(STDIN_FILENO))
                    return;

                if (tcgetattr(STDIN_FILENO, &_old_termios) == 0)
                {
                    struct termios new_termios = _old_termios;
                    new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
                    new_termios.c_iflag &= ~(IXON | ICRNL);
                    new_termios.c_cc[VMIN] = 0;
                    new_termios.c_cc[VTIME] = 1; // 100ms timeout

                    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == 0)
                    {
                        _raw_mode = true;
                    }
                }
#endif
            }

            /**
             * @brief Restore normal terminal mode
             */
            void disable_raw_mode()
            {
#ifndef _WIN32
                if (_raw_mode)
                {
                    tcsetattr(STDIN_FILENO, TCSANOW, &_old_termios);
                    _raw_mode = false;
                }
#endif
            }

            /**
             * @brief Enable mouse tracking (ANSI sequences)
             */
            void enable_mouse_tracking()
            {
                if (_mouse_enabled)
                    return;

                // Enable various mouse tracking modes:
                // ?1000 - Basic mouse reporting
                // ?1002 - Button-event tracking (report button press/release)
                // ?1003 - Any-event tracking (also report movement)
                // ?1006 - SGR extended mode (for larger terminals, accurate coords)
                std::cout << "\033[?1000h" // Enable mouse reporting
                          << "\033[?1002h" // Enable button-event tracking
                          << "\033[?1003h" // Enable any-event tracking
                          << "\033[?1006h" // Enable SGR extended coordinates
                          << std::flush;
                _mouse_enabled = true;
            }

            /**
             * @brief Disable mouse tracking
             */
            void disable_mouse_tracking()
            {
                if (!_mouse_enabled)
                    return;

                std::cout << "\033[?1006l"
                          << "\033[?1003l"
                          << "\033[?1002l"
                          << "\033[?1000l"
                          << std::flush;
                _mouse_enabled = false;
            }

            // ==================== Input Handling ====================

            /**
             * @brief Parse SGR mouse sequence
             *
             * Format: ESC [ < Cb ; Cx ; Cy M/m
             * Where Cb is button code, Cx/Cy are coordinates, M=press, m=release
             */
            bool parse_sgr_mouse(const std::string &seq, MouseEvent &event)
            {
                if (seq.size() < 6 || seq[0] != '\033' || seq[1] != '[' || seq[2] != '<')
                    return false;

                // Find the button code, x, y and terminator
                size_t pos = 3;
                int button = 0, x = 0, y = 0;
                char terminator = 0;

                // Parse button
                while (pos < seq.size() && seq[pos] != ';')
                {
                    if (seq[pos] >= '0' && seq[pos] <= '9')
                        button = button * 10 + (seq[pos] - '0');
                    pos++;
                }
                pos++; // Skip ';'

                // Parse X
                while (pos < seq.size() && seq[pos] != ';')
                {
                    if (seq[pos] >= '0' && seq[pos] <= '9')
                        x = x * 10 + (seq[pos] - '0');
                    pos++;
                }
                pos++; // Skip ';'

                // Parse Y and terminator
                while (pos < seq.size())
                {
                    if (seq[pos] >= '0' && seq[pos] <= '9')
                    {
                        y = y * 10 + (seq[pos] - '0');
                    }
                    else if (seq[pos] == 'M' || seq[pos] == 'm')
                    {
                        terminator = seq[pos];
                        break;
                    }
                    pos++;
                }

                if (terminator == 0)
                    return false;

                // Decode button (bit field)
                // Bits 0-1: button (0=left, 1=middle, 2=right, 3=release)
                // Bit 5: shift
                // Bit 4: meta/alt
                // Bit 3: ctrl
                // Bits 6-7: button modifier (64=scroll up, 65=scroll down)

                int btn = button & 0x03;
                bool shift = (button & 0x04) != 0;
                bool ctrl = (button & 0x10) != 0;
                bool motion = (button & 0x20) != 0;
                bool scroll = (button & 0x40) != 0;

                event.shift_held = shift;
                event.ctrl_held = ctrl;
                event.cell_x = x - 1; // 1-based to 0-based
                event.cell_y = y - 1;

                // Calculate sub-pixel and actual pixel coordinates
                // For colored mode: each cell is 1 pixel wide, 2 pixels tall
                // Adjust pixel coordinates based on draw mode
                if (_draw_mode == DrawMode::braille)
                {
                    // Braille: 2x4 pixels per character
                    event.sub_x = 0;
                    event.sub_y = 0;
                    event.pixel_x = event.cell_x * 2;
                    event.pixel_y = event.cell_y * 4;
                }
                else
                {
                    // Block: 1x2 pixels per character
                    event.sub_x = 0;
                    event.sub_y = 0;
                    event.pixel_x = event.cell_x;
                    event.pixel_y = event.cell_y * 2;
                }

                if (scroll)
                {
                    event.type = (btn == 0) ? MouseEventType::scroll_up : MouseEventType::scroll_down;
                    event.button = MouseButton::none;
                }
                else if (motion)
                {
                    event.type = MouseEventType::move;
                    event.button = (btn == 0) ? MouseButton::left : (btn == 1) ? MouseButton::middle
                                                                : (btn == 2)   ? MouseButton::right
                                                                               : MouseButton::none;
                }
                else if (terminator == 'm')
                {
                    event.type = MouseEventType::release;
                    event.button = (btn == 0) ? MouseButton::left : (btn == 1) ? MouseButton::middle
                                                                : (btn == 2)   ? MouseButton::right
                                                                               : MouseButton::none;
                }
                else
                {
                    event.type = MouseEventType::press;
                    event.button = (btn == 0) ? MouseButton::left : (btn == 1) ? MouseButton::middle
                                                                : (btn == 2)   ? MouseButton::right
                                                                               : MouseButton::none;
                }

                return true;
            }

            /**
             * @brief Read input with timeout
             */
            std::string read_input()
            {
                std::string result;
                char c;

#ifdef _WIN32
                if (_kbhit())
                {
                    result += static_cast<char>(_getch());
                    // Check for more characters (escape sequences)
                    while (_kbhit())
                    {
                        result += static_cast<char>(_getch());
                    }
                }
#else
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(STDIN_FILENO, &fds);

                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 50000; // 50ms

                if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0)
                {
                    while (read(STDIN_FILENO, &c, 1) == 1)
                    {
                        result += c;
                        // Check if more data available
                        FD_ZERO(&fds);
                        FD_SET(STDIN_FILENO, &fds);
                        tv.tv_sec = 0;
                        tv.tv_usec = 1000; // 1ms for subsequent chars
                        if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0)
                            break;
                    }
                }
#endif
                return result;
            }

            // ==================== Drawing Operations ====================

            /**
             * @brief Set a pixel with alpha blending
             */
            void set_pixel(int x, int y, const RGBA &color)
            {
                if (x < 0 || x >= (int)_pixel_width || y < 0 || y >= (int)_pixel_height)
                    return;

                _pixels[y][x] = color.blend_over(_pixels[y][x]);
            }

            /**
             * @brief Set a pixel in the preview layer
             */
            void set_preview_pixel(int x, int y, const RGBA &color)
            {
                if (x < 0 || x >= (int)_pixel_width || y < 0 || y >= (int)_pixel_height)
                    return;

                _preview[y][x] = color;
                _preview_active = true; // Enable preview rendering
            }

            /**
             * @brief Clear the preview layer
             */
            void clear_preview()
            {
                for (auto &row : _preview)
                {
                    std::fill(row.begin(), row.end(), RGBA(0, 0, 0, 0));
                }
                _preview_active = false;
            }

            /**
             * @brief Get a pixel color
             */
            RGBA get_pixel(int x, int y) const
            {
                if (x < 0 || x >= (int)_pixel_width || y < 0 || y >= (int)_pixel_height)
                    return _background;
                return _pixels[y][x];
            }

            /**
             * @brief Draw with brush at position
             */
            void draw_brush(int x, int y, const RGBA &color, bool to_preview = false)
            {
                int r = _brush_size;
                for (int dy = -r + 1; dy < r; ++dy)
                {
                    for (int dx = -r + 1; dx < r; ++dx)
                    {
                        if (dx * dx + dy * dy < r * r)
                        {
                            if (to_preview)
                                set_preview_pixel(x + dx, y + dy, color);
                            else
                                set_pixel(x + dx, y + dy, color);
                        }
                    }
                }
            }

            /**
             * @brief Draw line using Bresenham's algorithm
             */
            void draw_line(int x0, int y0, int x1, int y1, const RGBA &color, bool to_preview = false)
            {
                int dx = std::abs(x1 - x0);
                int dy = std::abs(y1 - y0);
                int sx = x0 < x1 ? 1 : -1;
                int sy = y0 < y1 ? 1 : -1;
                int err = dx - dy;

                while (true)
                {
                    draw_brush(x0, y0, color, to_preview);
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
             * @brief Draw circle outline
             */
            void draw_circle(int cx, int cy, int radius, const RGBA &color, bool to_preview = false)
            {
                int x = radius;
                int y = 0;
                int err = 0;

                while (x >= y)
                {
                    draw_brush(cx + x, cy + y, color, to_preview);
                    draw_brush(cx + y, cy + x, color, to_preview);
                    draw_brush(cx - y, cy + x, color, to_preview);
                    draw_brush(cx - x, cy + y, color, to_preview);
                    draw_brush(cx - x, cy - y, color, to_preview);
                    draw_brush(cx - y, cy - x, color, to_preview);
                    draw_brush(cx + y, cy - x, color, to_preview);
                    draw_brush(cx + x, cy - y, color, to_preview);

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
             * @brief Draw rectangle outline
             */
            void draw_rect(int x0, int y0, int x1, int y1, const RGBA &color, bool to_preview = false)
            {
                draw_line(x0, y0, x1, y0, color, to_preview); // Top
                draw_line(x1, y0, x1, y1, color, to_preview); // Right
                draw_line(x1, y1, x0, y1, color, to_preview); // Bottom
                draw_line(x0, y1, x0, y0, color, to_preview); // Left
            }

            /**
             * @brief Flood fill using scanline algorithm
             */
            void flood_fill(int x, int y, const RGBA &fill_color)
            {
                if (x < 0 || x >= (int)_pixel_width || y < 0 || y >= (int)_pixel_height)
                    return;

                RGBA target_color = _pixels[y][x];
                if (target_color == fill_color)
                    return;

                std::vector<std::pair<int, int>> stack;
                stack.push_back({x, y});

                while (!stack.empty())
                {
                    auto [px, py] = stack.back();
                    stack.pop_back();

                    if (px < 0 || px >= (int)_pixel_width || py < 0 || py >= (int)_pixel_height)
                        continue;
                    if (_pixels[py][px] != target_color)
                        continue;

                    _pixels[py][px] = fill_color;

                    stack.push_back({px + 1, py});
                    stack.push_back({px - 1, py});
                    stack.push_back({px, py + 1});
                    stack.push_back({px, py - 1});
                }
            }

            // ==================== History ====================

            /**
             * @brief Push current state to undo stack
             */
            void push_undo()
            {
                if (_undo_stack.size() >= MAX_UNDO)
                {
                    // Remove oldest state (convert stack to temp)
                    std::stack<CanvasState> temp;
                    while (_undo_stack.size() > 1)
                    {
                        temp.push(std::move(_undo_stack.top()));
                        _undo_stack.pop();
                    }
                    _undo_stack.pop(); // Remove oldest
                    while (!temp.empty())
                    {
                        _undo_stack.push(std::move(temp.top()));
                        temp.pop();
                    }
                }
                _undo_stack.push(CanvasState(_pixels));
                // Clear redo stack when new action is performed
                while (!_redo_stack.empty())
                    _redo_stack.pop();
            }

            /**
             * @brief Undo last action
             */
            void undo()
            {
                if (_undo_stack.empty())
                    return;
                _redo_stack.push(CanvasState(_pixels));
                _pixels = _undo_stack.top().pixels;
                _undo_stack.pop();
            }

            /**
             * @brief Redo last undone action
             */
            void redo()
            {
                if (_redo_stack.empty())
                    return;
                _undo_stack.push(CanvasState(_pixels));
                _pixels = _redo_stack.top().pixels;
                _redo_stack.pop();
            }

            // ==================== Rendering ====================

            /**
             * @brief Get effective pixel (preview layer composited over main pixels)
             */
            RGBA get_effective_pixel(size_t py, size_t px) const
            {
                if (_preview_active && py < _pixel_height && px < _pixel_width)
                {
                    const RGBA &preview_pix = _preview[py][px];
                    if (preview_pix.a > 0)
                    {
                        // Alpha blend preview over main pixel
                        const RGBA &main_pix = _pixels[py][px];
                        return preview_pix.blend_over(main_pix);
                    }
                }
                if (py < _pixel_height && px < _pixel_width)
                    return _pixels[py][px];
                return _background;
            }

            /**
             * @brief Check if pixel should show brush cursor preview
             */
            bool is_brush_cursor(size_t py, size_t px) const
            {
                if (_mouse_x < 0 || _mouse_y < 0)
                    return false;

                // Draw circle outline around cursor position
                int dx = static_cast<int>(px) - _mouse_x;
                int dy = static_cast<int>(py) - _mouse_y;
                double dist = std::sqrt(dx * dx + dy * dy);

                // Show brush outline (ring from brush_size-0.5 to brush_size+0.5)
                return std::abs(dist - _brush_size) < 0.8;
            }

            /**
             * @brief Render canvas to terminal with UI
             */
            std::string render() const
            {
                std::string out;
                out.reserve((_char_height + STATUS_HEIGHT) * (_char_width + UI_PANEL_WIDTH + 5) * 30);

                // Move cursor home (clear happens per-line with \033[K)
                out += "\033[H";

                RGB prev_fg(-1, -1, -1), prev_bg(-1, -1, -1);

                if (_draw_mode == DrawMode::braille)
                {
                    // Braille mode: 2×4 pixels per character cell
                    // Braille dot positions (bits):
                    //   [0] [3]   Row 0
                    //   [1] [4]   Row 1
                    //   [2] [5]   Row 2
                    //   [6] [7]   Row 3
                    constexpr uint8_t BRAILLE_DOTS[4][2] = {
                        {0x01, 0x08}, // Row 0
                        {0x02, 0x10}, // Row 1
                        {0x04, 0x20}, // Row 2
                        {0x40, 0x80}  // Row 3
                    };

                    for (size_t cy = 0; cy < _char_height; ++cy)
                    {
                        for (size_t cx = 0; cx < _char_width; ++cx)
                        {
                            uint8_t pattern = 0;
                            RGB fg_color(255, 255, 255); // Default white
                            bool has_fg = false;

                            // Sample 2×4 pixel region
                            for (int row = 0; row < 4; ++row)
                            {
                                for (int col = 0; col < 2; ++col)
                                {
                                    size_t py = cy * 4 + row;
                                    size_t px = cx * 2 + col;

                                    RGBA pix = get_effective_pixel(py, px);
                                    bool is_cursor = is_brush_cursor(py, px);

                                    // Check if pixel is "on" (not background)
                                    bool is_lit = (pix.a > 128) &&
                                                  (pix.r != _background.r ||
                                                   pix.g != _background.g ||
                                                   pix.b != _background.b);

                                    if (is_cursor)
                                    {
                                        // Cursor outline always shows
                                        pattern |= BRAILLE_DOTS[row][col];
                                        if (!has_fg)
                                        {
                                            fg_color = RGB(255, 255, 0); // Yellow cursor
                                            has_fg = true;
                                        }
                                    }
                                    else if (is_lit)
                                    {
                                        pattern |= BRAILLE_DOTS[row][col];
                                        if (!has_fg)
                                        {
                                            fg_color = pix.to_rgb();
                                            has_fg = true;
                                        }
                                    }
                                }
                            }

                            // Output colored braille character
                            if (fg_color != prev_fg)
                            {
                                out += ansi::fg_color(fg_color.r, fg_color.g, fg_color.b);
                                prev_fg = fg_color;
                            }

                            // Convert pattern to UTF-8 braille character (U+2800 + pattern)
                            char32_t codepoint = 0x2800 + pattern;
                            out += static_cast<char>(0xE0 | (codepoint >> 12));
                            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (codepoint & 0x3F));
                        }

                        out += ansi::RESET;

                        // Draw UI panel
                        out += " │ ";
                        out += render_ui_line(cy);
                        out += "\033[K\n"; // Clear to end of line before newline
                        prev_fg = RGB(-1, -1, -1);
                    }
                }
                else
                {
                    // Block mode: 1×2 pixels per character (upper half block)
                    const char *UPPER_HALF = "\xe2\x96\x80";

                    for (size_t cy = 0; cy < _char_height; ++cy)
                    {
                        size_t py_top = cy * 2;
                        size_t py_bot = py_top + 1;

                        // Draw canvas
                        for (size_t cx = 0; cx < _char_width; ++cx)
                        {
                            RGBA top = get_effective_pixel(py_top, cx);
                            RGBA bot = get_effective_pixel(py_bot, cx);

                            // Check for brush cursor overlay
                            bool cursor_top = is_brush_cursor(py_top, cx);
                            bool cursor_bot = is_brush_cursor(py_bot, cx);

                            if (cursor_top)
                                top = RGBA(255, 255, 0, 255); // Yellow cursor
                            if (cursor_bot)
                                bot = RGBA(255, 255, 0, 255);

                            RGB top_rgb = top.to_rgb();
                            RGB bot_rgb = bot.to_rgb();

                            if (top_rgb != prev_fg)
                            {
                                out += ansi::fg_color(top_rgb.r, top_rgb.g, top_rgb.b);
                                prev_fg = top_rgb;
                            }
                            if (bot_rgb != prev_bg)
                            {
                                out += ansi::bg_color(bot_rgb.r, bot_rgb.g, bot_rgb.b);
                                prev_bg = bot_rgb;
                            }
                            out += UPPER_HALF;
                        }

                        out += ansi::RESET;

                        // Draw UI panel separator
                        out += " │ ";
                        out += render_ui_line(cy);
                        out += "\033[K\n"; // Clear to end of line before newline
                        prev_fg = RGB(-1, -1, -1);
                        prev_bg = RGB(-1, -1, -1);
                    }
                }

                // Status bar
                out += "\033[K\n"; // Clear line
                out += "File: " + _output_file;
                out += " | Size: " + std::to_string(_pixel_width) + "x" + std::to_string(_pixel_height);
                out += " | Mode: ";
                out += (_draw_mode == DrawMode::braille ? "Braille" : "Block");
                out += "\033[K"; // Clear to end of line

                return out;
            }

            /**
             * @brief Render UI panel line content
             */
            std::string render_ui_line(size_t cy) const
            {
                std::string out;

                if (cy == 0)
                {
                    out += "Tool: ";
                    switch (_current_tool)
                    {
                    case Tool::pen:
                        out += "Pen";
                        break;
                    case Tool::line:
                        out += "Line";
                        break;
                    case Tool::circle:
                        out += "Circle";
                        break;
                    case Tool::rectangle:
                        out += "Rect";
                        break;
                    case Tool::fill:
                        out += "Fill";
                        break;
                    case Tool::eraser:
                        out += "Eraser";
                        break;
                    }
                }
                else if (cy == 2)
                {
                    out += "Color: ";
                    out += ansi::fg_color(_foreground.r, _foreground.g, _foreground.b);
                    out += "████";
                    out += ansi::RESET;
                }
                else if (cy == 3)
                {
                    out += "R:" + std::to_string(_foreground.r);
                    if (_active_channel == ColorChannel::red)
                    {
                        out += " [" + (_input_buffer.empty() ? "_" : _input_buffer) + "]";
                    }
                }
                else if (cy == 4)
                {
                    out += "G:" + std::to_string(_foreground.g);
                    if (_active_channel == ColorChannel::green)
                    {
                        out += " [" + (_input_buffer.empty() ? "_" : _input_buffer) + "]";
                    }
                }
                else if (cy == 5)
                {
                    out += "B:" + std::to_string(_foreground.b);
                    if (_active_channel == ColorChannel::blue)
                    {
                        out += " [" + (_input_buffer.empty() ? "_" : _input_buffer) + "]";
                    }
                }
                else if (cy == 6)
                {
                    out += "A:" + std::to_string(_foreground.a);
                    if (_active_channel == ColorChannel::alpha)
                    {
                        out += " [" + (_input_buffer.empty() ? "_" : _input_buffer) + "]";
                    }
                }
                else if (cy == 7)
                {
                    out += "(Enter to apply)";
                }
                else if (cy == 9)
                {
                    out += "Brush: " + std::to_string(_brush_size);
                }
                else if (cy == 11)
                {
                    out += "Keys:";
                }
                else if (cy == 12)
                {
                    out += "p=pen l=line";
                }
                else if (cy == 13)
                {
                    out += "c=circle x=rect";
                }
                else if (cy == 14)
                {
                    out += "f=fill e=eraser";
                }
                else if (cy == 15)
                {
                    out += "r/g/b/a=color";
                }
                else if (cy == 16)
                {
                    out += "0-9+Enter=value";
                }
                else if (cy == 17)
                {
                    out += "+/-=brush size";
                }
                else if (cy == 18)
                {
                    out += "u=undo y=redo";
                }
                else if (cy == 19)
                {
                    out += "s=save q=quit";
                }

                return out;
            }

            // ==================== Save/Load ====================

            /**
             * @brief Save canvas to .pi file
             */
            bool save(const std::string &filename = "")
            {
                std::string path = filename.empty() ? _output_file : filename;

                // Convert RGBA to RGB (PPM format)
                std::vector<uint8_t> rgb_data;
                rgb_data.reserve(_pixel_width * _pixel_height * 3);

                for (size_t y = 0; y < _pixel_height; ++y)
                {
                    for (size_t x = 0; x < _pixel_width; ++x)
                    {
                        const RGBA &pixel = _pixels[y][x];
                        // Alpha blend with background for export
                        RGBA blended = pixel.blend_over(_background);
                        rgb_data.push_back(blended.r);
                        rgb_data.push_back(blended.g);
                        rgb_data.push_back(blended.b);
                    }
                }

                // Write as PPM first
                std::string ppm_path = path + ".ppm";
                std::ofstream ppm(ppm_path, std::ios::binary);
                if (!ppm)
                    return false;

                ppm << "P6\n"
                    << _pixel_width << " " << _pixel_height << "\n255\n";
                ppm.write(reinterpret_cast<const char *>(rgb_data.data()), rgb_data.size());
                ppm.close();

                // Convert to .pi format
                try
                {
                    pythonic::media::convert(ppm_path, pythonic::media::MediaType::image, true);
                    // Remove temp PPM
                    std::remove(ppm_path.c_str());
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }

            /**
             * @brief Clear canvas
             */
            void clear()
            {
                push_undo();
                for (auto &row : _pixels)
                {
                    std::fill(row.begin(), row.end(), _background);
                }
            }

            // ==================== Main Loop ====================

            /**
             * @brief Apply buffered input value to active color channel
             */
            void apply_color_input()
            {
                if (_input_buffer.empty() || _active_channel == ColorChannel::none)
                    return;

                int value = std::stoi(_input_buffer);
                if (value > 255)
                    value = 255;
                if (value < 0)
                    value = 0;

                switch (_active_channel)
                {
                case ColorChannel::red:
                    _foreground.r = static_cast<uint8_t>(value);
                    break;
                case ColorChannel::green:
                    _foreground.g = static_cast<uint8_t>(value);
                    break;
                case ColorChannel::blue:
                    _foreground.b = static_cast<uint8_t>(value);
                    break;
                case ColorChannel::alpha:
                    _foreground.a = static_cast<uint8_t>(value);
                    break;
                default:
                    break;
                }
                _input_buffer.clear();
            }

            /**
             * @brief Handle keyboard input
             */
            void handle_key(char key)
            {
                // Color channel digit input (accumulate in buffer)
                if (_active_channel != ColorChannel::none && key >= '0' && key <= '9')
                {
                    if (_input_buffer.size() < 3)
                    { // Max 3 digits for 0-255
                        _input_buffer += key;
                    }
                    return;
                }

                // Enter confirms color input
                if (_active_channel != ColorChannel::none && (key == '\r' || key == '\n'))
                {
                    apply_color_input();
                    return;
                }

                // Backspace removes last digit
                if (_active_channel != ColorChannel::none && (key == '\b' || key == 127))
                {
                    if (!_input_buffer.empty())
                    {
                        _input_buffer.pop_back();
                    }
                    return;
                }

                switch (key)
                {
                // Tools
                case 'p':
                    _current_tool = Tool::pen;
                    break;
                case 'l':
                    _current_tool = Tool::line;
                    break;
                case 'c':
                    _current_tool = Tool::circle;
                    break;
                case 'x':
                    _current_tool = Tool::rectangle;
                    break;
                case 'f':
                    _current_tool = Tool::fill;
                    break;
                case 'e':
                    _current_tool = Tool::eraser;
                    break;

                // Color channels (clear input buffer when switching)
                case 'r':
                    _input_buffer.clear();
                    _active_channel = (_active_channel == ColorChannel::red)
                                          ? ColorChannel::none
                                          : ColorChannel::red;
                    break;
                case 'g':
                    _input_buffer.clear();
                    _active_channel = (_active_channel == ColorChannel::green)
                                          ? ColorChannel::none
                                          : ColorChannel::green;
                    break;
                case 'b':
                    _input_buffer.clear();
                    _active_channel = (_active_channel == ColorChannel::blue)
                                          ? ColorChannel::none
                                          : ColorChannel::blue;
                    break;
                case 'a':
                    _input_buffer.clear();
                    _active_channel = (_active_channel == ColorChannel::alpha)
                                          ? ColorChannel::none
                                          : ColorChannel::alpha;
                    break;
                case 'n':
                    _input_buffer.clear();
                    _active_channel = ColorChannel::none;
                    break;

                // Brush size
                case '+':
                case '=':
                    _brush_size = std::min(20, (int)_brush_size + 1);
                    break;
                case '-':
                case '_':
                    _brush_size = std::max(1, (int)_brush_size - 1);
                    break;

                // Undo/Redo
                case 'u':
                    undo();
                    break;
                case 'y':
                    redo();
                    break;

                // Save
                case 's':
                    save();
                    break;

                // Clear
                case 'C':
                    clear();
                    break;

                // Quit
                case 'q':
                case 27: // Escape
                    _running = false;
                    break;
                }
            }

            /**
             * @brief Handle mouse event
             */
            void handle_mouse(const MouseEvent &event)
            {
                // Calculate pixel coordinates based on draw mode
                int px, py;
                if (_draw_mode == DrawMode::braille)
                {
                    // Braille: 2x4 pixels per character
                    px = event.cell_x * 2;
                    py = event.cell_y * 4;
                }
                else
                {
                    // Block: 1x2 pixels per character
                    px = event.cell_x;
                    py = event.cell_y * 2;
                }

                // Update mouse position for cursor preview
                _mouse_x = px;
                _mouse_y = py;

                // Check if within canvas bounds
                if (px < 0 || px >= (int)_pixel_width)
                    return;

                switch (event.type)
                {
                case MouseEventType::press:
                    if (event.button == MouseButton::left)
                    {
                        push_undo();
                        _drawing = true;
                        _start_x = px;
                        _start_y = py;
                        _last_x = px;
                        _last_y = py;

                        if (_current_tool == Tool::pen || _current_tool == Tool::eraser)
                        {
                            RGBA color = (_current_tool == Tool::eraser) ? _background : _foreground;
                            draw_brush(px, py, color);
                        }
                        else if (_current_tool == Tool::fill)
                        {
                            flood_fill(px, py, _foreground);
                        }
                        else
                        {
                            // Shape tools: activate preview mode
                            _preview_active = true;
                            clear_preview();
                        }
                    }
                    break;

                case MouseEventType::move:
                    if (_drawing && event.button == MouseButton::left)
                    {
                        if (_current_tool == Tool::pen || _current_tool == Tool::eraser)
                        {
                            RGBA color = (_current_tool == Tool::eraser) ? _background : _foreground;
                            draw_line(_last_x, _last_y, px, py, color);
                        }
                        else
                        {
                            // Update preview for shape tools
                            clear_preview();
                            switch (_current_tool)
                            {
                            case Tool::line:
                                draw_line(_start_x, _start_y, px, py, _foreground, true);
                                break;
                            case Tool::circle:
                            {
                                int radius = static_cast<int>(
                                    std::sqrt(std::pow(px - _start_x, 2) + std::pow(py - _start_y, 2)));
                                draw_circle(_start_x, _start_y, radius, _foreground, true);
                                break;
                            }
                            case Tool::rectangle:
                                draw_rect(_start_x, _start_y, px, py, _foreground, true);
                                break;
                            default:
                                break;
                            }
                        }
                        _last_x = px;
                        _last_y = py;
                    }
                    break;

                case MouseEventType::release:
                    if (_drawing)
                    {
                        _drawing = false;
                        _preview_active = false;
                        clear_preview();

                        switch (_current_tool)
                        {
                        case Tool::line:
                            draw_line(_start_x, _start_y, px, py, _foreground);
                            break;
                        case Tool::circle:
                        {
                            int radius = static_cast<int>(
                                std::sqrt(std::pow(px - _start_x, 2) + std::pow(py - _start_y, 2)));
                            draw_circle(_start_x, _start_y, radius, _foreground);
                            break;
                        }
                        case Tool::rectangle:
                            draw_rect(_start_x, _start_y, px, py, _foreground);
                            break;
                        default:
                            break;
                        }
                    }
                    break;

                case MouseEventType::scroll_up:
                    _brush_size = std::min(20, (int)_brush_size + 1);
                    break;

                case MouseEventType::scroll_down:
                    _brush_size = std::max(1, (int)_brush_size - 1);
                    break;
                }
            }

            /**
             * @brief Run the interactive drawing session
             */
            void run()
            {
                _running = true;

                // Setup terminal
                enable_raw_mode();
                enable_mouse_tracking();

                // Hide cursor and clear screen
                std::cout << "\033[?25l" // Hide cursor
                          << "\033[2J"   // Clear screen
                          << "\033[H"    // Move to home
                          << std::flush;

                while (_running)
                {
                    // Render canvas
                    std::cout << render() << std::flush;

                    // Read input
                    std::string input = read_input();

                    if (input.empty())
                        continue;

                    // Check for mouse event (SGR format)
                    if (input.size() >= 6 && input[0] == '\033' && input[1] == '[' && input[2] == '<')
                    {
                        MouseEvent event;
                        if (parse_sgr_mouse(input, event))
                        {
                            handle_mouse(event);
                        }
                    }
                    else
                    {
                        // Handle keyboard input
                        for (char c : input)
                        {
                            if (c != '\033') // Ignore escape sequences we don't handle
                            {
                                handle_key(c);
                            }
                        }
                    }
                }

                // Cleanup
                disable_mouse_tracking();
                disable_raw_mode();

                // Show cursor and clear screen
                std::cout << "\033[?25h" // Show cursor
                          << "\033[2J"   // Clear screen
                          << "\033[H"    // Move to home
                          << std::flush;
            }
        };

        // ==================== Simple Entry Points ====================

        /**
         * @brief Start an interactive drawing session
         *
         * Simple entry point for live drawing.
         *
         * @param width Canvas width in characters (default: 60)
         * @param height Canvas height in characters (default: 30)
         * @param output_file Default filename for saving (default: "drawing.pi")
         * @param mode Drawing mode: DrawMode::block (1x2 pixels/char) or DrawMode::braille (2x4 pixels/char)
         */
        inline void live_draw(int width = 60, int height = 30,
                              const std::string &output_file = "drawing.pi",
                              DrawMode mode = DrawMode::block)
        {
            LiveCanvas canvas(width, height, output_file, mode);
            canvas.run();
        }

        /**
         * @brief Alias for live_draw() in block mode
         */
        inline void draw()
        {
            live_draw();
        }

        /**
         * @brief Start live drawing session in Braille mode (higher resolution)
         * @param width Canvas width in characters (default: 60)
         * @param height Canvas height in characters (default: 30)
         * @param output_file Default filename for saving (default: "drawing.pi")
         */
        inline void live_draw_braille(int width = 60, int height = 30,
                                      const std::string &output_file = "drawing.pi")
        {
            LiveCanvas canvas(width, height, output_file, DrawMode::braille);
            canvas.run();
        }

    } // namespace draw
} // namespace pythonic
