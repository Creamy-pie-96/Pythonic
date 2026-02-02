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

        // ==================== Live Canvas ====================

        /**
         * @brief Interactive drawing canvas with mouse and keyboard input
         *
         * The canvas uses Braille characters for display but stores full
         * RGBA pixel data internally for color support and alpha blending.
         */
        class LiveCanvas
        {
        private:
            // Canvas dimensions
            size_t _char_width;   // Width in terminal characters
            size_t _char_height;  // Height in terminal characters
            size_t _pixel_width;  // Width in pixels (char_width for colored mode)
            size_t _pixel_height; // Height in pixels (char_height * 2 for half-block)

            // Pixel storage (RGBA for full color with alpha)
            std::vector<std::vector<RGBA>> _pixels;

            // Current state
            Tool _current_tool;
            RGBA _foreground;
            RGBA _background;
            ColorChannel _active_channel;
            int _pending_digit; // For two-digit color input
            uint8_t _brush_size;

            // Tool state
            bool _drawing;
            int _start_x, _start_y; // For line/rect/circle tools
            int _last_x, _last_y;   // Last mouse position

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
             * @brief Create a live drawing canvas
             * @param char_width Width in terminal characters
             * @param char_height Height in terminal characters
             * @param output_file Default filename for saving (optional)
             */
            LiveCanvas(size_t char_width = 60, size_t char_height = 30,
                       const std::string &output_file = "drawing.pi")
                : _char_width(char_width), _char_height(char_height), _pixel_width(char_width), _pixel_height(char_height * 2), _pixels(_pixel_height, std::vector<RGBA>(_pixel_width, RGBA(0, 0, 0, 255))),
                  _current_tool(Tool::pen), _foreground(255, 255, 255, 255), _background(0, 0, 0, 255), _active_channel(ColorChannel::none), _pending_digit(-1), _brush_size(1), _drawing(false), _start_x(0), _start_y(0), _last_x(0), _last_y(0),
#ifndef _WIN32
                  _raw_mode(false),
#endif
                  _mouse_enabled(false), _running(false), _output_file(output_file)
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
                event.sub_x = 0;
                event.sub_y = 0;
                event.pixel_x = event.cell_x;
                event.pixel_y = event.cell_y * 2; // Half-block mode

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
            void draw_brush(int x, int y, const RGBA &color)
            {
                int r = _brush_size;
                for (int dy = -r + 1; dy < r; ++dy)
                {
                    for (int dx = -r + 1; dx < r; ++dx)
                    {
                        if (dx * dx + dy * dy < r * r)
                        {
                            set_pixel(x + dx, y + dy, color);
                        }
                    }
                }
            }

            /**
             * @brief Draw line using Bresenham's algorithm
             */
            void draw_line(int x0, int y0, int x1, int y1, const RGBA &color)
            {
                int dx = std::abs(x1 - x0);
                int dy = std::abs(y1 - y0);
                int sx = x0 < x1 ? 1 : -1;
                int sy = y0 < y1 ? 1 : -1;
                int err = dx - dy;

                while (true)
                {
                    draw_brush(x0, y0, color);
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
            void draw_circle(int cx, int cy, int radius, const RGBA &color)
            {
                int x = radius;
                int y = 0;
                int err = 0;

                while (x >= y)
                {
                    draw_brush(cx + x, cy + y, color);
                    draw_brush(cx + y, cy + x, color);
                    draw_brush(cx - y, cy + x, color);
                    draw_brush(cx - x, cy + y, color);
                    draw_brush(cx - x, cy - y, color);
                    draw_brush(cx - y, cy - x, color);
                    draw_brush(cx + y, cy - x, color);
                    draw_brush(cx + x, cy - y, color);

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
            void draw_rect(int x0, int y0, int x1, int y1, const RGBA &color)
            {
                draw_line(x0, y0, x1, y0, color); // Top
                draw_line(x1, y0, x1, y1, color); // Right
                draw_line(x1, y1, x0, y1, color); // Bottom
                draw_line(x0, y1, x0, y0, color); // Left
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
             * @brief Render canvas to terminal with UI
             */
            std::string render() const
            {
                std::string out;
                out.reserve((_char_height + STATUS_HEIGHT) * (_char_width + UI_PANEL_WIDTH + 5) * 30);

                // Clear screen and move cursor home
                out += "\033[H";

                // Upper half block character
                const char *UPPER_HALF = "\xe2\x96\x80";

                RGB prev_fg(-1, -1, -1), prev_bg(-1, -1, -1);

                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    size_t py_top = cy * 2;
                    size_t py_bot = py_top + 1;

                    // Draw canvas
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        RGBA top = _pixels[py_top][cx];
                        RGBA bot = (py_bot < _pixel_height) ? _pixels[py_bot][cx] : _background;

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

                    // Draw UI panel content
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
                        out += (_active_channel == ColorChannel::red ? " <" : "");
                    }
                    else if (cy == 4)
                    {
                        out += "G:" + std::to_string(_foreground.g);
                        out += (_active_channel == ColorChannel::green ? " <" : "");
                    }
                    else if (cy == 5)
                    {
                        out += "B:" + std::to_string(_foreground.b);
                        out += (_active_channel == ColorChannel::blue ? " <" : "");
                    }
                    else if (cy == 6)
                    {
                        out += "A:" + std::to_string(_foreground.a);
                        out += (_active_channel == ColorChannel::alpha ? " <" : "");
                    }
                    else if (cy == 8)
                    {
                        out += "Brush: " + std::to_string(_brush_size);
                    }
                    else if (cy == 10)
                    {
                        out += "Keys:";
                    }
                    else if (cy == 11)
                    {
                        out += "p=pen l=line";
                    }
                    else if (cy == 12)
                    {
                        out += "c=circle x=rect";
                    }
                    else if (cy == 13)
                    {
                        out += "f=fill e=eraser";
                    }
                    else if (cy == 14)
                    {
                        out += "r/g/b/a=color";
                    }
                    else if (cy == 15)
                    {
                        out += "0-9=value n=done";
                    }
                    else if (cy == 16)
                    {
                        out += "+/-=brush size";
                    }
                    else if (cy == 17)
                    {
                        out += "u=undo y=redo";
                    }
                    else if (cy == 18)
                    {
                        out += "s=save q=quit";
                    }

                    out += '\n';
                    prev_fg = RGB(-1, -1, -1);
                    prev_bg = RGB(-1, -1, -1);
                }

                // Status bar
                out += "\n";
                out += "File: " + _output_file;
                out += " | Size: " + std::to_string(_pixel_width) + "x" + std::to_string(_pixel_height);

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
                    pythonic::media::convert(ppm_path, pythonic::media::Type::image, true);
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
             * @brief Handle keyboard input
             */
            void handle_key(char key)
            {
                // Color channel input
                if (_active_channel != ColorChannel::none && key >= '0' && key <= '9')
                {
                    int digit = key - '0';
                    int value;

                    if (_pending_digit >= 0)
                    {
                        // Second digit - complete the value
                        value = _pending_digit * 10 + digit;
                        if (value > 255)
                            value = 255;
                        _pending_digit = -1;
                    }
                    else
                    {
                        // First digit - check if could be start of two-digit
                        if (digit == 0)
                        {
                            value = 0;
                        }
                        else
                        {
                            _pending_digit = digit;
                            return; // Wait for second digit
                        }
                    }

                    // Apply value to channel
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
                    return;
                }

                // Complete pending digit if any other key pressed
                if (_pending_digit >= 0)
                {
                    int value = _pending_digit * 10; // Assume 0 for second digit
                    if (value > 255)
                        value = 255;

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
                    _pending_digit = -1;
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

                // Color channels
                case 'r':
                    _active_channel = (_active_channel == ColorChannel::red)
                                          ? ColorChannel::none
                                          : ColorChannel::red;
                    break;
                case 'g':
                    _active_channel = (_active_channel == ColorChannel::green)
                                          ? ColorChannel::none
                                          : ColorChannel::green;
                    break;
                case 'b':
                    _active_channel = (_active_channel == ColorChannel::blue)
                                          ? ColorChannel::none
                                          : ColorChannel::blue;
                    break;
                case 'a':
                    _active_channel = (_active_channel == ColorChannel::alpha)
                                          ? ColorChannel::none
                                          : ColorChannel::alpha;
                    break;
                case 'n':
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
                // Adjust for canvas bounds
                int px = event.cell_x;
                int py = event.cell_y * 2; // Half-block mode

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
                        _last_x = px;
                        _last_y = py;
                    }
                    break;

                case MouseEventType::release:
                    if (_drawing)
                    {
                        _drawing = false;

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
         */
        inline void live_draw(int width = 60, int height = 30,
                              const std::string &output_file = "drawing.pi")
        {
            LiveCanvas canvas(width, height, output_file);
            canvas.run();
        }

        /**
         * @brief Alias for live_draw()
         */
        inline void draw()
        {
            live_draw();
        }

    } // namespace draw
} // namespace pythonic
