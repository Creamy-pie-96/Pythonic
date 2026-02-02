#pragma once
/**
 * @file pythonicPlot.hpp
 * @brief matplotlib-style plotting library for terminal graphics
 *
 * This module provides a plotting library inspired by Python's matplotlib
 * and Desmos's dynamic graphing capabilities. It uses BrailleCanvas for
 * high-resolution terminal output.
 *
 * Features:
 * - Plot mathematical functions using lambdas
 * - Time-variant (animated) graphs with 't' variable
 * - Parameter sliders like Desmos (adjustable variables)
 * - Multiple plot types: line, scatter, bar, histogram
 * - Auto-scaling axes with tick marks
 * - Multiple plots on same axes with different colors
 * - Alpha blending for overlapping plots
 * - Real-time animation with configurable FPS
 * - Support for pythonic::vars::var types
 *
 * Desmos-style Dynamic Plotting:
 * - Variables can be defined and adjusted in real-time
 * - Time variable 't' for animations
 * - Parametric equations (x(t), y(t))
 * - Implicit equations (future feature)
 *
 * Example usage:
 *   using namespace pythonic::plot;
 *
 *   // Simple function plot
 *   plot([](double x) { return sin(x); }, -PI, PI);
 *
 *   // With pythonic var lambdas
 *   var f = lambda_(x, x * x);
 *   plot(f, -10, 10);
 *
 *   // Time-varying animation (like Desmos)
 *   animate([](double t, double x) { return sin(x + t); }, -PI, PI);
 *
 *   // Parametric plot
 *   parametric(
 *       [](double t) { return cos(t); },  // x(t)
 *       [](double t) { return sin(t); },  // y(t)
 *       0, 2*PI
 *   );
 *
 *   // Multiple plots with Figure
 *   Figure fig(80, 40);
 *   fig.plot([](double x) { return sin(x); }, -PI, PI, "red");
 *   fig.plot([](double x) { return cos(x); }, -PI, PI, "blue");
 *   fig.show();
 */

#include <functional>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <map>
#include <set>
#include <memory>
#include <optional>
#include <sstream>
#include <iomanip>
#include <limits>

#include "pythonicDraw.hpp"
#include "pythonicVars.hpp"
#include "pythonicFunction.hpp"
#include "pythonicLiveDraw.hpp"

namespace pythonic
{
    namespace plot
    {
        using namespace pythonic::vars;
        using namespace pythonic::draw;

        // Mathematical constants
        constexpr double PI = 3.14159265358979323846;
        constexpr double E = 2.71828182845904523536;
        constexpr double TAU = 2.0 * PI;

        // ==================== Color Palette ====================

        /**
         * @brief Named colors for plotting
         */
        namespace colors
        {
            inline RGBA red(255, 0, 0);
            inline RGBA green(0, 255, 0);
            inline RGBA blue(0, 0, 255);
            inline RGBA yellow(255, 255, 0);
            inline RGBA cyan(0, 255, 255);
            inline RGBA magenta(255, 0, 255);
            inline RGBA orange(255, 165, 0);
            inline RGBA purple(128, 0, 128);
            inline RGBA white(255, 255, 255);
            inline RGBA black(0, 0, 0);
            inline RGBA gray(128, 128, 128);

            /**
             * @brief Get color by name
             */
            inline RGBA from_name(const std::string &name)
            {
                static std::map<std::string, RGBA> color_map = {
                    {"red", red},
                    {"green", green},
                    {"blue", blue},
                    {"yellow", yellow},
                    {"cyan", cyan},
                    {"magenta", magenta},
                    {"orange", orange},
                    {"purple", purple},
                    {"white", white},
                    {"black", black},
                    {"gray", gray},
                    {"grey", gray}};

                auto it = color_map.find(name);
                if (it != color_map.end())
                    return it->second;
                return white; // Default
            }

            /**
             * @brief Color palette for multiple plots (auto-cycle)
             */
            inline const std::vector<RGBA> &palette()
            {
                static std::vector<RGBA> p = {
                    RGBA(31, 119, 180),  // Tableau blue
                    RGBA(255, 127, 14),  // Tableau orange
                    RGBA(44, 160, 44),   // Tableau green
                    RGBA(214, 39, 40),   // Tableau red
                    RGBA(148, 103, 189), // Tableau purple
                    RGBA(140, 86, 75),   // Tableau brown
                    RGBA(227, 119, 194), // Tableau pink
                    RGBA(127, 127, 127), // Tableau gray
                    RGBA(188, 189, 34),  // Tableau olive
                    RGBA(23, 190, 207)   // Tableau cyan
                };
                return p;
            }
        }

        // ==================== Pixel Font for Braille Text ====================

        /**
         * @brief Tiny 3x5 pixel font for rendering text in Braille graphics
         *
         * Each glyph is stored as 5 rows of 3-bit patterns (left to right).
         * This is a very compact font suitable for axis labels.
         */
        namespace font
        {
            // Each char is 3 pixels wide x 5 pixels tall
            // Stored as array of 5 bytes, each representing a row
            // Bit 2 = leftmost pixel, bit 0 = rightmost pixel
            struct Glyph
            {
                uint8_t rows[5];
            };

            // Simple 3x5 pixel font (subset for labels)
            inline const std::map<char, Glyph> &get_font()
            {
                static std::map<char, Glyph> font = {
                    // Numbers (most important for axis labels)
                    {'0', {0b111, 0b101, 0b101, 0b101, 0b111}},
                    {'1', {0b010, 0b110, 0b010, 0b010, 0b111}},
                    {'2', {0b111, 0b001, 0b111, 0b100, 0b111}},
                    {'3', {0b111, 0b001, 0b111, 0b001, 0b111}},
                    {'4', {0b101, 0b101, 0b111, 0b001, 0b001}},
                    {'5', {0b111, 0b100, 0b111, 0b001, 0b111}},
                    {'6', {0b111, 0b100, 0b111, 0b101, 0b111}},
                    {'7', {0b111, 0b001, 0b001, 0b001, 0b001}},
                    {'8', {0b111, 0b101, 0b111, 0b101, 0b111}},
                    {'9', {0b111, 0b101, 0b111, 0b001, 0b111}},
                    // Letters
                    {'A', {0b010, 0b101, 0b111, 0b101, 0b101}},
                    {'B', {0b110, 0b101, 0b110, 0b101, 0b110}},
                    {'C', {0b011, 0b100, 0b100, 0b100, 0b011}},
                    {'D', {0b110, 0b101, 0b101, 0b101, 0b110}},
                    {'E', {0b111, 0b100, 0b110, 0b100, 0b111}},
                    {'F', {0b111, 0b100, 0b110, 0b100, 0b100}},
                    {'G', {0b011, 0b100, 0b101, 0b101, 0b011}},
                    {'H', {0b101, 0b101, 0b111, 0b101, 0b101}},
                    {'I', {0b111, 0b010, 0b010, 0b010, 0b111}},
                    {'J', {0b001, 0b001, 0b001, 0b101, 0b010}},
                    {'K', {0b101, 0b110, 0b100, 0b110, 0b101}},
                    {'L', {0b100, 0b100, 0b100, 0b100, 0b111}},
                    {'M', {0b101, 0b111, 0b101, 0b101, 0b101}},
                    {'N', {0b101, 0b111, 0b111, 0b101, 0b101}},
                    {'O', {0b010, 0b101, 0b101, 0b101, 0b010}},
                    {'P', {0b110, 0b101, 0b110, 0b100, 0b100}},
                    {'Q', {0b010, 0b101, 0b101, 0b111, 0b011}},
                    {'R', {0b110, 0b101, 0b110, 0b101, 0b101}},
                    {'S', {0b011, 0b100, 0b010, 0b001, 0b110}},
                    {'T', {0b111, 0b010, 0b010, 0b010, 0b010}},
                    {'U', {0b101, 0b101, 0b101, 0b101, 0b010}},
                    {'V', {0b101, 0b101, 0b101, 0b010, 0b010}},
                    {'W', {0b101, 0b101, 0b101, 0b111, 0b101}},
                    {'X', {0b101, 0b101, 0b010, 0b101, 0b101}},
                    {'Y', {0b101, 0b101, 0b010, 0b010, 0b010}},
                    {'Z', {0b111, 0b001, 0b010, 0b100, 0b111}},
                    // Lowercase (same as uppercase for simplicity)
                    {'a', {0b010, 0b101, 0b111, 0b101, 0b101}},
                    {'b', {0b110, 0b101, 0b110, 0b101, 0b110}},
                    {'c', {0b011, 0b100, 0b100, 0b100, 0b011}},
                    {'d', {0b110, 0b101, 0b101, 0b101, 0b110}},
                    {'e', {0b111, 0b100, 0b110, 0b100, 0b111}},
                    {'f', {0b111, 0b100, 0b110, 0b100, 0b100}},
                    {'g', {0b011, 0b100, 0b101, 0b101, 0b011}},
                    {'h', {0b101, 0b101, 0b111, 0b101, 0b101}},
                    {'i', {0b111, 0b010, 0b010, 0b010, 0b111}},
                    {'j', {0b001, 0b001, 0b001, 0b101, 0b010}},
                    {'k', {0b101, 0b110, 0b100, 0b110, 0b101}},
                    {'l', {0b100, 0b100, 0b100, 0b100, 0b111}},
                    {'m', {0b101, 0b111, 0b101, 0b101, 0b101}},
                    {'n', {0b101, 0b111, 0b111, 0b101, 0b101}},
                    {'o', {0b010, 0b101, 0b101, 0b101, 0b010}},
                    {'p', {0b110, 0b101, 0b110, 0b100, 0b100}},
                    {'q', {0b010, 0b101, 0b101, 0b111, 0b011}},
                    {'r', {0b110, 0b101, 0b110, 0b101, 0b101}},
                    {'s', {0b011, 0b100, 0b010, 0b001, 0b110}},
                    {'t', {0b111, 0b010, 0b010, 0b010, 0b010}},
                    {'u', {0b101, 0b101, 0b101, 0b101, 0b010}},
                    {'v', {0b101, 0b101, 0b101, 0b010, 0b010}},
                    {'w', {0b101, 0b101, 0b101, 0b111, 0b101}},
                    {'x', {0b101, 0b101, 0b010, 0b101, 0b101}},
                    {'y', {0b101, 0b101, 0b010, 0b010, 0b010}},
                    {'z', {0b111, 0b001, 0b010, 0b100, 0b111}},
                    // Symbols
                    {' ', {0b000, 0b000, 0b000, 0b000, 0b000}},
                    {'.', {0b000, 0b000, 0b000, 0b000, 0b010}},
                    {',', {0b000, 0b000, 0b000, 0b010, 0b100}},
                    {':', {0b000, 0b010, 0b000, 0b010, 0b000}},
                    {'-', {0b000, 0b000, 0b111, 0b000, 0b000}},
                    {'+', {0b000, 0b010, 0b111, 0b010, 0b000}},
                    {'=', {0b000, 0b111, 0b000, 0b111, 0b000}},
                    {'(', {0b001, 0b010, 0b010, 0b010, 0b001}},
                    {')', {0b100, 0b010, 0b010, 0b010, 0b100}},
                    {'*', {0b000, 0b101, 0b010, 0b101, 0b000}},
                    {'/', {0b001, 0b001, 0b010, 0b100, 0b100}},
                    {'_', {0b000, 0b000, 0b000, 0b000, 0b111}},
                };
                return font;
            }

            /**
             * @brief Get text width in pixels
             */
            inline int text_width(const std::string &text)
            {
                // Each char is 3 pixels wide + 1 pixel spacing
                return static_cast<int>(text.length()) * 4 - 1;
            }

            /**
             * @brief Get text height in pixels
             */
            inline constexpr int text_height()
            {
                return 5; // All glyphs are 5 pixels tall
            }
        }

        // ==================== Axis Range ====================

        /**
         * @brief Axis range with min/max values
         */
        struct Range
        {
            double min, max;

            Range() : min(-10), max(10) {}
            Range(double min_, double max_) : min(min_), max(max_) {}

            double span() const { return max - min; }
            double center() const { return (min + max) / 2.0; }

            /**
             * @brief Expand range by a factor (for margins)
             */
            Range expand(double factor) const
            {
                double center = this->center();
                double half_span = span() / 2.0 * factor;
                return Range(center - half_span, center + half_span);
            }

            /**
             * @brief Include a value in the range
             */
            void include(double value)
            {
                if (std::isfinite(value))
                {
                    min = std::min(min, value);
                    max = std::max(max, value);
                }
            }

            /**
             * @brief Create a nice range for axis (with round numbers)
             */
            static Range nice(double data_min, double data_max, int num_ticks = 5)
            {
                double range = data_max - data_min;
                if (range == 0)
                    range = 1;

                // Find a nice tick spacing
                double rough_tick = range / (num_ticks - 1);
                double pow10 = std::pow(10, std::floor(std::log10(rough_tick)));
                double nice_tick;

                double normalized = rough_tick / pow10;
                if (normalized < 1.5)
                    nice_tick = 1 * pow10;
                else if (normalized < 3)
                    nice_tick = 2 * pow10;
                else if (normalized < 7)
                    nice_tick = 5 * pow10;
                else
                    nice_tick = 10 * pow10;

                double nice_min = std::floor(data_min / nice_tick) * nice_tick;
                double nice_max = std::ceil(data_max / nice_tick) * nice_tick;

                return Range(nice_min, nice_max);
            }
        };

        // ==================== Plot Data ====================

        /**
         * @brief Single plot data (points and style)
         */
        struct PlotData
        {
            std::vector<double> x_data;
            std::vector<double> y_data;
            RGBA color;
            std::string label;
            int line_width;
            bool show_points;

            PlotData() : color(colors::white), line_width(1), show_points(false) {}
        };

        // ==================== Variable System (Desmos-style) ====================

        /**
         * @brief A variable that can be adjusted (like Desmos sliders)
         */
        class Variable
        {
        private:
            std::string _name;
            double _value;
            double _min, _max;
            double _step;

        public:
            Variable(const std::string &name = "x", double value = 0,
                     double min = -10, double max = 10, double step = 0.1)
                : _name(name), _value(value), _min(min), _max(max), _step(step) {}

            const std::string &name() const { return _name; }
            double value() const { return _value; }
            double min() const { return _min; }
            double max() const { return _max; }
            double step() const { return _step; }

            void set_value(double v) { _value = std::clamp(v, _min, _max); }
            void increment() { set_value(_value + _step); }
            void decrement() { set_value(_value - _step); }
        };

        // ==================== Figure Class ====================

        /**
         * @brief Render mode for plots
         */
        enum class PlotMode
        {
            braille_bw,      // High-resolution B&W Braille
            braille_colored, // High-resolution colored Braille
            block_colored    // Half-block colored (lower res, better color)
        };

        /**
         * @brief Main figure class for creating plots (matplotlib-style)
         *
         * Supports multiple plots, axes configuration, legends, and rendering.
         * Uses Braille characters for 8x resolution terminal graphics.
         */
        class Figure
        {
        private:
            // Canvas dimensions
            size_t _char_width;
            size_t _char_height;
            size_t _pixel_width;  // For Braille: char_width * 2
            size_t _pixel_height; // For Braille: char_height * 4

            // Margins (in pixels)
            int _margin_left;
            int _margin_right;
            int _margin_top;
            int _margin_bottom;

            // Plot area (in pixels)
            int _plot_x0, _plot_y0;
            int _plot_x1, _plot_y1;
            size_t _plot_width, _plot_height;

            // Axis ranges
            Range _x_range;
            Range _y_range;
            bool _auto_scale;

            // Plot data
            std::vector<PlotData> _plots;
            size_t _color_index;

            // High-resolution pixel storage for colored Braille
            // Each pixel stores RGBA for proper blending
            std::vector<std::vector<RGBA>> _pixels;

            // Labels
            std::string _title;
            std::string _x_label;
            std::string _y_label;
            bool _show_legend;

            // Grid
            bool _show_grid;
            RGBA _grid_color;
            RGBA _axis_color;
            RGBA _bg_color;

            // Variables for dynamic plots
            std::map<std::string, Variable> _variables;
            double _time;

            // Render mode
            PlotMode _mode;

            // Text annotations (drawn after axes/plots are set up)
            struct TextAnnotation
            {
                std::string text;
                double x, y; // In data coordinates
                RGBA color;
            };
            std::vector<TextAnnotation> _text_annotations;

        public:
            /**
             * @brief Create a new figure
             * @param char_width Width in terminal characters
             * @param char_height Height in terminal characters
             * @param mode Rendering mode (default: braille_colored for best quality)
             */
            Figure(size_t char_width = 80, size_t char_height = 24,
                   PlotMode mode = PlotMode::braille_colored)
                : _char_width(char_width), _char_height(char_height),
                  // Braille gives us 2x width and 4x height resolution
                  _pixel_width(char_width * 2), _pixel_height(char_height * 4),
                  // Margins in pixels (scaled for Braille) - extra left margin for Y labels
                  _margin_left(20), _margin_right(6), _margin_top(10), _margin_bottom(10),
                  _x_range(-10, 10), _y_range(-10, 10), _auto_scale(true), _color_index(0),
                  // Initialize pixel buffer
                  _pixels(_pixel_height, std::vector<RGBA>(_pixel_width, RGBA(0, 0, 0, 255))),
                  _show_legend(true),
                  _show_grid(true),
                  _grid_color(30, 30, 50, 255),
                  _axis_color(80, 80, 120, 255),
                  _bg_color(0, 0, 0, 255),
                  _time(0),
                  _mode(mode)
            {
                update_plot_area();
            }

            // ==================== Configuration ====================

            /**
             * @brief Set axis ranges
             */
            Figure &xlim(double min, double max)
            {
                _x_range = Range(min, max);
                _auto_scale = false;
                return *this;
            }

            Figure &ylim(double min, double max)
            {
                _y_range = Range(min, max);
                _auto_scale = false;
                return *this;
            }

            /**
             * @brief Set labels
             */
            Figure &title(const std::string &t)
            {
                _title = t;
                return *this;
            }
            Figure &xlabel(const std::string &l)
            {
                _x_label = l;
                return *this;
            }
            Figure &ylabel(const std::string &l)
            {
                _y_label = l;
                return *this;
            }

            /**
             * @brief Enable/disable grid
             */
            Figure &grid(bool show)
            {
                _show_grid = show;
                return *this;
            }

            /**
             * @brief Enable/disable legend
             */
            Figure &legend(bool show)
            {
                _show_legend = show;
                return *this;
            }

            /**
             * @brief Draw text on the figure using pixel font (rendered in Braille)
             * @param text Text to draw
             * @param x X position in data coordinates
             * @param y Y position in data coordinates
             * @param color Text color (name or RGBA)
             * @return Reference to this Figure for chaining
             *
             * Example:
             *   fig.print("Hello", 0, 5, "cyan");  // Draw "Hello" at data coords (0, 5)
             */
            Figure &print(const std::string &text, double x, double y,
                          const std::string &color = "white")
            {
                RGBA c = colors::from_name(color);
                // Store text for later drawing (after ranges are finalized)
                _text_annotations.push_back({text, x, y, c});
                return *this;
            }

            /**
             * @brief Draw text on the figure with RGBA color
             */
            Figure &print(const std::string &text, double x, double y, const RGBA &color)
            {
                _text_annotations.push_back({text, x, y, color});
                return *this;
            }

            /**
             * @brief Add a variable (Desmos-style slider)
             */
            Figure &add_variable(const std::string &name, double value,
                                 double min = -10, double max = 10, double step = 0.1)
            {
                _variables[name] = Variable(name, value, min, max, step);
                return *this;
            }

            /**
             * @brief Set variable value
             */
            Figure &set_var(const std::string &name, double value)
            {
                auto it = _variables.find(name);
                if (it != _variables.end())
                {
                    it->second.set_value(value);
                }
                return *this;
            }

            /**
             * @brief Get variable value
             */
            double get_var(const std::string &name) const
            {
                auto it = _variables.find(name);
                if (it != _variables.end())
                {
                    return it->second.value();
                }
                return 0;
            }

            /**
             * @brief Set time value (for animations)
             */
            Figure &set_time(double t)
            {
                _time = t;
                return *this;
            }

            // ==================== Plotting Functions ====================

            /**
             * @brief Plot a function y = f(x)
             *
             * @param f Function taking double x, returning double y
             * @param x_min X range minimum
             * @param x_max X range maximum
             * @param color Plot color (name or RGBA)
             * @param label Legend label
             * @param num_points Number of sample points
             */
            template <typename Func>
            Figure &plot(Func &&f, double x_min, double x_max,
                         const std::string &color = "",
                         const std::string &label = "",
                         int num_points = 500)
            {
                PlotData data;
                data.color = color.empty() ? next_color() : colors::from_name(color);
                data.label = label;

                double step = (x_max - x_min) / num_points;
                for (int i = 0; i <= num_points; ++i)
                {
                    double x = x_min + i * step;
                    double y;

                    // Call function (handle both regular functions and var lambdas)
                    if constexpr (std::is_invocable_v<Func, double>)
                    {
                        y = static_cast<double>(f(x));
                    }
                    else if constexpr (std::is_invocable_v<Func, var>)
                    {
                        var result = f(var(x));
                        y = result.template get<double>();
                    }
                    else
                    {
                        y = x; // Fallback
                    }

                    if (std::isfinite(y))
                    {
                        data.x_data.push_back(x);
                        data.y_data.push_back(y);
                    }
                    else
                    {
                        // Break the line for discontinuities
                        if (!data.x_data.empty())
                        {
                            _plots.push_back(data);
                            data.x_data.clear();
                            data.y_data.clear();
                        }
                    }
                }

                if (!data.x_data.empty())
                {
                    _plots.push_back(data);
                }

                if (_auto_scale)
                {
                    update_ranges();
                }

                return *this;
            }

            /**
             * @brief Plot a time-varying function y = f(t, x)
             *
             * Used for animations where 't' is the time variable.
             */
            template <typename Func>
            Figure &plot_animated(Func &&f, double x_min, double x_max,
                                  const std::string &color = "",
                                  int num_points = 500)
            {
                PlotData data;
                data.color = color.empty() ? next_color() : colors::from_name(color);

                double step = (x_max - x_min) / num_points;
                for (int i = 0; i <= num_points; ++i)
                {
                    double x = x_min + i * step;
                    double y = f(_time, x);

                    if (std::isfinite(y))
                    {
                        data.x_data.push_back(x);
                        data.y_data.push_back(y);
                    }
                }

                if (!data.x_data.empty())
                {
                    _plots.push_back(data);
                }

                return *this;
            }

            /**
             * @brief Plot a parametric curve (x(t), y(t))
             */
            template <typename FuncX, typename FuncY>
            Figure &parametric(FuncX &&fx, FuncY &&fy, double t_min, double t_max,
                               const std::string &color = "",
                               const std::string &label = "",
                               int num_points = 500)
            {
                PlotData data;
                data.color = color.empty() ? next_color() : colors::from_name(color);
                data.label = label;

                double step = (t_max - t_min) / num_points;
                for (int i = 0; i <= num_points; ++i)
                {
                    double t = t_min + i * step;
                    double x = fx(t);
                    double y = fy(t);

                    if (std::isfinite(x) && std::isfinite(y))
                    {
                        data.x_data.push_back(x);
                        data.y_data.push_back(y);
                    }
                }

                if (!data.x_data.empty())
                {
                    _plots.push_back(data);
                }

                if (_auto_scale)
                {
                    update_ranges();
                }

                return *this;
            }

            /**
             * @brief Scatter plot from data arrays
             */
            Figure &scatter(const std::vector<double> &x, const std::vector<double> &y,
                            const std::string &color = "",
                            const std::string &label = "")
            {
                PlotData data;
                data.color = color.empty() ? next_color() : colors::from_name(color);
                data.label = label;
                data.show_points = true;
                data.x_data = x;
                data.y_data = y;

                _plots.push_back(data);

                if (_auto_scale)
                {
                    update_ranges();
                }

                return *this;
            }

            /**
             * @brief Plot from pythonic var lists
             */
            Figure &scatter(const var &x_list, const var &y_list,
                            const std::string &color = "")
            {
                std::vector<double> x_data, y_data;

                for (const auto &item : x_list)
                {
                    x_data.push_back(item.template get<double>());
                }
                for (const auto &item : y_list)
                {
                    y_data.push_back(item.template get<double>());
                }

                return scatter(x_data, y_data, color);
            }

            /**
             * @brief Clear all plots
             */
            Figure &clear()
            {
                _plots.clear();
                _color_index = 0;
                _auto_scale = true;
                return *this;
            }

            // ==================== Rendering ====================

            /**
             * @brief Set a pixel in the buffer with bounds checking
             */
            void set_pixel(int x, int y, const RGBA &color)
            {
                if (x >= 0 && x < _pixel_width && y >= 0 && y < _pixel_height)
                {
                    _pixels[y][x] = color.blend_over(_pixels[y][x]);
                }
            }

            /**
             * @brief Set a pixel with solid color (no blending)
             */
            void set_pixel_solid(int x, int y, const RGBA &color)
            {
                if (x >= 0 && x < _pixel_width && y >= 0 && y < _pixel_height)
                {
                    _pixels[y][x] = color;
                }
            }

            /**
             * @brief Get a pixel from the buffer
             */
            RGBA get_pixel(int x, int y) const
            {
                if (x >= 0 && x < _pixel_width && y >= 0 && y < _pixel_height)
                {
                    return _pixels[y][x];
                }
                return _bg_color;
            }

            /**
             * @brief Clear the pixel buffer
             */
            void clear_pixels()
            {
                for (auto &row : _pixels)
                {
                    std::fill(row.begin(), row.end(), _bg_color);
                }
            }

            /**
             * @brief Render the figure and output to stdout
             *
             * Use render_to_string() if you need the output as a string.
             */
            void render()
            {
                std::cout << render_to_string() << std::flush;
            }

            /**
             * @brief Display the figure to stdout (alias for render())
             */
            void show()
            {
                render();
            }

            /**
             * @brief Render the figure to a string using colored Braille
             * @return The rendered figure as a string
             */
            std::string render_to_string()
            {
                clear_pixels();

                // Draw grid if enabled
                if (_show_grid)
                {
                    draw_grid();
                }

                // Draw axes
                draw_axes();

                // Draw all plots
                for (const auto &plot : _plots)
                {
                    draw_plot(plot);
                }

                // Draw labels in the pixel buffer (axis labels only, not legend/title)
                draw_labels_to_pixels();

                // Build output: header (title + legend) then Braille graph
                std::ostringstream result;

                // Render title and legend as text above the graph
                result << render_header();

                // Now render the pixel buffer to colored Braille characters
                result << render_braille();

                return result.str();
            }

            /**
             * @brief Render the header (title and legend) as formatted text
             */
            std::string render_header()
            {
                std::ostringstream out;

                // Title
                if (!_title.empty())
                {
                    int padding = (static_cast<int>(_char_width) - static_cast<int>(_title.length())) / 2;
                    out << std::string(std::max(0, padding), ' ')
                        << "\033[1;37m" << _title << "\033[0m\n\n";
                }

                // Legend entries (above the graph)
                if (_show_legend)
                {
                    std::vector<std::pair<std::string, RGBA>> legend_entries;
                    std::set<std::string> seen_labels;
                    for (const auto &plot : _plots)
                    {
                        if (!plot.label.empty() && seen_labels.find(plot.label) == seen_labels.end())
                        {
                            legend_entries.push_back({plot.label, plot.color});
                            seen_labels.insert(plot.label);
                        }
                    }

                    // Render each legend entry
                    for (const auto &entry : legend_entries)
                    {
                        out << "  \033[38;2;" << (int)entry.second.r << ";"
                            << (int)entry.second.g << ";" << (int)entry.second.b << "m"
                            << "━━━━ \033[0m" << entry.first << "\n";
                    }

                    if (!legend_entries.empty())
                    {
                        out << "\n";
                    }
                }

                return out.str();
            }

            /**
             * @brief Draw axis labels to the pixel buffer
             */
            void draw_labels_to_pixels()
            {
                // Use a gray/muted color for axis labels so they don't clash with plot colors
                RGBA label_color(100, 120, 140, 255); // Muted steel blue
                RGBA range_color(80, 100, 120, 255);  // Slightly darker for numbers

                // Draw Y label at top of Y axis (above plot area)
                if (_x_range.min <= 0 && _x_range.max >= 0)
                {
                    int y_axis_x = data_to_pixel_x(0);
                    draw_text("Y", y_axis_x - 2, _plot_y0 - 8, label_color);
                }
                else
                {
                    draw_text("Y", _plot_x0 - 2, _plot_y0 - 8, label_color);
                }

                // Draw X label at end of X axis (right of plot area)
                if (_y_range.min <= 0 && _y_range.max >= 0)
                {
                    int x_axis_y = data_to_pixel_y(0);
                    draw_text("X", _plot_x1 + 2, x_axis_y - 2, label_color);
                }
                else
                {
                    draw_text("X", _plot_x1 + 2, _plot_y1 - 3, label_color);
                }

                // Note: We don't draw "O" at origin anymore - it's unnecessary clutter

                // Draw axis range labels OUTSIDE the plot area
                std::string x_min_str = format_number(_x_range.min);
                std::string x_max_str = format_number(_x_range.max);
                std::string y_min_str = format_number(_y_range.min);
                std::string y_max_str = format_number(_y_range.max);

                // X axis range - below the plot area
                draw_text(x_min_str, _plot_x0, _plot_y1 + 2, range_color);
                draw_text(x_max_str, _plot_x1 - font::text_width(x_max_str), _plot_y1 + 2, range_color);

                // Y axis range - left of plot area (labels positioned to not overlap with plot)
                int y_label_x = _plot_x0 - font::text_width(y_max_str) - 2;
                if (y_label_x < 0)
                    y_label_x = 0;
                draw_text(y_max_str, y_label_x, _plot_y0, range_color);

                y_label_x = _plot_x0 - font::text_width(y_min_str) - 2;
                if (y_label_x < 0)
                    y_label_x = 0;
                draw_text(y_min_str, y_label_x, _plot_y1 - 5, range_color);

                // Draw user text annotations
                for (const auto &annot : _text_annotations)
                {
                    int px = data_to_pixel_x(annot.x);
                    int py = data_to_pixel_y(annot.y);
                    draw_text(annot.text, px, py - 3, annot.color);
                }
            }

            /**
             * @brief Convert pixel buffer to colored Braille string
             *
             * Braille characters use a 2x4 dot pattern per character.
             * We'll pick the dominant color for each cell.
             */
            std::string render_braille()
            {
                std::ostringstream out;

                // Braille dot positions (Unicode offset from 0x2800):
                // [0] [3]    bit 0, bit 3
                // [1] [4]    bit 1, bit 4
                // [2] [5]    bit 2, bit 5
                // [6] [7]    bit 6, bit 7
                static const int dot_map[4][2] = {
                    {0, 3}, // row 0
                    {1, 4}, // row 1
                    {2, 5}, // row 2
                    {6, 7}  // row 3
                };

                // Iterate over character cells
                for (int cy = 0; cy < static_cast<int>(_char_height); ++cy)
                {
                    RGBA prev_color(0, 0, 0, 0);
                    bool has_prev_color = false;

                    for (int cx = 0; cx < static_cast<int>(_char_width); ++cx)
                    {
                        // Pixel coordinates for this character cell
                        int px0 = cx * 2;
                        int py0 = cy * 4;

                        // Collect the pattern and dominant color
                        uint8_t pattern = 0;
                        int total_r = 0, total_g = 0, total_b = 0;
                        int active_dots = 0;

                        for (int dy = 0; dy < 4; ++dy)
                        {
                            for (int dx = 0; dx < 2; ++dx)
                            {
                                int px = px0 + dx;
                                int py = py0 + dy;

                                if (py < _pixel_height && px < _pixel_width)
                                {
                                    const RGBA &pixel = _pixels[py][px];
                                    // Consider pixel "active" if it's not the background
                                    if (pixel.a > 128 &&
                                        (pixel.r != _bg_color.r ||
                                         pixel.g != _bg_color.g ||
                                         pixel.b != _bg_color.b))
                                    {
                                        pattern |= (1 << dot_map[dy][dx]);
                                        total_r += pixel.r;
                                        total_g += pixel.g;
                                        total_b += pixel.b;
                                        active_dots++;
                                    }
                                }
                            }
                        }

                        // Output the Braille character with color
                        if (pattern != 0 && active_dots > 0)
                        {
                            // Average color of active dots
                            uint8_t avg_r = static_cast<uint8_t>(total_r / active_dots);
                            uint8_t avg_g = static_cast<uint8_t>(total_g / active_dots);
                            uint8_t avg_b = static_cast<uint8_t>(total_b / active_dots);
                            RGBA cell_color(avg_r, avg_g, avg_b);

                            // Only output color escape if color changed
                            if (!has_prev_color ||
                                cell_color.r != prev_color.r ||
                                cell_color.g != prev_color.g ||
                                cell_color.b != prev_color.b)
                            {
                                out << "\033[38;2;" << (int)avg_r << ";"
                                    << (int)avg_g << ";" << (int)avg_b << "m";
                                prev_color = cell_color;
                                has_prev_color = true;
                            }

                            // Braille character
                            char32_t braille = 0x2800 + pattern;
                            // Convert to UTF-8
                            if (braille < 0x800)
                            {
                                out << static_cast<char>(0xC0 | (braille >> 6))
                                    << static_cast<char>(0x80 | (braille & 0x3F));
                            }
                            else
                            {
                                out << static_cast<char>(0xE0 | (braille >> 12))
                                    << static_cast<char>(0x80 | ((braille >> 6) & 0x3F))
                                    << static_cast<char>(0x80 | (braille & 0x3F));
                            }
                        }
                        else
                        {
                            // Empty cell - just a space
                            out << ' ';
                        }
                    }
                    out << "\033[0m\n"; // Reset color at end of line
                }

                return out.str();
            }

            /**
             * @brief Format a number nicely for axis labels
             */
            std::string format_number(double val) const
            {
                if (val == static_cast<int>(val))
                {
                    return std::to_string(static_cast<int>(val));
                }
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << val;
                std::string s = oss.str();
                // Remove trailing zeros
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (s.back() == '.')
                    s.pop_back();
                return s;
            }

        private:
            // ==================== Internal Methods ====================

            void update_plot_area()
            {
                _plot_x0 = _margin_left;
                _plot_y0 = _margin_top;
                _plot_x1 = _pixel_width - _margin_right;
                _plot_y1 = _pixel_height - _margin_bottom;
                _plot_width = _plot_x1 - _plot_x0;
                _plot_height = _plot_y1 - _plot_y0;
            }

            RGBA next_color()
            {
                const auto &pal = colors::palette();
                RGBA c = pal[_color_index % pal.size()];
                _color_index++;
                return c;
            }

            void update_ranges()
            {
                if (_plots.empty())
                    return;

                double x_min = std::numeric_limits<double>::max();
                double x_max = std::numeric_limits<double>::lowest();
                double y_min = std::numeric_limits<double>::max();
                double y_max = std::numeric_limits<double>::lowest();

                for (const auto &plot : _plots)
                {
                    for (double x : plot.x_data)
                    {
                        x_min = std::min(x_min, x);
                        x_max = std::max(x_max, x);
                    }
                    for (double y : plot.y_data)
                    {
                        y_min = std::min(y_min, y);
                        y_max = std::max(y_max, y);
                    }
                }

                // Add margin
                _x_range = Range::nice(x_min, x_max);
                _y_range = Range::nice(y_min, y_max);
            }

            // Coordinate transformation: data -> pixel
            int data_to_pixel_x(double x) const
            {
                double normalized = (x - _x_range.min) / _x_range.span();
                return _plot_x0 + static_cast<int>(normalized * _plot_width);
            }

            int data_to_pixel_y(double y) const
            {
                double normalized = (y - _y_range.min) / _y_range.span();
                // Y is inverted in screen coordinates
                return _plot_y1 - static_cast<int>(normalized * _plot_height);
            }

            /**
             * @brief Draw text to the pixel buffer using the pixel font
             * @param text Text to draw
             * @param x Left edge X coordinate (pixels)
             * @param y Top edge Y coordinate (pixels)
             * @param color Text color
             */
            void draw_text(const std::string &text, int x, int y, const RGBA &color)
            {
                const auto &glyphs = font::get_font();
                int cursor_x = x;

                for (char c : text)
                {
                    auto it = glyphs.find(c);
                    if (it == glyphs.end())
                    {
                        // Unknown char, use space
                        cursor_x += 5;
                        continue;
                    }

                    const font::Glyph &glyph = it->second;
                    for (int row = 0; row < 5; ++row) // 3x5 font
                    {
                        uint8_t bits = glyph.rows[row];
                        for (int col = 0; col < 3; ++col) // 3 pixels wide
                        {
                            // Bit 2 is leftmost, bit 0 is rightmost
                            if (bits & (1 << (2 - col)))
                            {
                                set_pixel(cursor_x + col, y + row, color);
                            }
                        }
                    }
                    cursor_x += 4; // 3 pixels wide + 1 pixel spacing
                }
            }

            /**
             * @brief Draw a horizontal line (used for legend bars)
             * @param x0 Start X
             * @param x1 End X
             * @param y Y coordinate
             * @param color Line color
             * @param thickness Line thickness in pixels
             */
            void draw_hline(int x0, int x1, int y, const RGBA &color, int thickness = 2)
            {
                for (int dy = 0; dy < thickness; ++dy)
                {
                    for (int px = x0; px <= x1; ++px)
                    {
                        set_pixel(px, y + dy, color);
                    }
                }
            }

            void draw_grid()
            {
                // Vertical lines
                int num_ticks = 10;
                double x_step = _x_range.span() / num_ticks;
                for (int i = 0; i <= num_ticks; ++i)
                {
                    double x = _x_range.min + i * x_step;
                    int px = data_to_pixel_x(x);
                    for (int py = _plot_y0; py < _plot_y1; py += 2) // Dashed
                    {
                        set_pixel_solid(px, py, _grid_color);
                    }
                }

                // Horizontal lines
                double y_step = _y_range.span() / num_ticks;
                for (int i = 0; i <= num_ticks; ++i)
                {
                    double y = _y_range.min + i * y_step;
                    int py = data_to_pixel_y(y);
                    for (int px = _plot_x0; px < _plot_x1; px += 2) // Dashed
                    {
                        set_pixel_solid(px, py, _grid_color);
                    }
                }
            }

            void draw_axes()
            {
                // Draw x-axis if visible
                if (_y_range.min <= 0 && _y_range.max >= 0)
                {
                    int y0 = data_to_pixel_y(0);
                    for (int px = _plot_x0; px < _plot_x1; ++px)
                    {
                        set_pixel_solid(px, y0, _axis_color);
                    }
                }

                // Draw y-axis if visible
                if (_x_range.min <= 0 && _x_range.max >= 0)
                {
                    int x0 = data_to_pixel_x(0);
                    for (int py = _plot_y0; py < _plot_y1; ++py)
                    {
                        set_pixel_solid(x0, py, _axis_color);
                    }
                }

                // Draw plot border
                RGBA border_color(80, 80, 80, 255);
                for (int px = _plot_x0; px <= _plot_x1; ++px)
                {
                    set_pixel_solid(px, _plot_y0, border_color);
                    set_pixel_solid(px, _plot_y1, border_color);
                }
                for (int py = _plot_y0; py <= _plot_y1; ++py)
                {
                    set_pixel_solid(_plot_x0, py, border_color);
                    set_pixel_solid(_plot_x1, py, border_color);
                }
            }

            void draw_plot(const PlotData &plot)
            {
                if (plot.x_data.empty())
                    return;

                // Draw lines connecting points
                for (size_t i = 1; i < plot.x_data.size(); ++i)
                {
                    int x0 = data_to_pixel_x(plot.x_data[i - 1]);
                    int y0 = data_to_pixel_y(plot.y_data[i - 1]);
                    int x1 = data_to_pixel_x(plot.x_data[i]);
                    int y1 = data_to_pixel_y(plot.y_data[i]);

                    // Draw line using anti-aliasing
                    draw_line_aa(x0, y0, x1, y1, plot.color);
                }

                // Draw points if enabled
                if (plot.show_points)
                {
                    for (size_t i = 0; i < plot.x_data.size(); ++i)
                    {
                        int px = data_to_pixel_x(plot.x_data[i]);
                        int py = data_to_pixel_y(plot.y_data[i]);

                        if (px >= _plot_x0 && px <= _plot_x1 &&
                            py >= _plot_y0 && py <= _plot_y1)
                        {
                            // Draw a small filled circle for each point
                            for (int dy = -3; dy <= 3; ++dy)
                            {
                                for (int dx = -3; dx <= 3; ++dx)
                                {
                                    if (dx * dx + dy * dy <= 9)
                                    {
                                        set_pixel(px + dx, py + dy, plot.color);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            /**
             * @brief Draw anti-aliased line using Wu's algorithm
             */
            void draw_line_aa(int x0, int y0, int x1, int y1, const RGBA &color)
            {
                bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
                if (steep)
                {
                    std::swap(x0, y0);
                    std::swap(x1, y1);
                }
                if (x0 > x1)
                {
                    std::swap(x0, x1);
                    std::swap(y0, y1);
                }

                double dx = x1 - x0;
                double dy = y1 - y0;
                double gradient = (dx == 0) ? 1.0 : dy / dx;

                // Lambda to plot a pixel with alpha blending
                auto plot_pixel = [&](int x, int y, double brightness)
                {
                    if (steep)
                        std::swap(x, y);
                    if (x >= _plot_x0 && x <= _plot_x1 && y >= _plot_y0 && y <= _plot_y1)
                    {
                        RGBA blended(color.r, color.g, color.b,
                                     static_cast<uint8_t>(color.a * brightness));
                        set_pixel(x, y, blended);
                    }
                };

                // First endpoint
                double xend = std::round(x0);
                double yend = y0 + gradient * (xend - x0);
                double xgap = 1.0 - (x0 + 0.5 - std::floor(x0 + 0.5));
                int xpxl1 = static_cast<int>(xend);
                int ypxl1 = static_cast<int>(std::floor(yend));

                plot_pixel(xpxl1, ypxl1, (1.0 - (yend - std::floor(yend))) * xgap);
                plot_pixel(xpxl1, ypxl1 + 1, (yend - std::floor(yend)) * xgap);
                double intery = yend + gradient;

                // Second endpoint
                xend = std::round(x1);
                yend = y1 + gradient * (xend - x1);
                xgap = x1 + 0.5 - std::floor(x1 + 0.5);
                int xpxl2 = static_cast<int>(xend);
                int ypxl2 = static_cast<int>(std::floor(yend));

                plot_pixel(xpxl2, ypxl2, (1.0 - (yend - std::floor(yend))) * xgap);
                plot_pixel(xpxl2, ypxl2 + 1, (yend - std::floor(yend)) * xgap);

                // Main loop
                for (int x = xpxl1 + 1; x < xpxl2; ++x)
                {
                    int y = static_cast<int>(std::floor(intery));
                    double frac = intery - std::floor(intery);
                    plot_pixel(x, y, 1.0 - frac);
                    plot_pixel(x, y + 1, frac);
                    intery += gradient;
                }
            }

            // draw_labels is now handled outside - Braille is for pure graphics
        };

        // ==================== Animation Support ====================

        /**
         * @brief Animate a time-varying plot with optional dependency functions
         *
         * Supports both simple animations and complex multi-parameter animations:
         *
         * Simple usage:
         *   animate([](double t, double x) { return sin(x + t); }, -PI, PI);
         *
         * With dependencies:
         *   animate(
         *       [](double t, double x, double a, double b) { return a * sin(x) + b * cos(t); },
         *       x_min, x_max, duration, fps, width, height,
         *       [](double t) { return 1.0 + 0.5 * sin(t); },     // dep1 -> a
         *       [](double t) { return cos(2 * t); }               // dep2 -> b
         *   );
         *
         * @param f Main function f(t, x, deps...) taking time, x variable, and optional dependency values
         * @param x_min X range minimum
         * @param x_max X range maximum
         * @param duration Animation duration in seconds
         * @param fps Frames per second
         * @param width Figure width in characters
         * @param height Figure height in characters
         * @param deps... Optional dependency functions, each takes t and returns a value
         */
        template <typename MainFunc, typename... DepFuncs>
        inline void animate(MainFunc &&f, double x_min, double x_max,
                            double duration = 10.0, double fps = 30,
                            int width = 80, int height = 24,
                            DepFuncs &&...deps)
        {
            Figure fig(width, height);
            fig.xlim(x_min, x_max);

            // Helper to evaluate all dependency functions
            auto eval_deps = [&deps...](double t)
            {
                return std::make_tuple(deps(t)...);
            };

            // First pass: estimate y range by sampling over time and x
            double y_min = std::numeric_limits<double>::max();
            double y_max = std::numeric_limits<double>::lowest();

            for (double t = 0; t <= duration; t += 0.5)
            {
                auto dep_values = eval_deps(t);
                for (double x = x_min; x <= x_max; x += (x_max - x_min) / 100)
                {
                    // Apply f with unpacked dependency values
                    double y = std::apply([&](auto... dvals)
                                          { return f(t, x, dvals...); }, dep_values);

                    if (std::isfinite(y))
                    {
                        y_min = std::min(y_min, y);
                        y_max = std::max(y_max, y);
                    }
                }
            }
            fig.ylim(y_min - 0.1 * (y_max - y_min), y_max + 0.1 * (y_max - y_min));

            // Hide cursor
            std::cout << "\033[?25l" << std::flush;

            auto frame_time = std::chrono::microseconds(static_cast<int>(1000000.0 / fps));
            auto start_time = std::chrono::steady_clock::now();

            try
            {
                while (true)
                {
                    auto now = std::chrono::steady_clock::now();
                    double t = std::chrono::duration<double>(now - start_time).count();

                    if (t > duration)
                    {
                        t = std::fmod(t, duration); // Loop
                    }

                    fig.clear();
                    fig.set_time(t);

                    // Evaluate dependency values at this time
                    auto dep_values = eval_deps(t);

                    // Create a wrapper function for plot_animated that captures dep_values
                    auto f_at_t = [&f, &dep_values](double t_, double x)
                    {
                        return std::apply([&](auto... dvals)
                                          { return f(t_, x, dvals...); }, dep_values);
                    };

                    fig.plot_animated(f_at_t, x_min, x_max, "cyan");

                    std::cout << "\033[H" << fig.render_to_string();
                    std::cout << "\nt = " << std::fixed << std::setprecision(2) << t
                              << "s (Press Ctrl+C to stop)" << std::flush;

                    std::this_thread::sleep_for(frame_time);
                }
            }
            catch (...)
            {
            }

            // Show cursor
            std::cout << "\033[?25h" << std::flush;
        }

        /**
         * @brief Animate with multiple plot functions rendered together
         *
         * Each entry is a tuple of (function, color_name)
         * All functions receive the same t and x values.
         *
         * Example:
         *   animate_plots(x_min, x_max, duration, fps, width, height,
         *       std::make_tuple([](double t, double x) { return sin(x + t); }, "red"),
         *       std::make_tuple([](double t, double x) { return cos(x - t); }, "blue")
         *   );
         */
        template <typename... PlotEntries>
        inline void animate_plots(double x_min, double x_max,
                                  double duration = 10.0, double fps = 30,
                                  int width = 80, int height = 24,
                                  PlotEntries &&...plots)
        {
            Figure fig(width, height);
            fig.xlim(x_min, x_max);

            // First pass: estimate y range across all plot functions
            double y_min = std::numeric_limits<double>::max();
            double y_max = std::numeric_limits<double>::lowest();

            auto sample_range = [&](auto &&plot_tuple)
            {
                auto &f = std::get<0>(plot_tuple);
                for (double t = 0; t <= duration; t += 0.5)
                {
                    for (double x = x_min; x <= x_max; x += (x_max - x_min) / 100)
                    {
                        double y = f(t, x);
                        if (std::isfinite(y))
                        {
                            y_min = std::min(y_min, y);
                            y_max = std::max(y_max, y);
                        }
                    }
                }
            };

            // Sample each plot function
            (sample_range(plots), ...);

            fig.ylim(y_min - 0.1 * (y_max - y_min), y_max + 0.1 * (y_max - y_min));

            // Hide cursor
            std::cout << "\033[?25l" << std::flush;

            auto frame_time = std::chrono::microseconds(static_cast<int>(1000000.0 / fps));
            auto start_time = std::chrono::steady_clock::now();

            try
            {
                while (true)
                {
                    auto now = std::chrono::steady_clock::now();
                    double t = std::chrono::duration<double>(now - start_time).count();

                    if (t > duration)
                    {
                        t = std::fmod(t, duration); // Loop
                    }

                    fig.clear();
                    fig.set_time(t);

                    // Plot each function with its color
                    auto plot_one = [&](auto &&plot_tuple)
                    {
                        auto &f = std::get<0>(plot_tuple);
                        const auto &color = std::get<1>(plot_tuple);
                        fig.plot_animated(f, x_min, x_max, color);
                    };

                    (plot_one(plots), ...);

                    std::cout << "\033[H" << fig.render_to_string();
                    std::cout << "\nt = " << std::fixed << std::setprecision(2) << t
                              << "s (Press Ctrl+C to stop)" << std::flush;

                    std::this_thread::sleep_for(frame_time);
                }
            }
            catch (...)
            {
            }

            // Show cursor
            std::cout << "\033[?25h" << std::flush;
        }

        // ==================== Simple Function Wrappers ====================

        /**
         * @brief Quick plot of a function
         */
        template <typename Func>
        inline void plot(Func &&f, double x_min, double x_max,
                         const std::string &color = "", int width = 80, int height = 24)
        {
            Figure fig(width, height);
            fig.plot(std::forward<Func>(f), x_min, x_max, color);
            fig.show();
        }

        /**
         * @brief Quick parametric plot
         */
        template <typename FuncX, typename FuncY>
        inline void parametric(FuncX &&fx, FuncY &&fy, double t_min, double t_max,
                               const std::string &color = "", int width = 80, int height = 24)
        {
            Figure fig(width, height);
            fig.parametric(std::forward<FuncX>(fx), std::forward<FuncY>(fy),
                           t_min, t_max, color);
            fig.show();
        }

        /**
         * @brief Quick scatter plot
         */
        inline void scatter(const std::vector<double> &x, const std::vector<double> &y,
                            const std::string &color = "", int width = 80, int height = 24)
        {
            Figure fig(width, height);
            fig.scatter(x, y, color);
            fig.show();
        }

        // ==================== Pythonic var Integration ====================

        /**
         * @brief Plot a var lambda function
         *
         * Converts pythonic var types to raw C++ types for speed.
         */
        inline void plot(const var &lambda_func, const var &x_min, const var &x_max,
                         const std::string &color = "", int width = 80, int height = 24)
        {
            double xmin = x_min.get<double>();
            double xmax = x_max.get<double>();

            // Wrap the var lambda to extract double result
            auto wrapped = [&lambda_func](double x) -> double
            {
                var result = const_cast<var &>(lambda_func)(var(x));
                return result.get<double>();
            };

            Figure fig(width, height);
            fig.plot(wrapped, xmin, xmax, color);
            fig.show();
        }

    } // namespace plot
} // namespace pythonic
