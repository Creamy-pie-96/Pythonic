#pragma once
#include <sstream>
#include <iostream>
#include "pythonicVars.hpp"
#include "pythonicDraw.hpp"
#include "pythonicMedia.hpp"

namespace pythonic
{
    namespace print
    {

        using namespace pythonic::vars;
        using pythonic::draw::Audio;  // Import Audio enum from draw namespace
        using pythonic::draw::Mode;   // Import Mode enum from draw namespace
        using pythonic::draw::Parser; // Import Parser enum from draw namespace
        using pythonic::draw::Shell;  // Import Shell enum from draw namespace

        // Legacy alias for backward compatibility
        using Render = Mode;

        /**
         * @brief Media type hints for the print function
         *
         * Usage:
         *   print("file.png", Type::image);       // Force treat as image
         *   print("file.mp4", Type::video);       // Force treat as video
         *   print("0", Type::webcam);             // Capture from webcam (requires OpenCV)
         *   print("info.mp4", Type::video_info);  // Show video info only
         *   print("file.png", Type::auto_detect); // Auto-detect from extension
         */
        enum class Type
        {
            auto_detect, // Detect from file extension (default)
            image,       // Force treat as image
            video,       // Force treat as video (play it)
            webcam,      // Capture from webcam (requires OpenCV)
            video_info,  // Show video metadata only (no playback)
            text         // Force treat as plain text
        };

        // Forward declaration for recursive pretty printing
        inline std::string format_value(const var &v, size_t indent = 0, size_t indent_step = 2, bool top_level = true);

        // Helper to format a var value with optional indentation
        inline std::string format_value(const var &v, size_t indent, size_t indent_step, bool top_level)
        {
            std::string ind(indent, ' ');
            std::string inner_ind(indent + indent_step, ' ');

            // Use the type() method to determine container type
            std::string t = v.type();

            if (t == "list")
            {
                const auto &lst = v.get<List>();
                if (lst.empty())
                    return "[]";

                // Check if it's simple (no nested containers)
                bool simple = true;
                for (const auto &item : lst)
                {
                    std::string it = item.type();
                    if (it == "list" || it == "dict" || it == "set")
                    {
                        simple = false;
                        break;
                    }
                }

                if (simple && lst.size() <= 5)
                {
                    // Compact format for simple lists
                    return v.str();
                }

                // Pretty format for complex lists
                std::ostringstream ss;
                ss << "[\n";
                for (size_t i = 0; i < lst.size(); ++i)
                {
                    ss << inner_ind << format_value(lst[i], indent + indent_step, indent_step, false);
                    if (i < lst.size() - 1)
                        ss << ",";
                    ss << "\n";
                }
                ss << ind << "]";
                return ss.str();
            }
            else if (t == "dict")
            {
                const auto &dict = v.get<Dict>();
                if (dict.empty())
                    return "{}";

                // Check if it's simple
                bool simple = true;
                for (const auto &[k, val] : dict)
                {
                    std::string vt = val.type();
                    if (vt == "list" || vt == "dict" || vt == "set")
                    {
                        simple = false;
                        break;
                    }
                }

                if (simple && dict.size() <= 3)
                {
                    return v.str();
                }

                std::ostringstream ss;
                ss << "{\n";
                size_t i = 0;
                for (const auto &[k, val] : dict)
                {
                    ss << inner_ind << "\"" << k << "\": "
                       << format_value(val, indent + indent_step, indent_step, false);
                    if (i < dict.size() - 1)
                        ss << ",";
                    ss << "\n";
                    ++i;
                }
                ss << ind << "}";
                return ss.str();
            }
            else if (t == "set")
            {
                const auto &s = v.get<Set>();
                if (s.empty())
                    return "{}";

                bool simple = true;
                for (const auto &item : s)
                {
                    std::string it = item.type();
                    if (it == "list" || it == "dict" || it == "set")
                    {
                        simple = false;
                        break;
                    }
                }

                if (simple && s.size() <= 5)
                {
                    return v.str();
                }

                std::ostringstream ss;
                ss << "{\n";
                size_t i = 0;
                for (const auto &item : s)
                {
                    ss << inner_ind << format_value(item, indent + indent_step, indent_step, false);
                    if (i < s.size() - 1)
                        ss << ",";
                    ss << "\n";
                    ++i;
                }
                ss << ind << "}";
                return ss.str();
            }
            else if (t == "str")
            {
                // Strings get quotes in containers but not at top level
                if (!top_level)
                {
                    return "\"" + v.get<std::string>() + "\"";
                }
                return v.get<std::string>();
            }
            else
            {
                return v.str();
            }
        }

        // Pretty print helper for var types
        template <typename T>
        inline std::string to_print_str(const T &arg)
        {
            std::ostringstream ss;
            ss << arg;
            return ss.str();
        }

        // Specialization for var - use simple str() formatting
        template <>
        inline std::string to_print_str<var>(const var &arg)
        {
            return arg.str(); // Use simple str() instead of format_value
        }

        // Main print function - handles any types
        template <typename... Args>
        void print(const Args &...args)
        {
            std::ostringstream ss;
            ((ss << to_print_str(args) << ' '), ...);
            std::string out = ss.str();
            if (!out.empty())
                out.pop_back();
            std::cout << out << std::endl;
        }

        // pprint - force pretty print with configurable indent
        // For graphs, this shows the 2D visualization via pretty_str()
        inline void pprint(const var &v, size_t indent_step = 2)
        {
            std::cout << v.pretty_str(0, indent_step) << std::endl;
        }

        /**
         * @brief Print an image file to the terminal using braille characters
         *
         * Supports PNG, JPG, BMP, PPM, PGM, and other common image formats.
         * Requires ImageMagick for non-PPM/PGM formats.
         *
         * @param filepath Path to the image file
         * @param max_width Maximum width in terminal characters (default: 80)
         * @param threshold Brightness threshold for binary conversion (default: 128)
         */
        inline void print_image(const std::string &filepath, int max_width = 80, int threshold = 128)
        {
            pythonic::draw::print_image(filepath, max_width, threshold);
        }

        /**
         * @brief Print with explicit media type hint, render mode, parser, and audio option
         *
         * New API (v2):
         *   print("file.png", Type::image);                                    // BW braille (default)
         *   print("file.png", Type::image, Mode::colored);                     // True color blocks
         *   print("file.png", Type::image, Mode::bw);                          // BW blocks
         *   print("file.png", Type::image, Mode::colored_dot);                 // Colored braille
         *   print("file.mp4", Type::video, Mode::bw_dot, Parser::opencv);      // Use OpenCV backend
         *   print("0", Type::webcam);                                          // Webcam capture (requires OpenCV)
         *   print("file.mp4", Type::video, Mode::bw_dot, Parser::default_parser, Audio::on);  // With audio
         *
         * Modes:
         *   Mode::bw       - Black & white using half-block characters (▀▄█)
         *   Mode::bw_dot   - Black & white using Braille patterns (default, higher resolution)
         *   Mode::colored  - True color (24-bit) using half-block characters
         *   Mode::colored_dot - True color using Braille patterns (one color per cell)
         *
         * Parsers:
         *   Parser::default_parser - FFmpeg for video, ImageMagick for images (default)
         *   Parser::opencv         - OpenCV for everything (also supports webcam)
         *
         * @param filepath Path to media file, webcam source ("0", "/dev/video0", "webcam"), or text
         * @param type Media type hint (auto_detect, image, video, webcam, video_info, text)
         * @param mode Render mode (bw_dot default for highest resolution)
         * @param parser Parser backend (default_parser or opencv)
         * @param audio Audio mode (Audio::off default, Audio::on for audio playback)
         * @param max_width Terminal width for media rendering (default: 80)
         * @param threshold Brightness threshold for BW modes (0-255, default: 128)
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume video playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop video playback (default 's', '\0' to disable)
         */
        inline void print(const std::string &filepath, Type type = Type::auto_detect,
                          Mode mode = Mode::bw_dot, Parser parser = Parser::default_parser,
                          Audio audio = Audio::off, int max_width = 80, int threshold = 128,
                          Shell shell = Shell::noninteractive,
                          char pause_key = 'p', char stop_key = 's')
        {
            // Helper to handle Pythonic format files
            auto handle_pythonic_format = [&](const std::string &path) -> std::string
            {
                if (pythonic::draw::is_pythonic_image_file(path) ||
                    pythonic::draw::is_pythonic_video_file(path))
                {
                    return pythonic::media::extract_to_temp(path);
                }
                return path;
            };

            // Helper to render image with appropriate mode and parser
            auto render_image = [&](const std::string &path)
            {
                std::string actual_path = handle_pythonic_format(path);
                bool is_temp = (actual_path != path);

                if (parser == Parser::opencv)
                {
                    pythonic::draw::print_image_opencv(actual_path, max_width, threshold, mode);
                }
                else
                {
                    // Use unified mode-aware rendering function
                    pythonic::draw::print_image_with_mode(actual_path, max_width, threshold, mode);
                }

                if (is_temp)
                    std::remove(actual_path.c_str());
            };

            // Helper to play video with appropriate mode and parser
            auto play_video_impl = [&](const std::string &path)
            {
                std::string actual_path = handle_pythonic_format(path);
                bool is_temp = (actual_path != path);

                if (parser == Parser::opencv)
                {
                    // OpenCV supports all 4 modes
                    pythonic::draw::play_video_opencv(actual_path, max_width, mode, threshold,
                                                      shell, pause_key, stop_key);
                }
                else if (audio == Audio::on)
                {
                    // Audio player supports all modes
                    pythonic::draw::play_video_audio(actual_path, max_width, mode,
                                                     shell, pause_key, stop_key);
                }
                else
                {
                    // FFmpeg-based players - use unified function for all modes
                    pythonic::draw::play_video_with_mode(actual_path, max_width, mode, threshold,
                                                         shell, pause_key, stop_key);
                }

                if (is_temp)
                    std::remove(actual_path.c_str());
            };

            switch (type)
            {
            case Type::image:
                render_image(filepath);
                break;

            case Type::video:
                play_video_impl(filepath);
                break;

            case Type::webcam:
                // Webcam always requires OpenCV
                pythonic::draw::play_webcam(filepath, max_width, mode, threshold, shell, pause_key, stop_key);
                break;

            case Type::video_info:
                pythonic::draw::print_video_info(filepath);
                break;

            case Type::text:
                std::cout << filepath << std::endl;
                break;

            case Type::auto_detect:
            default:
                // Check for webcam source first
                if (pythonic::draw::is_webcam_source(filepath))
                {
                    pythonic::draw::play_webcam(filepath, max_width, mode, threshold, shell, pause_key, stop_key);
                }
                else if (pythonic::draw::is_video_file(filepath))
                {
                    play_video_impl(filepath);
                }
                else if (pythonic::draw::is_image_file(filepath))
                {
                    render_image(filepath);
                }
                else
                {
                    // Not a media file, just print as text
                    std::cout << filepath << std::endl;
                }
                break;
            }
        }

        // Overload for const char*
        inline void print(const char *filepath, Type type = Type::auto_detect,
                          Mode mode = Mode::bw_dot, Parser parser = Parser::default_parser,
                          Audio audio = Audio::off, int max_width = 80, int threshold = 128)
        {
            print(std::string(filepath), type, mode, parser, audio, max_width, threshold);
        }

        /**
         * @brief Specialized print for media files (std::string overload)
         */
        inline void print(const std::string &filepath)
        {
            print(filepath, Type::auto_detect);
        }

    } // namespace print
} // namespace pythonic