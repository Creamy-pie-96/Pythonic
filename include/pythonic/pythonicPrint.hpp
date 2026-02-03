#pragma once
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#if __cplusplus >= 201703L && __has_include(<filesystem>)
#include <filesystem>
#endif
#include "pythonicVars.hpp"
#include "pythonicDraw.hpp"
#include "pythonicMedia.hpp"
#include "pythonicExport.hpp"
#include "pythonicAccel.hpp"

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
            bool _indeterminate; // For stages where total is unknown

        public:
            ExportProgress(size_t total_frames = 0, int bar_width = 40)
                : _total_frames(total_frames), _current_frame(0),
                  _bar_width(bar_width), _stage("Initializing..."), _indeterminate(total_frames == 0)
            {
                _start_time = std::chrono::steady_clock::now();
            }

            void set_stage(const std::string &stage)
            {
                _stage = stage;
                render();
            }

            void set_total(size_t total)
            {
                _total_frames = total;
                _indeterminate = (total == 0);
            }

            void set_indeterminate(bool value) { _indeterminate = value; }

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

            // Force a render update (useful for indeterminate progress animation)
            void tick()
            {
                render();
            }

            void finish()
            {
                _current_frame = _total_frames;
                _stage = "Complete!";
                _indeterminate = false;
                render();
                std::cout << std::endl;
            }

            void reset()
            {
                _current_frame = 0;
                _start_time = std::chrono::steady_clock::now();
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

            // Animation characters for indeterminate progress (UTF-8 strings)
            const char *spinner_char() const
            {
                static const char *spinners[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
                auto now = std::chrono::steady_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _start_time).count();
                return spinners[(ms / 100) % 10];
            }

            void render()
            {
                auto now = std::chrono::steady_clock::now();
                double elapsed = std::chrono::duration<double>(now - _start_time).count();

                std::ostringstream bar;
                bar << "\033[2K\r"; // Clear line and return to start

                // Stage name
                bar << "\033[36m" << _stage << "\033[0m ";

                if (_indeterminate)
                {
                    // Indeterminate progress bar (animated)
                    static const char *spinners[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _start_time).count();
                    int idx = (ms / 100) % 10;

                    bar << "\033[93m" << spinners[idx] << "\033[0m ";

                    // Animated bar
                    int offset = (ms / 150) % _bar_width;
                    bar << "\033[90m[\033[0m";
                    for (int i = 0; i < _bar_width; i++)
                    {
                        int dist = std::abs(i - offset);
                        if (dist < 3)
                            bar << "\033[92m▓\033[0m";
                        else if (dist < 5)
                            bar << "\033[32m▒\033[0m";
                        else
                            bar << "\033[90m░\033[0m";
                    }
                    bar << "\033[90m]\033[0m ";

                    // Time elapsed only (no ETA for indeterminate)
                    bar << "\033[35m" << format_time(elapsed) << "\033[0m";
                }
                else
                {
                    // Determinate progress bar
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

                    // Build progress bar
                    int filled = static_cast<int>(progress * _bar_width);
                    int empty = _bar_width - filled;

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
                }

                std::cout << bar.str() << std::flush;
            }
        };

        /**
         * @brief Count total frames in a directory matching a pattern
         */
        inline size_t count_frames(const std::string &dir, const std::string &pattern = "frame_")
        {
#ifdef _WIN32
            std::string cmd = "dir /b \"" + dir + "\" 2>nul | findstr /c:\"" + pattern + "\" | find /c /v \"\"";
#else
            std::string cmd = "ls -1 \"" + dir + "\" 2>/dev/null | grep \"" + pattern + "\" | wc -l";
#endif
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

        /**
         * @brief Get video duration in seconds using ffprobe
         */
        inline double get_video_duration(const std::string &filepath)
        {
            std::string cmd = "ffprobe -v quiet -show_entries format=duration "
                              "-of csv=p=0 \"" +
                              filepath + "\" 2>/dev/null";

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
                return std::stod(result);
            }
            catch (...)
            {
                return 0;
            }
        }

        /**
         * @brief Estimate total frames based on duration and fps
         */
        inline size_t estimate_frame_count(const std::string &filepath, double fps)
        {
            double duration = get_video_duration(filepath);
            if (duration <= 0 || fps <= 0)
                return 0;
            return static_cast<size_t>(duration * fps);
        }

        /**
         * @brief Extract frames with progress callback using FFmpeg's progress output
         * @param input_path Source video path
         * @param output_dir Output directory for frames
         * @param fps Target FPS
         * @param progress Progress callback - receives (current_time, total_duration)
         * @return true on success
         */
        inline bool extract_frames_with_progress(
            const std::string &input_path,
            const std::string &output_dir,
            int fps,
            std::function<void(double, double)> progress_callback)
        {
            // Get video duration first
            double duration = get_video_duration(input_path);
            std::string fps_str = std::to_string(fps);

            // Create progress pipe file
            std::string progress_file = output_dir + "/ffmpeg_progress.txt";

            // Build FFmpeg command with progress output
#ifdef _WIN32
            std::string cmd = "ffmpeg -y -progress \"" + progress_file + "\" -i \"" + input_path +
                              "\" -vf \"fps=" + fps_str + "\" \"" + output_dir + "/frame_%05d.png\" >nul 2>&1";
#else
            std::string cmd = "ffmpeg -y -progress \"" + progress_file + "\" -i \"" + input_path +
                              "\" -vf \"fps=" + fps_str + "\" \"" + output_dir + "/frame_%05d.png\" >/dev/null 2>&1 &";
#endif

            // Start FFmpeg in background
            int result = std::system(cmd.c_str());

#ifndef _WIN32
            // On Unix, wait for FFmpeg to start
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Monitor progress file
            while (true)
            {
                std::ifstream pf(progress_file);
                if (pf.good())
                {
                    std::string line;
                    double current_time = 0;
                    bool ended = false;

                    while (std::getline(pf, line))
                    {
                        if (line.find("out_time_ms=") == 0)
                        {
                            try
                            {
                                long long ms = std::stoll(line.substr(12));
                                current_time = ms / 1000000.0;
                            }
                            catch (...)
                            {
                            }
                        }
                        else if (line.find("progress=end") != std::string::npos)
                        {
                            ended = true;
                        }
                    }
                    pf.close();

                    if (progress_callback && duration > 0)
                    {
                        progress_callback(current_time, duration);
                    }

                    if (ended)
                        break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Cleanup progress file
            std::remove(progress_file.c_str());
#endif
            return true;
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
        // Use Type from draw namespace
        using Type = pythonic::draw::Type;

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
        // Use Format from draw namespace
        using Format = pythonic::draw::Format;

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
            case Mode::bw_dithered:
                result = pythonic::draw::render_image_dithered(actual_path, max_width);
                break;
            case Mode::grayscale_dot:
                // Grayscale-colored dots with dithering for best quality
                result = pythonic::draw::render_image_grayscale(actual_path, max_width, threshold, true);
                break;
            case Mode::flood_dot:
                // All dots lit, colored by average cell brightness - smoothest appearance
                result = pythonic::draw::render_image_flood(actual_path, max_width);
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
         *   export_media("input.mp4", "output", Type::video, Format::video, Mode::bw_dot, 80, 128, Audio::off, 0, {}, true, -1, -1); // Full video
         *   export_media("input.mp4", "output", Type::video, Format::video, Mode::bw_dot, 80, 128, Audio::off, 10, {}, true, 60, 120); // 1:00 to 2:00
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
         * @param use_gpu Use GPU/hardware acceleration if available (default true). When false, uses CPU only.
         * @param start_time Start time in seconds for video export (-1 = from beginning, default)
         * @param end_time End time in seconds for video export (-1 = to end, default)
         * @return true on success, false on failure
         */
        inline bool export_media(const std::string &input_path, const std::string &output_name,
                                 Type type = Type::auto_detect,
                                 Format format = Format::text,
                                 Mode mode = Mode::bw_dot,
                                 int max_width = 80, int threshold = 128,
                                 Audio audio = Audio::off,
                                 int fps = 0,
                                 const ex::ExportConfig &config = ex::ExportConfig(),
                                 bool use_gpu = true,
                                 double start_time = -1.0,
                                 double end_time = -1.0)
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

                    // Get estimated frame count early for temp directory decision
                    double video_duration = get_video_duration(actual_path);
                    size_t estimated_frames = static_cast<size_t>(video_duration * actual_fps);

                    // Create temp directory for frames
                    // For large videos (>1000 frames), prefer disk-based temp to avoid tmpfs limits
                    // /var/tmp persists across reboots and typically has more space than /tmp
                    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
                    std::string temp_base;
#ifdef _WIN32
                    temp_base = std::getenv("TEMP") ? std::getenv("TEMP") : "C:\\Temp";
#else
                    if (estimated_frames > 1000)
                    {
                        // Large video - use /var/tmp (disk-backed, not tmpfs)
                        temp_base = "/var/tmp";
                    }
                    else
                    {
                        // Small video - use /tmp (fast tmpfs)
                        temp_base = "/tmp";
                    }
#endif
                    std::string temp_dir = temp_base + "/pythonic_export_" + std::to_string(now % 1000000);

                    // Use filesystem to create directory (more reliable than system call)
#if __cplusplus >= 201703L && __has_include(<filesystem>)
                    try
                    {
                        std::filesystem::create_directories(temp_dir);
                    }
                    catch (...)
                    {
                        // Fallback to system call
#endif
#ifdef _WIN32
                        std::string mkdir_cmd = "mkdir \"" + temp_dir + "\" 2>nul";
#else
                    std::string mkdir_cmd = "mkdir -p \"" + temp_dir + "\"";
#endif
                        std::system(mkdir_cmd.c_str());
#if __cplusplus >= 201703L && __has_include(<filesystem>)
                    }
#endif

                    // Initialize progress bar with preprocessing stage
                    ExportProgress progress(0, 50); // Indeterminate initially
                    progress.set_indeterminate(true);
                    progress.set_stage("Preprocessing");
                    progress.update(0);

                    // Build timestamp options for FFmpeg
                    std::string time_opts;
                    if (start_time >= 0)
                    {
                        // -ss before -i for fast seeking
                        time_opts = "-ss " + std::to_string(start_time) + " ";
                    }

                    std::string duration_opt;
                    if (end_time >= 0)
                    {
                        double duration = (start_time >= 0) ? (end_time - start_time) : end_time;
                        if (duration > 0)
                        {
                            duration_opt = " -t " + std::to_string(duration);
                        }
                    }

                    // Extract frames from video at the target fps
                    // Note: -ss before -i seeks input (fast), duration/time filters after
                    std::string extract_cmd = "ffmpeg -y " + time_opts + "-i \"" + actual_path + "\"" + duration_opt +
                                              " -vf \"fps=" + fps_str + "\" \"" +
                                              temp_dir + "/frame_%05d.png\" 2>/dev/null";

                    // Atomic flag to signal extraction done
                    std::atomic<bool> extraction_done{false};

                    // Start progress update thread - this monitors frame extraction
                    std::thread update_thread([&progress, &temp_dir, estimated_frames, &extraction_done]()
                                              {
                        // Periodically check extracted frame count and update progress
                        while (!extraction_done.load())
                        {
                            size_t current = count_frames(temp_dir, "frame_");
                            if (current > 0)
                            {
                                progress.set_indeterminate(false);
                                progress.set_total(estimated_frames > 0 ? estimated_frames : current * 2);
                                progress.update(current);
                            }
                            else
                            {
                                // Still extracting first frames, render the spinner
                                progress.tick();
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        } });

                    // Run ffmpeg extraction (this blocks until done)
                    int result = std::system(extract_cmd.c_str());

                    // Signal update thread to stop and wait for it
                    extraction_done.store(true);
                    update_thread.join();

                    if (is_temp_video)
                        std::remove(actual_path.c_str());

                    // Count extracted frames - this is more reliable than exit code
                    // FFmpeg may return non-zero even when frames are extracted successfully
                    size_t total_frames = count_frames(temp_dir, "frame_");

                    if (total_frames == 0)
                    {
                        // Only fail if no frames were extracted at all
                        std::cout << "\n\033[31mError: Failed to extract frames from video (exit code: "
                                  << result << ")\033[0m\n";
#ifdef _WIN32
                        std::string rm_cmd = "rmdir /s /q \"" + temp_dir + "\"";
#else
                        std::string rm_cmd = "rm -rf \"" + temp_dir + "\"";
#endif
                        std::system(rm_cmd.c_str());
                        return false;
                    }

                    // Update progress bar with actual frame count
                    progress.reset(); // Reset timer for ASCII rendering phase
                    progress.set_indeterminate(false);
                    progress.set_total(total_frames);
                    progress.set_stage("Rendering ASCII art");
                    progress.update(0);

                    // Determine number of worker threads
                    // Leave 2 threads free for other system tasks
                    int num_threads = std::thread::hardware_concurrency();
                    if (num_threads == 0)
                        num_threads = 4; // Fallback
                    else if (num_threads > 2)
                        num_threads -= 2; // Leave 2 threads free
                    if (num_threads > 16)
                        num_threads = 16; // Cap at 16 threads max for rendering

                    // For smaller videos, fewer threads make sense
                    if (total_frames < 100)
                        num_threads = std::min(num_threads, 4);
                    if (total_frames < 50)
                        num_threads = std::min(num_threads, 2);

                    // ==================== FAST PATH: Direct BW Block Export ====================
                    // For bw_block mode, bypass ANSI string generation for 5-10x speedup
                    if (mode == Mode::bw)
                    {
                        progress.set_stage("Direct grayscale export");
                        progress.set_total(total_frames);
                        progress.update(0);

                        std::atomic<size_t> fast_frames_done{0};
                        std::atomic<bool> fast_done{false};

                        std::thread fast_progress_thread([&progress, &fast_frames_done, &fast_done, total_frames]()
                                                         {
                            while (!fast_done.load()) {
                                progress.update(fast_frames_done.load());
                                if (fast_frames_done.load() >= total_frames) break;
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            } });

                        auto fast_worker = [&](size_t start, size_t end)
                        {
                            for (size_t frame_num = start; frame_num <= end && frame_num <= total_frames; frame_num++)
                            {
                                char frame_name[128];
                                snprintf(frame_name, sizeof(frame_name), "%s/frame_%05zu.png", temp_dir.c_str(), frame_num);

                                // Convert PNG to PPM using ImageMagick
                                std::string temp_ppm = temp_dir + "/temp_" + std::to_string(frame_num) + ".ppm";
                                std::string cmd = "convert \"" + std::string(frame_name) + "\" -resize " +
                                                  std::to_string(max_width) + "x -depth 8 \"" + temp_ppm + "\" 2>/dev/null";
                                if (std::system(cmd.c_str()) != 0)
                                {
                                    fast_frames_done++;
                                    continue;
                                }

                                // Read PPM directly
                                std::ifstream ppm(temp_ppm, std::ios::binary);
                                if (!ppm)
                                {
                                    std::remove(temp_ppm.c_str());
                                    fast_frames_done++;
                                    continue;
                                }

                                std::string magic;
                                ppm >> magic;
                                char c;
                                ppm.get(c);
                                while (ppm.peek() == '#')
                                {
                                    std::string comment;
                                    std::getline(ppm, comment);
                                }
                                int width, height, maxval;
                                ppm >> width >> height >> maxval;
                                ppm.get(c);

                                // Load into BWBlockCanvas
                                pythonic::draw::BWBlockCanvas canvas = pythonic::draw::BWBlockCanvas::from_pixels(width, height);
                                std::vector<uint8_t> rgb_data(width * height * 3);
                                ppm.read(reinterpret_cast<char *>(rgb_data.data()), rgb_data.size());
                                ppm.close();
                                std::remove(temp_ppm.c_str());

                                canvas.load_frame_rgb(rgb_data.data(), width, height, threshold);

                                // Direct export using fast path
                                auto img = pythonic::ex::render_half_block_direct(
                                    canvas.get_pixels(), canvas.width(), canvas.height(), config.dot_size);

                                char img_name[128];
                                snprintf(img_name, sizeof(img_name), "%s/ascii_%05zu.png", temp_dir.c_str(), frame_num);
                                pythonic::ex::write_png(img, img_name);

                                fast_frames_done++;
                            }
                        };

                        // Launch workers
                        std::vector<std::thread> fast_workers;
                        size_t frames_per_thread = (total_frames + num_threads - 1) / num_threads;
                        for (int t = 0; t < num_threads; t++)
                        {
                            size_t start = t * frames_per_thread + 1;
                            size_t end = std::min(start + frames_per_thread - 1, total_frames);
                            if (start <= total_frames)
                            {
                                fast_workers.emplace_back(fast_worker, start, end);
                            }
                        }
                        for (auto &w : fast_workers)
                            w.join();
                        fast_done.store(true);
                        fast_progress_thread.join();
                        progress.update(total_frames);

                        // Skip to video encoding phase
                        goto encode_video;
                    }

                    // ==================== PHASE 1: Multi-threaded ASCII Rendering ====================
                    // Render all frames to ASCII strings in parallel (CPU-bound work)
                    // Store in a vector indexed by frame number
                    {
                        std::vector<std::string> rendered_frames(total_frames + 1);
                        std::atomic<size_t> frames_rendered{0};
                        std::atomic<bool> rendering_done{false};

                        // Progress update thread for rendering phase
                        std::thread render_progress_thread([&progress, &frames_rendered, &rendering_done, total_frames]()
                                                           {
                        while (!rendering_done.load())
                        {
                            size_t completed = frames_rendered.load();
                            progress.update(completed);
                            if (completed >= total_frames) break;
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        } });

                        // Worker function for rendering phase (CPU-intensive, parallelizable)
                        auto render_worker = [&](size_t start, size_t end)
                        {
                            for (size_t frame_num = start; frame_num <= end && frame_num <= total_frames; frame_num++)
                            {
                                char frame_name[128];
                                snprintf(frame_name, sizeof(frame_name), "%s/frame_%05zu.png", temp_dir.c_str(), frame_num);

                                std::ifstream test(frame_name);
                                if (!test.good())
                                {
                                    rendered_frames[frame_num] = ""; // Mark as failed
                                    frames_rendered++;
                                    continue;
                                }
                                test.close();

                                // Render frame to ASCII art (CPU-bound, thread-safe)
                                rendered_frames[frame_num] = render_image_to_string(frame_name, mode, max_width, threshold);
                                frames_rendered++;
                            }
                        };

                        // Distribute rendering across threads
                        std::vector<std::thread> render_workers;
                        size_t frames_per_thread = (total_frames + num_threads - 1) / num_threads;

                        for (int t = 0; t < num_threads; t++)
                        {
                            size_t start = t * frames_per_thread + 1;
                            size_t end = std::min(start + frames_per_thread - 1, total_frames);
                            if (start <= total_frames)
                            {
                                render_workers.emplace_back(render_worker, start, end);
                            }
                        }

                        // Wait for all renderers to complete
                        for (auto &worker : render_workers)
                        {
                            worker.join();
                        }
                        rendering_done.store(true);
                        render_progress_thread.join();
                        progress.update(total_frames);

                        // ==================== PHASE 2: PNG Export (I/O-bound) ====================
                        // Export rendered frames to PNG files
                        // Use fewer threads for I/O to avoid disk contention
                        progress.reset();
                        progress.set_stage("Exporting frames");
                        progress.set_total(total_frames);
                        progress.update(0);

                        std::atomic<size_t> frames_exported{0};
                        std::atomic<bool> export_done{false};

                        // Progress update thread for export phase
                        std::thread export_progress_thread([&progress, &frames_exported, &export_done, total_frames]()
                                                           {
                        while (!export_done.load())
                        {
                            size_t completed = frames_exported.load();
                            progress.update(completed);
                            if (completed >= total_frames) break;
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        } });

                        // Worker function for PNG export (I/O-bound)
                        auto export_worker = [&](size_t start, size_t end)
                        {
                            for (size_t frame_num = start; frame_num <= end && frame_num <= total_frames; frame_num++)
                            {
                                const std::string &rendered = rendered_frames[frame_num];
                                if (rendered.empty())
                                {
                                    frames_exported++;
                                    continue; // Skip failed frames
                                }

                                char img_name[128];
                                snprintf(img_name, sizeof(img_name), "%s/ascii_%05zu.png", temp_dir.c_str(), frame_num);
                                pythonic::ex::export_art_to_png(rendered, img_name, config);
                                frames_exported++;
                            }
                        };

                        // Use fewer threads for I/O work (4-6 is usually optimal for disk I/O)
                        int io_threads = std::min(num_threads, 6);

                        std::vector<std::thread> export_workers;
                        frames_per_thread = (total_frames + io_threads - 1) / io_threads;

                        for (int t = 0; t < io_threads; t++)
                        {
                            size_t start = t * frames_per_thread + 1;
                            size_t end = std::min(start + frames_per_thread - 1, total_frames);
                            if (start <= total_frames)
                            {
                                export_workers.emplace_back(export_worker, start, end);
                            }
                        }

                        // Wait for all exporters to complete
                        for (auto &worker : export_workers)
                        {
                            worker.join();
                        }
                        export_done.store(true);
                        export_progress_thread.join();

                        // Free memory from rendered frames (no longer needed)
                        rendered_frames.clear();
                        rendered_frames.shrink_to_fit();

                        // Verify that ASCII frames were created before attempting to encode
                        size_t ascii_frames_count = count_frames(temp_dir, "ascii_");
                        if (ascii_frames_count == 0)
                        {
                            std::cout << "\n\033[31mError: No ASCII frames were generated. Cannot encode video.\033[0m\n";
#ifdef _WIN32
                            std::string rm_cmd = "rmdir /s /q \"" + temp_dir + "\"";
#else
                            std::string rm_cmd = "rm -rf \"" + temp_dir + "\"";
#endif
                            std::system(rm_cmd.c_str());
                            return false;
                        }

                        // Final progress update
                        progress.update(total_frames);
                    } // End of regular (non-fast-path) export scope

                encode_video:
                    // Update progress for encoding stage
                    progress.set_indeterminate(true);
                    progress.set_stage("Encoding video");
                    progress.update(0);

                    // Detect available hardware encoders (only if use_gpu is true)
                    std::string encoder = "libx264"; // CPU default
                    if (use_gpu)
                    {
                        auto hw_encoders = pythonic::accel::detect_hw_encoders();
                        encoder = hw_encoders.best_h264_encoder();

                        // If GPU was requested but not available, inform user
                        if (encoder == "libx264")
                        {
                            std::cout << "\n\033[33mNote: GPU requested but no hardware encoder found, falling back to CPU\033[0m\n";
                        }
                    }

                    // Log encoder being used
                    if (encoder != "libx264")
                    {
                        std::cout << "\n\033[90mUsing hardware encoder: " << encoder << "\033[0m\n";
                    }
                    else if (!use_gpu)
                    {
                        std::cout << "\n\033[90mUsing CPU encoder (GPU disabled by user)\033[0m\n";
                    }

                    // Build encoder-specific options (returns pair: encoder_opts, is_hw_encoder)
                    auto build_encoder_opts = [](const std::string &enc) -> std::pair<std::string, bool>
                    {
                        if (enc == "h264_nvenc")
                            return {"-c:v h264_nvenc -preset fast -rc vbr -cq 23", true};
                        if (enc == "h264_qsv")
                            return {"-c:v h264_qsv -preset faster -global_quality 23", true};
                        if (enc == "h264_vaapi")
                            return {"-vaapi_device /dev/dri/renderD128 -c:v h264_vaapi -qp 23", true};
                        if (enc == "h264_videotoolbox")
                            return {"-c:v h264_videotoolbox -q:v 65", true};
                        return {"-c:v libx264 -preset faster -crf 23", false};
                    };

                    auto [encoder_opts, is_hw_encoder] = build_encoder_opts(encoder);

                    // Video filter to ensure dimensions are divisible by 2 (required for yuv420p)
                    // Properly escaped for shell execution
                    std::string scale_filter = "-vf \"scale=trunc(iw/2)*2:trunc(ih/2)*2\"";

                    // Lambda to build and execute FFmpeg command with fallback
                    auto run_encode = [&](const std::string &audio_path) -> int
                    {
                        // Use image2 demuxer with sequential frame pattern
                        // FFmpeg image2 demuxer needs -framerate before -i for proper timing
                        // -start_number 1 tells FFmpeg our frames start from 1 (not 0)
                        std::string base_input = "-framerate " + fps_str + " -start_number 1 -i \"" + temp_dir + "/ascii_%05d.png\"";
                        std::string audio_input = audio_path.empty() ? "" : " -i \"" + audio_path + "\"";
                        std::string audio_opts = audio_path.empty() ? "" : " -c:a aac -shortest";
                        std::string pix_fmt = " -pix_fmt yuv420p";

                        // Try with hardware encoder first
                        std::string video_cmd = "ffmpeg -y " + base_input + audio_input + " " +
                                                scale_filter + " " + encoder_opts + audio_opts + pix_fmt +
                                                " \"" + output_path + "\"";
                        int res = std::system(video_cmd.c_str());

                        // If hardware encoder fails, fallback to CPU encoder
                        if (res != 0 && is_hw_encoder)
                        {
                            std::cout << "\n\033[33mHardware encoder failed, falling back to CPU (libx264)\033[0m\n";
                            std::string cpu_opts = "-c:v libx264 -preset faster -crf 23";
                            video_cmd = "ffmpeg -y " + base_input + audio_input + " " +
                                        scale_filter + " " + cpu_opts + audio_opts + pix_fmt +
                                        " \"" + output_path + "\"";
                            res = std::system(video_cmd.c_str());
                        }

                        return res;
                    };

                    // Combine ASCII frames into video
                    if (audio == Audio::on)
                    {
                        // Extract audio from source and combine with ASCII video
                        std::string audio_path = temp_dir + "/audio.aac";
                        std::string extract_audio_cmd = "ffmpeg -y -i \"" + input_path + "\" -vn -acodec aac \"" + audio_path + "\" 2>/dev/null";
                        int audio_result = std::system(extract_audio_cmd.c_str());

                        if (audio_result == 0)
                        {
                            result = run_encode(audio_path);
                        }
                        else
                        {
                            // No audio in source or extraction failed, create video without audio
                            result = run_encode("");
                        }
                    }
                    else
                    {
                        // Create video without audio
                        result = run_encode("");
                    }

                    // Cleanup - always clean up temp directory
#ifdef _WIN32
                    std::string rm_cmd = "rmdir /s /q \"" + temp_dir + "\"";
#else
                    std::string rm_cmd = "rm -rf \"" + temp_dir + "\"";
#endif
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
         * @param use_gpu Use GPU/hardware acceleration if available (default true)
         * @return true on success
         */
        inline bool export_media(const std::string &input_path, const std::string &output_name,
                                 const ExportConfig &config,
                                 Type type = Type::auto_detect,
                                 Format format = Format::image,
                                 Mode mode = Mode::bw_dot,
                                 int max_width = 80, int threshold = 128,
                                 Audio audio = Audio::off,
                                 bool use_gpu = true)
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
            return export_media(input_path, output_name, type, format, mode, max_width, threshold, audio, 0, {}, use_gpu);
        }

        // ==================== Simplified Config-based API ====================

        // Re-export RenderConfig and Dithering from draw namespace for convenience
        using RenderConfig = pythonic::draw::RenderConfig;
        using Dithering = pythonic::draw::Dithering;

        /**
         * @brief Print/display media file with unified configuration
         *
         * Simplified API that uses RenderConfig for all parameters.
         * All parameters have sensible defaults.
         *
         * Usage:
         *   // With all defaults (auto-detect type, bw_dot mode, 80 width)
         *   print("image.png");
         *
         *   // With custom config
         *   print("video.mp4", RenderConfig().set_mode(Mode::colored).set_max_width(120));
         *
         *   // With dithering
         *   print("photo.jpg", RenderConfig().set_dithering(Dithering::ordered));
         *
         *   // Interactive video playback with audio
         *   print("movie.mp4", RenderConfig().with_audio().interactive());
         *
         * @param filepath Path to media file
         * @param config Configuration with all rendering parameters
         */
        inline void print(const std::string &filepath, const RenderConfig &config)
        {
            // Delegate to the full print function with all config parameters
            print(filepath, config.type, config.mode, config.parser, config.audio,
                  config.max_width, config.threshold, config.shell,
                  config.pause_key, config.stop_key);
        }

        /**
         * @brief Export media file with unified configuration
         *
         * Simplified API that uses RenderConfig for all parameters.
         * All parameters have sensible defaults.
         *
         * Usage:
         *   // Export with defaults (text output)
         *   export_media("image.png", "output");
         *
         *   // Export video as ASCII video
         *   export_media("video.mp4", "output", RenderConfig()
         *       .set_format(Format::video)
         *       .set_mode(Mode::colored)
         *       .set_dithering(Dithering::ordered));
         *
         *   // Export clip with timestamps
         *   export_media("movie.mp4", "clip", RenderConfig()
         *       .set_format(Format::video)
         *       .set_start_time(60)  // Start at 1:00
         *       .set_end_time(120)); // End at 2:00
         *
         * @param input_path Path to source media file
         * @param output_name Output filename (extension added based on format)
         * @param config Configuration with all rendering parameters
         * @return true on success, false on failure
         */
        inline bool export_media(const std::string &input_path, const std::string &output_name,
                                 const RenderConfig &config)
        {
            return export_media(input_path, output_name, config.type, config.format,
                                config.mode, config.max_width, config.threshold,
                                config.audio, config.fps, ex::ExportConfig(),
                                config.use_gpu, config.start_time, config.end_time);
        }

    } // namespace print
} // namespace pythonic