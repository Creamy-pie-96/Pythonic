#pragma once
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include "pythonicVars.hpp"
#include "pythonicDraw.hpp"
#include "pythonicMedia.hpp"
#include "pythonicExport.hpp"

namespace pythonic
{
    namespace print
    {

        // ==================== Export Progress Bar ====================

        /**
         * @brief Visual progress bar for video export using Braille graphics
         *
         * Displays progress percentage, elapsed time, estimated time remaining,
         * and a visual progress bar using Unicode block characters.
         */
        class ExportProgress
        {
        private:
            size_t _total_frames;
            size_t _current_frame;
            std::chrono::steady_clock::time_point _start_time;
            int _bar_width;
            std::string _stage;

        public:
            ExportProgress(size_t total_frames, int bar_width = 40)
                : _total_frames(total_frames), _current_frame(0),
                  _bar_width(bar_width), _stage("Initializing...")
            {
                _start_time = std::chrono::steady_clock::now();
            }

            void set_stage(const std::string &stage) { _stage = stage; }
            void set_total(size_t total) { _total_frames = total; }

            void update(size_t frame)
            {
                _current_frame = frame;
                render();
            }

            void increment()
            {
                _current_frame++;
                render();
            }

            void finish()
            {
                _current_frame = _total_frames;
                _stage = "Complete!";
                render();
                std::cout << std::endl;
            }

        private:
            std::string format_time(double seconds) const
            {
                int hours = static_cast<int>(seconds) / 3600;
                int minutes = (static_cast<int>(seconds) % 3600) / 60;
                int secs = static_cast<int>(seconds) % 60;

                std::ostringstream oss;
                if (hours > 0)
                    oss << hours << "h " << minutes << "m " << secs << "s";
                else if (minutes > 0)
                    oss << minutes << "m " << secs << "s";
                else
                    oss << std::fixed << std::setprecision(1) << seconds << "s";
                return oss.str();
            }

            void render()
            {
                auto now = std::chrono::steady_clock::now();
                double elapsed = std::chrono::duration<double>(now - _start_time).count();

                // Calculate progress
                double progress = (_total_frames > 0) ? static_cast<double>(_current_frame) / _total_frames : 0.0;
                int percent = static_cast<int>(progress * 100);

                // Estimate time remaining
                double eta = 0.0;
                if (_current_frame > 0 && progress < 1.0)
                {
                    double time_per_frame = elapsed / _current_frame;
                    size_t remaining_frames = _total_frames - _current_frame;
                    eta = time_per_frame * remaining_frames;
                }

                // Build progress bar using Unicode block characters
                // ▓ (U+2593) for filled, ░ (U+2591) for empty
                int filled = static_cast<int>(progress * _bar_width);
                int empty = _bar_width - filled;

                std::ostringstream bar;
                bar << "\033[2K\r"; // Clear line and return to start

                // Stage name
                bar << "\033[36m" << _stage << "\033[0m ";

                // Progress bar
                bar << "\033[90m[\033[0m";
                bar << "\033[92m"; // Green for filled
                for (int i = 0; i < filled; i++)
                    bar << "▓";
                bar << "\033[90m"; // Gray for empty
                for (int i = 0; i < empty; i++)
                    bar << "░";
                bar << "\033[90m]\033[0m ";

                // Percentage
                bar << "\033[93m" << std::setw(3) << percent << "%\033[0m ";

                // Frame counter
                bar << "\033[90m(" << _current_frame << "/" << _total_frames << ")\033[0m ";

                // Time info
                bar << "\033[35m" << format_time(elapsed) << "\033[0m";
                if (eta > 0 && progress < 1.0)
                {
                    bar << " \033[90m| ETA:\033[0m \033[33m" << format_time(eta) << "\033[0m";
                }

                std::cout << bar.str() << std::flush;
            }
        };

        /**
         * @brief Count total frames in a directory matching a pattern
         */
        inline size_t count_frames(const std::string &dir, const std::string &pattern = "frame_")
        {
            std::string cmd = "ls -1 \"" + dir + "\" 2>/dev/null | grep \"" + pattern + "\" | wc -l";
            FILE *pipe = popen(cmd.c_str(), "r");
            if (!pipe)
                return 0;

            char buffer[64];
            std::string result;
            if (fgets(buffer, sizeof(buffer), pipe))
                result = buffer;
            pclose(pipe);

            try
            {
                return std::stoull(result);
            }
            catch (...)
            {
                return 0;
            }
        }

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

        // ==================== Export Format ====================

        /**
         * @brief Output format for export function
         */
        enum class Format
        {
            pythonic,     // Save as .pi (image) or .pv (video) - Pythonic's encrypted format
            text,         // Save as .txt (ASCII art text file)
            image,        // Save as .png image (render ASCII art to image)
            video,        // Save as .mp4 video (render ASCII art to video frames)
            normal = text // Alias for backward compatibility
        };

        /**
         * @brief Get basename without extension, truncating any given extension
         */
        inline std::string truncate_extension(const std::string &name)
        {
            // Find last dot that's not at position 0
            size_t dot = name.rfind('.');
            if (dot != std::string::npos && dot != 0)
            {
                return name.substr(0, dot);
            }
            return name;
        }

        /**
         * @brief Render an image to ASCII/braille art and return as string
         *
         * This is a helper that wraps the various render functions from pythonicDraw
         */
        inline std::string render_image_to_string(const std::string &filepath, Mode mode = Mode::bw_dot,
                                                  int max_width = 80, int threshold = 128)
        {
            // Handle Pythonic format files
            std::string actual_path = filepath;
            bool is_temp = false;
            if (pythonic::draw::is_pythonic_image_file(filepath))
            {
                actual_path = pythonic::media::extract_to_temp(filepath);
                is_temp = true;
            }

            std::string result;
            switch (mode)
            {
            case Mode::bw:
                result = pythonic::draw::render_image_bw_block(actual_path, max_width, threshold);
                break;
            case Mode::bw_dot:
                result = pythonic::draw::render_image(actual_path, max_width, threshold);
                break;
            case Mode::colored:
                result = pythonic::draw::render_image_colored(actual_path, max_width);
                break;
            case Mode::colored_dot:
                result = pythonic::draw::render_image_colored_dot(actual_path, max_width, threshold);
                break;
            }

            if (is_temp)
                std::remove(actual_path.c_str());

            return result;
        }

        /**
         * @brief Strip ANSI escape codes from a string
         */
        inline std::string strip_ansi(const std::string &str)
        {
            std::string result;
            result.reserve(str.size());
            bool in_escape = false;
            for (char c : str)
            {
                if (c == '\033')
                {
                    in_escape = true;
                }
                else if (in_escape)
                {
                    if (c == 'm')
                        in_escape = false;
                }
                else
                {
                    result += c;
                }
            }
            return result;
        }

        /**
         * @brief Get video FPS using ffprobe
         * @param filepath Path to video file
         * @return FPS as double, or 0 if unknown
         */
        inline double get_video_fps(const std::string &filepath)
        {
            std::string cmd = "ffprobe -v quiet -select_streams v:0 "
                              "-show_entries stream=r_frame_rate "
                              "-of csv=p=0 \"" +
                              filepath + "\" 2>/dev/null";

            FILE *pipe = popen(cmd.c_str(), "r");
            if (!pipe)
                return 0;

            char buffer[256];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe))
                result += buffer;
            pclose(pipe);

            // Parse: fps_num/fps_den
            size_t slash = result.find('/');
            if (slash != std::string::npos)
            {
                try
                {
                    double num = std::stod(result.substr(0, slash));
                    double den = std::stod(result.substr(slash + 1));
                    if (den > 0)
                        return num / den;
                }
                catch (...)
                {
                }
            }
            return 0;
        }

        /**
         * @brief Export media as ASCII/braille art
         *
         * Renders the image or video to terminal art (braille, blocks, colored)
         * and saves it in the specified format.
         *
         * Usage:
         *   export_media("input.png", "output");                                          // text file (default)
         *   export_media("input.png", "output", Type::image, Format::text);              // output.txt
         *   export_media("input.png", "output", Type::image, Format::image);             // output.png (ASCII as image)
         *   export_media("input.mp4", "output", Type::video, Format::video);             // output.mp4 (ASCII video)
         *   export_media("input.png", "output", Type::image, Format::pythonic);          // output.pi
         *
         * @param input_path Path to source media file
         * @param output_name Output filename (extension will be ignored/replaced)
         * @param type Media type hint (auto_detect, image, video)
         * @param format Output format (text=.txt, image=.png, video=.mp4, pythonic=.pi/.pv)
         * @param mode Render mode (bw_dot, bw, colored, colored_dot)
         * @param max_width Width for rendering (default 80)
         * @param threshold Brightness threshold for BW modes (0-255)
         * @param audio Audio mode for video export (Audio::on to include audio track)
         * @param fps Frame rate for video export (0 = use original video fps, default)
         * @param config Export configuration for customizing rendering (dot_size, density, colors)
         * @return true on success, false on failure
         */
        inline bool export_media(const std::string &input_path, const std::string &output_name,
                                 Type type = Type::auto_detect,
                                 Format format = Format::text,
                                 Mode mode = Mode::bw_dot,
                                 int max_width = 80, int threshold = 128,
                                 Audio audio = Audio::off,
                                 int fps = 0,
                                 const ex::ExportConfig &config = ex::ExportConfig())
        {
            // Truncate any extension from output_name
            std::string basename = truncate_extension(output_name);

            // Fix max_width if invalid
            if (max_width <= 0)
                max_width = 80;

            // Determine actual type from input
            Type actual_type = type;
            if (type == Type::auto_detect)
            {
                if (pythonic::draw::is_video_file(input_path) ||
                    pythonic::draw::is_pythonic_video_file(input_path))
                {
                    actual_type = Type::video;
                }
                else if (pythonic::draw::is_image_file(input_path) ||
                         pythonic::draw::is_pythonic_image_file(input_path))
                {
                    actual_type = Type::image;
                }
                else
                {
                    actual_type = Type::text;
                }
            }

            // ===================== PYTHONIC FORMAT =====================
            if (format == Format::pythonic)
            {
                try
                {
                    pythonic::media::MediaType media_type;
                    if (actual_type == Type::image)
                        media_type = pythonic::media::MediaType::image;
                    else if (actual_type == Type::video)
                        media_type = pythonic::media::MediaType::video;
                    else
                        media_type = pythonic::media::MediaType::auto_detect;

                    std::string output_ext = (actual_type == Type::video) ? ".pv" : ".pi";
                    std::string output_path = basename + output_ext;

                    // If already pythonic format, just copy
                    if (pythonic::media::is_pythonic_format(input_path))
                    {
                        std::ifstream src(input_path, std::ios::binary);
                        std::ofstream dst(output_path, std::ios::binary);
                        if (!src || !dst)
                            return false;
                        dst << src.rdbuf();
                        return dst.good();
                    }

                    // Convert to pythonic format
                    std::string result = pythonic::media::convert(input_path, media_type, true);
                    if (!result.empty() && result != output_path)
                    {
                        std::rename(result.c_str(), output_path.c_str());
                    }
                    return !result.empty();
                }
                catch (const std::exception &)
                {
                    return false;
                }
            }

            // ===================== TEXT FORMAT =====================
            if (format == Format::text || format == Format::normal)
            {
                std::string output_path = basename + ".txt";

                try
                {
                    std::string rendered;

                    if (actual_type == Type::image)
                    {
                        rendered = render_image_to_string(input_path, mode, max_width, threshold);
                    }
                    else if (actual_type == Type::video)
                    {
                        // For video, extract first frame and render
                        std::string actual_path = input_path;
                        bool is_temp = false;
                        if (pythonic::draw::is_pythonic_video_file(input_path))
                        {
                            actual_path = pythonic::media::extract_to_temp(input_path);
                            is_temp = true;
                        }

                        std::string temp_frame = "/tmp/pythonic_export_frame_" +
                                                 std::to_string(std::hash<std::string>{}(input_path)) + ".png";
                        std::string cmd = "ffmpeg -y -i \"" + actual_path + "\" -vframes 1 \"" + temp_frame + "\" 2>/dev/null";
                        int result = std::system(cmd.c_str());

                        if (is_temp)
                            std::remove(actual_path.c_str());

                        if (result != 0)
                            return false;

                        rendered = render_image_to_string(temp_frame, mode, max_width, threshold);
                        std::remove(temp_frame.c_str());
                    }
                    else
                    {
                        std::ifstream src(input_path);
                        if (!src)
                            return false;
                        std::ostringstream ss;
                        ss << src.rdbuf();
                        rendered = ss.str();
                    }

                    std::ofstream out(output_path);
                    if (!out)
                        return false;
                    out << rendered;
                    return out.good();
                }
                catch (const std::exception &)
                {
                    return false;
                }
            }

            // ===================== IMAGE FORMAT (PNG) =====================
            // Render ASCII art to an actual image file with proper Braille rendering
            if (format == Format::image)
            {
                std::string output_path = basename + ".png";

                try
                {
                    std::string rendered;
                    if (actual_type == Type::video)
                    {
                        // Extract first frame
                        std::string actual_path = input_path;
                        bool is_temp = false;
                        if (pythonic::draw::is_pythonic_video_file(input_path))
                        {
                            actual_path = pythonic::media::extract_to_temp(input_path);
                            is_temp = true;
                        }

                        std::string temp_frame = "/tmp/pythonic_export_frame_" +
                                                 std::to_string(std::hash<std::string>{}(input_path)) + ".png";
                        std::string cmd = "ffmpeg -y -i \"" + actual_path + "\" -vframes 1 \"" + temp_frame + "\" 2>/dev/null";
                        int result = std::system(cmd.c_str());

                        if (is_temp)
                            std::remove(actual_path.c_str());

                        if (result != 0)
                            return false;

                        rendered = render_image_to_string(temp_frame, mode, max_width, threshold);
                        std::remove(temp_frame.c_str());
                    }
                    else
                    {
                        rendered = render_image_to_string(input_path, mode, max_width, threshold);
                    }

                    // Use the new export module for proper Braille/ASCII rendering
                    // This properly renders each braille character as a 2×4 dot pattern
                    return pythonic::ex::export_art_to_png(rendered, output_path, config);
                }
                catch (const std::exception &)
                {
                    return false;
                }
            }

            // ===================== VIDEO FORMAT (MP4) =====================
            // Render each frame as ASCII art and create a video
            if (format == Format::video)
            {
                std::string output_path = basename + ".mp4";

                try
                {
                    // Get actual input path
                    std::string actual_path = input_path;
                    bool is_temp_video = false;
                    if (pythonic::draw::is_pythonic_video_file(input_path))
                    {
                        actual_path = pythonic::media::extract_to_temp(input_path);
                        is_temp_video = true;
                    }

                    // Determine the FPS to use
                    int actual_fps = fps;
                    if (actual_fps <= 0)
                    {
                        // Get original video fps
                        double original_fps = get_video_fps(actual_path);
                        actual_fps = (original_fps > 0) ? static_cast<int>(std::round(original_fps)) : 24;
                    }
                    // Clamp fps to reasonable range
                    if (actual_fps < 1)
                        actual_fps = 1;
                    if (actual_fps > 60)
                        actual_fps = 60;

                    std::string fps_str = std::to_string(actual_fps);

                    // Create temp directory for frames
                    std::string temp_dir = "/tmp/pythonic_video_export_" +
                                           std::to_string(std::hash<std::string>{}(input_path));
                    std::string mkdir_cmd = "mkdir -p \"" + temp_dir + "\"";
                    std::system(mkdir_cmd.c_str());

                    // Initialize progress bar (estimate frames from duration)
                    ExportProgress progress(100, 50);  // Will update after extraction
                    progress.set_stage("Extracting frames");
                    progress.update(0);

                    // Extract frames from video at the target fps
                    std::string extract_cmd = "ffmpeg -y -i \"" + actual_path + "\" -vf \"fps=" + fps_str + "\" \"" +
                                              temp_dir + "/frame_%05d.png\" 2>/dev/null";
                    int result = std::system(extract_cmd.c_str());

                    if (is_temp_video)
                        std::remove(actual_path.c_str());

                    if (result != 0)
                    {
                        std::cout << "\n\033[31mError: Failed to extract frames from video\033[0m\n";
                        std::string rm_cmd = "rm -rf \"" + temp_dir + "\"";
                        std::system(rm_cmd.c_str());
                        return false;
                    }

                    // Count total frames
                    size_t total_frames = count_frames(temp_dir, "frame_");
                    if (total_frames == 0)
                    {
                        std::cout << "\n\033[31mError: No frames extracted from video\033[0m\n";
                        std::string rm_cmd = "rm -rf \"" + temp_dir + "\"";
                        std::system(rm_cmd.c_str());
                        return false;
                    }

                    // Update progress bar with actual frame count
                    progress.set_total(total_frames);
                    progress.set_stage("Rendering ASCII art");
                    progress.update(0);

                    // Process each frame - render ASCII art to proper image
                    size_t frame_num = 1;
                    while (frame_num <= total_frames)
                    {
                        char frame_name[128];
                        snprintf(frame_name, sizeof(frame_name), "%s/frame_%05zu.png", temp_dir.c_str(), frame_num);

                        std::ifstream test(frame_name);
                        if (!test.good())
                            break;
                        test.close();

                        // Render frame to ASCII art
                        std::string rendered = render_image_to_string(frame_name, mode, max_width, threshold);

                        // Render ASCII art to proper image using new export module
                        char img_name[128];
                        snprintf(img_name, sizeof(img_name), "%s/ascii_%05zu.png", temp_dir.c_str(), frame_num);
                        pythonic::ex::export_art_to_png(rendered, img_name, config);

                        // Update progress
                        progress.update(frame_num);

                        frame_num++;
                        if (frame_num > 100000)
                            break; // Safety limit
                    }

                    // Update progress for encoding stage
                    progress.set_stage("Encoding video");
                    progress.update(total_frames);

                    // Combine ASCII frames into video
                    if (audio == Audio::on)
                    {
                        // Extract audio from source and combine with ASCII video
                        std::string audio_path = temp_dir + "/audio.aac";
                        std::string extract_audio_cmd = "ffmpeg -y -i \"" + input_path + "\" -vn -acodec aac \"" + audio_path + "\" 2>/dev/null";
                        int audio_result = std::system(extract_audio_cmd.c_str());

                        if (audio_result == 0)
                        {
                            // Create video with audio
                            std::string video_cmd = "ffmpeg -y -framerate " + fps_str + " -i \"" + temp_dir + "/ascii_%05d.png\" "
                                                                                                              "-i \"" +
                                                    audio_path + "\" -c:v libx264 -c:a aac -pix_fmt yuv420p "
                                                                 "-shortest \"" +
                                                    output_path + "\" 2>/dev/null";
                            result = std::system(video_cmd.c_str());
                        }
                        else
                        {
                            // No audio in source or extraction failed, create video without audio
                            std::string video_cmd = "ffmpeg -y -framerate " + fps_str + " -i \"" + temp_dir + "/ascii_%05d.png\" "
                                                                                                              "-c:v libx264 -pix_fmt yuv420p \"" +
                                                    output_path + "\" 2>/dev/null";
                            result = std::system(video_cmd.c_str());
                        }
                    }
                    else
                    {
                        // Create video without audio
                        std::string video_cmd = "ffmpeg -y -framerate " + fps_str + " -i \"" + temp_dir + "/ascii_%05d.png\" "
                                                                                                          "-c:v libx264 -pix_fmt yuv420p \"" +
                                                output_path + "\" 2>/dev/null";
                        result = std::system(video_cmd.c_str());
                    }

                    // Cleanup
                    std::string rm_cmd = "rm -rf \"" + temp_dir + "\"";
                    std::system(rm_cmd.c_str());

                    // Finish progress
                    if (result == 0)
                    {
                        progress.finish();
                        std::cout << "\033[32mExported to: " << output_path << "\033[0m\n";
                    }
                    else
                    {
                        std::cout << "\n\033[31mError: Failed to encode video\033[0m\n";
                    }

                    return (result == 0);
                }
                catch (const std::exception &)
                {
                    return false;
                }
            }

            return false;
        }

        /**
         * @brief Export alias with simpler signature
         */
        inline bool export_media(const std::string &input_path, const std::string &output_name)
        {
            return export_media(input_path, output_name, Type::auto_detect);
        }

        // Re-export ExportConfig from ex namespace for convenience
        using ExportConfig = pythonic::ex::ExportConfig;

        /**
         * @brief Export media with custom rendering configuration
         *
         * Allows fine-grained control over dot size, density, and colors.
         *
         * Usage:
         *   ExportConfig config;
         *   config.set_dot_size(3).set_density(4).set_background(32, 32, 32);
         *   export_media("input.png", "output", config, Type::image, Format::image);
         *
         * @param input_path Path to source media file
         * @param output_name Output filename (extension added based on format)
         * @param config Export configuration (dot_size, density, colors)
         * @param type Media type hint (auto_detect, image, video)
         * @param format Output format (text, image, video, pythonic)
         * @param mode Render mode (bw_dot, bw, colored, colored_dot)
         * @param max_width Width for ASCII rendering
         * @param threshold Brightness threshold for BW modes
         * @param audio Audio mode for video export
         * @return true on success
         */
        inline bool export_media(const std::string &input_path, const std::string &output_name,
                                 const ExportConfig &config,
                                 Type type = Type::auto_detect,
                                 Format format = Format::image,
                                 Mode mode = Mode::bw_dot,
                                 int max_width = 80, int threshold = 128,
                                 Audio audio = Audio::off)
        {
            // Truncate any extension from output_name
            std::string basename = truncate_extension(output_name);

            // Fix max_width if invalid
            if (max_width <= 0)
                max_width = 80;

            // Determine actual type from input
            Type actual_type = type;
            if (type == Type::auto_detect)
            {
                if (pythonic::draw::is_video_file(input_path) ||
                    pythonic::draw::is_pythonic_video_file(input_path))
                {
                    actual_type = Type::video;
                }
                else
                {
                    actual_type = Type::image;
                }
            }

            // Handle IMAGE format with ExportConfig
            if (format == Format::image)
            {
                std::string output_path = basename + ".png";
                std::string rendered;

                if (actual_type == Type::video)
                {
                    // Extract first frame
                    std::string actual_path = input_path;
                    bool is_temp = false;
                    if (pythonic::draw::is_pythonic_video_file(input_path))
                    {
                        actual_path = pythonic::media::extract_to_temp(input_path);
                        is_temp = true;
                    }

                    std::string temp_frame = "/tmp/pythonic_export_frame_" +
                                             std::to_string(std::hash<std::string>{}(input_path)) + ".png";
                    std::string cmd = "ffmpeg -y -i \"" + actual_path + "\" -vframes 1 \"" + temp_frame + "\" 2>/dev/null";
                    int result = std::system(cmd.c_str());

                    if (is_temp)
                        std::remove(actual_path.c_str());

                    if (result != 0)
                        return false;

                    rendered = render_image_to_string(temp_frame, mode, max_width, threshold);
                    std::remove(temp_frame.c_str());
                }
                else
                {
                    rendered = render_image_to_string(input_path, mode, max_width, threshold);
                }

                return pythonic::ex::export_art_to_png(rendered, output_path, config);
            }

            // For other formats, delegate to the main function
            return export_media(input_path, output_name, type, format, mode, max_width, threshold, audio);
        }

    } // namespace print
} // namespace pythonic