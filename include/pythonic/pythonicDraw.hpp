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
 * - True color (24-bit) rendering support with ANSI escape codes
 * - Optimized block-based rendering for real-time video
 * - FFmpeg integration for video streaming
 * - SDL2/PortAudio support for audio playback (optional)
 * - Double-buffering with ANSI escape codes to avoid flickering
 * - Robust signal handling for proper terminal cleanup on Ctrl+C
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
 *
 *   // Colored image rendering:
 *   print_image_colored("image.png", 80);
 */

#include <vector>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <cctype>
#include <cstring>
#include <functional>
#include <array>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <csignal>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
// Prevent Windows min/max macros from interfering with std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Prevent Windows RGB macro from interfering with our RGB struct
#ifdef RGB
#undef RGB
#endif
#include <windows.h>
// Undefine RGB again in case windows.h re-defined it
#ifdef RGB
#undef RGB
#endif
#include <io.h>
#include <conio.h>
#define write _write
// Define STDOUT_FILENO for Windows
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#else
#include <unistd.h>
#include <termios.h>
#endif

// Optional SDL2 support for audio playback
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
#include <SDL2/SDL.h>
#endif

// Optional PortAudio support for audio playback
#ifdef PYTHONIC_ENABLE_PORTAUDIO
#include <portaudio.h>
#endif

// Optional OpenCL support for GPU-accelerated rendering
#ifdef PYTHONIC_ENABLE_OPENCL
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/opencl.hpp>
#endif

// Optional OpenCV support for image/video processing
#ifdef PYTHONIC_ENABLE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#endif

#include <iomanip>
#include <tuple>

#include "pythonicAccel.hpp"

// Forward declare media functions
namespace pythonic
{
    namespace media
    {
        inline bool is_pythonic_image(const std::string &filename);
        inline bool is_pythonic_video(const std::string &filename);
        inline bool is_pythonic_format(const std::string &filename);
        inline std::string extract_to_temp(const std::string &filepath);
    }
}

namespace pythonic
{
    namespace draw
    {
        // ==================== Global Signal Handling for Terminal Cleanup ====================

        /**
         * @brief Global state for signal-safe terminal cleanup
         *
         * This provides robust cleanup when the user presses Ctrl+C or the program
         * is terminated unexpectedly. The terminal cursor and attributes are restored
         * even if playback is interrupted.
         */
        namespace signal_handler
        {
            // Global flag indicating video playback is active
            inline std::atomic<bool> &playback_active()
            {
                static std::atomic<bool> active{false};
                return active;
            }

            // Flag to track if we've installed our signal handler
            inline std::atomic<bool> &handler_installed()
            {
                static std::atomic<bool> installed{false};
                return installed;
            }

            // Store the previous signal handler to restore it later
            inline std::sig_atomic_t &interrupted()
            {
                static std::sig_atomic_t flag{0};
                return flag;
            }

#ifndef _WIN32
            // Global backup of the original terminal state (before any raw mode)
            inline struct termios &saved_termios()
            {
                static struct termios t;
                return t;
            }
            inline bool &termios_saved()
            {
                static bool saved = false;
                return saved;
            }
#endif

            // Save the current terminal state globally (call before entering raw mode)
            inline void save_terminal_state()
            {
#ifndef _WIN32
                if (!termios_saved())
                {
                    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &saved_termios()) == 0)
                        termios_saved() = true;
                }
#endif
            }

            // The cleanup function that restores terminal state
            inline void restore_terminal()
            {
#ifndef _WIN32
                // Restore termios (critical — this is what "un-breaks" the terminal)
                if (termios_saved())
                    tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios());
#endif
                // Show cursor, reset attributes, leave alternate screen buffer
                // (leaving alt screen restores original terminal content)
                const char *restore = "\033[?25h\033[0m\033[?1049l";
                [[maybe_unused]] auto result = write(STDOUT_FILENO, restore, strlen(restore));
            }

            // Signal handler function - must be async-signal-safe
            inline void signal_handler_func(int signum)
            {
                interrupted() = 1;
                restore_terminal();

                // Re-raise the signal with default handler to allow normal termination
                std::signal(signum, SIG_DFL);
                std::raise(signum);
            }

            // Install the signal handler (called automatically when playback starts)
            inline void install()
            {
                if (!handler_installed().exchange(true))
                {
                    // Save terminal state BEFORE any raw mode changes
                    save_terminal_state();
                    std::signal(SIGINT, signal_handler_func);
                    std::signal(SIGTERM, signal_handler_func);
#ifndef _WIN32
                    std::signal(SIGHUP, signal_handler_func);
#endif
                }
            }

            // Check if playback was interrupted
            inline bool was_interrupted()
            {
                return interrupted() != 0;
            }

            // Mark playback as started (installs handler if needed)
            inline void start_playback()
            {
                install();
                playback_active() = true;
                interrupted() = 0;
            }

            // Mark playback as ended — restore terminal state
            inline void end_playback()
            {
                playback_active() = false;
#ifndef _WIN32
                // Always restore termios on end to ensure cooked mode is back
                if (termios_saved())
                    tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios());
#endif
            }
        } // namespace signal_handler

        /**
         * @brief Rendering mode for terminal graphics
         *
         * Mode determines how pixels are rendered to the terminal:
         *   - bw:          Black & white using half-block characters (▀)
         *   - bw_dot:      Black & white using Braille patterns (⠿) - higher resolution
         *   - colored:     True color (24-bit) using half-block characters
         *   - colored_dot: True color using Braille patterns with averaged colors
         *   - bw_dithered:  Black & white with ordered dithering (smooth grayscale shading)
         *   - grayscale_dot: Grayscale-colored braille dots (smooth appearance)
         *   - flood_dot:   Flood-fill braille - all dots lit, colored by average cell brightness
         *   - flood_dot_colored: Flood-fill braille with RGB colors (all dots lit, averaged colors)
         *   - colored_dithered: Colored braille with dithering for smoother appearance
         */
        enum class Mode
        {
            bw,                // Black & white with block characters (current colored renderer logic, no color)
            bw_dot,            // Black & white with braille patterns (default, higher resolution)
            colored,           // True color with block characters
            colored_dot,       // True color with braille patterns (one color per cell)
            bw_dithered,       // Black & white with ordered dithering for grayscale shading
            grayscale_dot,     // Grayscale-colored braille dots (dots colored by brightness)
            flood_dot,         // Flood-fill: all dots lit (⣿), colored by average cell grayscale
            flood_dot_colored, // Flood-fill: all dots lit (⣿), colored by average cell RGB color
            colored_dithered   // Colored braille with dithering for smoother appearance
        };

        // Legacy alias for backward compatibility
        using Render = Mode;

        /**
         * @brief Tag type to disambiguate media printing from text printing
         *
         * Use this when you want to explicitly render a file as media:
         *   print(Draw, "image.png");  // Always renders as image
         *   print("image.png");        // May be ambiguous with text
         */
        struct DrawTag
        {
        };
        constexpr DrawTag Draw{};

        /**
         * @brief Media type hint for print() function
         *
         * Controls how the input file is interpreted:
         *   - auto_detect: Detect from file extension (default)
         *   - image: Force treat as image
         *   - video: Force treat as video
         *   - webcam: Capture from webcam (requires OpenCV)
         *   - video_info: Show video metadata only
         *   - text: Force treat as plain text
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

        /**
         * @brief Output format for export_media() function
         *
         * Controls the output file format:
         *   - pythonic: Save as .pi (image) or .pv (video) - Pythonic's encrypted format
         *   - text: Save as .txt (ASCII art text file)
         *   - image: Save as .png image (render ASCII art to image)
         *   - video: Save as .mp4 video (render ASCII art to video frames)
         */
        enum class Format
        {
            pythonic,     // Save as .pi (image) or .pv (video)
            text,         // Save as .txt (ASCII art text file)
            image,        // Save as .png image
            video,        // Save as .mp4 video
            normal = text // Alias for backward compatibility
        };

        /**
         * @brief Parser backend for media processing
         *
         * Determines which library is used for decoding images and videos:
         *   - default_parser: FFmpeg for video, ImageMagick for images (current behavior)
         *   - opencv:         OpenCV for both images and videos (also supports webcam)
         */
        enum class Parser
        {
            default_parser, // FFmpeg for video, ImageMagick for images (default)
            opencv          // OpenCV for everything (images, videos, webcam)
        };

        // Convenience constants for Parser enum
        // Allows using `opencv` instead of `Parser::opencv`
        constexpr Parser default_parser = Parser::default_parser;
        constexpr Parser opencv = Parser::opencv;

        /**
         * @brief Audio playback mode for video
         */
        enum class Audio
        {
            off, // No audio (default)
            on   // Play audio with video (requires SDL2 or PortAudio)
        };

        /**
         * @brief Shell mode for keyboard input handling
         *
         * Controls whether keyboard input (pause/stop controls) is enabled:
         *   - noninteractive: No keyboard input handling (default, safe for scripts/pipes)
         *   - interactive:    Enable keyboard controls (pause/stop keys work)
         *
         * Use interactive mode when running in a real terminal where user input is expected.
         * Use noninteractive (default) when running in scripts, CI/CD, or piped environments.
         */
        enum class Shell
        {
            noninteractive, // No keyboard input (default, safe for scripts)
            interactive     // Enable keyboard controls for pause/stop
        };

        /**
         * @brief Dithering algorithm for bw/grayscale rendering
         */
        enum class Dithering
        {
            none,           // Simple threshold (default) - pixels either on or off
            ordered,        // Ordered dithering - smooth gradients, fast, stable for video
            floyd_steinberg // Floyd-Steinberg error diffusion - best quality, slower
        };

        /**
         * @brief Unified configuration for rendering images/videos to terminal art
         *
         * This consolidates ALL rendering parameters into a single configuration
         * that can be passed to both print() and export_media() functions.
         *
         * Simplified API usage:
         *   // Print with all defaults
         *   print("image.png");
         *
         *   // Print with custom config
         *   RenderConfig config;
         *   config.mode = Mode::grayscale_dot;
         *   config.dithering = Dithering::ordered;
         *   print("video.mp4", config);
         *
         *   // Builder pattern for one-liners
         *   print("image.png", RenderConfig().set_mode(Mode::colored).set_max_width(120));
         *
         *   // Export with config
         *   export_media("video.mp4", "output", RenderConfig().set_dithering(Dithering::ordered));
         */
        struct RenderConfig
        {
            // === Media type and rendering mode ===
            Type type = Type::auto_detect;          // Media type (auto_detect, image, video, webcam, text)
            Mode mode = Mode::bw_dot;               // Rendering mode (bw, bw_dot, colored, colored_dot, etc.)
            Parser parser = Parser::default_parser; // Backend (default_parser or opencv)
            Format format = Format::text;           // Output format for export (text, image, video, pythonic)

            // === Rendering parameters ===
            int threshold = 128;                   // Brightness threshold (0-255) for bw modes
            int max_width = 80;                    // Maximum output width in characters
            Dithering dithering = Dithering::none; // Dithering algorithm

            // === Dot appearance (for braille modes) ===
            bool grayscale_dots = false; // Use grayscale ANSI colors for dots based on brightness

            // === Color options ===
            bool invert = false; // Invert colors (white bg, black fg)

            // === Video playback options ===
            int fps = 0;                         // Target FPS (0 = use source video FPS)
            double start_time = -1.0;            // Start time in seconds (-1 = from beginning)
            double end_time = -1.0;              // End time in seconds (-1 = to end)
            Audio audio = Audio::off;            // Audio playback mode
            Shell shell = Shell::noninteractive; // Shell mode for video playback
            char pause_key = 'p';                // Key to pause/resume (0 to disable)
            char stop_key = 's';                 // Key to stop (0 to disable)

            // === Interactive playback controls (arrow keys) ===
            int vol_up_key = 0x1B5B41;        // Up arrow (ESC [ A) - volume up
            int vol_down_key = 0x1B5B42;      // Down arrow (ESC [ B) - volume down
            int seek_backward_key = 0x1B5B44; // Left arrow (ESC [ D) - seek backward
            int seek_forward_key = 0x1B5B43;  // Right arrow (ESC [ C) - seek forward
            int seek_frames = 90;             // Number of frames to seek (default ~3 seconds at 30fps)
            int volume = 100;                 // Initial volume (0-100)
            int volume_step = 10;             // Volume change per key press (1-100, default 10)

            // === Buffering options ===
            int buffer_ahead_frames = 60;  // Frames to preload ahead (default ~2 seconds at 30fps)
            int buffer_behind_frames = 90; // Frames to keep behind for seeking back (default ~3 seconds)

            // === Export options ===
            bool use_gpu = true; // Use GPU acceleration for video encoding

            // Default constructor
            RenderConfig() = default;

            // Builder pattern for easy chaining
            RenderConfig &set_type(Type t)
            {
                type = t;
                return *this;
            }
            RenderConfig &set_mode(Mode m)
            {
                mode = m;
                return *this;
            }
            RenderConfig &set_parser(Parser p)
            {
                parser = p;
                return *this;
            }
            RenderConfig &set_format(Format f)
            {
                format = f;
                return *this;
            }
            RenderConfig &set_threshold(int t)
            {
                threshold = t;
                return *this;
            }
            RenderConfig &set_max_width(int w)
            {
                max_width = w;
                return *this;
            }
            RenderConfig &set_dithering(Dithering d)
            {
                dithering = d;
                return *this;
            }
            RenderConfig &set_grayscale_dots(bool g)
            {
                grayscale_dots = g;
                return *this;
            }
            RenderConfig &set_invert(bool i)
            {
                invert = i;
                return *this;
            }
            RenderConfig &set_fps(int f)
            {
                fps = f;
                return *this;
            }
            RenderConfig &set_start_time(double t)
            {
                start_time = t;
                return *this;
            }
            RenderConfig &set_end_time(double t)
            {
                end_time = t;
                return *this;
            }
            RenderConfig &set_audio(Audio a)
            {
                audio = a;
                return *this;
            }
            RenderConfig &set_shell(Shell s)
            {
                shell = s;
                return *this;
            }
            RenderConfig &set_pause_key(char k)
            {
                pause_key = k;
                return *this;
            }
            RenderConfig &set_stop_key(char k)
            {
                stop_key = k;
                return *this;
            }
            RenderConfig &set_volume(int v)
            {
                volume = (v < 0) ? 0 : (v > 100) ? 100
                                                 : v;
                return *this;
            }
            RenderConfig &set_volume_step(int step)
            {
                // Clamp to valid range: 1-100
                if (step <= 0)
                    step = 1;
                else if (step > 100)
                    step = 100;
                volume_step = step;
                return *this;
            }
            RenderConfig &set_seek_frames(int f)
            {
                seek_frames = (f < 1) ? 1 : f;
                return *this;
            }
            RenderConfig &set_buffer_ahead(int frames)
            {
                buffer_ahead_frames = (frames < 10) ? 10 : frames;
                return *this;
            }
            RenderConfig &set_buffer_behind(int frames)
            {
                buffer_behind_frames = (frames < 10) ? 10 : frames;
                return *this;
            }
            RenderConfig &set_gpu(bool g)
            {
                use_gpu = g;
                return *this;
            }

            // Convenience methods
            RenderConfig &with_audio()
            {
                audio = Audio::on;
                return *this;
            }
            RenderConfig &interactive()
            {
                shell = Shell::interactive;
                return *this;
            }
            RenderConfig &no_gpu()
            {
                use_gpu = false;
                return *this;
            }
        };

        // ==================== Constants for Aspect Ratio Correction ====================

        /**
         * Terminal characters are typically taller than wide (~1:2 ratio).
         * Braille cells are 2×4 dots. To prevent vertical squashing,
         * we apply an aspect ratio correction factor.
         *
         * Typical terminal cell ratio: width:height ≈ 1:2
         * Braille aspect: 2 dots wide × 4 dots high
         * Correction factor: (cell_height / cell_width) * (braille_width / braille_height)
         *                  = 2.0 * (2/4) = 1.0 (no correction for braille)
         *
         * For colored mode (half-block): 1 char wide × 2 pixels high
         * Correction factor needed: ~0.5 (stretch horizontally or compress vertically)
         */
        constexpr double TERMINAL_ASPECT_RATIO = 0.5;       // Typical terminal char height/width
        constexpr double BRAILLE_ASPECT_CORRECTION = 1.0;   // Braille is already ~square
        constexpr double HALFBLOCK_ASPECT_CORRECTION = 0.5; // Half-block needs correction

        // ==================== GPU Acceleration Support ====================

#ifdef PYTHONIC_ENABLE_OPENCL
        /**
         * @brief GPU-accelerated frame renderer using OpenCL
         *
         * Provides hardware-accelerated frame processing for smoother video playback.
         * Falls back to CPU rendering if GPU is not available.
         */
        class GPURenderer
        {
        private:
            cl::Context _context;
            cl::CommandQueue _queue;
            cl::Program _program;
            cl::Kernel _rgb_to_ansi_kernel;
            cl::Buffer _input_buffer;
            cl::Buffer _output_buffer;
            bool _initialized = false;
            size_t _buffer_size = 0;

            // OpenCL kernel for RGB to ANSI conversion
            static constexpr const char *KERNEL_SOURCE = R"(
                __kernel void rgb_to_ansi(
                    __global const uchar* input,   // RGB pixels
                    __global uchar* output,        // Output buffer
                    int width,
                    int height,
                    int output_width)              // Width of output in chars
                {
                    int gid = get_global_id(0);
                    int char_y = gid / output_width;
                    int char_x = gid % output_width;
                    
                    if (char_y >= (height / 2) || char_x >= output_width)
                        return;
                    
                    // Top and bottom pixel indices
                    int top_y = char_y * 2;
                    int bot_y = top_y + 1;
                    int x = char_x;
                    
                    // Get RGB values
                    int top_idx = (top_y * width + x) * 3;
                    int bot_idx = (bot_y * width + x) * 3;
                    
                    uchar tr = input[top_idx];
                    uchar tg = input[top_idx + 1];
                    uchar tb = input[top_idx + 2];
                    
                    uchar br = (bot_y < height) ? input[bot_idx] : 0;
                    uchar bg = (bot_y < height) ? input[bot_idx + 1] : 0;
                    uchar bb = (bot_y < height) ? input[bot_idx + 2] : 0;
                    
                    // Output: 6 bytes per char (fg R,G,B + bg R,G,B)
                    int out_idx = gid * 6;
                    output[out_idx] = tr;
                    output[out_idx + 1] = tg;
                    output[out_idx + 2] = tb;
                    output[out_idx + 3] = br;
                    output[out_idx + 4] = bg;
                    output[out_idx + 5] = bb;
                }
            )";

        public:
            GPURenderer() = default;

            bool init(size_t width, size_t height)
            {
                if (_initialized && _buffer_size >= width * height * 3)
                    return true;

                try
                {
                    // Get platforms and devices
                    std::vector<cl::Platform> platforms;
                    cl::Platform::get(&platforms);
                    if (platforms.empty())
                        return false;

                    // Try to find a GPU device
                    cl::Device device;
                    bool found = false;

                    for (auto &platform : platforms)
                    {
                        std::vector<cl::Device> devices;
                        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
                        if (!devices.empty())
                        {
                            device = devices[0];
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        // Fall back to any device (CPU)
                        for (auto &platform : platforms)
                        {
                            std::vector<cl::Device> devices;
                            platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
                            if (!devices.empty())
                            {
                                device = devices[0];
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found)
                        return false;

                    // Create context and command queue
                    _context = cl::Context(device);
                    _queue = cl::CommandQueue(_context, device);

                    // Build program
                    _program = cl::Program(_context, KERNEL_SOURCE);
                    _program.build({device});

                    // Create kernel
                    _rgb_to_ansi_kernel = cl::Kernel(_program, "rgb_to_ansi");

                    // Allocate buffers
                    size_t input_size = width * height * 3;
                    size_t output_size = (width * (height / 2)) * 6;

                    _input_buffer = cl::Buffer(_context, CL_MEM_READ_ONLY, input_size);
                    _output_buffer = cl::Buffer(_context, CL_MEM_WRITE_ONLY, output_size);
                    _buffer_size = input_size;

                    _initialized = true;
                    return true;
                }
                catch (...)
                {
                    _initialized = false;
                    return false;
                }
            }

            bool is_available() const { return _initialized; }

            // Process RGB frame and get color values for each character cell
            bool process_frame(const uint8_t *rgb_data, size_t width, size_t height,
                               std::vector<uint8_t> &output)
            {
                if (!_initialized)
                    return false;

                try
                {
                    size_t input_size = width * height * 3;
                    size_t num_chars = width * (height / 2);
                    size_t output_size = num_chars * 6;

                    // Write input data
                    _queue.enqueueWriteBuffer(_input_buffer, CL_TRUE, 0, input_size, rgb_data);

                    // Set kernel arguments
                    _rgb_to_ansi_kernel.setArg(0, _input_buffer);
                    _rgb_to_ansi_kernel.setArg(1, _output_buffer);
                    _rgb_to_ansi_kernel.setArg(2, static_cast<int>(width));
                    _rgb_to_ansi_kernel.setArg(3, static_cast<int>(height));
                    _rgb_to_ansi_kernel.setArg(4, static_cast<int>(width));

                    // Execute kernel
                    _queue.enqueueNDRangeKernel(_rgb_to_ansi_kernel, cl::NullRange,
                                                cl::NDRange(num_chars), cl::NullRange);

                    // Read output
                    output.resize(output_size);
                    _queue.enqueueReadBuffer(_output_buffer, CL_TRUE, 0, output_size, output.data());

                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }
        };

        // Global GPU renderer instance
        inline GPURenderer &get_gpu_renderer()
        {
            static GPURenderer renderer;
            return renderer;
        }

        inline bool has_gpu_support()
        {
            static bool checked = false;
            static bool available = false;
            if (!checked)
            {
                GPURenderer test;
                available = test.init(320, 240);
                checked = true;
            }
            return available;
        }
#else
        inline bool has_gpu_support() { return false; }
#endif

        // ==================== Platform-specific Terminal Setup ====================

        /**
         * @brief Enable ANSI escape sequences on Windows
         * Call this once at startup for colored output on Windows terminals
         */
        inline void enable_ansi_support()
        {
#ifdef _WIN32
            static bool initialized = false;
            if (!initialized)
            {
                HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                if (hOut != INVALID_HANDLE_VALUE)
                {
                    DWORD dwMode = 0;
                    if (GetConsoleMode(hOut, &dwMode))
                    {
                        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                        SetConsoleMode(hOut, dwMode);
                    }
                    SetConsoleOutputCP(CP_UTF8); // Force UTF-8 for Braille and Unicode
                }
                initialized = true;
            }
#endif
            // Linux/macOS terminals support ANSI by default
        }

        /**
         * @brief Check if the terminal likely supports true color
         */
        inline bool terminal_supports_truecolor()
        {
#ifdef _WIN32
            // Windows Terminal and modern ConHost support true color
            const char *wt = std::getenv("WT_SESSION");
            const char *term_program = std::getenv("TERM_PROGRAM");
            return wt != nullptr || (term_program && std::string(term_program) == "vscode");
#else
            const char *colorterm = std::getenv("COLORTERM");
            if (colorterm)
            {
                std::string ct(colorterm);
                return ct == "truecolor" || ct == "24bit";
            }
            const char *term = std::getenv("TERM");
            if (term)
            {
                std::string t(term);
                return t.find("256color") != std::string::npos ||
                       t.find("truecolor") != std::string::npos;
            }
            return false;
#endif
        }

        // Braille dot bit values — defined in pythonicAccel.hpp
        using pythonic::accel::braille::DOTS;
        // Alias for backward compatibility within this file
        static constexpr auto &BRAILLE_DOTS = DOTS;

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
         * Returns a const reference to avoid heap allocation (LUT-backed)
         */
        inline const std::string &braille_to_utf8(uint8_t bits)
        {
            return braille_lut().get(bits);
        }

        /**
         * @brief ANSI escape codes for terminal control
         */
        namespace ansi
        {
            constexpr const char *CURSOR_HOME = "\033[H";         // Move cursor to top-left
            constexpr const char *CLEAR_SCREEN = "\033[2J";       // Clear entire screen
            constexpr const char *HIDE_CURSOR = "\033[?25l";      // Hide cursor
            constexpr const char *SHOW_CURSOR = "\033[?25h";      // Show cursor
            constexpr const char *RESET = "\033[0m";              // Reset all attributes
            constexpr const char *ALT_SCREEN_ON = "\033[?1049h";  // Enter alternate screen buffer
            constexpr const char *ALT_SCREEN_OFF = "\033[?1049l"; // Leave alternate screen buffer (restores)

            inline std::string cursor_to(int row, int col)
            {
                return "\033[" + std::to_string(row + 1) + ";" + std::to_string(col + 1) + "H";
            }

            /**
             * @brief Generate ANSI true color foreground escape code
             * Optimized: uses snprintf into stack buffer to avoid heap allocations
             */
            inline std::string fg_color(uint8_t r, uint8_t g, uint8_t b)
            {
                char buf[24];
                int n = snprintf(buf, sizeof(buf), "\033[38;2;%u;%u;%um", (unsigned)r, (unsigned)g, (unsigned)b);
                return std::string(buf, n);
            }

            /**
             * @brief Append ANSI true color foreground escape code to a string (zero-alloc)
             */
            inline void fg_color_append(std::string &out, uint8_t r, uint8_t g, uint8_t b)
            {
                char buf[24];
                int n = snprintf(buf, sizeof(buf), "\033[38;2;%u;%u;%um", (unsigned)r, (unsigned)g, (unsigned)b);
                out.append(buf, n);
            }

            /**
             * @brief Generate ANSI true color background escape code
             * Optimized: uses snprintf into stack buffer to avoid heap allocations
             */
            inline std::string bg_color(uint8_t r, uint8_t g, uint8_t b)
            {
                char buf[24];
                int n = snprintf(buf, sizeof(buf), "\033[48;2;%u;%u;%um", (unsigned)r, (unsigned)g, (unsigned)b);
                return std::string(buf, n);
            }

            /**
             * @brief Append ANSI true color background escape code to a string (zero-alloc)
             */
            inline void bg_color_append(std::string &out, uint8_t r, uint8_t g, uint8_t b)
            {
                char buf[24];
                int n = snprintf(buf, sizeof(buf), "\033[48;2;%u;%u;%um", (unsigned)r, (unsigned)g, (unsigned)b);
                out.append(buf, n);
            }
        } // namespace ansi

        // ==================== Color Canvas for True Color Rendering ====================

        /**
         * @brief RGB color structure
         */
        struct RGB
        {
            uint8_t r, g, b;

            RGB() : r(0), g(0), b(0) {}
            RGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

            bool operator==(const RGB &other) const
            {
                return r == other.r && g == other.g && b == other.b;
            }

            bool operator!=(const RGB &other) const
            {
                return !(*this == other);
            }

            /**
             * @brief Convert to grayscale value
             */
            uint8_t to_gray() const
            {
                return accel::pixel::to_gray(r, g, b);
            }
        };

        /**
         * @brief Color canvas for true color (24-bit) terminal rendering
         *
         * Uses Unicode half-block characters (▀ U+2580) with foreground and
         * background colors to achieve 2 vertical pixels per character cell.
         * This provides better color fidelity than braille patterns.
         */
        class ColorCanvas
        {
        private:
            size_t _char_width;   // Width in characters
            size_t _char_height;  // Height in characters (2 pixels per char)
            size_t _pixel_width;  // Width in pixels (same as char_width)
            size_t _pixel_height; // Height in pixels (char_height * 2)

            // Storage: RGB per pixel
            std::vector<std::vector<RGB>> _pixels;

        public:
            /**
             * @brief Create a color canvas with given character dimensions
             */
            ColorCanvas(size_t char_width = 80, size_t char_height = 24)
                : _char_width(char_width), _char_height(char_height), _pixel_width(char_width), _pixel_height(char_height * 2), _pixels(_pixel_height, std::vector<RGB>(_pixel_width))
            {
            }

            /**
             * @brief Create canvas from pixel dimensions
             */
            static ColorCanvas from_pixels(size_t pixel_width, size_t pixel_height)
            {
                size_t cw = pixel_width;
                size_t ch = (pixel_height + 1) / 2; // 2 pixels per character vertically
                return ColorCanvas(cw, ch);
            }

            // Dimensions
            size_t char_width() const { return _char_width; }
            size_t char_height() const { return _char_height; }
            size_t pixel_width() const { return _pixel_width; }
            size_t pixel_height() const { return _pixel_height; }

            /**
             * @brief Clear the canvas to black
             */
            void clear()
            {
                for (auto &row : _pixels)
                    std::fill(row.begin(), row.end(), RGB(0, 0, 0));
            }

            /**
             * @brief Set a pixel color
             */
            void set_pixel(int x, int y, RGB color)
            {
                if (x >= 0 && x < (int)_pixel_width && y >= 0 && y < (int)_pixel_height)
                    _pixels[y][x] = color;
            }

            /**
             * @brief Get a pixel color
             */
            RGB get_pixel(int x, int y) const
            {
                if (x >= 0 && x < (int)_pixel_width && y >= 0 && y < (int)_pixel_height)
                    return _pixels[y][x];
                return RGB(0, 0, 0);
            }

            /**
             * @brief Load RGB frame data directly
             */
            void load_frame_rgb(const uint8_t *data, int width, int height)
            {
                // Resize if needed
                if ((size_t)width != _pixel_width || (size_t)height != _pixel_height)
                {
                    _pixel_width = width;
                    _pixel_height = height;
                    _char_width = width;
                    _char_height = (height + 1) / 2;
                    _pixels.assign(_pixel_height, std::vector<RGB>(_pixel_width));
                }

                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        size_t idx = (y * width + x) * 3;
                        _pixels[y][x] = RGB(data[idx], data[idx + 1], data[idx + 2]);
                    }
                }
            }

            /**
             * @brief Render canvas to string with ANSI true color codes
             *
             * Uses the upper half block character (▀) with:
             * - Foreground color for top pixel
             * - Background color for bottom pixel
             */
            std::string render() const
            {
                enable_ansi_support();

                // Upper half block (▀) - UTF-8 encoding
                const char UPPER_HALF[] = "\xe2\x96\x80";

                // Pre-allocate estimated buffer size to avoid reallocations
                // Each cell needs ~30 bytes for colors + 3 for char
                std::string out;
                out.reserve(_char_height * _char_width * 35 + _char_height * 10);

                RGB prev_fg(-1, -1, -1), prev_bg(-1, -1, -1);

                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    size_t py_top = cy * 2;
                    size_t py_bot = py_top + 1;

                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        RGB top = _pixels[py_top][cx];
                        RGB bot = (py_bot < _pixel_height) ? _pixels[py_bot][cx] : RGB(0, 0, 0);

                        // Optimize: only emit color codes when they change
                        // Use zero-alloc append versions
                        if (top != prev_fg)
                        {
                            ansi::fg_color_append(out, top.r, top.g, top.b);
                            prev_fg = top;
                        }
                        if (bot != prev_bg)
                        {
                            ansi::bg_color_append(out, bot.r, bot.g, bot.b);
                            prev_bg = bot;
                        }
                        out.append(UPPER_HALF, 3);
                    }
                    // Reset at end of line and newline
                    out.append(ansi::RESET);
                    out += '\n';
                    prev_fg = RGB(-1, -1, -1);
                    prev_bg = RGB(-1, -1, -1);
                }

                return out;
            }

#ifdef PYTHONIC_ENABLE_OPENCL
            /**
             * @brief GPU-accelerated render using OpenCL
             *
             * Uses GPU to process RGB data and generate ANSI escape sequences.
             * Falls back to CPU render() if GPU is not available.
             */
            std::string render_gpu(const uint8_t *raw_rgb_data, size_t width, size_t height) const
            {
                enable_ansi_support();

                auto &renderer = get_gpu_renderer();
                if (!renderer.init(width, height))
                {
                    // Fall back to CPU rendering
                    return render();
                }

                std::vector<uint8_t> gpu_output;
                if (!renderer.process_frame(raw_rgb_data, width, height, gpu_output))
                {
                    return render();
                }

                // Build ANSI string from GPU output
                const char *UPPER_HALF = "\xe2\x96\x80";

                size_t num_chars = width * (height / 2);
                std::string out;
                out.reserve(num_chars * 35 + (height / 2) * 10);

                RGB prev_fg(-1, -1, -1), prev_bg(-1, -1, -1);

                for (size_t i = 0; i < num_chars; ++i)
                {
                    size_t idx = i * 6;
                    RGB fg(gpu_output[idx], gpu_output[idx + 1], gpu_output[idx + 2]);
                    RGB bg(gpu_output[idx + 3], gpu_output[idx + 4], gpu_output[idx + 5]);

                    if (fg != prev_fg)
                    {
                        out += ansi::fg_color(fg.r, fg.g, fg.b);
                        prev_fg = fg;
                    }
                    if (bg != prev_bg)
                    {
                        out += ansi::bg_color(bg.r, bg.g, bg.b);
                        prev_bg = bg;
                    }
                    out += UPPER_HALF;

                    // End of row
                    if ((i + 1) % width == 0)
                    {
                        out += ansi::RESET;
                        out += '\n';
                        prev_fg = RGB(-1, -1, -1);
                        prev_bg = RGB(-1, -1, -1);
                    }
                }

                return out;
            }
#endif
        };

        /**
         * @brief Colored Braille canvas for high-resolution colored terminal graphics
         *
         * Combines the high-resolution of Braille patterns (2×4 dots per character)
         * with foreground color support. Since a terminal cell only supports one
         * foreground color, all dots in a Braille character share the same color.
         *
         * The color is computed as the average of all "on" pixel colors in the 2×4 grid.
         *
         * Features:
         * - 8x resolution compared to colored block mode
         * - One foreground color per Braille cell (averaged from source pixels)
         * - Optional background color support
         * - Floyd-Steinberg dithering for better edge definition
         */
        class ColoredBrailleCanvas
        {
        private:
            size_t _char_width;   // Width in characters
            size_t _char_height;  // Height in characters
            size_t _pixel_width;  // Width in pixels (char_width * 2)
            size_t _pixel_height; // Height in pixels (char_height * 4)

            // Storage: one byte pattern + RGB color per character cell
            std::vector<std::vector<uint8_t>> _patterns;
            std::vector<std::vector<RGB>> _colors;

        public:
            ColoredBrailleCanvas(size_t char_width = 80, size_t char_height = 24)
                : _char_width(char_width), _char_height(char_height), _pixel_width(char_width * 2), _pixel_height(char_height * 4), _patterns(char_height, std::vector<uint8_t>(char_width, 0)), _colors(char_height, std::vector<RGB>(char_width))
            {
            }

            static ColoredBrailleCanvas from_pixels(size_t pixel_width, size_t pixel_height)
            {
                size_t cw = (pixel_width + 1) / 2;
                size_t ch = (pixel_height + 3) / 4;
                return ColoredBrailleCanvas(cw, ch);
            }

            size_t char_width() const { return _char_width; }
            size_t char_height() const { return _char_height; }
            size_t pixel_width() const { return _pixel_width; }
            size_t pixel_height() const { return _pixel_height; }

            void clear()
            {
                for (auto &row : _patterns)
                    std::fill(row.begin(), row.end(), 0);
                for (auto &row : _colors)
                    std::fill(row.begin(), row.end(), RGB(0, 0, 0));
            }

            /**
             * @brief Set the Braille dot pattern for a character cell
             * @param cx Character column (0-based)
             * @param cy Character row (0-based)
             * @param pattern 8-bit pattern where each bit represents a dot
             */
            void set_pattern(size_t cx, size_t cy, uint8_t pattern)
            {
                if (cx < _char_width && cy < _char_height)
                {
                    _patterns[cy][cx] = pattern;
                }
            }

            /**
             * @brief Get the Braille dot pattern for a character cell
             * @param cx Character column (0-based)
             * @param cy Character row (0-based)
             * @return 8-bit pattern where each bit represents a dot, or 0 if out of bounds
             */
            uint8_t get_pattern(size_t cx, size_t cy) const
            {
                if (cx < _char_width && cy < _char_height)
                {
                    return _patterns[cy][cx];
                }
                return 0;
            }

            /**
             * @brief Set the color for a character cell
             * @param cx Character column (0-based)
             * @param cy Character row (0-based)
             * @param r Red component (0-255)
             * @param g Green component (0-255)
             * @param b Blue component (0-255)
             */
            void set_color(size_t cx, size_t cy, uint8_t r, uint8_t g, uint8_t b)
            {
                if (cx < _char_width && cy < _char_height)
                {
                    _colors[cy][cx] = RGB(r, g, b);
                }
            }

            /**
             * @brief Load RGB frame and convert to colored Braille
             *
             * For each 2×4 cell:
             * 1. Compute grayscale for thresholding to determine which dots are "on"
             * 2. Average the RGB colors of all "on" pixels to get cell color
             */
            void load_frame_rgb(const uint8_t *data, int width, int height, uint8_t threshold = 128)
            {
                // Resize if needed
                size_t new_cw = (width + 1) / 2;
                size_t new_ch = (height + 3) / 4;

                if (new_cw != _char_width || new_ch != _char_height)
                {
                    _char_width = new_cw;
                    _char_height = new_ch;
                    _pixel_width = _char_width * 2;
                    _pixel_height = _char_height * 4;
                    _patterns.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                    _colors.assign(_char_height, std::vector<RGB>(_char_width));
                }

                // Process each character cell
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        int px = cx * 2;
                        int py = cy * 4;

                        uint8_t pattern = 0;
                        int r_sum = 0, g_sum = 0, b_sum = 0;
                        int on_count = 0;

                        // Process 2×4 block
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
                                uint8_t r = data[idx];
                                uint8_t g = data[idx + 1];
                                uint8_t b = data[idx + 2];

                                // Grayscale for threshold
                                uint8_t gray = accel::pixel::to_gray(r, g, b);

                                if (gray >= threshold)
                                {
                                    // Set corresponding Braille bit
                                    pattern |= BRAILLE_DOTS[row][col];
                                    r_sum += r;
                                    g_sum += g;
                                    b_sum += b;
                                    on_count++;
                                }
                            }
                        }

                        _patterns[cy][cx] = pattern;

                        // Average color of "on" pixels
                        if (on_count > 0)
                        {
                            _colors[cy][cx] = RGB(
                                static_cast<uint8_t>(r_sum / on_count),
                                static_cast<uint8_t>(g_sum / on_count),
                                static_cast<uint8_t>(b_sum / on_count));
                        }
                        else
                        {
                            _colors[cy][cx] = RGB(0, 0, 0);
                        }
                    }
                }
            }

            /**
             * @brief Load RGB frame with luminance-based thresholding (adaptive)
             *
             * Uses local contrast to determine threshold, producing better results
             * for images with varying brightness levels.
             */
            void load_frame_rgb_adaptive(const uint8_t *data, int width, int height)
            {
                // Use a dynamic threshold based on local average
                load_frame_rgb(data, width, height, 128); // For now, use fixed threshold
            }

            /**
             * @brief Load RGB frame with flood fill (all dots on, colored by average RGB)
             *
             * For each 2×4 cell:
             * - All 8 dots are lit (pattern = 0xFF = ⣿)
             * - Color is the average RGB of all 8 pixels
             *
             * This creates the smoothest appearance with true colors.
             */
            void load_frame_rgb_flood(const uint8_t *data, int width, int height)
            {
                // Resize if needed
                size_t new_cw = (width + 1) / 2;
                size_t new_ch = (height + 3) / 4;

                if (new_cw != _char_width || new_ch != _char_height)
                {
                    _char_width = new_cw;
                    _char_height = new_ch;
                    _pixel_width = _char_width * 2;
                    _pixel_height = _char_height * 4;
                    _patterns.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                    _colors.assign(_char_height, std::vector<RGB>(_char_width));
                }

                // Process each character cell
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        int px = cx * 2;
                        int py = cy * 4;

                        int r_sum = 0, g_sum = 0, b_sum = 0;
                        int pixel_count = 0;

                        // Process 2×4 block - average ALL pixels
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
                                r_sum += data[idx];
                                g_sum += data[idx + 1];
                                b_sum += data[idx + 2];
                                pixel_count++;
                            }
                        }

                        // All dots lit
                        _patterns[cy][cx] = 0xFF;

                        // Average color of ALL pixels in cell
                        if (pixel_count > 0)
                        {
                            _colors[cy][cx] = RGB(
                                static_cast<uint8_t>(r_sum / pixel_count),
                                static_cast<uint8_t>(g_sum / pixel_count),
                                static_cast<uint8_t>(b_sum / pixel_count));
                        }
                        else
                        {
                            _colors[cy][cx] = RGB(0, 0, 0);
                        }
                    }
                }
            }

            /**
             * @brief Load RGB frame with ordered dithering for colored output
             *
             * Uses Bayer 2x2 dithering matrix to determine which dots are lit,
             * then colors them based on average RGB. Provides smoother appearance
             * than simple threshold for colored content.
             */
            void load_frame_rgb_dithered(const uint8_t *data, int width, int height)
            {
                // Bayer 2x2 dithering matrix — defined in pythonicAccel.hpp
                static constexpr auto &BAYER = pythonic::accel::dither::BAYER_2x2;

                // Resize if needed
                size_t new_cw = (width + 1) / 2;
                size_t new_ch = (height + 3) / 4;

                if (new_cw != _char_width || new_ch != _char_height)
                {
                    _char_width = new_cw;
                    _char_height = new_ch;
                    _pixel_width = _char_width * 2;
                    _pixel_height = _char_height * 4;
                    _patterns.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                    _colors.assign(_char_height, std::vector<RGB>(_char_width));
                }

                // Process each character cell
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        int px = cx * 2;
                        int py = cy * 4;

                        uint8_t pattern = 0;
                        int r_sum = 0, g_sum = 0, b_sum = 0;
                        int on_count = 0;

                        // Process 2×4 block with dithering
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
                                uint8_t r = data[idx];
                                uint8_t g = data[idx + 1];
                                uint8_t b = data[idx + 2];

                                // Grayscale for dithering decision
                                uint8_t gray = accel::pixel::to_gray(r, g, b);

                                // Apply dithering threshold
                                int dither_threshold = BAYER[row % 2][col % 2];
                                if (gray >= dither_threshold)
                                {
                                    pattern |= BRAILLE_DOTS[row][col];
                                    r_sum += r;
                                    g_sum += g;
                                    b_sum += b;
                                    on_count++;
                                }
                            }
                        }

                        _patterns[cy][cx] = pattern;

                        // Average color of "on" pixels
                        if (on_count > 0)
                        {
                            _colors[cy][cx] = RGB(
                                static_cast<uint8_t>(r_sum / on_count),
                                static_cast<uint8_t>(g_sum / on_count),
                                static_cast<uint8_t>(b_sum / on_count));
                        }
                        else
                        {
                            _colors[cy][cx] = RGB(128, 128, 128); // Default gray for empty cells
                        }
                    }
                }
            }

            /**
             * @brief Load a PPM file with flood fill mode (all dots on, RGB colored)
             *
             * For each 2×4 cell, all 8 dots are lit and colored by average RGB.
             *
             * @param filename Path to PPM file (P6 format)
             * @return true on success
             */
            bool load_ppm_flood(const std::string &filename)
            {
                auto img = pythonic::accel::image_io::load_ppm_pgm(filename);
                if (!img.valid() || !img.is_color)
                    return false;
                int width = img.width;
                int height = img.height;
                const auto &rgbbuf = img.data;

                // Resize canvas
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _patterns.assign(_char_height, std::vector<uint8_t>(_char_width, 0xFF)); // All dots lit
                _colors.assign(_char_height, std::vector<RGB>(_char_width));

                // Process each character cell - compute average RGB
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        int px = cx * 2;
                        int py = cy * 4;
                        int r_sum = 0, g_sum = 0, b_sum = 0;
                        int count = 0;

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
                                r_sum += rgbbuf[idx];
                                g_sum += rgbbuf[idx + 1];
                                b_sum += rgbbuf[idx + 2];
                                count++;
                            }
                        }

                        if (count > 0)
                        {
                            _colors[cy][cx] = RGB(r_sum / count, g_sum / count, b_sum / count);
                        }
                    }
                }

                return true;
            }

            /**
             * @brief Load a PPM file with dithering (colored dithered dots)
             *
             * Uses Bayer 2x2 dithering for dot patterns with RGB coloring.
             *
             * @param filename Path to PPM file (P6 format)
             * @return true on success
             */
            bool load_ppm_dithered(const std::string &filename)
            {
                auto img = pythonic::accel::image_io::load_ppm_pgm(filename);
                if (!img.valid() || !img.is_color)
                    return false;
                int width = img.width;
                int height = img.height;
                const auto &rgbbuf = img.data;

                // Bayer 2x2 matrix — defined in pythonicAccel.hpp
                static constexpr auto &BAYER = pythonic::accel::dither::BAYER_2x2;

                // Resize canvas
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _patterns.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                _colors.assign(_char_height, std::vector<RGB>(_char_width));

                // Process each character cell
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        int px = cx * 2;
                        int py = cy * 4;
                        uint8_t pattern = 0;
                        int r_sum = 0, g_sum = 0, b_sum = 0;
                        int on_count = 0;
                        int total_count = 0;

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
                                uint8_t r = rgbbuf[idx];
                                uint8_t g = rgbbuf[idx + 1];
                                uint8_t b = rgbbuf[idx + 2];
                                r_sum += r;
                                g_sum += g;
                                b_sum += b;
                                total_count++;

                                // Grayscale for dithering
                                uint8_t gray = accel::pixel::to_gray(r, g, b);
                                if (gray >= BAYER[row % 2][col % 2])
                                {
                                    pattern |= BRAILLE_DOTS[row][col];
                                    on_count++;
                                }
                            }
                        }

                        _patterns[cy][cx] = pattern;

                        // Average color of all pixels (not just "on" ones for better color fidelity)
                        if (total_count > 0)
                        {
                            _colors[cy][cx] = RGB(r_sum / total_count, g_sum / total_count, b_sum / total_count);
                        }
                    }
                }

                return true;
            }

            /**
             * @brief Load a PPM image with Floyd-Steinberg dithering (colored)
             *
             * Floyd-Steinberg error diffusion provides higher quality dithering
             * with better edge preservation. Slightly slower than ordered dithering.
             *
             * @param filename Path to PPM file (P6 format)
             * @return true on success
             */
            bool load_ppm_dithered_floyd(const std::string &filename)
            {
                auto img = pythonic::accel::image_io::load_ppm_pgm(filename);
                if (!img.valid() || !img.is_color)
                    return false;
                int width = img.width;
                int height = img.height;
                const auto &rgbbuf = img.data;

                // Resize canvas
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _patterns.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                _colors.assign(_char_height, std::vector<RGB>(_char_width));

                // Convert to grayscale float buffer for error diffusion
                std::vector<std::vector<float>> gray(height, std::vector<float>(width));
                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        size_t idx = (y * width + x) * 3;
                        gray[y][x] = static_cast<float>(accel::pixel::to_gray(rgbbuf[idx], rgbbuf[idx + 1], rgbbuf[idx + 2]));
                    }
                }

                // Apply Floyd-Steinberg dithering
                std::vector<std::vector<bool>> dithered(height, std::vector<bool>(width, false));
                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        float old_pixel = gray[y][x];
                        float new_pixel = (old_pixel >= 128.0f) ? 255.0f : 0.0f;
                        dithered[y][x] = (new_pixel > 0);

                        float error = old_pixel - new_pixel;

                        // Distribute error to neighbors
                        if (x + 1 < width)
                            gray[y][x + 1] += error * 7.0f / 16.0f;
                        if (y + 1 < height)
                        {
                            if (x > 0)
                                gray[y + 1][x - 1] += error * 3.0f / 16.0f;
                            gray[y + 1][x] += error * 5.0f / 16.0f;
                            if (x + 1 < width)
                                gray[y + 1][x + 1] += error * 1.0f / 16.0f;
                        }
                    }
                }

                // Build patterns and colors from dithered result
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        int px = cx * 2;
                        int py = cy * 4;
                        uint8_t pattern = 0;
                        int r_sum = 0, g_sum = 0, b_sum = 0;
                        int total_count = 0;

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
                                r_sum += rgbbuf[idx];
                                g_sum += rgbbuf[idx + 1];
                                b_sum += rgbbuf[idx + 2];
                                total_count++;

                                if (dithered[y][x])
                                {
                                    pattern |= BRAILLE_DOTS[row][col];
                                }
                            }
                        }

                        _patterns[cy][cx] = pattern;

                        if (total_count > 0)
                        {
                            _colors[cy][cx] = RGB(r_sum / total_count, g_sum / total_count, b_sum / total_count);
                        }
                    }
                }

                return true;
            }

            /**
             * @brief Render to ANSI string with colored Braille characters
             * Optimized: uses zero-alloc fg_color_append
             */
            std::string render() const
            {
                enable_ansi_support();

                std::string out;
                out.reserve(_char_height * _char_width * 30);

                RGB prev_color(-1, -1, -1);

                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        uint8_t pattern = _patterns[cy][cx];
                        RGB color = _colors[cy][cx];

                        // Only output color code if it changed and pattern is non-empty
                        if (pattern != 0 && color != prev_color)
                        {
                            ansi::fg_color_append(out, color.r, color.g, color.b);
                            prev_color = color;
                        }

                        const std::string &ch = braille_to_utf8(pattern);
                        out.append(ch);
                    }

                    out += ansi::RESET;
                    out += '\n';
                    prev_color = RGB(-1, -1, -1);
                }

                return out;
            }
        };

        /**
         * @brief Grayscale canvas using half-block characters
         *
         * Uses ANSI 256-color grayscale palette (colors 232-255) to render
         * grayscale images with 24 levels of gray for smoother, more detailed
         * uncolored output than pure black/white.
         */
        class BWBlockCanvas
        {
        private:
            size_t _char_width;
            size_t _char_height;
            size_t _pixel_width;
            size_t _pixel_height;

            // Store actual grayscale values for top and bottom pixels
            std::vector<std::vector<std::pair<uint8_t, uint8_t>>> _pixels;

        public:
            BWBlockCanvas(size_t char_width = 80, size_t char_height = 24)
                : _char_width(char_width), _char_height(char_height), _pixel_width(char_width), _pixel_height(char_height * 2), _pixels(char_height, std::vector<std::pair<uint8_t, uint8_t>>(char_width, {0, 0}))
            {
            }

            static BWBlockCanvas from_pixels(size_t pixel_width, size_t pixel_height)
            {
                size_t cw = pixel_width;
                size_t ch = (pixel_height + 1) / 2;
                return BWBlockCanvas(cw, ch);
            }

            size_t char_width() const { return _char_width; }
            size_t char_height() const { return _char_height; }
            size_t pixel_width() const { return _pixel_width; }
            size_t pixel_height() const { return _pixel_height; }

            void clear()
            {
                for (auto &row : _pixels)
                    std::fill(row.begin(), row.end(), std::make_pair<uint8_t, uint8_t>(0, 0));
            }

            /// Convert 0-255 grayscale to ANSI 256-color grayscale (232-255, 24 levels)
            static int gray_to_ansi256(uint8_t gray)
            {
                return 232 + (gray * 23 / 255);
            }

            void load_frame_gray(const uint8_t *data, int width, int height, [[maybe_unused]] uint8_t threshold = 128)
            {
                size_t new_cw = width;
                size_t new_ch = (height + 1) / 2;

                if (new_cw != _char_width || new_ch != _char_height)
                {
                    _char_width = new_cw;
                    _char_height = new_ch;
                    _pixel_width = width;
                    _pixel_height = _char_height * 2;
                    _pixels.assign(_char_height, std::vector<std::pair<uint8_t, uint8_t>>(_char_width, {0, 0}));
                }

                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    size_t py_top = cy * 2;
                    size_t py_bot = py_top + 1;

                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        uint8_t gray_top = 0;
                        uint8_t gray_bot = 0;

                        // Top pixel - store actual grayscale value
                        if (py_top < static_cast<size_t>(height))
                        {
                            gray_top = data[py_top * width + cx];
                        }

                        // Bottom pixel - store actual grayscale value
                        if (py_bot < static_cast<size_t>(height))
                        {
                            gray_bot = data[py_bot * width + cx];
                        }

                        _pixels[cy][cx] = {gray_top, gray_bot};
                    }
                }
            }

            void load_frame_rgb(const uint8_t *data, int width, int height, [[maybe_unused]] uint8_t threshold = 128)
            {
                size_t new_cw = width;
                size_t new_ch = (height + 1) / 2;

                if (new_cw != _char_width || new_ch != _char_height)
                {
                    _char_width = new_cw;
                    _char_height = new_ch;
                    _pixel_width = width;
                    _pixel_height = _char_height * 2;
                    _pixels.assign(_char_height, std::vector<std::pair<uint8_t, uint8_t>>(_char_width, {0, 0}));
                }

                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    size_t py_top = cy * 2;
                    size_t py_bot = py_top + 1;

                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        uint8_t gray_top = 0;
                        uint8_t gray_bot = 0;

                        // Top pixel - convert RGB to grayscale
                        if (py_top < static_cast<size_t>(height))
                        {
                            size_t idx = (py_top * width + cx) * 3;
                            gray_top = accel::pixel::to_gray(data[idx], data[idx + 1], data[idx + 2]);
                        }

                        // Bottom pixel - convert RGB to grayscale
                        if (py_bot < static_cast<size_t>(height))
                        {
                            size_t idx = (py_bot * width + cx) * 3;
                            gray_bot = accel::pixel::to_gray(data[idx], data[idx + 1], data[idx + 2]);
                        }

                        _pixels[cy][cx] = {gray_top, gray_bot};
                    }
                }
            }

            std::string render() const
            {
                // Uses ANSI 256-color grayscale (colors 232-255, 24 levels)
                // with ▀ (top half block) character:
                // - Foreground color = top pixel grayscale
                // - Background color = bottom pixel grayscale

                const char *TOP_HALF = "\xe2\x96\x80"; // ▀
                const char *RESET = "\033[0m";

                std::string out;
                out.reserve(_char_height * (_char_width * 25 + 10)); // Account for ANSI codes

                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        auto [gray_top, gray_bot] = _pixels[cy][cx];
                        int fg = gray_to_ansi256(gray_top);
                        int bg = gray_to_ansi256(gray_bot);

                        // Set foreground and background colors using ANSI 256-color
                        out += "\033[38;5;";
                        out += std::to_string(fg);
                        out += ";48;5;";
                        out += std::to_string(bg);
                        out += "m";
                        out += TOP_HALF;
                    }
                    out += RESET;
                    out += '\n';
                }

                return out;
            }

            /**
             * @brief Fast render using 24-bit ANSI true color (faster than 256-color)
             * Uses true color which most modern terminals support.
             */
            std::string render_truecolor() const
            {
                const char *TOP_HALF = "\xe2\x96\x80"; // ▀
                const char *RESET = "\033[0m";

                std::string out;
                out.reserve(_char_height * (_char_width * 40 + 10));

                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        auto [gray_top, gray_bot] = _pixels[cy][cx];

                        // True color: \033[38;2;r;g;b;48;2;r;g;bm
                        out += "\033[38;2;";
                        out += std::to_string(gray_top);
                        out += ";";
                        out += std::to_string(gray_top);
                        out += ";";
                        out += std::to_string(gray_top);
                        out += ";48;2;";
                        out += std::to_string(gray_bot);
                        out += ";";
                        out += std::to_string(gray_bot);
                        out += ";";
                        out += std::to_string(gray_bot);
                        out += "m";
                        out += TOP_HALF;
                    }
                    out += RESET;
                    out += '\n';
                }

                return out;
            }

            /**
             * @brief Get raw grayscale pixel data for direct export (bypasses ANSI string)
             *
             * Returns pixels in row-major order, 2 bytes per character cell (top, bottom).
             * Use this for fast video export to avoid ANSI string parsing overhead.
             *
             * @return Vector of (top_gray, bottom_gray) pairs, row-major
             */
            const std::vector<std::vector<std::pair<uint8_t, uint8_t>>> &get_pixels() const
            {
                return _pixels;
            }

            size_t width() const { return _char_width; }
            size_t height() const { return _char_height; }
        };

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

            // Optional grayscale storage: average brightness per cell (0-255)
            // Only populated when grayscale rendering is needed
            std::vector<std::vector<uint8_t>> _grayscale;

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
             * @brief ORDERED DITHERING: Set block with 8-level grayscale shading
             *
             * Uses ordered dithering (Bayer-like pattern) optimized for braille's 2×4 dot grid.
             * This creates proper grayscale shading by turning on more/fewer dots based on brightness.
             *
             * The 8 dots in a braille cell naturally support 9 levels of brightness (0-8 dots lit).
             * Each dot position has a different threshold, creating smooth gradients.
             *
             * @param char_x Character cell X coordinate
             * @param char_y Character cell Y coordinate
             * @param gray Array of 8 grayscale values [row0_col0, row0_col1, row1_col0, ...]
             */
            void set_block_gray_dithered(int char_x, int char_y, const uint8_t gray[8])
            {
                if (char_x < 0 || char_x >= (int)_char_width ||
                    char_y < 0 || char_y >= (int)_char_height)
                    return;

                // Proper 2×4 ordered dithering matrix based on Bayer pattern
                // Values are evenly distributed from 0-255 across 8 positions
                // to create smooth gradients. Each threshold determines at what
                // brightness level that dot turns on.
                //
                // Matrix layout for braille (2 cols × 4 rows):
                //   [0,0]  [0,1]     thresholds:  16  144
                //   [1,0]  [1,1]                  80  208
                //   [2,0]  [2,1]                 112  240
                //   [3,0]  [3,1]                  48  176
                //
                // Dots light up progressively: at 10% brightness only the
                // lowest threshold dot lights; at 90% almost all light.
                // Ordered dither thresholds — defined in pythonicAccel.hpp
                static constexpr auto &dither_thresholds = pythonic::accel::dither::BRAILLE_ORDERED;

                // Compute pattern: dot lights if pixel brightness >= threshold
                uint8_t pattern = 0;
                pattern |= (gray[0] >= dither_thresholds[0]) ? 0x01 : 0; // row 0, col 0
                pattern |= (gray[1] >= dither_thresholds[1]) ? 0x08 : 0; // row 0, col 1
                pattern |= (gray[2] >= dither_thresholds[2]) ? 0x02 : 0; // row 1, col 0
                pattern |= (gray[3] >= dither_thresholds[3]) ? 0x10 : 0; // row 1, col 1
                pattern |= (gray[4] >= dither_thresholds[4]) ? 0x04 : 0; // row 2, col 0
                pattern |= (gray[5] >= dither_thresholds[5]) ? 0x20 : 0; // row 2, col 1
                pattern |= (gray[6] >= dither_thresholds[6]) ? 0x40 : 0; // row 3, col 0
                pattern |= (gray[7] >= dither_thresholds[7]) ? 0x80 : 0; // row 3, col 1

                _canvas[char_y][char_x] = pattern;
            }

            /**
             * @brief Set block and store average grayscale for later grayscale rendering
             *
             * Use this when you want to render with grayscale-colored dots.
             * The average brightness of the 8 pixels is stored per cell.
             *
             * @param char_x Character cell X coordinate
             * @param char_y Character cell Y coordinate
             * @param gray Array of 8 grayscale values
             * @param threshold Threshold for dot visibility
             */
            void set_block_gray_with_brightness(int char_x, int char_y, const uint8_t gray[8], uint8_t threshold)
            {
                if (char_x < 0 || char_x >= (int)_char_width ||
                    char_y < 0 || char_y >= (int)_char_height)
                    return;

                // Ensure grayscale storage is allocated
                if (_grayscale.empty() || _grayscale.size() != _char_height ||
                    (!_grayscale.empty() && _grayscale[0].size() != _char_width))
                {
                    _grayscale.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                }

                // Compute pattern with threshold
                uint8_t pattern = 0;
                int sum = 0;
                for (int i = 0; i < 8; ++i)
                {
                    sum += gray[i];
                }

                pattern |= (gray[0] >= threshold) ? 0x01 : 0;
                pattern |= (gray[1] >= threshold) ? 0x08 : 0;
                pattern |= (gray[2] >= threshold) ? 0x02 : 0;
                pattern |= (gray[3] >= threshold) ? 0x10 : 0;
                pattern |= (gray[4] >= threshold) ? 0x04 : 0;
                pattern |= (gray[5] >= threshold) ? 0x20 : 0;
                pattern |= (gray[6] >= threshold) ? 0x40 : 0;
                pattern |= (gray[7] >= threshold) ? 0x80 : 0;

                _canvas[char_y][char_x] = pattern;
                _grayscale[char_y][char_x] = static_cast<uint8_t>(sum / 8); // Average brightness
            }

            /**
             * @brief Set block with dithering and store grayscale for colored rendering
             *
             * @param char_x Character cell X coordinate
             * @param char_y Character cell Y coordinate
             * @param gray Array of 8 grayscale values
             */
            void set_block_gray_dithered_with_brightness(int char_x, int char_y, const uint8_t gray[8])
            {
                if (char_x < 0 || char_x >= (int)_char_width ||
                    char_y < 0 || char_y >= (int)_char_height)
                    return;

                // Ensure grayscale storage is allocated
                if (_grayscale.empty() || _grayscale.size() != _char_height ||
                    (!_grayscale.empty() && _grayscale[0].size() != _char_width))
                {
                    _grayscale.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                }

                // Ordered dither thresholds — defined in pythonicAccel.hpp
                static constexpr auto &dither_thresholds = pythonic::accel::dither::BRAILLE_ORDERED;

                uint8_t pattern = 0;
                int sum = 0;
                for (int i = 0; i < 8; ++i)
                {
                    sum += gray[i];
                }

                // Apply dithering thresholds
                pattern |= (gray[0] >= dither_thresholds[0]) ? 0x01 : 0;
                pattern |= (gray[1] >= dither_thresholds[1]) ? 0x08 : 0;
                pattern |= (gray[2] >= dither_thresholds[2]) ? 0x02 : 0;
                pattern |= (gray[3] >= dither_thresholds[3]) ? 0x10 : 0;
                pattern |= (gray[4] >= dither_thresholds[4]) ? 0x04 : 0;
                pattern |= (gray[5] >= dither_thresholds[5]) ? 0x20 : 0;
                pattern |= (gray[6] >= dither_thresholds[6]) ? 0x40 : 0;
                pattern |= (gray[7] >= dither_thresholds[7]) ? 0x80 : 0;

                _canvas[char_y][char_x] = pattern;
                _grayscale[char_y][char_x] = static_cast<uint8_t>(sum / 8);
            }

            /**
             * @brief FLOOD FILL: Set all dots lit and store average grayscale
             *
             * Unlike threshold or dithering modes, this lights up ALL 8 dots
             * in every cell and uses the average grayscale for coloring.
             * This creates the smoothest appearance for photos and videos
             * by relying entirely on color gradation rather than dot patterns.
             *
             * @param char_x Character cell X coordinate
             * @param char_y Character cell Y coordinate
             * @param gray Array of 8 grayscale values
             */
            void set_block_flood_fill(int char_x, int char_y, const uint8_t gray[8])
            {
                if (char_x < 0 || char_x >= (int)_char_width ||
                    char_y < 0 || char_y >= (int)_char_height)
                    return;

                // Ensure grayscale storage is allocated
                if (_grayscale.empty() || _grayscale.size() != _char_height ||
                    (!_grayscale.empty() && _grayscale[0].size() != _char_width))
                {
                    _grayscale.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                }

                // All dots lit (⣿)
                _canvas[char_y][char_x] = 0xFF;

                // Store average brightness for rendering
                int sum = 0;
                for (int i = 0; i < 8; ++i)
                {
                    sum += gray[i];
                }
                _grayscale[char_y][char_x] = static_cast<uint8_t>(sum / 8);
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
             * @brief ORDERED DITHERED: Load grayscale frame with ordered dithering for smooth gradients
             *
             * This version uses ordered dithering instead of simple thresholding,
             * creating proper grayscale shading by varying the number of lit dots.
             * Produces much better visual results for photos and videos.
             *
             * Faster than Floyd-Steinberg (no error propagation) and more stable
             * for video (no temporal noise artifacts).
             *
             * @param data Grayscale pixel data (row-major)
             * @param width Image width in pixels
             * @param height Image height in pixels
             */
            void load_frame_ordered_dithered(const uint8_t *data, int width, int height)
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

                // Process each character cell using dithered block operations
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

                        set_block_gray_dithered(cx, cy, gray);
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
                                gray[row * 2 + col] = accel::pixel::to_gray(data[idx], data[idx + 1], data[idx + 2]);
                            }
                        }

                        set_block_gray(cx, cy, gray, threshold);
                    }
                }
            }

            /**
             * @brief Load grayscale frame with Floyd-Steinberg dithering
             *
             * Floyd-Steinberg dithering distributes quantization error to neighboring
             * pixels, creating the illusion of shading and significantly improving
             * edge sharpness compared to simple thresholding.
             *
             * Error diffusion pattern:
             *        [ * ]  7/16
             *   3/16  5/16  1/16
             */
            void load_frame_dithered(const uint8_t *data, int width, int height)
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
                else
                {
                    clear();
                }

                // Create a working copy with float precision for error diffusion
                std::vector<std::vector<float>> buffer(height, std::vector<float>(width));
                for (int y = 0; y < height; ++y)
                    for (int x = 0; x < width; ++x)
                        buffer[y][x] = static_cast<float>(data[y * width + x]);

                // Apply Floyd-Steinberg dithering
                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        float old_pixel = buffer[y][x];
                        float new_pixel = (old_pixel >= 128.0f) ? 255.0f : 0.0f;
                        buffer[y][x] = new_pixel;

                        float error = old_pixel - new_pixel;

                        // Distribute error to neighbors
                        if (x + 1 < width)
                            buffer[y][x + 1] += error * 7.0f / 16.0f;
                        if (y + 1 < height)
                        {
                            if (x > 0)
                                buffer[y + 1][x - 1] += error * 3.0f / 16.0f;
                            buffer[y + 1][x] += error * 5.0f / 16.0f;
                            if (x + 1 < width)
                                buffer[y + 1][x + 1] += error * 1.0f / 16.0f;
                        }

                        // Set the pixel
                        if (new_pixel > 0)
                            set_pixel(x, y, true);
                    }
                }
            }

            /**
             * @brief Load RGB frame with Floyd-Steinberg dithering
             */
            void load_frame_rgb_dithered(const uint8_t *data, int width, int height)
            {
                // Convert to grayscale first
                std::vector<uint8_t> gray(width * height);
                for (int i = 0; i < width * height; ++i)
                {
                    size_t idx = i * 3;
                    gray[i] = accel::pixel::to_gray(data[idx], data[idx + 1], data[idx + 2]);
                }
                load_frame_dithered(gray.data(), width, height);
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
             * @brief Draw an anti-aliased line using Wu's algorithm
             *
             * Wu's algorithm uses sub-pixel precision and adjusts dot density
             * based on line position within the 2×4 braille grid, creating
             * smoother, less jagged curves.
             *
             * @param x0 Start X coordinate (can be float for sub-pixel precision)
             * @param y0 Start Y coordinate
             * @param x1 End X coordinate
             * @param y1 End Y coordinate
             */
            void line_aa(double x0, double y0, double x1, double y1)
            {
                auto ipart = [](double x)
                { return std::floor(x); };
                auto fpart = [](double x)
                { return x - std::floor(x); };
                auto rfpart = [&](double x)
                { return 1.0 - fpart(x); };

                // Plot a pixel with intensity (0.0-1.0)
                // For braille, we use a threshold to decide if dot is on
                auto plot = [this](int x, int y, double intensity)
                {
                    // Use intensity as probability threshold
                    // Higher intensity = more likely to set pixel
                    if (intensity > 0.3) // Threshold for visibility
                        set_pixel(x, y, true);
                };

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

                // Handle first endpoint
                double xend = std::round(x0);
                double yend = y0 + gradient * (xend - x0);
                double xgap = rfpart(x0 + 0.5);
                int xpxl1 = static_cast<int>(xend);
                int ypxl1 = static_cast<int>(ipart(yend));

                if (steep)
                {
                    plot(ypxl1, xpxl1, rfpart(yend) * xgap);
                    plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap);
                }
                else
                {
                    plot(xpxl1, ypxl1, rfpart(yend) * xgap);
                    plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
                }

                double intery = yend + gradient;

                // Handle second endpoint
                xend = std::round(x1);
                yend = y1 + gradient * (xend - x1);
                xgap = fpart(x1 + 0.5);
                int xpxl2 = static_cast<int>(xend);
                int ypxl2 = static_cast<int>(ipart(yend));

                if (steep)
                {
                    plot(ypxl2, xpxl2, rfpart(yend) * xgap);
                    plot(ypxl2 + 1, xpxl2, fpart(yend) * xgap);
                }
                else
                {
                    plot(xpxl2, ypxl2, rfpart(yend) * xgap);
                    plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap);
                }

                // Main loop
                if (steep)
                {
                    for (int x = xpxl1 + 1; x < xpxl2; ++x)
                    {
                        plot(static_cast<int>(ipart(intery)), x, rfpart(intery));
                        plot(static_cast<int>(ipart(intery)) + 1, x, fpart(intery));
                        intery += gradient;
                    }
                }
                else
                {
                    for (int x = xpxl1 + 1; x < xpxl2; ++x)
                    {
                        plot(x, static_cast<int>(ipart(intery)), rfpart(intery));
                        plot(x, static_cast<int>(ipart(intery)) + 1, fpart(intery));
                        intery += gradient;
                    }
                }
            }

            /**
             * @brief Draw an anti-aliased circle using Wu's algorithm
             */
            void circle_aa(int cx, int cy, int radius)
            {
                auto plot = [this](int x, int y, double intensity)
                {
                    if (intensity > 0.3)
                        set_pixel(x, y, true);
                };

                auto plot_4_points = [&](int x, int y, double intensity)
                {
                    plot(cx + x, cy + y, intensity);
                    plot(cx - x, cy + y, intensity);
                    plot(cx + x, cy - y, intensity);
                    plot(cx - x, cy - y, intensity);
                    if (x != y)
                    {
                        plot(cx + y, cy + x, intensity);
                        plot(cx - y, cy + x, intensity);
                        plot(cx + y, cy - x, intensity);
                        plot(cx - y, cy - x, intensity);
                    }
                };

                int x = radius;
                int y = 0;
                double last_fade = 0;

                plot_4_points(x, y, 1.0);

                while (x > y)
                {
                    ++y;
                    double ideal_x = std::sqrt(radius * radius - y * y);
                    double fade = std::ceil(ideal_x) - ideal_x;

                    if (fade < last_fade)
                        --x;
                    last_fade = fade;

                    plot_4_points(x, y, 1.0 - fade);
                    if (x > 0)
                        plot_4_points(x - 1, y, fade);
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
                auto img = pythonic::accel::image_io::load_ppm_pgm(filename);
                if (!img.valid())
                    return false;
                int width = img.width;
                int height = img.height;

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
                        if (img.is_color)
                        {
                            size_t idx = (y * width + x) * 3;
                            gray = accel::pixel::to_gray(img.data[idx], img.data[idx + 1], img.data[idx + 2]);
                        }
                        else
                        {
                            gray = img.data[y * width + x];
                        }

                        if (gray >= threshold)
                            set_pixel(x, y);
                    }
                }

                return true;
            }

            /**
             * @brief Load a PGM/PPM image with ordered dithering for grayscale shading
             *
             * Unlike regular load_pgm_ppm which uses simple thresholding, this version
             * uses ordered dithering to create smooth grayscale gradients. Brighter areas
             * have more dots, darker areas have fewer dots.
             */
            bool load_pgm_ppm_dithered(const std::string &filename)
            {
                auto img = pythonic::accel::image_io::load_ppm_pgm(filename);
                if (!img.valid())
                    return false;
                int width = img.width;
                int height = img.height;

                // Resize canvas to fit image
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0));

                // Convert to grayscale buffer
                std::vector<uint8_t> grayscale;
                if (img.is_color)
                {
                    grayscale.resize(width * height);
                    for (int i = 0; i < width * height; ++i)
                        grayscale[i] = accel::pixel::to_gray(img.data[i * 3], img.data[i * 3 + 1], img.data[i * 3 + 2]);
                }
                else
                {
                    grayscale = std::move(img.data);
                }

                // Apply dithered rendering using block operations
                load_frame_ordered_dithered(grayscale.data(), width, height);

                return true;
            }

            /**
             * @brief Load a PGM/PPM image with Floyd-Steinberg error diffusion dithering
             *
             * Floyd-Steinberg dithering provides the best quality for images with
             * smooth gradients, distributing quantization error to neighboring pixels.
             * Better for still images, slightly slower than ordered dithering.
             */
            bool load_pgm_ppm_floyd_steinberg(const std::string &filename)
            {
                auto img = pythonic::accel::image_io::load_ppm_pgm(filename);
                if (!img.valid())
                    return false;
                int width = img.width;
                int height = img.height;

                // Convert to grayscale buffer
                std::vector<uint8_t> grayscale;
                if (img.is_color)
                {
                    grayscale.resize(width * height);
                    for (int i = 0; i < width * height; ++i)
                        grayscale[i] = accel::pixel::to_gray(img.data[i * 3], img.data[i * 3 + 1], img.data[i * 3 + 2]);
                }
                else
                {
                    grayscale = std::move(img.data);
                }

                // Use Floyd-Steinberg dithering
                load_frame_dithered(grayscale.data(), width, height);

                return true;
            }

            /**
             * @brief Load a PGM/PPM image with grayscale-colored dots
             *
             * Loads image and stores both the braille pattern (with threshold)
             * and the average brightness per cell for grayscale coloring.
             * Use with render_grayscale() to get ANSI grayscale-colored output.
             *
             * @param filename Path to PGM/PPM file
             * @param threshold Brightness threshold for dot visibility
             * @param use_dithering If true, use ordered dithering instead of simple threshold
             * @return true on success
             */
            bool load_pgm_ppm_grayscale(const std::string &filename, int threshold = 128, bool use_dithering = false)
            {
                auto img = pythonic::accel::image_io::load_ppm_pgm(filename);
                if (!img.valid())
                    return false;
                int width = img.width;
                int height = img.height;

                // Resize canvas and grayscale storage
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                _grayscale.assign(_char_height, std::vector<uint8_t>(_char_width, 0));

                // Convert to grayscale buffer
                std::vector<uint8_t> graybuf;
                if (img.is_color)
                {
                    graybuf.resize(width * height);
                    for (int i = 0; i < width * height; ++i)
                        graybuf[i] = accel::pixel::to_gray(img.data[i * 3], img.data[i * 3 + 1], img.data[i * 3 + 2]);
                }
                else
                {
                    graybuf = std::move(img.data);
                }

                // Process each character cell
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        uint8_t gray[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                        int px = cx * 2;
                        int py = cy * 4;

                        // Extract 2×4 pixel block
                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col;
                                int y = py + row;
                                if (x < width && y < height)
                                {
                                    gray[row * 2 + col] = graybuf[y * width + x];
                                }
                            }
                        }

                        if (use_dithering)
                        {
                            set_block_gray_dithered_with_brightness(cx, cy, gray);
                        }
                        else
                        {
                            set_block_gray_with_brightness(cx, cy, gray, static_cast<uint8_t>(threshold));
                        }
                    }
                }

                return true;
            }

            /**
             * @brief FLOOD DOT: Load PGM/PPM and fill all dots with average grayscale color
             *
             * This mode lights up ALL 8 dots in every cell and uses the average
             * grayscale of the 8 pixels for coloring. Creates the smoothest
             * appearance for photos and videos.
             *
             * @param filename Path to PGM/PPM file
             * @return true on success
             */
            bool load_pgm_ppm_flood(const std::string &filename)
            {
                auto img = pythonic::accel::image_io::load_ppm_pgm(filename);
                if (!img.valid())
                    return false;
                int width = img.width;
                int height = img.height;

                // Resize canvas and grayscale storage
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0xFF)); // All dots lit
                _grayscale.assign(_char_height, std::vector<uint8_t>(_char_width, 0));

                // Convert to grayscale buffer
                std::vector<uint8_t> graybuf;
                if (img.is_color)
                {
                    graybuf.resize(width * height);
                    for (int i = 0; i < width * height; ++i)
                        graybuf[i] = accel::pixel::to_gray(img.data[i * 3], img.data[i * 3 + 1], img.data[i * 3 + 2]);
                }
                else
                {
                    graybuf = std::move(img.data);
                }

                // Process each character cell - compute average grayscale
                for (size_t cy = 0; cy < _char_height; ++cy)
                {
                    for (size_t cx = 0; cx < _char_width; ++cx)
                    {
                        int sum = 0;
                        int count = 0;
                        int px = cx * 2;
                        int py = cy * 4;

                        // Average over 2×4 pixel block
                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col;
                                int y = py + row;
                                if (x < width && y < height)
                                {
                                    sum += graybuf[y * width + x];
                                    count++;
                                }
                            }
                        }

                        _grayscale[cy][cx] = (count > 0) ? static_cast<uint8_t>(sum / count) : 0;
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
                            int gray = accel::pixel::to_gray(data[idx], data[idx + 1], data[idx + 2]);
                            if (gray >= threshold)
                                set_pixel(x, y);
                        }
                    }
                }
            }

            // ==================== Rendering ====================

            /**
             * @brief Render canvas to string
             * Optimized: uses pre-allocated string with direct append
             */
            std::string render() const
            {
                // Each braille char is 3 bytes UTF-8 + 1 byte newline per row
                std::string out;
                out.reserve(_char_height * (_char_width * 3 + 1));

                for (size_t y = 0; y < _char_height; ++y)
                {
                    for (size_t x = 0; x < _char_width; ++x)
                    {
                        const std::string &ch = braille_to_utf8(_canvas[y][x]);
                        out.append(ch);
                    }
                    if (y < _char_height - 1)
                        out += '\n';
                }

                return out;
            }

            /**
             * @brief Render canvas with grayscale-colored dots
             *
             * Uses ANSI true color to render dots where brightness varies
             * based on the average pixel brightness of each cell.
             * Creates smoother, more natural-looking grayscale images.
             *
             * @param invert If true, render white-on-black (dark dots on light bg)
             */
            std::string render_grayscale(bool invert = false) const
            {
                enable_ansi_support();

                std::string out;
                // Pre-allocate: ~35 bytes per char for ANSI codes + braille
                out.reserve(_char_height * _char_width * 40 + _char_height * 10);

                uint8_t prev_gray = 255; // Invalid to force first color output

                for (size_t y = 0; y < _char_height; ++y)
                {
                    for (size_t x = 0; x < _char_width; ++x)
                    {
                        uint8_t pattern = _canvas[y][x];

                        // Get grayscale value if available, default to white
                        uint8_t gray = 255;
                        if (!_grayscale.empty() && y < _grayscale.size() && x < _grayscale[y].size())
                        {
                            gray = _grayscale[y][x];
                        }

                        if (invert)
                            gray = 255 - gray;

                        // Only emit color code when gray changes
                        // Use zero-alloc append version
                        if (gray != prev_gray)
                        {
                            ansi::fg_color_append(out, gray, gray, gray);
                            prev_gray = gray;
                        }

                        const std::string &ch = braille_to_utf8(pattern);
                        out.append(ch);
                    }
                    out += ansi::RESET;
                    out += '\n';
                    prev_gray = 255; // Reset for next line
                }

                return out;
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
                   ext == ".pgm" || ext == ".pbm" || ext == ".pi"; // .pi = Pythonic Image
        }

        /**
         * @brief Check if file is a Pythonic proprietary image format
         */
        inline bool is_pythonic_image_file(const std::string &filename)
        {
            std::string ext = filename;
            size_t dot = ext.rfind('.');
            if (dot == std::string::npos)
                return false;
            ext = ext.substr(dot);
            for (auto &c : ext)
                c = std::tolower(c);
            return ext == ".pi";
        }

        /**
         * @brief Check if file is a Pythonic proprietary video format
         */
        inline bool is_pythonic_video_file(const std::string &filename)
        {
            std::string ext = filename;
            size_t dot = ext.rfind('.');
            if (dot == std::string::npos)
                return false;
            ext = ext.substr(dot);
            for (auto &c : ext)
                c = std::tolower(c);
            return ext == ".pv";
        }

        /**
         * @brief Convert any image to PPM using ImageMagick's convert command
         * @return Path to temporary PPM file, or empty string on failure
         */
        inline std::string convert_to_ppm(const std::string &input_file, int max_width = 160)
        {
            // Create temp file path
            std::string temp_ppm = "/tmp/pythonic_img_" + std::to_string(std::hash<std::string>{}(input_file)) + ".ppm";

            // Register for auto-cleanup
            pythonic::accel::temp_manager().register_temp(temp_ppm);

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

            // Register for auto-cleanup
            pythonic::accel::temp_manager().register_temp(temp_dot);
            pythonic::accel::temp_manager().register_temp(temp_ppm);

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
         * @brief Render an image file with ordered dithering for smooth grayscale shading
         *
         * Unlike simple thresholding, this uses ordered dithering to create
         * proper grayscale shading. Brighter areas have more dots lit, darker areas
         * have fewer dots - creating natural-looking gradients.
         *
         * Perfect for photos and videos where you want to see detail in both
         * highlights and shadows instead of just black & white.
         *
         * @param filename Path to image file
         * @param max_width Maximum width in characters
         * @param dithering Dithering algorithm (ordered=Bayer, floyd_steinberg=error diffusion, none=threshold)
         */
        inline std::string render_image_dithered(const std::string &filename, int max_width = 80,
                                                 Dithering dithering = Dithering::ordered)
        {
            // Check file exists
            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            BrailleCanvas canvas;

            // For other formats, try ImageMagick conversion
            std::string ppm_file = convert_to_ppm(filename, max_width * 2); // 2 pixels per braille char width
            if (!ppm_file.empty())
            {
                // Use appropriate dithering method based on config
                bool loaded = false;
                if (dithering == Dithering::floyd_steinberg)
                {
                    loaded = canvas.load_pgm_ppm_floyd_steinberg(ppm_file);
                }
                else if (dithering == Dithering::ordered)
                {
                    loaded = canvas.load_pgm_ppm_dithered(ppm_file); // Bayer ordered dithering
                }
                else
                {
                    loaded = canvas.load_pgm_ppm(ppm_file, 128); // Simple threshold, no dithering
                }

                std::remove(ppm_file.c_str());
                if (loaded)
                    return canvas.render();
            }

            return "Error: Could not load image. Install ImageMagick for PNG/JPG support.\n";
        }

        /**
         * @brief Render an image file with grayscale-colored braille dots
         *
         * Creates smooth grayscale rendering where each braille cell's dots
         * are colored based on the average brightness of that cell.
         * Combines with dithering for best visual quality.
         *
         * @param filename Path to image file
         * @param max_width Maximum width in characters
         * @param threshold Brightness threshold for dot visibility (only if not dithering)
         * @param use_dithering Use ordered dithering for dot patterns
         * @return Rendered string with ANSI color codes
         */
        inline std::string render_image_grayscale(const std::string &filename, int max_width = 80,
                                                  int threshold = 128, bool use_dithering = true)
        {
            // Check file exists
            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            BrailleCanvas canvas;

            // Convert to PPM for processing
            std::string ppm_file = convert_to_ppm(filename, max_width * 2);
            if (!ppm_file.empty())
            {
                if (canvas.load_pgm_ppm_grayscale(ppm_file, threshold, use_dithering))
                {
                    std::remove(ppm_file.c_str());
                    return canvas.render_grayscale();
                }
                std::remove(ppm_file.c_str());
            }

            return "Error: Could not load image. Install ImageMagick for PNG/JPG support.\n";
        }

        /**
         * @brief FLOOD DOT: Render with all dots lit, colored by average cell brightness
         *
         * This creates the smoothest appearance for photos and videos by lighting
         * ALL 8 dots in every cell and using grayscale color to represent brightness.
         * No threshold or dithering noise - just smooth grayscale gradients.
         *
         * @param filename Path to image file
         * @param max_width Maximum width in characters
         * @return Rendered string with ANSI grayscale color codes
         */
        inline std::string render_image_flood(const std::string &filename, int max_width = 80)
        {
            // Check file exists
            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            BrailleCanvas canvas;

            // Convert to PPM for processing
            std::string ppm_file = convert_to_ppm(filename, max_width * 2);
            if (!ppm_file.empty())
            {
                if (canvas.load_pgm_ppm_flood(ppm_file))
                {
                    std::remove(ppm_file.c_str());
                    return canvas.render_grayscale();
                }
                std::remove(ppm_file.c_str());
            }

            return "Error: Could not load image. Install ImageMagick for PNG/JPG support.\n";
        }

        /**
         * @brief FLOOD DOT COLORED: Render with all dots lit, colored by average RGB
         *
         * Similar to flood_dot but uses full RGB color instead of grayscale.
         * ALL 8 dots in every cell are lit and colored by the average color.
         *
         * @param filename Path to image file
         * @param max_width Maximum width in characters
         * @return Rendered string with ANSI RGB color codes
         */
        inline std::string render_image_flood_colored(const std::string &filename, int max_width = 80)
        {
            enable_ansi_support();

            // Check file exists
            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            ColoredBrailleCanvas canvas;

            // Convert to PPM for processing
            std::string ppm_file = convert_to_ppm(filename, max_width * 2);
            if (!ppm_file.empty())
            {
                if (canvas.load_ppm_flood(ppm_file))
                {
                    std::remove(ppm_file.c_str());
                    return canvas.render();
                }
                std::remove(ppm_file.c_str());
            }

            return "Error: Could not load image. Install ImageMagick for PNG/JPG support.\n";
        }

        /**
         * @brief COLORED DITHERED: Render with dithered dots, colored by average RGB
         *
         * Uses configured dithering for dot patterns while coloring each cell
         * by its average RGB color. Good balance of detail and color.
         *
         * @param filename Path to image file
         * @param max_width Maximum width in characters
         * @param dithering Dithering algorithm (ordered=Bayer, floyd_steinberg=error diffusion)
         * @return Rendered string with ANSI RGB color codes
         */
        inline std::string render_image_colored_dithered(const std::string &filename, int max_width = 80,
                                                         Dithering dithering = Dithering::ordered)
        {
            enable_ansi_support();

            // Check file exists
            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            ColoredBrailleCanvas canvas;

            // Convert to PPM for processing
            std::string ppm_file = convert_to_ppm(filename, max_width * 2);
            if (!ppm_file.empty())
            {
                bool loaded = false;
                if (dithering == Dithering::floyd_steinberg)
                {
                    loaded = canvas.load_ppm_dithered_floyd(ppm_file);
                }
                else
                {
                    loaded = canvas.load_ppm_dithered(ppm_file); // Bayer ordered dithering
                }

                std::remove(ppm_file.c_str());
                if (loaded)
                    return canvas.render();
                ;
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
         * @brief Render an image file with true color (24-bit ANSI) support
         */
        inline std::string render_image_colored(const std::string &filename, int max_width = 80)
        {
            enable_ansi_support();

            // Check file exists
            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            // Create temp PPM for ImageMagick conversion
            std::string temp_ppm = "/tmp/pythonic_color_" +
                                   std::to_string(std::hash<std::string>{}(filename)) + ".ppm";

            // Use ImageMagick to convert and resize
            std::string cmd = "convert \"" + filename + "\" -resize " +
                              std::to_string(max_width) + "x -depth 8 \"" + temp_ppm + "\" 2>/dev/null";

            int result = std::system(cmd.c_str());
            if (result != 0)
                return "Error: Could not convert image. Install ImageMagick.\n";

            // Load PPM file
            auto img = pythonic::accel::image_io::load_ppm_pgm(temp_ppm);
            std::remove(temp_ppm.c_str());
            if (!img.valid() || !img.is_color)
                return "Error: Invalid PPM format.\n";
            int width = img.width;
            int height = img.height;
            const auto &rgb_data = img.data;

            // Create color canvas and render
            ColorCanvas canvas = ColorCanvas::from_pixels(width, height);
            canvas.load_frame_rgb(rgb_data.data(), width, height);

            return canvas.render();
        }

        /**
         * @brief Print an image file with true color (24-bit ANSI) support
         */
        inline void print_image_colored(const std::string &filename, int max_width = 80)
        {
            std::cout << render_image_colored(filename, max_width);
        }

        /**
         * @brief Render an image file in BW half-block mode (Mode::bw)
         * Uses ImageMagick for conversion, renders using half-block characters (▀▄█)
         */
        inline std::string render_image_bw_block(const std::string &filename, int max_width = 80, int threshold = 128)
        {
            enable_ansi_support();

            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            std::string temp_ppm = "/tmp/pythonic_bw_block_" +
                                   std::to_string(std::hash<std::string>{}(filename)) + ".ppm";

            // ImageMagick conversion - keep original aspect ratio
            std::string cmd = "convert \"" + filename + "\" -resize " +
                              std::to_string(max_width) + "x -depth 8 \"" + temp_ppm + "\" 2>/dev/null";

            int result = std::system(cmd.c_str());
            if (result != 0)
                return "Error: Could not convert image. Install ImageMagick.\n";

            auto img = pythonic::accel::image_io::load_ppm_pgm(temp_ppm);
            std::remove(temp_ppm.c_str());
            if (!img.valid() || !img.is_color)
                return "Error: Invalid PPM format.\n";
            int width = img.width;
            int height = img.height;
            const auto &rgb_data = img.data;

            BWBlockCanvas canvas = BWBlockCanvas::from_pixels(width, height);
            canvas.load_frame_rgb(rgb_data.data(), width, height, threshold);

            return canvas.render();
        }

        /**
         * @brief Print an image file in BW half-block mode
         */
        inline void print_image_bw_block(const std::string &filename, int max_width = 80, int threshold = 128)
        {
            std::cout << render_image_bw_block(filename, max_width, threshold);
        }

        /**
         * @brief Render an image file in colored braille mode (Mode::colored_dot)
         * Uses ImageMagick for conversion, renders using colored braille characters
         */
        inline std::string render_image_colored_dot(const std::string &filename, int max_width = 80, int threshold = 128)
        {
            enable_ansi_support();

            std::ifstream test(filename);
            if (!test.good())
                return "Error: Cannot open file '" + filename + "'\n";
            test.close();

            // For braille, we need 2x the character width in pixels
            int pixel_width = max_width * 2;

            std::string temp_ppm = "/tmp/pythonic_colored_dot_" +
                                   std::to_string(std::hash<std::string>{}(filename)) + ".ppm";

            std::string cmd = "convert \"" + filename + "\" -resize " +
                              std::to_string(pixel_width) + "x -depth 8 \"" + temp_ppm + "\" 2>/dev/null";

            int result = std::system(cmd.c_str());
            if (result != 0)
                return "Error: Could not convert image. Install ImageMagick.\n";

            auto img = pythonic::accel::image_io::load_ppm_pgm(temp_ppm);
            std::remove(temp_ppm.c_str());
            if (!img.valid() || !img.is_color)
                return "Error: Invalid PPM format.\n";
            int width = img.width;
            int height = img.height;
            const auto &rgb_data = img.data;

            ColoredBrailleCanvas canvas = ColoredBrailleCanvas::from_pixels(width, height);
            canvas.load_frame_rgb(rgb_data.data(), width, height, threshold);

            return canvas.render();
        }

        /**
         * @brief Print an image file in colored braille mode
         */
        inline void print_image_colored_dot(const std::string &filename, int max_width = 80, int threshold = 128)
        {
            std::cout << render_image_colored_dot(filename, max_width, threshold);
        }

        /**
         * @brief Unified image rendering function that handles all modes
         * @param filename Path to image file
         * @param max_width Maximum width in terminal characters
         * @param threshold Brightness threshold (0-255) for BW modes
         * @param mode Rendering mode (bw, bw_dot, colored, colored_dot, bw_dithered, grayscale_dot, flood_dot, flood_dot_colored, colored_dithered)
         * @param dithering Dithering algorithm to use for dithered modes (default: ordered)
         */
        inline void print_image_with_mode(const std::string &filename, int max_width = 80,
                                          int threshold = 128, Mode mode = Mode::bw_dot,
                                          Dithering dithering = Dithering::ordered)
        {
            switch (mode)
            {
            case Mode::bw:
                print_image_bw_block(filename, max_width, threshold);
                break;
            case Mode::bw_dot:
                print_image(filename, max_width, threshold);
                break;
            case Mode::colored:
                print_image_colored(filename, max_width);
                break;
            case Mode::colored_dot:
                print_image_colored_dot(filename, max_width, threshold);
                break;
            case Mode::bw_dithered:
                // Use configured dithering algorithm
                std::cout << render_image_dithered(filename, max_width, dithering);
                break;
            case Mode::grayscale_dot:
                // Grayscale-colored braille dots
                std::cout << render_image_grayscale(filename, max_width, threshold, true);
                break;
            case Mode::flood_dot:
                // All dots lit, colored by average brightness - smoothest
                std::cout << render_image_flood(filename, max_width);
                break;
            case Mode::flood_dot_colored:
                // All dots lit, colored by average RGB
                std::cout << render_image_flood_colored(filename, max_width);
                break;
            case Mode::colored_dithered:
                // Colored braille with dithering
                std::cout << render_image_colored_dithered(filename, max_width, dithering);
                break;
            }
        }

        /**
         * @brief Print a DOT graph to stdout
         */
        inline void print_dot(const std::string &dot_content, int max_width = 80, int threshold = 128)
        {
            std::cout << render_dot(dot_content, max_width, threshold) << std::endl;
        }

        // ==================== OpenCV-based Rendering ====================

#ifdef PYTHONIC_ENABLE_OPENCV
        /**
         * @brief Render image using OpenCV
         *
         * Falls back to ImageMagick if OpenCV fails.
         */
        inline std::string render_image_opencv(const std::string &filename, int max_width = 80,
                                               int threshold = 128, Mode mode = Mode::bw_dot)
        {
            cv::Mat img = cv::imread(filename);
            if (img.empty())
            {
                // OpenCV failed, return empty to signal fallback
                return "";
            }

            // Resize while preserving aspect ratio
            double scale = static_cast<double>(max_width * 2) / img.cols; // *2 for braille width
            if (mode == Mode::bw || mode == Mode::colored)
                scale = static_cast<double>(max_width) / img.cols;

            cv::Mat resized;
            cv::resize(img, resized, cv::Size(), scale, scale, cv::INTER_AREA);

            // Convert to RGB (OpenCV loads as BGR)
            cv::Mat rgb;
            cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

            // Render based on mode
            switch (mode)
            {
            case Mode::bw_dot:
            {
                BrailleCanvas canvas = BrailleCanvas::from_pixels(rgb.cols, rgb.rows);
                canvas.load_frame_rgb_fast(rgb.data, rgb.cols, rgb.rows, threshold);
                return canvas.render();
            }
            case Mode::bw:
            {
                BWBlockCanvas canvas = BWBlockCanvas::from_pixels(rgb.cols, rgb.rows);
                canvas.load_frame_rgb(rgb.data, rgb.cols, rgb.rows, threshold);
                return canvas.render();
            }
            case Mode::colored:
            {
                ColorCanvas canvas = ColorCanvas::from_pixels(rgb.cols, rgb.rows);
                canvas.load_frame_rgb(rgb.data, rgb.cols, rgb.rows);
                return canvas.render();
            }
            case Mode::colored_dot:
            {
                ColoredBrailleCanvas canvas = ColoredBrailleCanvas::from_pixels(rgb.cols, rgb.rows);
                canvas.load_frame_rgb(rgb.data, rgb.cols, rgb.rows, threshold);
                return canvas.render();
            }
            case Mode::bw_dithered:
            {
                // Convert to grayscale and apply ordered dithering
                cv::Mat gray;
                cv::cvtColor(rgb, gray, cv::COLOR_RGB2GRAY);
                BrailleCanvas canvas = BrailleCanvas::from_pixels(gray.cols, gray.rows);
                canvas.load_frame_ordered_dithered(gray.data, gray.cols, gray.rows);
                return canvas.render();
            }
            case Mode::grayscale_dot:
            {
                // Convert to grayscale and apply grayscale coloring
                cv::Mat gray;
                cv::cvtColor(rgb, gray, cv::COLOR_RGB2GRAY);
                BrailleCanvas canvas = BrailleCanvas::from_pixels(gray.cols, gray.rows);
                // Use dithering with brightness storage
                for (int cy = 0; cy < (gray.rows + 3) / 4; ++cy)
                {
                    for (int cx = 0; cx < (gray.cols + 1) / 2; ++cx)
                    {
                        uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                        int px = cx * 2, py = cy * 4;
                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col, y = py + row;
                                if (x < gray.cols && y < gray.rows)
                                {
                                    grays[row * 2 + col] = gray.at<uint8_t>(y, x);
                                }
                            }
                        }
                        canvas.set_block_gray_dithered_with_brightness(cx, cy, grays);
                    }
                }
                return canvas.render_grayscale();
            }
            case Mode::flood_dot:
            {
                // Flood fill: all dots lit, colored by average brightness
                cv::Mat gray;
                cv::cvtColor(rgb, gray, cv::COLOR_RGB2GRAY);
                BrailleCanvas canvas = BrailleCanvas::from_pixels(gray.cols, gray.rows);
                for (int cy = 0; cy < (gray.rows + 3) / 4; ++cy)
                {
                    for (int cx = 0; cx < (gray.cols + 1) / 2; ++cx)
                    {
                        uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                        int px = cx * 2, py = cy * 4;
                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col, y = py + row;
                                if (x < gray.cols && y < gray.rows)
                                {
                                    grays[row * 2 + col] = gray.at<uint8_t>(y, x);
                                }
                            }
                        }
                        canvas.set_block_flood_fill(cx, cy, grays);
                    }
                }
                return canvas.render_grayscale();
            }
            case Mode::flood_dot_colored:
            {
                // Flood fill colored: all dots lit, colored by average RGB
                ColoredBrailleCanvas canvas = ColoredBrailleCanvas::from_pixels(rgb.cols, rgb.rows);
                for (int cy = 0; cy < (rgb.rows + 3) / 4; ++cy)
                {
                    for (int cx = 0; cx < (rgb.cols + 1) / 2; ++cx)
                    {
                        // Accumulate RGB
                        int sum_r = 0, sum_g = 0, sum_b = 0, count = 0;
                        int px = cx * 2, py = cy * 4;
                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col, y = py + row;
                                if (x < rgb.cols && y < rgb.rows)
                                {
                                    cv::Vec3b color = rgb.at<cv::Vec3b>(y, x);
                                    sum_r += color[0];
                                    sum_g += color[1];
                                    sum_b += color[2];
                                    count++;
                                }
                            }
                        }
                        if (count > 0)
                        {
                            uint8_t avg_r = sum_r / count;
                            uint8_t avg_g = sum_g / count;
                            uint8_t avg_b = sum_b / count;
                            canvas.set_pattern(cx, cy, 0xFF); // All 8 dots on
                            canvas.set_color(cx, cy, avg_r, avg_g, avg_b);
                        }
                    }
                }
                return canvas.render();
            }
            case Mode::colored_dithered:
            {
                // Colored dithered: use dithering for dots, color by average RGB
                // Bayer 2x2 matrix for ordered dithering
                const int bayer2x2[2][2] = {{0, 2}, {3, 1}};
                ColoredBrailleCanvas canvas = ColoredBrailleCanvas::from_pixels(rgb.cols, rgb.rows);

                for (int cy = 0; cy < (rgb.rows + 3) / 4; ++cy)
                {
                    for (int cx = 0; cx < (rgb.cols + 1) / 2; ++cx)
                    {
                        uint8_t pattern = 0;
                        int sum_r = 0, sum_g = 0, sum_b = 0, count = 0;
                        int px = cx * 2, py = cy * 4;

                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col, y = py + row;
                                if (x < rgb.cols && y < rgb.rows)
                                {
                                    cv::Vec3b color = rgb.at<cv::Vec3b>(y, x);
                                    sum_r += color[0];
                                    sum_g += color[1];
                                    sum_b += color[2];
                                    count++;

                                    // Calculate grayscale for dithering
                                    uint8_t gray = (color[0] * 77 + color[1] * 150 + color[2] * 29) >> 8;
                                    int bayer_x = col & 1;
                                    int bayer_y = row & 1;
                                    int threshold_val = ((bayer2x2[bayer_y][bayer_x] + 1) * 255) / 5;

                                    // Braille dot position mapping
                                    static const int dot_map[4][2] = {{0, 3}, {1, 4}, {2, 5}, {6, 7}};
                                    if (gray > threshold_val)
                                    {
                                        pattern |= (1 << dot_map[row][col]);
                                    }
                                }
                            }
                        }

                        if (count > 0)
                        {
                            uint8_t avg_r = sum_r / count;
                            uint8_t avg_g = sum_g / count;
                            uint8_t avg_b = sum_b / count;
                            canvas.set_pattern(cx, cy, pattern);
                            canvas.set_color(cx, cy, avg_r, avg_g, avg_b);
                        }
                    }
                }
                return canvas.render();
            }
            }
            return "";
        }

        /**
         * @brief Print image using OpenCV with fallback to default parser
         */
        inline void print_image_opencv(const std::string &filename, int max_width = 80,
                                       int threshold = 128, Mode mode = Mode::bw_dot)
        {
            std::string result = render_image_opencv(filename, max_width, threshold, mode);
            if (result.empty())
            {
                std::cerr << "Warning: OpenCV failed to load image, falling back to ImageMagick\n";
                if (mode == Mode::colored || mode == Mode::colored_dot)
                    print_image_colored(filename, max_width);
                else
                    print_image(filename, max_width, threshold);
                return;
            }
            std::cout << result;
        }
#else
        // Stub when OpenCV not available
        inline std::string render_image_opencv(const std::string &filename, int max_width = 80,
                                               int threshold = 128, Mode mode = Mode::bw_dot)
        {
            (void)filename;
            (void)max_width;
            (void)threshold;
            (void)mode;
            return ""; // Signal fallback needed
        }

        inline void print_image_opencv(const std::string &filename, int max_width = 80,
                                       int threshold = 128, Mode mode = Mode::bw_dot)
        {
            (void)mode;
            std::cerr << "Warning: OpenCV not available, using default parser\n";
            print_image(filename, max_width, threshold);
        }
#endif

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
                   ext == ".wmv" || ext == ".m4v" || ext == ".gif" ||
                   ext == ".pv"; // .pv = Pythonic Video
        }

        /**
         * @brief Check if input is a webcam source
         * Webcam sources are typically device paths or indices like:
         * - "0", "1", etc. (device index)
         * - "/dev/video0", "/dev/video1" (Linux)
         * - "webcam", "webcam:0" (convenience aliases)
         */
        inline bool is_webcam_source(const std::string &source)
        {
            // Check for numeric index
            if (!source.empty() && std::all_of(source.begin(), source.end(), ::isdigit))
                return true;

            // Check for Linux video device
            if (source.find("/dev/video") == 0)
                return true;

            // Check for convenience aliases
            std::string lower = source;
            for (auto &c : lower)
                c = std::tolower(c);

            return lower == "webcam" || lower.find("webcam:") == 0 ||
                   lower == "camera" || lower.find("camera:") == 0;
        }

        /**
         * @brief Parse webcam source to get device index
         * @return Device index (0 by default)
         */
        inline int parse_webcam_index(const std::string &source)
        {
            // Pure number
            if (!source.empty() && std::all_of(source.begin(), source.end(), ::isdigit))
                return std::stoi(source);

            // /dev/video<N>
            if (source.find("/dev/video") == 0)
            {
                try
                {
                    return std::stoi(source.substr(10));
                }
                catch (...)
                {
                    return 0;
                }
            }

            // webcam:<N> or camera:<N>
            size_t colon = source.find(':');
            if (colon != std::string::npos)
            {
                try
                {
                    return std::stoi(source.substr(colon + 1));
                }
                catch (...)
                {
                    return 0;
                }
            }

            return 0;
        }

        /**
         * @brief Check if OpenCV support is available
         */
        inline bool has_opencv_support()
        {
#ifdef PYTHONIC_ENABLE_OPENCV
            return true;
#else
            return false;
#endif
        }

        // ==================== Non-blocking Keyboard Input ====================

        /**
         * @brief Non-blocking keyboard input for video playback controls
         *
         * Uses termios (POSIX) or conio (Windows) to set terminal to raw mode
         * for immediate character input without waiting for Enter.
         * Restores terminal state on destruction.
         */
        class KeyboardInput
        {
        private:
#ifdef _WIN32
            bool _initialized;
#else
            struct termios _old_termios;
            bool _raw_mode;
#endif

        public:
            KeyboardInput()
#ifdef _WIN32
                : _initialized(true)
#else
                : _raw_mode(false)
#endif
            {
#ifndef _WIN32
                // Only enable raw mode if stdin is a real terminal
                if (!isatty(STDIN_FILENO))
                    return;

                // Get current terminal settings
                if (tcgetattr(STDIN_FILENO, &_old_termios) == 0)
                {
                    struct termios new_termios = _old_termios;
                    // Disable canonical mode (line buffering) and echo
                    new_termios.c_lflag &= ~(ICANON | ECHO);
                    // Set minimum characters for non-blocking read
                    new_termios.c_cc[VMIN] = 0;
                    new_termios.c_cc[VTIME] = 0;

                    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == 0)
                    {
                        _raw_mode = true;
                        // Flush any pending input that might be in the buffer
                        tcflush(STDIN_FILENO, TCIFLUSH);
                    }
                }
#endif
            }

            ~KeyboardInput()
            {
#ifndef _WIN32
                if (_raw_mode)
                {
                    tcsetattr(STDIN_FILENO, TCSANOW, &_old_termios);
                }
#endif
            }

            /**
             * @brief Check if a key has been pressed (non-blocking)
             * @return The character pressed, or -1 if no key pressed
             */
            int get_key()
            {
#ifdef _WIN32
                // Windows: Use _kbhit() and _getch() from conio.h
                if (_kbhit())
                {
                    return _getch();
                }
                return -1;
#else
                if (!_raw_mode)
                    return -1;

                char c;
                if (read(STDIN_FILENO, &c, 1) == 1)
                {
                    return static_cast<int>(c);
                }
                return -1;
#endif
            }

            /**
             * @brief Check if a specific key was pressed
             * @param key Character to check for
             * @return true if the specified key was pressed
             */
            bool is_key_pressed(char key)
            {
                int k = get_key();
                return k == static_cast<int>(key);
            }

            // Prevent copying
            KeyboardInput(const KeyboardInput &) = delete;
            KeyboardInput &operator=(const KeyboardInput &) = delete;
        };

        /**
         * @brief RAII helper to manage terminal state during video playback
         *
         * Ensures the cursor is restored and terminal state is reset even if
         * an exception is thrown, the program exits unexpectedly, or the user
         * presses Ctrl+C. Integrates with the global signal handler.
         */
        class TerminalStateGuard
        {
        private:
            bool _active;

        public:
            TerminalStateGuard() : _active(true)
            {
                // Register with signal handler for cleanup on Ctrl+C
                signal_handler::start_playback();
                // Enter alternate screen buffer (preserves user's terminal content)
                // and hide cursor
                std::cout << ansi::ALT_SCREEN_ON << ansi::HIDE_CURSOR << std::flush;
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
                    // Restore stdout buffering to default line-buffered mode
                    setvbuf(stdout, nullptr, _IOLBF, 0);
                    // Mark playback as ended (also restores termios)
                    signal_handler::end_playback();
                    // Show cursor, reset attributes, leave alternate screen
                    // (leaving alt screen automatically restores original terminal content)
                    std::cout << ansi::SHOW_CURSOR << ansi::RESET
                              << ansi::ALT_SCREEN_OFF << std::flush;
                }
            }

            // Check if playback was interrupted by signal
            bool was_interrupted() const
            {
                return signal_handler::was_interrupted();
            }

            // Prevent copying
            TerminalStateGuard(const TerminalStateGuard &) = delete;
            TerminalStateGuard &operator=(const TerminalStateGuard &) = delete;
        };

        // ==================== Text to Braille Art Rendering ====================

        /**
         * @brief 5x7 pixel font for text rendering in braille
         *
         * Each glyph is stored as 7 rows of 5-bit patterns.
         * This is the standard bitmap font size that properly renders
         * diagonals and distinguishes similar letters (M vs N vs H, etc.)
         */
        namespace text_font
        {
            struct Glyph
            {
                uint8_t rows[7];
            };

            inline const std::map<char, Glyph> &get_font()
            {
                static std::map<char, Glyph> font = {
                    // Numbers (5x7)
                    {'0', {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}},
                    {'1', {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},
                    {'2', {0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111}},
                    {'3', {0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}},
                    {'4', {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}},
                    {'5', {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}},
                    {'6', {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}},
                    {'7', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}},
                    {'8', {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}},
                    {'9', {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}},

                    // Uppercase letters (5x7)
                    {'A', {0b01110, 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001}},
                    {'B', {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}},
                    {'C', {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110}},
                    {'D', {0b11100, 0b10010, 0b10001, 0b10001, 0b10001, 0b10010, 0b11100}},
                    {'E', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}},
                    {'F', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}},
                    {'G', {0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111}},
                    {'H', {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}},
                    {'I', {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},
                    {'J', {0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100}},
                    {'K', {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001}},
                    {'L', {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}},
                    {'M', {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001}},
                    {'N', {0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001}},
                    {'O', {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
                    {'P', {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}},
                    {'Q', {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101}},
                    {'R', {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}},
                    {'S', {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}},
                    {'T', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}},
                    {'U', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
                    {'V', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}},
                    {'W', {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010}},
                    {'X', {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001}},
                    {'Y', {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}},
                    {'Z', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111}},

                    // Lowercase letters (5x7 - properly distinct from uppercase)
                    {'a', {0b00000, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111}},
                    {'b', {0b10000, 0b10000, 0b10110, 0b11001, 0b10001, 0b10001, 0b11110}},
                    {'c', {0b00000, 0b00000, 0b01110, 0b10000, 0b10000, 0b10001, 0b01110}},
                    {'d', {0b00001, 0b00001, 0b01101, 0b10011, 0b10001, 0b10001, 0b01111}},
                    {'e', {0b00000, 0b00000, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110}},
                    {'f', {0b00110, 0b01001, 0b01000, 0b11100, 0b01000, 0b01000, 0b01000}},
                    {'g', {0b00000, 0b01111, 0b10001, 0b10001, 0b01111, 0b00001, 0b01110}},
                    {'h', {0b10000, 0b10000, 0b10110, 0b11001, 0b10001, 0b10001, 0b10001}},
                    {'i', {0b00100, 0b00000, 0b01100, 0b00100, 0b00100, 0b00100, 0b01110}},
                    {'j', {0b00010, 0b00000, 0b00110, 0b00010, 0b00010, 0b10010, 0b01100}},
                    {'k', {0b10000, 0b10000, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010}},
                    {'l', {0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},
                    {'m', {0b00000, 0b00000, 0b11010, 0b10101, 0b10101, 0b10001, 0b10001}},
                    {'n', {0b00000, 0b00000, 0b10110, 0b11001, 0b10001, 0b10001, 0b10001}},
                    {'o', {0b00000, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}},
                    {'p', {0b00000, 0b00000, 0b11110, 0b10001, 0b11110, 0b10000, 0b10000}},
                    {'q', {0b00000, 0b00000, 0b01101, 0b10011, 0b01111, 0b00001, 0b00001}},
                    {'r', {0b00000, 0b00000, 0b10110, 0b11001, 0b10000, 0b10000, 0b10000}},
                    {'s', {0b00000, 0b00000, 0b01110, 0b10000, 0b01110, 0b00001, 0b11110}},
                    {'t', {0b01000, 0b01000, 0b11100, 0b01000, 0b01000, 0b01001, 0b00110}},
                    {'u', {0b00000, 0b00000, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101}},
                    {'v', {0b00000, 0b00000, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}},
                    {'w', {0b00000, 0b00000, 0b10001, 0b10001, 0b10101, 0b10101, 0b01010}},
                    {'x', {0b00000, 0b00000, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001}},
                    {'y', {0b00000, 0b00000, 0b10001, 0b10001, 0b01111, 0b00001, 0b01110}},
                    {'z', {0b00000, 0b00000, 0b11111, 0b00010, 0b00100, 0b01000, 0b11111}},

                    // Symbols (5x7)
                    {' ', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}},
                    {'.', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100}},
                    {',', {0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b00100, 0b01000}},
                    {'!', {0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100}},
                    {'?', {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100}},
                    {':', {0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b01100, 0b00000}},
                    {';', {0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b00100, 0b01000}},
                    {'-', {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000}},
                    {'+', {0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000}},
                    {'=', {0b00000, 0b00000, 0b11111, 0b00000, 0b11111, 0b00000, 0b00000}},
                    {'(', {0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010}},
                    {')', {0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000}},
                    {'[', {0b01110, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01110}},
                    {']', {0b01110, 0b00010, 0b00010, 0b00010, 0b00010, 0b00010, 0b01110}},
                    {'{', {0b00110, 0b01000, 0b01000, 0b11000, 0b01000, 0b01000, 0b00110}},
                    {'}', {0b01100, 0b00010, 0b00010, 0b00011, 0b00010, 0b00010, 0b01100}},
                    {'*', {0b00000, 0b00100, 0b10101, 0b01110, 0b10101, 0b00100, 0b00000}},
                    {'/', {0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000}},
                    {'\\', {0b10000, 0b01000, 0b01000, 0b00100, 0b00010, 0b00010, 0b00001}},
                    {'_', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111}},
                    {'\'', {0b00100, 0b00100, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000}},
                    {'"', {0b01010, 0b01010, 0b10100, 0b00000, 0b00000, 0b00000, 0b00000}},
                    {'@', {0b01110, 0b10001, 0b10111, 0b10101, 0b10110, 0b10000, 0b01111}},
                    {'#', {0b01010, 0b01010, 0b11111, 0b01010, 0b11111, 0b01010, 0b01010}},
                    {'$', {0b00100, 0b01111, 0b10100, 0b01110, 0b00101, 0b11110, 0b00100}},
                    {'%', {0b11000, 0b11001, 0b00010, 0b00100, 0b01000, 0b10011, 0b00011}},
                    {'&', {0b01100, 0b10010, 0b10100, 0b01000, 0b10101, 0b10010, 0b01101}},
                    {'<', {0b00010, 0b00100, 0b01000, 0b10000, 0b01000, 0b00100, 0b00010}},
                    {'>', {0b01000, 0b00100, 0b00010, 0b00001, 0b00010, 0b00100, 0b01000}},
                    {'^', {0b00100, 0b01010, 0b10001, 0b00000, 0b00000, 0b00000, 0b00000}},
                    {'|', {0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}},
                    {'`', {0b01000, 0b00100, 0b00010, 0b00000, 0b00000, 0b00000, 0b00000}},
                    {'~', {0b00000, 0b00000, 0b01000, 0b10101, 0b00010, 0b00000, 0b00000}},
                    {'\n', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}},
                };
                return font;
            }

            inline int char_width() { return 5; }
            inline int char_height() { return 7; }
            inline int char_spacing() { return 1; }
        }

        /**
         * @brief Render text as braille art
         *
         * @param text The text to render
         * @param mode Rendering mode (bw_dot, colored_dot, etc.)
         * @param max_width Maximum width in terminal characters (0 = auto)
         * @param fg_r Foreground red (for colored modes)
         * @param fg_g Foreground green
         * @param fg_b Foreground blue
         * @return Rendered string
         */
        inline std::string render_text_art(const std::string &text, Mode mode = Mode::bw_dot,
                                           int max_width = 0, uint8_t fg_r = 255, uint8_t fg_g = 255, uint8_t fg_b = 255)
        {
            enable_ansi_support();

            // Split text into lines
            std::vector<std::string> lines;
            std::string current_line;
            for (char c : text)
            {
                if (c == '\n')
                {
                    lines.push_back(current_line);
                    current_line.clear();
                }
                else
                {
                    current_line += c;
                }
            }
            if (!current_line.empty())
                lines.push_back(current_line);

            if (lines.empty())
                return "";

            // Calculate pixel dimensions
            int font_w = text_font::char_width();
            int font_h = text_font::char_height();
            int spacing = text_font::char_spacing();

            // Find max line length
            size_t max_chars = 0;
            for (const auto &line : lines)
                max_chars = std::max(max_chars, line.length());

            int pixel_w = max_chars * (font_w + spacing) - spacing;
            int pixel_h = lines.size() * (font_h + 1);

            // Create canvas based on mode
            if (mode == Mode::bw_dot || mode == Mode::bw)
            {
                BrailleCanvas canvas = BrailleCanvas::from_pixels(pixel_w, pixel_h);

                // Draw each line
                int y = 0;
                for (const auto &line : lines)
                {
                    int x = 0;
                    const auto &glyphs = text_font::get_font();
                    for (char c : line)
                    {
                        auto it = glyphs.find(c);
                        if (it == glyphs.end())
                        {
                            x += font_w + spacing;
                            continue;
                        }
                        const auto &glyph = it->second;
                        for (int row = 0; row < font_h; ++row)
                        {
                            uint8_t bits = glyph.rows[row];
                            for (int col = 0; col < font_w; ++col)
                            {
                                if (bits & (1 << (font_w - 1 - col)))
                                {
                                    canvas.set_pixel(x + col, y + row, true);
                                }
                            }
                        }
                        x += font_w + spacing;
                    }
                    y += font_h + 1;
                }
                return canvas.render();
            }
            else
            {
                // Colored modes
                ColoredBrailleCanvas canvas = ColoredBrailleCanvas::from_pixels(pixel_w, pixel_h);

                // Draw each line
                int y = 0;
                for (const auto &line : lines)
                {
                    int x = 0;
                    const auto &glyphs = text_font::get_font();
                    for (char c : line)
                    {
                        auto it = glyphs.find(c);
                        if (it == glyphs.end())
                        {
                            x += font_w + spacing;
                            continue;
                        }
                        const auto &glyph = it->second;
                        for (int row = 0; row < font_h; ++row)
                        {
                            uint8_t bits = glyph.rows[row];
                            for (int col = 0; col < font_w; ++col)
                            {
                                if (bits & (1 << (font_w - 1 - col)))
                                {
                                    int px = x + col;
                                    int py = y + row;
                                    int cx = px / 2;
                                    int cy = py / 4;
                                    int lx = px % 2;
                                    int ly = py % 4;
                                    static const int dot_map[4][2] = {{0, 3}, {1, 4}, {2, 5}, {6, 7}};
                                    uint8_t pattern = (1 << dot_map[ly][lx]);
                                    canvas.set_pattern(cx, cy, canvas.get_pattern(cx, cy) | pattern);
                                    canvas.set_color(cx, cy, fg_r, fg_g, fg_b);
                                }
                            }
                        }
                        x += font_w + spacing;
                    }
                    y += font_h + 1;
                }
                return canvas.render();
            }
        }

        /**
         * @brief Print text as braille art to stdout
         */
        inline void print_text_art(const std::string &text, Mode mode = Mode::bw_dot,
                                   uint8_t fg_r = 255, uint8_t fg_g = 255, uint8_t fg_b = 255)
        {
            std::cout << render_text_art(text, mode, 0, fg_r, fg_g, fg_b) << std::flush;
        }

        /**
         * @brief Multi-frame read-ahead buffer for smooth video playback.
         *
         * Decouples FFmpeg decode from rendering using a circular buffer of
         * pre-decoded frames. A background thread continuously reads frames
         * from the FFmpeg pipe, staying up to `capacity` frames ahead of the
         * renderer. This absorbs decode latency spikes and ensures the render
         * thread almost never has to wait for I/O.
         *
         * Usage:
         *   FrameReadAhead reader(pipe, frame_size, 8); // 8-frame buffer
         *   reader.start();
         *   while (auto* data = reader.next_frame()) {
         *       // process and display *data (frame_size bytes)
         *   }
         *   reader.stop();
         */
        class FrameReadAhead
        {
        private:
            FILE *_pipe;
            size_t _frame_size;
            size_t _capacity;                        // Max frames to buffer ahead
            std::vector<std::vector<uint8_t>> _ring; // Circular frame buffer
            std::vector<uint8_t> _display_buf;       // Dedicated display buffer (never touched by decode thread)
            size_t _write_pos = 0;                   // Next slot for decode thread to write
            size_t _read_pos = 0;                    // Next slot for render thread to read
            size_t _count = 0;                       // Number of frames currently buffered
            std::mutex _mtx;
            std::condition_variable _cv_not_full;  // Decode thread waits when buffer is full
            std::condition_variable _cv_not_empty; // Render thread waits when buffer is empty
            std::thread _thread;
            std::atomic<bool> _eof{false};
            std::atomic<bool> _running{false};

            void _read_loop()
            {
                while (_running)
                {
                    // Wait if buffer is full
                    {
                        std::unique_lock<std::mutex> lk(_mtx);
                        _cv_not_full.wait(lk, [this]
                                          { return _count < _capacity || !_running; });
                        if (!_running)
                            break;
                    }

                    // Read next frame from FFmpeg pipe (blocking I/O, outside lock)
                    size_t n = fread(_ring[_write_pos].data(), 1, _frame_size, _pipe);
                    if (n < _frame_size)
                    {
                        _eof = true;
                        _cv_not_empty.notify_one();
                        break;
                    }

                    // Advance write position
                    {
                        std::lock_guard<std::mutex> lk(_mtx);
                        _write_pos = (_write_pos + 1) % _capacity;
                        ++_count;
                    }
                    _cv_not_empty.notify_one();
                }
            }

        public:
            /**
             * @param pipe    FFmpeg decode pipe opened with open_decode_pipe()
             * @param frame_size  Size in bytes of one raw frame
             * @param capacity    Number of frames to buffer ahead (default 8)
             */
            FrameReadAhead(FILE *pipe, size_t frame_size, size_t capacity = 8)
                : _pipe(pipe), _frame_size(frame_size), _capacity(capacity),
                  _ring(capacity), _display_buf(frame_size)
            {
                for (auto &buf : _ring)
                    buf.resize(frame_size);
            }

            ~FrameReadAhead() { stop(); }

            /// Start the background decode/read thread
            void start()
            {
                _running = true;
                _eof = false;
                _write_pos = 0;
                _read_pos = 0;
                _count = 0;
                _thread = std::thread(&FrameReadAhead::_read_loop, this);
            }

            /// Stop the background thread and join
            void stop()
            {
                _running = false;
                _cv_not_full.notify_all();
                _cv_not_empty.notify_all();
                if (_thread.joinable())
                    _thread.join();
            }

            /**
             * @brief Get the next decoded frame. Blocks until available.
             * @return Pointer to frame data (frame_size bytes), or nullptr on EOF.
             *
             * The returned pointer is stable — it points to an internal display
             * buffer that the decode thread never touches. Valid until the next
             * call to next_frame().
             */
            const uint8_t *next_frame()
            {
                std::unique_lock<std::mutex> lk(_mtx);
                _cv_not_empty.wait(lk, [this]
                                   { return _count > 0 || _eof || !_running; });

                if (_count == 0)
                    return nullptr; // EOF or stopped

                // Copy to display buffer (safe from decode thread overwrites)
                std::memcpy(_display_buf.data(), _ring[_read_pos].data(), _frame_size);
                _read_pos = (_read_pos + 1) % _capacity;
                --_count;
                lk.unlock();
                _cv_not_full.notify_one();
                return _display_buf.data();
            }
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
         *   player.play();  // Blocking playback (non-interactive, no keyboard controls)
         *
         *   // With pause/stop controls (interactive mode):
         *   player.play(Shell::interactive, 'p', 's');  // Press 'p' to pause/resume, 's' to stop
         *
         *   // Or async:
         *   player.play_async(Shell::interactive);
         *   // ... do other work ...
         *   player.stop();
         */
        class VideoPlayer
        {
        private:
            std::string _filename;
            int _width;         // Output width in terminal characters
            int _threshold;     // Binarization threshold
            double _fps;        // Target FPS (0 = use video's native FPS)
            double _start_time; // Start time in seconds (-1 = from beginning)
            double _end_time;   // End time in seconds (-1 = to end)
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            BrailleCanvas _canvas;

        public:
            /**
             * @brief Create a video player
             * @param filename Path to video file
             * @param width Width in terminal characters (height auto-calculated)
             * @param threshold Brightness threshold for binary conversion (default: 128)
             * @param target_fps Target FPS, 0 = use video's native FPS
             * @param start_time Start time in seconds (-1 = from beginning)
             * @param end_time End time in seconds (-1 = to end)
             */
            VideoPlayer(const std::string &filename, int width = 80, int threshold = 128, double target_fps = 0,
                        double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(width), _threshold(threshold), _fps(target_fps),
                  _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
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
             * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
             * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
             * @param stop_key Key to stop playback (default 's', '\0' to disable)
             * @return true if playback completed successfully
             */
            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false; // Already running

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                bool result = _play_internal();
                _running = false;
                return result;
            }

            /**
             * @brief Start async playback in background thread
             * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
             * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
             * @param stop_key Key to stop playback (default 's', '\0' to disable)
             */
            void play_async(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return; // Already running

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

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
                _paused = false;
                if (_playback_thread.joinable())
                    _playback_thread.join();
            }

            /**
             * @brief Pause or resume playback
             */
            void toggle_pause()
            {
                _paused = !_paused;
            }

            /**
             * @brief Check if video is paused
             */
            bool is_paused() const { return _paused; }

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
                auto info = pythonic::accel::video::probe(_filename);
                return {info.width, info.height, info.fps, info.duration};
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

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "gray", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg. Is it installed?\n";
                    return false;
                }

                // Frame read-ahead for smooth playback (decouples decode from render)
                size_t frame_size = pixel_w * pixel_h;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();

                // Initialize canvas
                int char_height = pixel_h / 4;
                _canvas = BrailleCanvas(_width, char_height);

                // Use RAII guard for terminal state management
                TerminalStateGuard term_guard;

                // Only enable keyboard input in interactive mode
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                // Disable stdout buffering for smoother video output
                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output

                // Clear screen and position cursor at top
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

                // Pre-allocate output buffer for frame rendering
                std::string frame_output;
                frame_output.reserve(pixel_w * (pixel_h / 4) * 10);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    // Check for keyboard input (pause/stop) - only in interactive mode
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                                if (_paused)
                                {
                                    pause_start = std::chrono::steady_clock::now();
                                    // Show pause indicator
                                    std::cout << ansi::CURSOR_HOME << "[PAUSED - Press '" << _pause_key << "' to resume]" << std::flush;
                                }
                                else
                                {
                                    // Track time spent paused
                                    total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::steady_clock::now() - pause_start);
                                    // Reset absolute deadline after pause to prevent catch-up burst
                                    next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                                }
                            }
                        }
                    }

                    // If paused, just sleep and continue loop
                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    // Get pre-read frame from background thread
                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break; // End of video

                    // Convert frame to braille using optimized block operations
                    _canvas.load_frame_fast(frame_data, pixel_w, pixel_h, _threshold);

                    // Build complete frame string and write atomically with fwrite
                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    // Single atomic write prevents tearing/fragmentation
                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);

                // Restore terminal state (alt screen buffer restores original content)
                term_guard.restore();

                // Show statistics on the restored terminal
                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }
        };

        /**
         * @brief Play a video file in the terminal
         * @param filename Path to video file
         * @param width Terminal width in characters
         * @param threshold Brightness threshold (0-255)
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop playback (default 's', '\0' to disable)
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video(const std::string &filename, int width = 80, int threshold = 128,
                               Shell shell = Shell::noninteractive,
                               char pause_key = 'p', char stop_key = 's',
                               double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            VideoPlayer player(filename, width, threshold, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
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

        // ==================== Colored Video Player ====================

        /**
         * @brief Video player for terminal using true color (24-bit ANSI)
         *
         * Similar to VideoPlayer but uses ColorCanvas for full RGB output.
         * Supports pause/stop controls via keyboard input in interactive mode.
         */
        class ColoredVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            double _fps;
            double _start_time;
            double _end_time;
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            ColorCanvas _canvas;
            Shell _shell;
            char _pause_key;
            char _stop_key;

        public:
            ColoredVideoPlayer(const std::string &filename, int width = 80, double target_fps = 0,
                               double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(width), _fps(target_fps),
                  _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
                enable_ansi_support();
            }

            ~ColoredVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            /**
             * @brief Play video (blocking)
             * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
             * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
             * @param stop_key Key to stop playback (default 's', '\0' to disable)
             * @return true if playback completed successfully
             */
            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                bool result = _play_internal();
                _running = false;
                return result;
            }

            void stop()
            {
                _running = false;
                _paused = false;
                if (_playback_thread.joinable())
                    _playback_thread.join();
            }

            void toggle_pause() { _paused = !_paused; }
            bool is_paused() const { return _paused; }
            bool is_playing() const { return _running; }

            std::tuple<int, int, double, double> get_info() const
            {
                auto info = pythonic::accel::video::probe(_filename);
                return {info.width, info.height, info.fps, info.duration};
            }

        private:
            bool _play_internal()
            {
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info. Is FFmpeg installed?\n";
                    return false;
                }

                // ColorCanvas uses 1 pixel per char width, 2 pixels per char height
                int pixel_w = _width;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);
                pixel_h = ((pixel_h + 1) / 2) * 2; // Ensure even height

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "rgb24", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg.\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h * 3;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();

                _canvas = ColorCanvas::from_pixels(pixel_w, pixel_h);

                TerminalStateGuard term_guard;

                // Only enable keyboard input in interactive mode
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                // Disable stdout buffering for smoother video
                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

                // Pre-allocate output buffer
                std::string frame_output;
                frame_output.reserve(pixel_w * (pixel_h / 2) * 40);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    // Check for keyboard input (pause/stop) - only in interactive mode
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                                if (_paused)
                                {
                                    pause_start = std::chrono::steady_clock::now();
                                    std::cout << ansi::CURSOR_HOME << "[PAUSED - Press '" << _pause_key << "' to resume]" << std::flush;
                                }
                                else
                                {
                                    total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::steady_clock::now() - pause_start);
                                    // Reset absolute deadline after pause to prevent catch-up burst
                                    next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    _canvas.load_frame_rgb(frame_data, pixel_w, pixel_h);

                    // Build complete frame with cursor positioning
                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    // Single write for entire frame
                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }
        };

        /**
         * @brief Play video with true color rendering
         * @param filename Path to video file
         * @param width Terminal width in characters
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop playback (default 's', '\0' to disable)
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_colored(const std::string &filename, int width = 80,
                                       Shell shell = Shell::noninteractive,
                                       char pause_key = 'p', char stop_key = 's',
                                       double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            ColoredVideoPlayer player(filename, width, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
        }

        /**
         * @brief Video player using BW half-block characters (Mode::bw)
         *
         * Uses FFmpeg to decode video frames and renders them using
         * half-block Unicode characters (▀▄█) in grayscale.
         * Supports pause/stop controls via keyboard input in interactive mode.
         */
        class BWBlockVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            int _threshold;
            double _fps;
            double _start_time;
            double _end_time;
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            BWBlockCanvas _canvas;

        public:
            BWBlockVideoPlayer(const std::string &filename, int width = 80, int threshold = 128, double target_fps = 0,
                               double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(width), _threshold(threshold), _fps(target_fps),
                  _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
            }

            ~BWBlockVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            /**
             * @brief Play video (blocking)
             * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
             * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
             * @param stop_key Key to stop playback (default 's', '\0' to disable)
             * @return true if playback completed successfully
             */
            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                bool result = _play_internal();
                _running = false;
                return result;
            }

            void play_async(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                _playback_thread = std::thread([this]()
                                               {
                    _play_internal();
                    _running = false; });
            }

            void stop()
            {
                _running = false;
                _paused = false;
                if (_playback_thread.joinable())
                    _playback_thread.join();
            }

            void toggle_pause() { _paused = !_paused; }
            bool is_paused() const { return _paused; }
            bool is_playing() const { return _running; }

            std::tuple<int, int, double, double> get_info() const
            {
                VideoPlayer vp(_filename);
                return vp.get_info();
            }

        private:
            bool _play_internal()
            {
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info. Is FFmpeg installed?\n";
                    return false;
                }

                // BWBlockCanvas uses 1 pixel per char width, 2 pixels per char height
                int pixel_w = _width;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);
                pixel_h = ((pixel_h + 1) / 2) * 2; // Ensure even height

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "rgb24", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg.\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h * 3;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();

                _canvas = BWBlockCanvas::from_pixels(pixel_w, pixel_h);

                TerminalStateGuard term_guard;

                // Only enable keyboard input in interactive mode
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

                std::string frame_output;
                frame_output.reserve(pixel_w * (pixel_h / 2) * 30);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    // Check for keyboard input (pause/stop) - only in interactive mode
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                                if (_paused)
                                {
                                    pause_start = std::chrono::steady_clock::now();
                                    std::cout << ansi::CURSOR_HOME << "[PAUSED - Press '" << _pause_key << "' to resume]" << std::flush;
                                }
                                else
                                {
                                    total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::steady_clock::now() - pause_start);
                                    // Reset absolute deadline after pause to prevent catch-up burst
                                    next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    _canvas.load_frame_rgb(frame_data, pixel_w, pixel_h, _threshold);

                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }
        };

        /**
         * @brief Play video with BW half-block rendering
         * @param filename Path to video file
         * @param width Terminal width in characters
         * @param threshold Brightness threshold for BW modes
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop playback (default 's', '\0' to disable)
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_bw_block(const std::string &filename, int width = 80, int threshold = 128,
                                        Shell shell = Shell::noninteractive,
                                        char pause_key = 'p', char stop_key = 's',
                                        double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            BWBlockVideoPlayer player(filename, width, threshold, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
        }

        /**
         * @brief Video player using colored braille characters (Mode::colored_dot)
         *
         * Uses FFmpeg to decode video frames and renders them using
         * colored Unicode braille characters with averaged cell colors.
         * Supports pause/stop controls via keyboard input in interactive mode.
         */
        class ColoredBrailleVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            int _threshold;
            double _fps;
            double _start_time;
            double _end_time;
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            ColoredBrailleCanvas _canvas;

        public:
            ColoredBrailleVideoPlayer(const std::string &filename, int width = 80, int threshold = 128, double target_fps = 0,
                                      double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(width), _threshold(threshold), _fps(target_fps),
                  _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
            }

            ~ColoredBrailleVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            /**
             * @brief Play video (blocking)
             * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
             * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
             * @param stop_key Key to stop playback (default 's', '\0' to disable)
             * @return true if playback completed successfully
             */
            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                bool result = _play_internal();
                _running = false;
                return result;
            }

            void play_async(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                _playback_thread = std::thread([this]()
                                               {
                    _play_internal();
                    _running = false; });
            }

            void stop()
            {
                _running = false;
                _paused = false;
                if (_playback_thread.joinable())
                    _playback_thread.join();
            }

            void toggle_pause() { _paused = !_paused; }
            bool is_paused() const { return _paused; }
            bool is_playing() const { return _running; }

            std::tuple<int, int, double, double> get_info() const
            {
                VideoPlayer vp(_filename);
                return vp.get_info();
            }

        private:
            bool _play_internal()
            {
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info. Is FFmpeg installed?\n";
                    return false;
                }

                // ColoredBrailleCanvas uses 2 pixels per char width, 4 pixels per char height
                int pixel_w = _width * 2;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);
                pixel_h = (pixel_h + 3) / 4 * 4; // Ensure multiple of 4

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "rgb24", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg.\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h * 3;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();

                _canvas = ColoredBrailleCanvas::from_pixels(pixel_w, pixel_h);

                TerminalStateGuard term_guard;

                // Only enable keyboard input in interactive mode
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

                std::string frame_output;
                frame_output.reserve(_width * (pixel_h / 4) * 30);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    // Check for keyboard input (pause/stop) - only in interactive mode
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                                if (_paused)
                                {
                                    pause_start = std::chrono::steady_clock::now();
                                    std::cout << ansi::CURSOR_HOME << "[PAUSED - Press '" << _pause_key << "' to resume]" << std::flush;
                                }
                                else
                                {
                                    total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::steady_clock::now() - pause_start);
                                    // Reset absolute deadline after pause to prevent catch-up burst
                                    next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    _canvas.load_frame_rgb(frame_data, pixel_w, pixel_h, _threshold);

                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }
        };

        /**
         * @brief Play video with colored braille rendering
         * @param filename Path to video file
         * @param width Terminal width in characters
         * @param threshold Brightness threshold for BW modes
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop playback (default 's', '\0' to disable)
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_colored_dot(const std::string &filename, int width = 80, int threshold = 128,
                                           Shell shell = Shell::noninteractive,
                                           char pause_key = 'p', char stop_key = 's',
                                           double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            ColoredBrailleVideoPlayer player(filename, width, threshold, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
        }

        // ==================== Dithered Video Player ====================

        /**
         * @brief Video player using ordered dithering for smooth grayscale (Mode::bw_dithered)
         *
         * Uses FFmpeg to decode video frames and renders them using
         * braille characters with ordered dithering for natural grayscale gradients.
         */
        class DitheredVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            double _fps;
            double _start_time;
            double _end_time;
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            BrailleCanvas _canvas;

        public:
            DitheredVideoPlayer(const std::string &filename, int width = 80, double target_fps = 0,
                                double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(width), _fps(target_fps),
                  _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
            }

            ~DitheredVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                bool result = _play_internal();
                _running = false;
                return result;
            }

            void stop()
            {
                _running = false;
                _paused = false;
                if (_playback_thread.joinable())
                    _playback_thread.join();
            }

            void toggle_pause() { _paused = !_paused; }
            bool is_paused() const { return _paused; }
            bool is_playing() const { return _running; }

            std::tuple<int, int, double, double> get_info() const
            {
                auto info = pythonic::accel::video::probe(_filename);
                return {info.width, info.height, info.fps, info.duration};
            }

        private:
            bool _play_internal()
            {
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info. Is FFmpeg installed?\n";
                    return false;
                }

                int pixel_w = _width * 2;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);
                pixel_h = (pixel_h + 3) / 4 * 4;

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "gray", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg. Is it installed?\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();
                int char_height = pixel_h / 4;
                _canvas = BrailleCanvas(_width, char_height);

                TerminalStateGuard term_guard;
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                    keyboard = std::make_unique<KeyboardInput>();
                bool user_stopped = false;

                // Disable stdout buffering for smoother video output
                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

                // Pre-allocate output buffer
                std::string frame_output;
                frame_output.reserve(pixel_w * (pixel_h / 4) * 10);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                                if (_paused)
                                {
                                    pause_start = std::chrono::steady_clock::now();
                                    std::cout << ansi::CURSOR_HOME << "[PAUSED - Press '" << _pause_key << "' to resume]" << std::flush;
                                }
                                else
                                {
                                    total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::steady_clock::now() - pause_start);
                                    // Reset absolute deadline after pause to prevent catch-up burst
                                    next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    // Use ordered dithering instead of threshold
                    _canvas.load_frame_ordered_dithered(frame_data, pixel_w, pixel_h);

                    // Build complete frame and write atomically
                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());
                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }
        };

        /**
         * @brief Play video with ordered dithering for smooth grayscale
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_dithered(const std::string &filename, int width = 80,
                                        Shell shell = Shell::noninteractive,
                                        char pause_key = 'p', char stop_key = 's',
                                        double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            DitheredVideoPlayer player(filename, width, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
        }

        // ==================== Grayscale Video Player ====================

        /**
         * @brief Video player using grayscale-colored braille dots (Mode::grayscale_dot)
         *
         * Uses FFmpeg to decode video frames and renders them using
         * braille characters where each cell is colored based on average brightness.
         * Creates smoother, more visually appealing grayscale output.
         */
        class GrayscaleVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            double _fps;
            double _start_time;
            double _end_time;
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            BrailleCanvas _canvas;

        public:
            GrayscaleVideoPlayer(const std::string &filename, int width = 80, double target_fps = 0,
                                 double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(width), _fps(target_fps),
                  _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
                enable_ansi_support();
            }

            ~GrayscaleVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                bool result = _play_internal();
                _running = false;
                return result;
            }

            void stop()
            {
                _running = false;
                _paused = false;
                if (_playback_thread.joinable())
                    _playback_thread.join();
            }

            void toggle_pause() { _paused = !_paused; }
            bool is_paused() const { return _paused; }
            bool is_playing() const { return _running; }

            std::tuple<int, int, double, double> get_info() const
            {
                auto info = pythonic::accel::video::probe(_filename);
                return {info.width, info.height, info.fps, info.duration};
            }

        private:
            // GrayscaleVideoPlayer::_play_internal
            bool _play_internal()
            {
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info. Is FFmpeg installed?\n";
                    return false;
                }

                int pixel_w = _width * 2;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);
                pixel_h = (pixel_h + 3) / 4 * 4;

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "gray", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg. Is it installed?\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();
                int char_height = pixel_h / 4;
                _canvas = BrailleCanvas(_width, char_height);

                TerminalStateGuard term_guard;
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                    keyboard = std::make_unique<KeyboardInput>();
                bool user_stopped = false;

                // Disable stdout buffering for smoother video output
                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

                // Pre-allocate output buffer for atomic frame writes
                std::string frame_output;
                frame_output.reserve(pixel_w * (pixel_h / 4) * 40);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                                if (_paused)
                                {
                                    pause_start = std::chrono::steady_clock::now();
                                    std::cout << ansi::CURSOR_HOME << "[PAUSED - Press '" << _pause_key << "' to resume]" << std::flush;
                                }
                                else
                                {
                                    total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::steady_clock::now() - pause_start);
                                    // Reset absolute deadline after pause to prevent catch-up burst
                                    next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    // Load with dithering and grayscale storage
                    int cw = (pixel_w + 1) / 2;
                    int ch = (pixel_h + 3) / 4;

                    // Resize canvas if needed
                    if (cw != (int)_canvas.char_width() || ch != (int)_canvas.char_height())
                    {
                        _canvas = BrailleCanvas(cw, ch);
                    }

                    for (int cy = 0; cy < ch; ++cy)
                    {
                        for (int cx = 0; cx < cw; ++cx)
                        {
                            uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                            int px = cx * 2;
                            int py = cy * 4;

                            for (int row = 0; row < 4; ++row)
                            {
                                for (int col = 0; col < 2; ++col)
                                {
                                    int x = px + col;
                                    int y = py + row;
                                    if (x < pixel_w && y < pixel_h)
                                    {
                                        grays[row * 2 + col] = frame_data[y * pixel_w + x];
                                    }
                                }
                            }
                            _canvas.set_block_gray_dithered_with_brightness(cx, cy, grays);
                        }
                    }

                    // Build complete frame and write atomically
                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render_grayscale();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());
                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }
        };

        /**
         * @brief Play video with grayscale-colored braille dots
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_grayscale(const std::string &filename, int width = 80,
                                         Shell shell = Shell::noninteractive,
                                         char pause_key = 'p', char stop_key = 's',
                                         double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            GrayscaleVideoPlayer player(filename, width, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
        }

        // ==================== Flood Dot Video Player ====================

        /**
         * @brief Video player using flood-fill braille rendering (Mode::flood_dot)
         *
         * Lights ALL 8 dots in every character cell and colors them by average
         * brightness. Creates the smoothest appearance for videos by relying
         * entirely on color gradation rather than dot patterns.
         */
        class FloodDotVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            double _fps;
            double _start_time;
            double _end_time;
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            BrailleCanvas _canvas;

        public:
            FloodDotVideoPlayer(const std::string &filename, int width = 80, double target_fps = 0,
                                double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(width), _fps(target_fps),
                  _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
                enable_ansi_support();
            }

            ~FloodDotVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                bool result = _play_internal();
                _running = false;
                return result;
            }

            void stop()
            {
                _running = false;
                _paused = false;
                if (_playback_thread.joinable())
                    _playback_thread.join();
            }

            void toggle_pause() { _paused = !_paused; }
            bool is_paused() const { return _paused; }
            bool is_playing() const { return _running; }

            std::tuple<int, int, double, double> get_info() const
            {
                auto info = pythonic::accel::video::probe(_filename);
                return {info.width, info.height, info.fps, info.duration};
            }

        private:
            bool _play_internal()
            {
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info. Is FFmpeg installed?\n";
                    return false;
                }

                int pixel_w = _width * 2;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);
                pixel_h = (pixel_h + 3) / 4 * 4;

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "gray", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg. Is it installed?\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();
                int char_height = pixel_h / 4;
                _canvas = BrailleCanvas(_width, char_height);

                TerminalStateGuard term_guard;
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                    keyboard = std::make_unique<KeyboardInput>();
                bool user_stopped = false;

                // Disable stdout buffering for smoother video output
                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

                // Pre-allocate output buffer for atomic frame writes
                std::string frame_output;
                frame_output.reserve(pixel_w * (pixel_h / 4) * 40);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                                if (_paused)
                                {
                                    pause_start = std::chrono::steady_clock::now();
                                    std::cout << ansi::CURSOR_HOME << "[PAUSED - Press '" << _pause_key << "' to resume]" << std::flush;
                                }
                                else
                                {
                                    total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::steady_clock::now() - pause_start);
                                    // Reset absolute deadline after pause to prevent catch-up burst
                                    next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    // Load with flood fill (all dots on, colored by average brightness)
                    int cw = (pixel_w + 1) / 2;
                    int ch = (pixel_h + 3) / 4;

                    // Resize canvas if needed
                    if (cw != (int)_canvas.char_width() || ch != (int)_canvas.char_height())
                    {
                        _canvas = BrailleCanvas(cw, ch);
                    }

                    for (int cy = 0; cy < ch; ++cy)
                    {
                        for (int cx = 0; cx < cw; ++cx)
                        {
                            uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                            int px = cx * 2;
                            int py = cy * 4;

                            for (int row = 0; row < 4; ++row)
                            {
                                for (int col = 0; col < 2; ++col)
                                {
                                    int x = px + col;
                                    int y = py + row;
                                    if (x < pixel_w && y < pixel_h)
                                    {
                                        grays[row * 2 + col] = frame_data[y * pixel_w + x];
                                    }
                                }
                            }
                            // Use flood fill instead of dithering - all dots on
                            _canvas.set_block_flood_fill(cx, cy, grays);
                        }
                    }

                    // Build complete frame and write atomically
                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render_grayscale();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());
                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }
        };

        /**
         * @brief Play video with flood-fill braille dots (all dots on, colored by brightness)
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_flood(const std::string &filename, int width = 80,
                                     Shell shell = Shell::noninteractive,
                                     char pause_key = 'p', char stop_key = 's',
                                     double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            FloodDotVideoPlayer player(filename, width, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
        }

        /**
         * @brief Video player for colored flood-fill mode (all dots on, RGB colored)
         */
        class ColoredFloodVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            double _fps;
            double _start_time;
            double _end_time;
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            ColoredBrailleCanvas _canvas;

        public:
            ColoredFloodVideoPlayer(const std::string &filename, int max_width = 80,
                                    double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(max_width), _fps(fps), _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
                enable_ansi_support();
            }

            ~ColoredFloodVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                return _play_internal();
            }

            void stop() { _running = false; }

            std::tuple<int, int, double, double> get_info() const
            {
                auto info = pythonic::accel::video::probe(_filename);
                return {info.width, info.height, info.fps, info.duration};
            }

        private:
            bool _play_internal()
            {
                enable_ansi_support();

                // Get video info using ffprobe
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info.\n";
                    return false;
                }

                // Braille: 2 pixels wide, 4 pixels tall per char
                int pixel_w = _width * 2;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);
                pixel_h = (pixel_h + 3) / 4 * 4; // Round to multiple of 4

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "rgb24", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg video decoder.\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h * 3;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();

                _canvas = ColoredBrailleCanvas::from_pixels(pixel_w, pixel_h);

                TerminalStateGuard term_guard;

                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    // Load with flood fill (all dots on, RGB colored)
                    _load_frame_flood(frame_data, pixel_w, pixel_h);

                    // Build complete frame and write atomically
                    std::string frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Played " << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }

            void _load_frame_flood(const uint8_t *data, int width, int height)
            {
                int cols = (width + 1) / 2;
                int rows = (height + 3) / 4;

                for (int cy = 0; cy < rows; ++cy)
                {
                    for (int cx = 0; cx < cols; ++cx)
                    {
                        int sum_r = 0, sum_g = 0, sum_b = 0;
                        int count = 0;
                        int px = cx * 2, py = cy * 4;

                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col, y = py + row;
                                if (x < width && y < height)
                                {
                                    int idx = (y * width + x) * 3;
                                    sum_r += data[idx];
                                    sum_g += data[idx + 1];
                                    sum_b += data[idx + 2];
                                    count++;
                                }
                            }
                        }

                        if (count > 0)
                        {
                            uint8_t avg_r = sum_r / count;
                            uint8_t avg_g = sum_g / count;
                            uint8_t avg_b = sum_b / count;
                            _canvas.set_pattern(cx, cy, 0xFF); // All 8 dots on
                            _canvas.set_color(cx, cy, avg_r, avg_g, avg_b);
                        }
                    }
                }
            }
        };

        /**
         * @brief Play video with colored flood-fill braille dots (all dots on, RGB colored)
         */
        inline void play_video_colored_flood(const std::string &filename, int width = 80,
                                             Shell shell = Shell::noninteractive,
                                             char pause_key = 'p', char stop_key = 's',
                                             double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            ColoredFloodVideoPlayer player(filename, width, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
        }

        /**
         * @brief Video player for colored dithered mode (dithered dots, RGB colored)
         */
        class ColoredDitheredVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            double _fps;
            double _start_time;
            double _end_time;
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            ColoredBrailleCanvas _canvas;
            static constexpr int bayer2x2[2][2] = {{0, 2}, {3, 1}};

        public:
            ColoredDitheredVideoPlayer(const std::string &filename, int max_width = 80,
                                       double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(max_width), _fps(fps), _start_time(start_time), _end_time(end_time),
                  _running(false), _paused(false), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
                enable_ansi_support();
            }

            ~ColoredDitheredVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                return _play_internal();
            }

            void stop() { _running = false; }

            std::tuple<int, int, double, double> get_info() const
            {
                auto info = pythonic::accel::video::probe(_filename);
                return {info.width, info.height, info.fps, info.duration};
            }

        private:
            bool _play_internal()
            {
                enable_ansi_support();

                // Get video info using ffprobe
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info.\n";
                    return false;
                }

                // Braille: 2 pixels wide, 4 pixels tall per char
                int pixel_w = _width * 2;
                int pixel_h = (int)(pixel_w * vid_h / vid_w);
                pixel_h = (pixel_h + 3) / 4 * 4; // Round to multiple of 4

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                FILE *pipe = pythonic::accel::video::open_decode_pipe(
                    _filename, "rgb24", pixel_w, pixel_h, target_fps, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg video decoder.\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h * 3;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();

                _canvas = ColoredBrailleCanvas::from_pixels(pixel_w, pixel_h);

                TerminalStateGuard term_guard;

                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    // Load with dithering
                    _load_frame_dithered(frame_data, pixel_w, pixel_h);

                    // Build complete frame and write atomically
                    std::string frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Played " << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }

            void _load_frame_dithered(const uint8_t *data, int width, int height)
            {
                static const int dot_map[4][2] = {{0, 3}, {1, 4}, {2, 5}, {6, 7}};
                int cols = (width + 1) / 2;
                int rows = (height + 3) / 4;

                for (int cy = 0; cy < rows; ++cy)
                {
                    for (int cx = 0; cx < cols; ++cx)
                    {
                        uint8_t pattern = 0;
                        int sum_r = 0, sum_g = 0, sum_b = 0;
                        int count = 0;
                        int px = cx * 2, py = cy * 4;

                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 2; ++col)
                            {
                                int x = px + col, y = py + row;
                                if (x < width && y < height)
                                {
                                    int idx = (y * width + x) * 3;
                                    uint8_t r = data[idx];
                                    uint8_t g = data[idx + 1];
                                    uint8_t b = data[idx + 2];
                                    sum_r += r;
                                    sum_g += g;
                                    sum_b += b;
                                    count++;

                                    // Grayscale for dithering
                                    uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
                                    int bayer_x = col & 1;
                                    int bayer_y = row & 1;
                                    int threshold_val = ((bayer2x2[bayer_y][bayer_x] + 1) * 255) / 5;

                                    if (gray > threshold_val)
                                    {
                                        pattern |= (1 << dot_map[row][col]);
                                    }
                                }
                            }
                        }

                        if (count > 0)
                        {
                            uint8_t avg_r = sum_r / count;
                            uint8_t avg_g = sum_g / count;
                            uint8_t avg_b = sum_b / count;
                            _canvas.set_pattern(cx, cy, pattern);
                            _canvas.set_color(cx, cy, avg_r, avg_g, avg_b);
                        }
                    }
                }
            }
        };

        /**
         * @brief Play video with colored dithered braille dots
         */
        inline void play_video_colored_dithered(const std::string &filename, int width = 80,
                                                Shell shell = Shell::noninteractive,
                                                char pause_key = 'p', char stop_key = 's',
                                                double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            ColoredDitheredVideoPlayer player(filename, width, fps, start_time, end_time);
            player.play(shell, pause_key, stop_key);
        }

        /**
         * @brief Unified video playback function that handles all modes
         * @param filename Path to video file
         * @param width Terminal width in characters
         * @param mode Rendering mode (bw, bw_dot, colored, colored_dot, bw_dithered, grayscale_dot)
         * @param threshold Brightness threshold for BW modes
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop playback (default 's', '\0' to disable)
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_with_mode(const std::string &filename, int width = 80,
                                         Mode mode = Mode::bw_dot, int threshold = 128,
                                         Shell shell = Shell::noninteractive,
                                         char pause_key = 'p', char stop_key = 's',
                                         double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            switch (mode)
            {
            case Mode::bw:
                play_video_bw_block(filename, width, threshold, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            case Mode::bw_dot:
                play_video(filename, width, threshold, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            case Mode::colored:
                play_video_colored(filename, width, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            case Mode::colored_dot:
                play_video_colored_dot(filename, width, threshold, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            case Mode::bw_dithered:
                play_video_dithered(filename, width, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            case Mode::grayscale_dot:
                play_video_grayscale(filename, width, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            case Mode::flood_dot:
                play_video_flood(filename, width, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            case Mode::flood_dot_colored:
                play_video_colored_flood(filename, width, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            case Mode::colored_dithered:
                play_video_colored_dithered(filename, width, shell, pause_key, stop_key, fps, start_time, end_time);
                break;
            }
        }

        // ==================== Audio Support Detection ====================

        /**
         * @brief Check if SDL2 audio support is available
         */
        inline bool has_sdl2_audio()
        {
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
            return true;
#else
            return false;
#endif
        }

        /**
         * @brief Check if PortAudio support is available
         */
        inline bool has_portaudio()
        {
#ifdef PYTHONIC_ENABLE_PORTAUDIO
            return true;
#else
            return false;
#endif
        }

        /**
         * @brief Check if any audio backend is available
         */
        inline bool has_audio_support()
        {
            return has_sdl2_audio() || has_portaudio();
        }

        // ==================== Audio-Video Player with SDL2/PortAudio ====================

#if defined(PYTHONIC_ENABLE_SDL2_AUDIO) || defined(PYTHONIC_ENABLE_PORTAUDIO)

        /**
         * @brief Thread-safe audio buffer for synchronized playback
         */
        class AudioBuffer
        {
        private:
            std::queue<std::vector<uint8_t>> _queue;
            std::mutex _mutex;
            std::condition_variable _cv;
            std::atomic<bool> _finished{false};
            size_t _max_queue_size = 64; // Max buffered chunks

            // Leftover data from previous chunk that didn't fit in callback
            std::vector<uint8_t> _leftover;
            size_t _leftover_pos = 0;

        public:
            void push(std::vector<uint8_t> chunk)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this]
                         { return _queue.size() < _max_queue_size || _finished; });
                if (!_finished)
                {
                    _queue.push(std::move(chunk));
                    _cv.notify_one();
                }
            }

            // Non-blocking pop for audio callbacks - returns whatever is available
            bool try_pop(std::vector<uint8_t> &chunk)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (_queue.empty())
                    return false;
                chunk = std::move(_queue.front());
                _queue.pop();
                _cv.notify_one();
                return true;
            }

            // Fill buffer with audio data, non-blocking
            // Returns number of bytes actually filled
            size_t fill_buffer(uint8_t *dest, size_t len)
            {
                size_t filled = 0;

                // First, use any leftover from previous call
                if (_leftover_pos < _leftover.size())
                {
                    size_t avail = _leftover.size() - _leftover_pos;
                    size_t to_copy = std::min(avail, len);
                    std::memcpy(dest, _leftover.data() + _leftover_pos, to_copy);
                    _leftover_pos += to_copy;
                    filled += to_copy;
                    dest += to_copy;
                    len -= to_copy;
                }

                // Now get more chunks as needed
                while (len > 0)
                {
                    std::vector<uint8_t> chunk;
                    if (!try_pop(chunk))
                        break;

                    size_t to_copy = std::min(chunk.size(), len);
                    std::memcpy(dest, chunk.data(), to_copy);
                    filled += to_copy;
                    dest += to_copy;
                    len -= to_copy;

                    // Save leftover
                    if (to_copy < chunk.size())
                    {
                        _leftover = std::move(chunk);
                        _leftover_pos = to_copy;
                    }
                }

                return filled;
            }

            // Blocking pop for synchronous consumption
            bool pop(std::vector<uint8_t> &chunk)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this]
                         { return !_queue.empty() || _finished; });
                if (_queue.empty())
                    return false;
                chunk = std::move(_queue.front());
                _queue.pop();
                _cv.notify_one();
                return true;
            }

            void finish()
            {
                _finished = true;
                _cv.notify_all();
            }

            bool is_finished() const { return _finished && _queue.empty(); }
            void reset()
            {
                _finished = false;
                _leftover.clear();
                _leftover_pos = 0;
            }
        };

        // ==================== Threaded Video Playback System ====================

        /**
         * @brief Commands that can be sent from keyboard thread to player
         */
        enum class PlayerCommand
        {
            None,
            Pause,
            Stop,
            VolumeUp,
            VolumeDown,
            SeekBackward,
            SeekForward
        };

        /**
         * @brief Thread-safe command queue for non-blocking keyboard input
         */
        class CommandQueue
        {
        private:
            std::queue<PlayerCommand> _queue;
            mutable std::mutex _mutex;

        public:
            void push(PlayerCommand cmd)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _queue.push(cmd);
            }

            bool try_pop(PlayerCommand &cmd)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (_queue.empty())
                    return false;
                cmd = _queue.front();
                _queue.pop();
                return true;
            }

            void clear()
            {
                std::lock_guard<std::mutex> lock(_mutex);
                while (!_queue.empty())
                    _queue.pop();
            }
        };

        /**
         * @brief Non-blocking keyboard input manager that runs in its own thread
         *
         * Monitors keyboard for arrow keys and control characters without blocking
         * video playback. Commands are pushed to a queue for the main player to consume.
         */
        class KeyboardManager
        {
        private:
            std::atomic<bool> _running{false};
            std::thread _thread;
            CommandQueue &_cmd_queue;
            char _pause_key;
            char _stop_key;
            int _vol_up_key;
            int _vol_down_key;
            int _seek_back_key;
            int _seek_fwd_key;

#ifndef _WIN32
            struct termios _old_termios;
            bool _termios_saved = false;
#endif

            void set_raw_mode()
            {
#ifndef _WIN32
                struct termios raw;
                if (tcgetattr(STDIN_FILENO, &_old_termios) == 0)
                {
                    _termios_saved = true;
                    raw = _old_termios;
                    raw.c_lflag &= ~(ICANON | ECHO);
                    raw.c_cc[VMIN] = 0;
                    raw.c_cc[VTIME] = 0;
                    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
                }
#endif
            }

            void restore_terminal()
            {
#ifndef _WIN32
                if (_termios_saved)
                {
                    tcsetattr(STDIN_FILENO, TCSANOW, &_old_termios);
                    _termios_saved = false;
                }
#endif
            }

            int read_key_nonblocking()
            {
#ifdef _WIN32
                if (_kbhit())
                {
                    int ch = _getch();
                    if (ch == 0 || ch == 224)
                    {
                        int ext = _getch();
                        // Arrow keys: Up=72, Down=80, Left=75, Right=77
                        switch (ext)
                        {
                        case 72:
                            return 0x1B5B41; // Up
                        case 80:
                            return 0x1B5B42; // Down
                        case 75:
                            return 0x1B5B44; // Left
                        case 77:
                            return 0x1B5B43; // Right
                        }
                    }
                    return ch;
                }
                return -1;
#else
                char buf[4] = {0};
                ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
                if (n <= 0)
                    return -1;

                // Check for escape sequence (arrow keys)
                if (n >= 3 && buf[0] == '\x1B' && buf[1] == '[')
                {
                    // Encode as multi-byte: 0x1B5Bxx where xx is the arrow char
                    return (0x1B << 16) | ('[' << 8) | buf[2];
                }

                return static_cast<unsigned char>(buf[0]);
#endif
            }

            void keyboard_thread_func()
            {
                set_raw_mode();

                while (_running)
                {
                    int key = read_key_nonblocking();
                    if (key != -1)
                    {
                        if (_stop_key != '\0' && key == _stop_key)
                            _cmd_queue.push(PlayerCommand::Stop);
                        else if (_pause_key != '\0' && key == _pause_key)
                            _cmd_queue.push(PlayerCommand::Pause);
                        else if (key == _vol_up_key)
                            _cmd_queue.push(PlayerCommand::VolumeUp);
                        else if (key == _vol_down_key)
                            _cmd_queue.push(PlayerCommand::VolumeDown);
                        else if (key == _seek_back_key)
                            _cmd_queue.push(PlayerCommand::SeekBackward);
                        else if (key == _seek_fwd_key)
                            _cmd_queue.push(PlayerCommand::SeekForward);
                    }

                    // Small sleep to avoid busy-waiting
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                restore_terminal();
            }

        public:
            KeyboardManager(CommandQueue &queue, char pause_key, char stop_key,
                            int vol_up, int vol_down, int seek_back, int seek_fwd)
                : _cmd_queue(queue), _pause_key(pause_key), _stop_key(stop_key),
                  _vol_up_key(vol_up), _vol_down_key(vol_down),
                  _seek_back_key(seek_back), _seek_fwd_key(seek_fwd)
            {
            }

            ~KeyboardManager()
            {
                stop();
            }

            void start()
            {
                if (_running.exchange(true))
                    return;
                _thread = std::thread(&KeyboardManager::keyboard_thread_func, this);
            }

            void stop()
            {
                _running = false;
                if (_thread.joinable())
                    _thread.join();
            }

            bool is_running() const { return _running; }
        };

        /**
         * @brief Synchronized frame data for video buffer
         */
        struct VideoFrame
        {
            std::vector<uint8_t> data;
            int64_t frame_number = 0;
            double timestamp = 0.0; // in seconds
        };

        /**
         * @brief Synchronized audio chunk for audio buffer
         */
        struct AudioChunk
        {
            std::vector<uint8_t> data;
            double timestamp = 0.0; // in seconds
        };

        /**
         * @brief Thread-safe ring buffer for video frames with seeking support
         *
         * Maintains a sliding window of frames:
         * - Behind frames: for quick backward seeking
         * - Ahead frames: preloaded for smooth playback
         */
        class VideoFrameBuffer
        {
        private:
            std::deque<VideoFrame> _frames;
            mutable std::mutex _mutex;
            std::condition_variable _cv_producer; // For decoder waiting on full buffer
            std::condition_variable _cv_consumer; // For renderer waiting on empty buffer
            std::atomic<bool> _finished{false};

            size_t _max_ahead;
            size_t _max_behind;
            int64_t _current_frame = 0;
            int64_t _frame_offset = 0; // Offset added to frame numbers after seeking

            // Seeking state
            std::atomic<bool> _seek_requested{false};
            std::atomic<double> _seek_time{-1.0}; // Time in seconds to seek to

        public:
            VideoFrameBuffer(size_t ahead = 60, size_t behind = 90)
                : _max_ahead(ahead), _max_behind(behind)
            {
            }

            /**
             * @brief Push a decoded frame to the buffer (called by decoder thread)
             * Blocks if buffer is full, unless finished
             */
            void push(VideoFrame frame)
            {
                std::unique_lock<std::mutex> lock(_mutex);

                // Wait if we have too many future frames
                _cv_producer.wait(lock, [this]
                                  {
                    if (_finished || _seek_requested)
                        return true;
                    size_t ahead_count = 0;
                    for (const auto &f : _frames)
                    {
                        if (f.frame_number >= _current_frame)
                            ahead_count++;
                    }
                    return ahead_count < _max_ahead; });

                if (_finished || _seek_requested)
                    return;

                // Apply offset to frame number
                frame.frame_number += _frame_offset;
                _frames.push_back(std::move(frame));
                _cv_consumer.notify_one();
            }

            /**
             * @brief Get the next frame for rendering (called by render thread)
             * Blocks if buffer is empty, unless finished
             */
            bool pop(VideoFrame &frame)
            {
                std::unique_lock<std::mutex> lock(_mutex);

                _cv_consumer.wait(lock, [this]
                                  {
                    if (_finished)
                        return true;
                    if (_seek_requested)
                        return true;  // Wake up to let render loop handle seeking
                    for (const auto &f : _frames)
                    {
                        if (f.frame_number >= _current_frame)
                            return true;
                    }
                    return false; });

                if (_frames.empty() || _seek_requested)
                    return false;

                // Find the frame at or after current position
                for (auto it = _frames.begin(); it != _frames.end(); ++it)
                {
                    if (it->frame_number >= _current_frame)
                    {
                        frame = *it;
                        _current_frame = it->frame_number + 1;

                        // Remove old frames (keep _max_behind)
                        while (_frames.size() > _max_behind && _frames.front().frame_number < _current_frame - (int64_t)_max_behind)
                        {
                            _frames.pop_front();
                        }

                        _cv_producer.notify_one();
                        return true;
                    }
                }

                return false;
            }

            /**
             * @brief Request a seek to a specific time
             * @param time_seconds Time in seconds to seek to
             */
            void request_seek(double time_seconds)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _seek_time = time_seconds;
                _seek_requested = true;
                _cv_producer.notify_all();
                _cv_consumer.notify_all();
            }

            /**
             * @brief Get and clear seek request (called by decode thread)
             * @return The requested seek time, or -1 if no seek pending
             */
            double get_and_clear_seek_request()
            {
                if (!_seek_requested)
                    return -1.0;

                std::lock_guard<std::mutex> lock(_mutex);
                double t = _seek_time.load();
                _seek_requested = false;
                _seek_time = -1.0;
                return t;
            }

            /**
             * @brief Called after seeking is complete to reset buffer state
             * @param new_start_frame The frame number to start from after seek
             * @param fps Frames per second for calculating offset
             */
            void complete_seek(int64_t new_start_frame, double fps)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _frames.clear();
                _frame_offset = new_start_frame;
                _current_frame = new_start_frame;
                _seek_requested = false;
            }

            /**
             * @brief Initialize the frame offset for initial playback position
             * @param start_frame The frame offset for initial playback (based on start_time * fps)
             */
            void set_initial_offset(int64_t start_frame)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _frame_offset = start_frame;
                _current_frame = start_frame;
            }

            bool has_seek_request() const { return _seek_requested; }
            int64_t get_current_frame() const { return _current_frame; }
            int64_t get_frame_offset() const { return _frame_offset; }

            void finish()
            {
                _finished = true;
                _seek_requested = false; // Clear seek request to unblock render loop
                _cv_producer.notify_all();
                _cv_consumer.notify_all();
            }

            bool is_finished() const { return _finished; }

            void reset()
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _frames.clear();
                _finished = false;
                _seek_requested = false;
                _seek_time = -1.0;
                _current_frame = 0;
                _frame_offset = 0;
            }

            size_t size() const
            {
                std::lock_guard<std::mutex> lock(_mutex);
                return _frames.size();
            }
        };

        /**
         * @brief Thread-safe audio buffer with timestamp support for sync
         */
        class SyncedAudioBuffer
        {
        private:
            std::deque<AudioChunk> _chunks;
            mutable std::mutex _mutex;
            std::condition_variable _cv;
            std::atomic<bool> _finished{false};
            std::atomic<int> _volume{100}; // 0-100
            size_t _max_chunks = 128;

            // Leftover data from previous chunk
            std::vector<uint8_t> _leftover;
            size_t _leftover_pos = 0;

            // Seeking state
            std::atomic<bool> _seek_requested{false};
            std::atomic<double> _seek_time{-1.0};

        public:
            void push(AudioChunk chunk)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this]
                         { return _chunks.size() < _max_chunks || _finished || _seek_requested; });
                if (_finished || _seek_requested)
                    return;
                _chunks.push_back(std::move(chunk));
                _cv.notify_one();
            }

            /**
             * @brief Fill audio buffer with volume-adjusted samples
             */
            size_t fill_buffer(uint8_t *dest, size_t len)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                size_t filled = 0;
                int vol = _volume.load();

                auto apply_volume = [vol](int16_t sample) -> int16_t
                {
                    if (vol == 100)
                        return sample;
                    return static_cast<int16_t>((static_cast<int32_t>(sample) * vol) / 100);
                };

                // First use leftover
                while (_leftover_pos < _leftover.size() && len >= 2)
                {
                    int16_t sample;
                    std::memcpy(&sample, _leftover.data() + _leftover_pos, sizeof(sample));
                    sample = apply_volume(sample);
                    std::memcpy(dest, &sample, sizeof(sample));
                    dest += 2;
                    _leftover_pos += 2;
                    filled += 2;
                    len -= 2;
                }

                // Then get new chunks
                while (len >= 2 && !_chunks.empty())
                {
                    auto &chunk = _chunks.front();
                    size_t chunk_pos = 0;

                    while (chunk_pos < chunk.data.size() && len >= 2)
                    {
                        int16_t sample;
                        std::memcpy(&sample, chunk.data.data() + chunk_pos, sizeof(sample));
                        sample = apply_volume(sample);
                        std::memcpy(dest, &sample, sizeof(sample));
                        dest += 2;
                        chunk_pos += 2;
                        filled += 2;
                        len -= 2;
                    }

                    if (chunk_pos >= chunk.data.size())
                    {
                        _chunks.pop_front();
                        _cv.notify_one();
                    }
                    else
                    {
                        // Save leftover
                        _leftover.assign(chunk.data.begin() + chunk_pos, chunk.data.end());
                        _leftover_pos = 0;
                        _chunks.pop_front();
                        _cv.notify_one();
                    }
                }

                return filled;
            }

            void set_volume(int vol)
            {
                _volume = (vol < 0) ? 0 : (vol > 100) ? 100
                                                      : vol;
            }

            int get_volume() const { return _volume; }

            void clear()
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _chunks.clear();
                _leftover.clear();
                _leftover_pos = 0;
            }

            void finish()
            {
                _finished = true;
                _cv.notify_all();
            }

            bool is_finished() const { return _finished; }

            void reset()
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _chunks.clear();
                _leftover.clear();
                _leftover_pos = 0;
                _finished = false;
                _seek_requested = false;
                _seek_time = -1.0;
            }

            /**
             * @brief Request a seek to a specific time
             */
            void request_seek(double time_seconds)
            {
                _seek_time = time_seconds;
                _seek_requested = true;
                _cv.notify_all();
            }

            /**
             * @brief Get and clear seek request
             * @return The requested seek time, or -1 if no seek pending
             */
            double get_and_clear_seek_request()
            {
                if (!_seek_requested)
                    return -1.0;

                std::lock_guard<std::mutex> lock(_mutex);
                double t = _seek_time.load();
                _seek_requested = false;
                _seek_time = -1.0;
                return t;
            }

            /**
             * @brief Clear buffer after seeking
             */
            void complete_seek()
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _chunks.clear();
                _leftover.clear();
                _leftover_pos = 0;
                _seek_requested = false;
            }

            bool has_seek_request() const { return _seek_requested; }
        };

        /**
         * @brief Audio-Video player with synchronized audio playback
         *
         * Uses SDL2 or PortAudio for audio output with FFmpeg for decoding.
         * Falls back to silent video playback if audio backend is unavailable.
         */
        class AudioVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            double _fps;
            double _start_time;
            double _end_time;
            Render _render_mode;
            std::atomic<bool> _running{false};
            std::atomic<bool> _audio_initialized{false};

            std::thread _video_thread;
            std::thread _audio_thread;

            AudioBuffer _audio_buffer;

            // Audio parameters (detected from file)
            int _sample_rate = 44100;
            int _channels = 2;

#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
            SDL_AudioDeviceID _audio_device = 0;

            static void sdl_audio_callback(void *userdata, Uint8 *stream, int len)
            {
                auto *self = static_cast<AudioVideoPlayer *>(userdata);

                // Fill as much as we can from the buffer (non-blocking)
                size_t filled = self->_audio_buffer.fill_buffer(stream, static_cast<size_t>(len));

                // Zero out any remaining space
                if (filled < static_cast<size_t>(len))
                {
                    std::memset(stream + filled, 0, len - filled);
                }
            }
#endif

#ifdef PYTHONIC_ENABLE_PORTAUDIO
            PaStream *_pa_stream = nullptr;

            static int pa_audio_callback(const void *input, void *output,
                                         unsigned long frameCount,
                                         const PaStreamCallbackTimeInfo *timeInfo,
                                         PaStreamCallbackFlags statusFlags,
                                         void *userData)
            {
                (void)input;
                (void)timeInfo;
                (void)statusFlags;
                auto *self = static_cast<AudioVideoPlayer *>(userData);
                auto *out = static_cast<uint8_t *>(output);

                size_t bytes_needed = frameCount * self->_channels * sizeof(int16_t);

                // Fill as much as we can from the buffer (non-blocking)
                size_t filled = self->_audio_buffer.fill_buffer(out, bytes_needed);

                // Zero out any remaining space
                if (filled < bytes_needed)
                {
                    std::memset(out + filled, 0, bytes_needed - filled);
                }

                return self->_running ? paContinue : paComplete;
            }
#endif

        public:
            AudioVideoPlayer(const std::string &filename, int width = 80,
                             Render render_mode = Mode::bw_dot, double target_fps = 0,
                             double start_time = -1.0, double end_time = -1.0)
                : _filename(filename), _width(width), _fps(target_fps),
                  _start_time(start_time), _end_time(end_time), _render_mode(render_mode)
            {
                enable_ansi_support();
            }

            ~AudioVideoPlayer()
            {
                stop();
                cleanup_audio();
                // Ensure terminal state is fully restored
                signal_handler::end_playback();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
                setvbuf(stdout, nullptr, _IOLBF, 0);
            }

            bool play()
            {
                if (_running.exchange(true))
                    return false;

                // Detect audio parameters
                detect_audio_params();

                // Try to initialize audio
                bool audio_ok = init_audio();
                if (!audio_ok)
                {
                    std::cerr << "Warning: Audio not available, playing video only.\n";
                    std::cerr << "To enable audio, rebuild with -DPYTHONIC_ENABLE_SDL2_AUDIO=ON "
                              << "or -DPYTHONIC_ENABLE_PORTAUDIO=ON\n";
                }

                // Start audio decode thread if audio is available
                if (audio_ok)
                {
                    _audio_thread = std::thread(&AudioVideoPlayer::audio_decode_thread, this);
                }

                // Run video in main thread context
                video_playback();

                // Wait for audio to finish
                _audio_buffer.finish();
                if (_audio_thread.joinable())
                    _audio_thread.join();

                _running = false;
                return true;
            }

            void stop()
            {
                _running = false;
                _audio_buffer.finish();

                if (_video_thread.joinable())
                    _video_thread.join();
                if (_audio_thread.joinable())
                    _audio_thread.join();
            }

            std::tuple<int, int, double, double> get_info() const
            {
                VideoPlayer vp(_filename);
                return vp.get_info();
            }

        private:
            void detect_audio_params()
            {
                std::string cmd = "ffprobe -v quiet -select_streams a:0 "
                                  "-show_entries stream=sample_rate,channels "
                                  "-of csv=p=0 \"" +
                                  _filename + "\" 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                    return;

                char buffer[128];
                std::string result;
                while (fgets(buffer, sizeof(buffer), pipe))
                    result += buffer;
                pythonic::accel::video::close_decode_pipe(pipe);

                // Parse: sample_rate,channels
                size_t comma = result.find(',');
                if (comma != std::string::npos)
                {
                    try
                    {
                        _sample_rate = std::stoi(result.substr(0, comma));
                        _channels = std::stoi(result.substr(comma + 1));
                    }
                    catch (...)
                    {
                    }
                }

                // Clamp to reasonable values
                if (_sample_rate < 8000)
                    _sample_rate = 44100;
                if (_channels < 1 || _channels > 8)
                    _channels = 2;
            }

            bool init_audio()
            {
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
                if (SDL_Init(SDL_INIT_AUDIO) < 0)
                {
                    std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
                    return false;
                }

                SDL_AudioSpec want, have;
                SDL_zero(want);
                want.freq = _sample_rate;
                want.format = AUDIO_S16SYS;
                want.channels = _channels;
                want.samples = 4096;
                want.callback = sdl_audio_callback;
                want.userdata = this;

                _audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
                if (_audio_device == 0)
                {
                    std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << "\n";
                    SDL_Quit();
                    return false;
                }

                SDL_PauseAudioDevice(_audio_device, 0); // Start playback
                _audio_initialized = true;
                return true;

#elif defined(PYTHONIC_ENABLE_PORTAUDIO)
                PaError err = Pa_Initialize();
                if (err != paNoError)
                {
                    std::cerr << "PortAudio init failed: " << Pa_GetErrorText(err) << "\n";
                    return false;
                }

                err = Pa_OpenDefaultStream(&_pa_stream, 0, _channels, paInt16,
                                           _sample_rate, 1024,
                                           pa_audio_callback, this);
                if (err != paNoError)
                {
                    std::cerr << "PortAudio stream open failed: " << Pa_GetErrorText(err) << "\n";
                    Pa_Terminate();
                    return false;
                }

                err = Pa_StartStream(_pa_stream);
                if (err != paNoError)
                {
                    std::cerr << "PortAudio stream start failed: " << Pa_GetErrorText(err) << "\n";
                    Pa_CloseStream(_pa_stream);
                    Pa_Terminate();
                    return false;
                }

                _audio_initialized = true;
                return true;
#else
                return false;
#endif
            }

            void cleanup_audio()
            {
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
                if (_audio_device != 0)
                {
                    SDL_CloseAudioDevice(_audio_device);
                    _audio_device = 0;
                }
                if (_audio_initialized)
                {
                    SDL_Quit();
                    _audio_initialized = false;
                }
#elif defined(PYTHONIC_ENABLE_PORTAUDIO)
                if (_pa_stream)
                {
                    Pa_StopStream(_pa_stream);
                    Pa_CloseStream(_pa_stream);
                    _pa_stream = nullptr;
                }
                if (_audio_initialized)
                {
                    Pa_Terminate();
                    _audio_initialized = false;
                }
#endif
            }

            void audio_decode_thread()
            {
                // Build seek / duration options from members
                std::string time_opts_local;
                std::string duration_opt_local;
                if (_start_time >= 0)
                {
                    time_opts_local = "-ss " + std::to_string(_start_time) + " ";
                    if (_end_time > _start_time)
                        duration_opt_local = " -t " + std::to_string(_end_time - _start_time);
                }
                // FFmpeg command to decode audio to raw PCM
                std::string cmd = "ffmpeg " + time_opts_local + "-i \"" + _filename + "\"" + duration_opt_local +
                                  " -f s16le -acodec pcm_s16le -ar " +
                                  std::to_string(_sample_rate) +
                                  " -ac " + std::to_string(_channels) + " -v quiet - 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                {
                    std::cerr << "Failed to start FFmpeg audio decoder\n";
                    return;
                }

                const size_t chunk_size = 4096;
                std::vector<uint8_t> buffer(chunk_size);

                while (_running)
                {
                    size_t bytes_read = fread(buffer.data(), 1, chunk_size, pipe);
                    if (bytes_read == 0)
                        break;

                    std::vector<uint8_t> chunk(buffer.begin(), buffer.begin() + bytes_read);
                    _audio_buffer.push(std::move(chunk));
                }

                pythonic::accel::video::close_decode_pipe(pipe);
            }

            void video_playback()
            {
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info.\n";
                    return;
                }

                // Determine pixel dimensions based on mode
                int pixel_w, pixel_h;
                bool needs_rgb = false; // Whether we need RGB data (vs grayscale)

                switch (_render_mode)
                {
                case Mode::colored:
                    // ColorCanvas: 1 char = 1 pixel width, 2 pixels height (half-blocks)
                    pixel_w = _width;
                    pixel_h = (int)(pixel_w * vid_h / vid_w);
                    pixel_h = ((pixel_h + 1) / 2) * 2;
                    needs_rgb = true;
                    break;
                case Mode::colored_dot:
                case Mode::flood_dot_colored:
                case Mode::colored_dithered:
                    // ColoredBrailleCanvas: 1 char = 2 pixels width, 4 pixels height
                    pixel_w = _width * 2;
                    pixel_h = (int)(pixel_w * vid_h / vid_w);
                    pixel_h = (pixel_h + 3) / 4 * 4;
                    needs_rgb = true;
                    break;
                case Mode::bw:
                    // BWBlockCanvas: 1 char = 1 pixel width, 2 pixels height (half-blocks with grayscale)
                    pixel_w = _width;
                    pixel_h = (int)(pixel_w * vid_h / vid_w);
                    pixel_h = ((pixel_h + 1) / 2) * 2;
                    needs_rgb = true; // Need RGB to compute grayscale ourselves
                    break;
                default:
                    // All braille modes: BrailleCanvas - 1 char = 2 pixels width, 4 pixels height
                    pixel_w = _width * 2;
                    pixel_h = (int)(pixel_w * vid_h / vid_w);
                    pixel_h = (pixel_h + 3) / 4 * 4;
                    needs_rgb = (_render_mode == Mode::grayscale_dot || _render_mode == Mode::flood_dot);
                    break;
                }

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                std::string pix_fmt = needs_rgb ? "rgb24" : "gray";
                FILE *pipe = accel::video::open_decode_pipe(
                    _filename, pix_fmt, pixel_w, pixel_h, 0.0, _start_time, _end_time);
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg video decoder.\n";
                    return;
                }

                size_t bytes_per_pixel = needs_rgb ? 3 : 1;
                size_t frame_size = pixel_w * pixel_h * bytes_per_pixel;
                FrameReadAhead reader(pipe, frame_size);
                reader.start();

                // Create appropriate canvas based on mode
                BrailleCanvas braille_canvas;
                BWBlockCanvas bw_block_canvas;
                ColorCanvas color_canvas;
                ColoredBrailleCanvas colored_braille_canvas;

                switch (_render_mode)
                {
                case Mode::colored:
                    color_canvas = ColorCanvas::from_pixels(pixel_w, pixel_h);
                    break;
                case Mode::colored_dot:
                case Mode::flood_dot_colored:
                case Mode::colored_dithered:
                    colored_braille_canvas = ColoredBrailleCanvas::from_pixels(pixel_w, pixel_h);
                    break;
                case Mode::bw:
                    bw_block_canvas = BWBlockCanvas::from_pixels(pixel_w, pixel_h);
                    break;
                default:
                    braille_canvas = BrailleCanvas::from_pixels(pixel_w, pixel_h);
                    break;
                }

                TerminalStateGuard term_guard;

                // Disable stdout buffering for smoother video
                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame

                // Pre-allocate output buffer
                std::string frame_output;
                int char_height = (_render_mode == Mode::colored || _render_mode == Mode::bw) ? (pixel_h / 2) : (pixel_h / 4);
                frame_output.reserve(_width * char_height * 50);

                while (_running && !term_guard.was_interrupted())
                {

                    const uint8_t *frame_data = reader.next_frame();
                    if (!frame_data)
                        break;

                    // Build complete frame with cursor positioning
                    frame_output = ansi::CURSOR_HOME;

                    switch (_render_mode)
                    {
                    case Mode::colored:
                        color_canvas.load_frame_rgb(frame_data, pixel_w, pixel_h);
                        frame_output += color_canvas.render();
                        break;

                    case Mode::colored_dot:
                        colored_braille_canvas.load_frame_rgb(frame_data, pixel_w, pixel_h, 128);
                        frame_output += colored_braille_canvas.render();
                        break;

                    case Mode::bw:
                        bw_block_canvas.load_frame_rgb(frame_data, pixel_w, pixel_h, 128);
                        frame_output += bw_block_canvas.render();
                        break;

                    case Mode::bw_dot:
                        braille_canvas.load_frame_fast(frame_data, pixel_w, pixel_h, 128);
                        frame_output += braille_canvas.render();
                        break;

                    case Mode::bw_dithered:
                        braille_canvas.load_frame_ordered_dithered(frame_data, pixel_w, pixel_h);
                        frame_output += braille_canvas.render();
                        break;

                    case Mode::grayscale_dot:
                        // Load with grayscale values preserved for coloring
                        {
                            size_t char_w = (pixel_w + 1) / 2;
                            size_t char_h = (pixel_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < pixel_w && y < pixel_h)
                                            {
                                                size_t idx = (y * pixel_w + x) * 3;
                                                uint8_t r = frame_data[idx];
                                                uint8_t g = frame_data[idx + 1];
                                                uint8_t b = frame_data[idx + 2];
                                                grays[row * 2 + col] = accel::pixel::to_gray(r, g, b);
                                            }
                                        }
                                    }
                                    braille_canvas.set_block_gray_dithered_with_brightness(cx, cy, grays);
                                }
                            }
                        }
                        frame_output += braille_canvas.render_grayscale();
                        break;

                    case Mode::flood_dot:
                        // All dots on, colored by average brightness
                        {
                            size_t char_w = (pixel_w + 1) / 2;
                            size_t char_h = (pixel_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < pixel_w && y < pixel_h)
                                            {
                                                size_t idx = (y * pixel_w + x) * 3;
                                                uint8_t r = frame_data[idx];
                                                uint8_t g = frame_data[idx + 1];
                                                uint8_t b = frame_data[idx + 2];
                                                grays[row * 2 + col] = accel::pixel::to_gray(r, g, b);
                                            }
                                        }
                                    }
                                    braille_canvas.set_block_flood_fill(cx, cy, grays);
                                }
                            }
                        }
                        frame_output += braille_canvas.render_grayscale();
                        break;

                    case Mode::flood_dot_colored:
                        // All dots on, colored by average RGB
                        {
                            size_t char_w = (pixel_w + 1) / 2;
                            size_t char_h = (pixel_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    int sum_r = 0, sum_g = 0, sum_b = 0;
                                    int count = 0;
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < pixel_w && y < pixel_h)
                                            {
                                                size_t idx = (y * pixel_w + x) * 3;
                                                sum_r += frame_data[idx];
                                                sum_g += frame_data[idx + 1];
                                                sum_b += frame_data[idx + 2];
                                                count++;
                                            }
                                        }
                                    }
                                    if (count > 0)
                                    {
                                        colored_braille_canvas.set_pattern(cx, cy, 0xFF);
                                        colored_braille_canvas.set_color(cx, cy, sum_r / count, sum_g / count, sum_b / count);
                                    }
                                }
                            }
                        }
                        frame_output += colored_braille_canvas.render();
                        break;

                    case Mode::colored_dithered:
                        // Dithered dots, colored by average RGB
                        {
                            static const int bayer2x2[2][2] = {{0, 2}, {3, 1}};
                            static const int dot_map[4][2] = {{0, 3}, {1, 4}, {2, 5}, {6, 7}};
                            size_t char_w = (pixel_w + 1) / 2;
                            size_t char_h = (pixel_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t pattern = 0;
                                    int sum_r = 0, sum_g = 0, sum_b = 0;
                                    int count = 0;
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < pixel_w && y < pixel_h)
                                            {
                                                size_t idx = (y * pixel_w + x) * 3;
                                                uint8_t r = frame_data[idx];
                                                uint8_t g = frame_data[idx + 1];
                                                uint8_t b = frame_data[idx + 2];
                                                sum_r += r;
                                                sum_g += g;
                                                sum_b += b;
                                                count++;

                                                uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
                                                int bayer_x = col & 1;
                                                int bayer_y = row & 1;
                                                int threshold_val = ((bayer2x2[bayer_y][bayer_x] + 1) * 255) / 5;
                                                if (gray > threshold_val)
                                                {
                                                    pattern |= (1 << dot_map[row][col]);
                                                }
                                            }
                                        }
                                    }
                                    if (count > 0)
                                    {
                                        colored_braille_canvas.set_pattern(cx, cy, pattern);
                                        colored_braille_canvas.set_color(cx, cy, sum_r / count, sum_g / count, sum_b / count);
                                    }
                                }
                            }
                        }
                        frame_output += colored_braille_canvas.render();
                        break;
                    }

                    // Single write for entire frame
                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                reader.stop();
                pythonic::accel::video::close_decode_pipe(pipe);
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback finished: " << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";
            }
        };

        /**
         * @brief Threaded Audio-Video player with non-blocking controls
         *
         * Features:
         * - Non-blocking keyboard input (doesn't halt video playback)
         * - Volume control via up/down arrows
         * - Seeking via left/right arrows with frame buffer
         * - Pause/Stop with 'p' and 's' keys
         * - Thread-safe buffered decoding and playback
         */
        class ThreadedAudioVideoPlayer
        {
        private:
            std::string _filename;
            int _width;
            double _fps;
            double _start_time;
            double _end_time;
            Render _render_mode;
            RenderConfig _config;

            std::atomic<bool> _running{false};
            std::atomic<bool> _paused{false};
            std::atomic<bool> _audio_initialized{false};

            std::thread _decode_thread;
            std::thread _audio_decode_thread;

            VideoFrameBuffer _video_buffer;
            SyncedAudioBuffer _audio_buffer;
            CommandQueue _cmd_queue;

            // Video info
            int _vid_width = 0;
            int _vid_height = 0;
            double _vid_fps = 30.0;
            double _duration = 0.0;
            int64_t _total_frames = 0;

            // Audio parameters
            int _sample_rate = 44100;
            int _channels = 2;

#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
            SDL_AudioDeviceID _audio_device = 0;

            static void sdl_audio_callback(void *userdata, Uint8 *stream, int len)
            {
                auto *self = static_cast<ThreadedAudioVideoPlayer *>(userdata);

                // If not running or paused, output silence
                if (!self->_running || self->_paused)
                {
                    std::memset(stream, 0, len);
                    return;
                }

                size_t filled = self->_audio_buffer.fill_buffer(stream, static_cast<size_t>(len));
                if (filled < static_cast<size_t>(len))
                {
                    std::memset(stream + filled, 0, len - filled);
                }
            }
#endif

#ifdef PYTHONIC_ENABLE_PORTAUDIO
            PaStream *_pa_stream = nullptr;

            static int pa_audio_callback(const void *input, void *output,
                                         unsigned long frameCount,
                                         const PaStreamCallbackTimeInfo *timeInfo,
                                         PaStreamCallbackFlags statusFlags,
                                         void *userData)
            {
                (void)input;
                (void)timeInfo;
                (void)statusFlags;
                auto *self = static_cast<ThreadedAudioVideoPlayer *>(userData);
                auto *out = static_cast<uint8_t *>(output);

                // If not running or paused, output silence
                if (!self->_running || self->_paused)
                {
                    size_t bytes = frameCount * self->_channels * sizeof(int16_t);
                    std::memset(out, 0, bytes);
                    return self->_running ? paContinue : paComplete;
                }

                size_t bytes_needed = frameCount * self->_channels * sizeof(int16_t);
                size_t filled = self->_audio_buffer.fill_buffer(out, bytes_needed);
                if (filled < bytes_needed)
                {
                    std::memset(out + filled, 0, bytes_needed - filled);
                }

                return self->_running ? paContinue : paComplete;
            }
#endif

        public:
            ThreadedAudioVideoPlayer(const std::string &filename, const RenderConfig &config)
                : _filename(filename), _width(config.max_width), _fps(config.fps),
                  _start_time(config.start_time), _end_time(config.end_time),
                  _render_mode(config.mode), _config(config),
                  _video_buffer(config.buffer_ahead_frames, config.buffer_behind_frames)
            {
                _audio_buffer.set_volume(config.volume);
                enable_ansi_support();
            }

            ~ThreadedAudioVideoPlayer()
            {
                stop();
                cleanup_audio();
                // Ensure terminal state is fully restored
                signal_handler::end_playback();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
                setvbuf(stdout, nullptr, _IOLBF, 0);
            }

            bool play()
            {
                if (_running.exchange(true))
                    return false;

                // Get video info first
                if (!detect_video_info())
                {
                    std::cerr << "Error: Could not read video info.\n";
                    _running = false;
                    return false;
                }

                // Initialize audio only if requested
                bool audio_ok = false;
                if (_config.audio == Audio::on)
                {
                    // Detect audio params
                    detect_audio_params();

                    // Initialize audio
                    audio_ok = init_audio();
                    if (!audio_ok)
                    {
                        std::cerr << "Warning: Audio not available, playing video only.\n";
                    }
                }

                // Set initial frame offset based on user's start_time
                double target_fps = (_fps > 0) ? _fps : _vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;
                double initial_start = (_start_time >= 0) ? _start_time : 0.0;
                int64_t initial_frame_offset = static_cast<int64_t>(initial_start * target_fps);
                _video_buffer.set_initial_offset(initial_frame_offset);

                // Start decode threads
                _decode_thread = std::thread(&ThreadedAudioVideoPlayer::video_decode_thread, this);

                if (audio_ok)
                {
                    _audio_decode_thread = std::thread(&ThreadedAudioVideoPlayer::audio_decode_thread, this);
                }

                // Create keyboard manager for non-blocking input
                KeyboardManager keyboard(_cmd_queue, _config.pause_key, _config.stop_key,
                                         _config.vol_up_key, _config.vol_down_key,
                                         _config.seek_backward_key, _config.seek_forward_key);

                if (_config.shell == Shell::interactive)
                {
                    keyboard.start();
                }

                // Run rendering in main thread
                render_loop();

                // Signal all threads to stop
                _running = false;

                // Cleanup
                keyboard.stop();

                _video_buffer.finish();
                _audio_buffer.finish();

                if (_decode_thread.joinable())
                    _decode_thread.join();
                if (_audio_decode_thread.joinable())
                    _audio_decode_thread.join();

                return true;
            }

            void stop()
            {
                _running = false;
                _paused = false;
                _video_buffer.finish();
                _audio_buffer.finish();

                if (_decode_thread.joinable())
                    _decode_thread.join();
                if (_audio_decode_thread.joinable())
                    _audio_decode_thread.join();
            }

        private:
            bool detect_video_info()
            {
                auto info = pythonic::accel::video::probe(_filename);
                _vid_width = info.width;
                _vid_height = info.height;
                _vid_fps = info.fps;
                _duration = info.duration;

                // Calculate total frames from duration and fps
                if (_duration > 0 && _vid_fps > 0)
                    _total_frames = static_cast<int64_t>(_duration * _vid_fps);

                return _vid_width > 0 && _vid_height > 0;
            }

            void detect_audio_params()
            {
                std::string cmd = "ffprobe -v quiet -select_streams a:0 "
                                  "-show_entries stream=sample_rate,channels "
                                  "-of csv=p=0 \"" +
                                  _filename + "\" 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                    return;

                char buffer[128];
                std::string result;
                while (fgets(buffer, sizeof(buffer), pipe))
                    result += buffer;
                pythonic::accel::video::close_decode_pipe(pipe);

                size_t comma = result.find(',');
                if (comma != std::string::npos)
                {
                    try
                    {
                        _sample_rate = std::stoi(result.substr(0, comma));
                        _channels = std::stoi(result.substr(comma + 1));
                    }
                    catch (...)
                    {
                    }
                }

                if (_sample_rate < 8000)
                    _sample_rate = 44100;
                if (_channels < 1 || _channels > 8)
                    _channels = 2;
            }

            bool init_audio()
            {
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
                if (SDL_Init(SDL_INIT_AUDIO) < 0)
                {
                    return false;
                }

                SDL_AudioSpec want, have;
                SDL_zero(want);
                want.freq = _sample_rate;
                want.format = AUDIO_S16SYS;
                want.channels = _channels;
                want.samples = 4096;
                want.callback = sdl_audio_callback;
                want.userdata = this;

                _audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
                if (_audio_device == 0)
                {
                    SDL_Quit();
                    return false;
                }

                SDL_PauseAudioDevice(_audio_device, 0);
                _audio_initialized = true;
                return true;

#elif defined(PYTHONIC_ENABLE_PORTAUDIO)
                PaError err = Pa_Initialize();
                if (err != paNoError)
                    return false;

                err = Pa_OpenDefaultStream(&_pa_stream, 0, _channels, paInt16,
                                           _sample_rate, 1024,
                                           pa_audio_callback, this);
                if (err != paNoError)
                {
                    Pa_Terminate();
                    return false;
                }

                err = Pa_StartStream(_pa_stream);
                if (err != paNoError)
                {
                    Pa_CloseStream(_pa_stream);
                    Pa_Terminate();
                    return false;
                }

                _audio_initialized = true;
                return true;
#else
                return false;
#endif
            }

            void cleanup_audio()
            {
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
                if (_audio_device != 0)
                {
                    SDL_CloseAudioDevice(_audio_device);
                    _audio_device = 0;
                }
                if (_audio_initialized)
                {
                    SDL_Quit();
                    _audio_initialized = false;
                }
#elif defined(PYTHONIC_ENABLE_PORTAUDIO)
                if (_pa_stream)
                {
                    Pa_StopStream(_pa_stream);
                    Pa_CloseStream(_pa_stream);
                    _pa_stream = nullptr;
                }
                if (_audio_initialized)
                {
                    Pa_Terminate();
                    _audio_initialized = false;
                }
#endif
            }

            void video_decode_thread()
            {
                // Calculate pixel dimensions based on mode
                int pixel_w, pixel_h;
                bool needs_rgb = false;

                switch (_render_mode)
                {
                case Mode::colored:
                    pixel_w = _width;
                    pixel_h = (int)((double)pixel_w * _vid_height / _vid_width);
                    pixel_h = ((pixel_h + 1) / 2) * 2;
                    needs_rgb = true;
                    break;
                case Mode::colored_dot:
                case Mode::flood_dot_colored:
                case Mode::colored_dithered:
                    pixel_w = _width * 2;
                    pixel_h = (int)((double)pixel_w * _vid_height / _vid_width);
                    pixel_h = (pixel_h + 3) / 4 * 4;
                    needs_rgb = true;
                    break;
                case Mode::bw:
                    pixel_w = _width;
                    pixel_h = (int)((double)pixel_w * _vid_height / _vid_width);
                    pixel_h = ((pixel_h + 1) / 2) * 2;
                    needs_rgb = true;
                    break;
                default:
                    pixel_w = _width * 2;
                    pixel_h = (int)((double)pixel_w * _vid_height / _vid_width);
                    pixel_h = (pixel_h + 3) / 4 * 4;
                    needs_rgb = (_render_mode == Mode::grayscale_dot || _render_mode == Mode::flood_dot);
                    break;
                }

                double target_fps = (_fps > 0) ? _fps : _vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                std::string pix_fmt = needs_rgb ? "rgb24" : "gray";
                size_t bytes_per_pixel = needs_rgb ? 3 : 1;
                size_t frame_size = pixel_w * pixel_h * bytes_per_pixel;
                std::vector<uint8_t> buffer(frame_size);
                double frame_time = 1.0 / target_fps;

                double current_start_time = (_start_time >= 0) ? _start_time : 0.0;
                FILE *pipe = nullptr;

                auto start_ffmpeg = [&](double seek_time) -> bool
                {
                    if (pipe)
                    {
                        pythonic::accel::video::close_decode_pipe(pipe);
                        pipe = nullptr;
                    }

                    pipe = accel::video::open_decode_pipe(
                        _filename, pix_fmt, pixel_w, pixel_h, 0.0, seek_time, _end_time);
                    return pipe != nullptr;
                };

                // Start initial decoding
                if (!start_ffmpeg(current_start_time))
                {
                    return;
                }

                int64_t frame_num = 0;
                int64_t frame_offset = static_cast<int64_t>(current_start_time * target_fps);

                while (_running)
                {
                    // Check for seek request
                    double seek_time = _video_buffer.get_and_clear_seek_request();
                    if (seek_time >= 0)
                    {
                        // Restart FFmpeg at new position
                        current_start_time = seek_time;
                        frame_offset = static_cast<int64_t>(seek_time * target_fps);
                        frame_num = 0;

                        if (!start_ffmpeg(seek_time))
                            break;

                        // Signal buffer that seek is complete
                        _video_buffer.complete_seek(frame_offset, target_fps);
                        continue;
                    }

                    size_t bytes_read = fread(buffer.data(), 1, frame_size, pipe);
                    if (bytes_read < frame_size)
                        break;

                    VideoFrame frame;
                    frame.data = buffer;
                    frame.frame_number = frame_num; // Will be offset in push()
                    frame.timestamp = (frame_offset + frame_num) * frame_time;

                    _video_buffer.push(std::move(frame));
                    ++frame_num;
                }

                if (pipe)
                    pythonic::accel::video::close_decode_pipe(pipe);
                _video_buffer.finish();
            }

            void audio_decode_thread()
            {
                const size_t chunk_size = 4096;
                std::vector<uint8_t> buffer(chunk_size);
                double samples_per_sec = _sample_rate * _channels * 2; // 16-bit = 2 bytes

                double current_start_time = (_start_time >= 0) ? _start_time : 0.0;
                FILE *pipe = nullptr;

                auto start_ffmpeg = [&](double seek_time) -> bool
                {
                    if (pipe)
                    {
                        pythonic::accel::video::close_decode_pipe(pipe);
                        pipe = nullptr;
                    }

                    std::string time_opts = "-ss " + std::to_string(seek_time) + " ";
                    std::string duration_opt;
                    if (_end_time >= 0)
                    {
                        double dur = _end_time - seek_time;
                        if (dur > 0)
                            duration_opt = " -t " + std::to_string(dur);
                    }

                    std::string cmd = "ffmpeg " + time_opts + "-i \"" + _filename + "\"" + duration_opt +
                                      " -f s16le -acodec pcm_s16le -ar " +
                                      std::to_string(_sample_rate) +
                                      " -ac " + std::to_string(_channels) + " -v quiet - 2>/dev/null";

                    pipe = popen(cmd.c_str(), "r");
                    return pipe != nullptr;
                };

                // Start initial decoding
                if (!start_ffmpeg(current_start_time))
                {
                    return;
                }

                double timestamp = current_start_time;

                while (_running)
                {
                    // Check for seek request
                    double seek_time = _audio_buffer.get_and_clear_seek_request();
                    if (seek_time >= 0)
                    {
                        current_start_time = seek_time;
                        timestamp = seek_time;

                        if (!start_ffmpeg(seek_time))
                            break;

                        _audio_buffer.complete_seek();
                        continue;
                    }

                    size_t bytes_read = fread(buffer.data(), 1, chunk_size, pipe);
                    if (bytes_read == 0)
                        break;

                    AudioChunk chunk;
                    chunk.data.assign(buffer.begin(), buffer.begin() + bytes_read);
                    chunk.timestamp = timestamp;
                    timestamp += bytes_read / samples_per_sec;

                    _audio_buffer.push(std::move(chunk));
                }

                if (pipe)
                    pythonic::accel::video::close_decode_pipe(pipe);
                _audio_buffer.finish();
            }

            void render_loop()
            {
                // Calculate pixel dimensions
                int pixel_w, pixel_h;
                bool needs_rgb = false;

                switch (_render_mode)
                {
                case Mode::colored:
                    pixel_w = _width;
                    pixel_h = (int)((double)pixel_w * _vid_height / _vid_width);
                    pixel_h = ((pixel_h + 1) / 2) * 2;
                    needs_rgb = true;
                    break;
                case Mode::colored_dot:
                case Mode::flood_dot_colored:
                case Mode::colored_dithered:
                    pixel_w = _width * 2;
                    pixel_h = (int)((double)pixel_w * _vid_height / _vid_width);
                    pixel_h = (pixel_h + 3) / 4 * 4;
                    needs_rgb = true;
                    break;
                case Mode::bw:
                    pixel_w = _width;
                    pixel_h = (int)((double)pixel_w * _vid_height / _vid_width);
                    pixel_h = ((pixel_h + 1) / 2) * 2;
                    needs_rgb = true;
                    break;
                default:
                    pixel_w = _width * 2;
                    pixel_h = (int)((double)pixel_w * _vid_height / _vid_width);
                    pixel_h = (pixel_h + 3) / 4 * 4;
                    needs_rgb = (_render_mode == Mode::grayscale_dot || _render_mode == Mode::flood_dot);
                    break;
                }

                double target_fps = (_fps > 0) ? _fps : _vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                // Create canvases
                BrailleCanvas braille_canvas;
                BWBlockCanvas bw_block_canvas;
                ColorCanvas color_canvas;
                ColoredBrailleCanvas colored_braille_canvas;

                switch (_render_mode)
                {
                case Mode::colored:
                    color_canvas = ColorCanvas::from_pixels(pixel_w, pixel_h);
                    break;
                case Mode::colored_dot:
                case Mode::flood_dot_colored:
                case Mode::colored_dithered:
                    colored_braille_canvas = ColoredBrailleCanvas::from_pixels(pixel_w, pixel_h);
                    break;
                case Mode::bw:
                    bw_block_canvas = BWBlockCanvas::from_pixels(pixel_w, pixel_h);
                    break;
                default:
                    braille_canvas = BrailleCanvas::from_pixels(pixel_w, pixel_h);
                    break;
                }

                TerminalStateGuard term_guard;

                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_count = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::microseconds total_pause_time{0};
                std::chrono::steady_clock::time_point pause_start;
                bool user_stopped = false;

                std::string frame_output;
                frame_output.reserve(pixel_w * pixel_h * 50);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    // Process commands from keyboard thread
                    PlayerCommand cmd;
                    while (_cmd_queue.try_pop(cmd))
                    {
                        switch (cmd)
                        {
                        case PlayerCommand::Stop:
                            user_stopped = true;
                            break;
                        case PlayerCommand::Pause:
                            _paused = !_paused;
                            if (_paused)
                            {
                                pause_start = std::chrono::steady_clock::now();
                                std::cout << ansi::CURSOR_HOME
                                          << "[PAUSED - Press '" << _config.pause_key
                                          << "' to resume, Vol: " << _audio_buffer.get_volume() << "%]"
                                          << std::flush;
                            }
                            else
                            {
                                total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                    std::chrono::steady_clock::now() - pause_start);
                                // Reset absolute deadline after pause to prevent catch-up burst
                                next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                            }
                            break;
                        case PlayerCommand::VolumeUp:
                            _audio_buffer.set_volume(_audio_buffer.get_volume() + _config.volume_step);
                            if (_paused)
                            {
                                std::cout << ansi::CURSOR_HOME
                                          << "[PAUSED - Vol: " << _audio_buffer.get_volume() << "%]    "
                                          << std::flush;
                            }
                            break;
                        case PlayerCommand::VolumeDown:
                            _audio_buffer.set_volume(_audio_buffer.get_volume() - _config.volume_step);
                            if (_paused)
                            {
                                std::cout << ansi::CURSOR_HOME
                                          << "[PAUSED - Vol: " << _audio_buffer.get_volume() << "%]    "
                                          << std::flush;
                            }
                            break;
                        case PlayerCommand::SeekBackward:
                        {
                            // Calculate new seek time based on current position
                            double target_fps = (_fps > 0) ? _fps : _vid_fps;
                            if (target_fps <= 0)
                                target_fps = 30;

                            int64_t current_frame = _video_buffer.get_current_frame();
                            double current_time = current_frame / target_fps;
                            double seek_amount = _config.seek_frames / target_fps;

                            // Clamp to user's start_time, not absolute 0
                            double min_time = (_start_time >= 0) ? _start_time : 0.0;
                            double seek_time = std::max(min_time, current_time - seek_amount);

                            // Request seek on both audio and video
                            _video_buffer.request_seek(seek_time);
                            if (_config.audio == Audio::on)
                                _audio_buffer.request_seek(seek_time);
                            break;
                        }
                        case PlayerCommand::SeekForward:
                        {
                            // Calculate new seek time
                            double target_fps = (_fps > 0) ? _fps : _vid_fps;
                            if (target_fps <= 0)
                                target_fps = 30;

                            int64_t current_frame = _video_buffer.get_current_frame();
                            double current_time = current_frame / target_fps;
                            double seek_time = current_time + (_config.seek_frames / target_fps);

                            // Clamp to end_time if set, otherwise duration
                            double max_time = (_end_time >= 0) ? _end_time : (_duration > 0) ? _duration
                                                                                             : seek_time;
                            if (seek_time > max_time - 1.0)
                                seek_time = std::max((_start_time >= 0 ? _start_time : 0.0), max_time - 1.0);

                            // Request seek on both audio and video
                            _video_buffer.request_seek(seek_time);
                            if (_config.audio == Audio::on)
                                _audio_buffer.request_seek(seek_time);
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    if (user_stopped)
                        break;

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    // If seeking is in progress, wait (but check for finished too)
                    if (_video_buffer.has_seek_request())
                    {
                        // Don't wait forever - check if decoder finished
                        if (_video_buffer.is_finished())
                            break;
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }

                    VideoFrame frame;
                    if (!_video_buffer.pop(frame))
                    {
                        if (_video_buffer.is_finished())
                            break;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }

                    frame_output = ansi::CURSOR_HOME;

                    switch (_render_mode)
                    {
                    case Mode::colored:
                        color_canvas.load_frame_rgb(frame.data.data(), pixel_w, pixel_h);
                        frame_output += color_canvas.render();
                        break;
                    case Mode::colored_dot:
                        colored_braille_canvas.load_frame_rgb(frame.data.data(), pixel_w, pixel_h, 128);
                        frame_output += colored_braille_canvas.render();
                        break;
                    case Mode::bw:
                        bw_block_canvas.load_frame_rgb(frame.data.data(), pixel_w, pixel_h, 128);
                        frame_output += bw_block_canvas.render();
                        break;
                    case Mode::bw_dot:
                        braille_canvas.load_frame_fast(frame.data.data(), pixel_w, pixel_h, 128);
                        frame_output += braille_canvas.render();
                        break;
                    case Mode::bw_dithered:
                        braille_canvas.load_frame_ordered_dithered(frame.data.data(), pixel_w, pixel_h);
                        frame_output += braille_canvas.render();
                        break;
                    case Mode::grayscale_dot:
                        // Load with grayscale values preserved for coloring
                        {
                            size_t char_w = (pixel_w + 1) / 2;
                            size_t char_h = (pixel_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < pixel_w && y < pixel_h)
                                            {
                                                size_t idx = (y * pixel_w + x) * 3;
                                                uint8_t r = frame.data[idx];
                                                uint8_t g = frame.data[idx + 1];
                                                uint8_t b = frame.data[idx + 2];
                                                grays[row * 2 + col] = accel::pixel::to_gray(r, g, b);
                                            }
                                        }
                                    }
                                    braille_canvas.set_block_gray_dithered_with_brightness(cx, cy, grays);
                                }
                            }
                        }
                        frame_output += braille_canvas.render_grayscale();
                        break;
                    case Mode::flood_dot:
                        // All dots on, colored by average brightness
                        {
                            size_t char_w = (pixel_w + 1) / 2;
                            size_t char_h = (pixel_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < pixel_w && y < pixel_h)
                                            {
                                                size_t idx = (y * pixel_w + x) * 3;
                                                uint8_t r = frame.data[idx];
                                                uint8_t g = frame.data[idx + 1];
                                                uint8_t b = frame.data[idx + 2];
                                                grays[row * 2 + col] = accel::pixel::to_gray(r, g, b);
                                            }
                                        }
                                    }
                                    braille_canvas.set_block_flood_fill(cx, cy, grays);
                                }
                            }
                        }
                        frame_output += braille_canvas.render_grayscale();
                        break;
                    case Mode::flood_dot_colored:
                        // All dots on, colored by average RGB
                        {
                            size_t char_w = (pixel_w + 1) / 2;
                            size_t char_h = (pixel_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    int sum_r = 0, sum_g = 0, sum_b = 0;
                                    int count = 0;
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < pixel_w && y < pixel_h)
                                            {
                                                size_t idx = (y * pixel_w + x) * 3;
                                                sum_r += frame.data[idx];
                                                sum_g += frame.data[idx + 1];
                                                sum_b += frame.data[idx + 2];
                                                count++;
                                            }
                                        }
                                    }
                                    if (count > 0)
                                    {
                                        colored_braille_canvas.set_pattern(cx, cy, 0xFF);
                                        colored_braille_canvas.set_color(cx, cy, sum_r / count, sum_g / count, sum_b / count);
                                    }
                                }
                            }
                        }
                        frame_output += colored_braille_canvas.render();
                        break;
                    case Mode::colored_dithered:
                        // Dithered dots, colored by average RGB
                        {
                            static const int bayer2x2[2][2] = {{0, 2}, {3, 1}};
                            static const int dot_map[4][2] = {{0, 3}, {1, 4}, {2, 5}, {6, 7}};
                            size_t char_w = (pixel_w + 1) / 2;
                            size_t char_h = (pixel_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t pattern = 0;
                                    int sum_r = 0, sum_g = 0, sum_b = 0;
                                    int count = 0;
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < pixel_w && y < pixel_h)
                                            {
                                                size_t idx = (y * pixel_w + x) * 3;
                                                uint8_t r = frame.data[idx];
                                                uint8_t g = frame.data[idx + 1];
                                                uint8_t b = frame.data[idx + 2];
                                                sum_r += r;
                                                sum_g += g;
                                                sum_b += b;
                                                count++;

                                                uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
                                                int bayer_x = col & 1;
                                                int bayer_y = row & 1;
                                                int threshold_val = ((bayer2x2[bayer_y][bayer_x] + 1) * 255) / 5;
                                                if (gray > threshold_val)
                                                {
                                                    pattern |= (1 << dot_map[row][col]);
                                                }
                                            }
                                        }
                                    }
                                    if (count > 0)
                                    {
                                        colored_braille_canvas.set_pattern(cx, cy, pattern);
                                        colored_braille_canvas.set_color(cx, cy, sum_r / count, sum_g / count, sum_b / count);
                                    }
                                }
                            }
                        }
                        frame_output += colored_braille_canvas.render();
                        break;
                    default:
                        braille_canvas.load_frame_fast(frame.data.data(), pixel_w, pixel_h, _config.threshold);
                        frame_output += braille_canvas.render();
                        break;
                    }

                    // Render progress bar at bottom
                    // Calculate progress percentage
                    double current_time = frame.timestamp;
                    double actual_start = _start_time > 0 ? _start_time : 0.0;
                    double actual_end = _end_time > 0 ? _end_time : (_duration > 0 ? _duration : 100.0);
                    double total_duration = actual_end - actual_start;
                    if (total_duration <= 0)
                        total_duration = 1.0; // Avoid division by zero

                    // Progress is relative to user's start/end range
                    double progress = std::max(0.0, std::min(1.0, (current_time - actual_start) / total_duration));

                    // Format times as MM:SS
                    auto format_time = [](double secs) -> std::string
                    {
                        int total = static_cast<int>(secs);
                        int mins = total / 60;
                        int s = total % 60;
                        char buf[16];
                        snprintf(buf, sizeof(buf), "%02d:%02d", mins, s);
                        return std::string(buf);
                    };

                    std::string start_str = format_time(actual_start);
                    std::string current_str = format_time(current_time);
                    std::string end_str = format_time(actual_end);

                    // Progress bar width (account for time labels and padding)
                    int bar_width = _width - 16; // Leave room for "MM:SS [ ... ] MM:SS"
                    if (bar_width < 10)
                        bar_width = 10;

                    // Build progress bar using colored braille (white for played, gray for remaining)
                    frame_output += "\n"; // New line for progress bar
                    frame_output += start_str + " ";

                    // Render progress bar with colored half-blocks
                    int filled = static_cast<int>(progress * bar_width);
                    for (int i = 0; i < bar_width; ++i)
                    {
                        if (i < filled)
                        {
                            // Played: White
                            frame_output += "\033[38;2;255;255;255m█\033[0m";
                        }
                        else
                        {
                            // Remaining: Dark gray
                            frame_output += "\033[38;2;80;80;80m█\033[0m";
                        }
                    }

                    frame_output += " " + end_str;

                    // Add current time in the middle and volume indicator
                    int vol = _audio_buffer.get_volume();
                    frame_output += "  [" + current_str + "]";

                    // Volume bar: use gradient from green (0%) to yellow (50%) to red (100%)
                    if (_config.audio == Audio::on)
                    {
                        frame_output += " Vol:";

                        // 10 segment volume bar using braille dots for smoother fill
                        // Each segment = 10% (matches 10% increment)
                        const int vol_segments = 10;
                        int vol_filled = vol / 10;  // 0-10 filled segments
                        int vol_partial = vol % 10; // Partial fill for current segment (0-9)

                        for (int i = 0; i < vol_segments; ++i)
                        {
                            // Calculate color based on position (gradient: green -> yellow -> red)
                            int r, g, b;
                            float pos = (float)i / (vol_segments - 1);
                            if (pos < 0.5f)
                            {
                                // Green to Yellow (0.0 - 0.5)
                                r = (int)(pos * 2 * 255);
                                g = 255;
                                b = 0;
                            }
                            else
                            {
                                // Yellow to Red (0.5 - 1.0)
                                r = 255;
                                g = (int)((1.0f - (pos - 0.5f) * 2) * 255);
                                b = 0;
                            }

                            if (i < vol_filled)
                            {
                                // Fully filled segment
                                char color[40];
                                snprintf(color, sizeof(color), "\033[38;2;%d;%d;%dm⣿\033[0m", r, g, b);
                                frame_output += color;
                            }
                            else if (i == vol_filled && vol_partial > 0)
                            {
                                // Partially filled using braille patterns for smooth transition
                                // ⡀⡄⡆⡇⣇⣧⣷⣿ represent 1-8 dots (12.5% each)
                                const char *partial_braille[] = {"⡀", "⡄", "⡆", "⡇", "⣇", "⣧", "⣷", "⣿"};
                                int partial_idx = (vol_partial * 7) / 9; // Map 1-9 to 0-7
                                char color[50];
                                snprintf(color, sizeof(color), "\033[38;2;%d;%d;%dm%s\033[0m",
                                         r, g, b, partial_braille[partial_idx]);
                                frame_output += color;
                            }
                            else
                            {
                                // Empty segment: dark gray braille
                                frame_output += "\033[38;2;50;50;50m⣀\033[0m";
                            }
                        }
                    }

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_count;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_count / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_count << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";
            }
        };

        /**
         * @brief Play video with audio using SDL2 or PortAudio
         * @param filename Path to video file
         * @param width Terminal width in characters
         * @param render_mode Rendering mode
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop playback (default 's', '\0' to disable)
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         *
         * Note: Audio playback currently doesn't support pause/stop - video continues
         * while audio may desync. For full pause/stop support, use non-audio playback.
         */
        inline void play_video_audio(const std::string &filename, int width = 80,
                                     Render render_mode = Mode::bw_dot,
                                     [[maybe_unused]] Shell shell = Shell::noninteractive,
                                     [[maybe_unused]] char pause_key = 'p',
                                     [[maybe_unused]] char stop_key = 's',
                                     double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            AudioVideoPlayer player(filename, width, render_mode, fps, start_time, end_time);
            player.play();
        }

#else // No audio support compiled in

        /**
         * @brief Fallback video+audio player when no audio backend is available
         * Simply plays video without audio and shows a warning
         */
        inline void play_video_audio(const std::string &filename, int width = 80,
                                     Render render_mode = Mode::bw_dot,
                                     Shell shell = Shell::noninteractive,
                                     char pause_key = 'p', char stop_key = 's',
                                     double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            std::cerr << "Warning: Audio playback not available.\n"
                      << "Rebuild with -DPYTHONIC_ENABLE_SDL2_AUDIO=ON or -DPYTHONIC_ENABLE_PORTAUDIO=ON\n"
                      << "Falling back to silent video playback...\n\n";

            // Use play_video_with_mode to properly handle all render modes
            play_video_with_mode(filename, width, render_mode, 128, shell, pause_key, stop_key, fps, start_time, end_time);
        }

#endif // Audio support

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

        // ==================== OpenCV Video Player ====================

#ifdef PYTHONIC_ENABLE_OPENCV
        /**
         * @brief Video player using OpenCV backend
         *
         * Can play video files and capture from webcam.
         * Falls back to FFmpeg if OpenCV fails for video files.
         * Supports pause/stop controls via keyboard input in interactive mode.
         */
        class OpenCVVideoPlayer
        {
        private:
            std::string _source;
            int _width;
            int _threshold;
            Mode _mode;
            double _fps;
            double _start_time;
            double _end_time;
            bool _is_webcam;
            std::atomic<bool> _running{false};
            std::atomic<bool> _paused{false};
            Shell _shell;
            char _pause_key;
            char _stop_key;

        public:
            OpenCVVideoPlayer(const std::string &source, int width = 80,
                              Mode mode = Mode::bw_dot, int threshold = 128,
                              double target_fps = 0.0, double start_time = -1.0, double end_time = -1.0)
                : _source(source), _width(width), _threshold(threshold), _mode(mode),
                  _fps(target_fps), _start_time(start_time), _end_time(end_time),
                  _is_webcam(is_webcam_source(source)), _shell(Shell::noninteractive), _pause_key('p'), _stop_key('s')
            {
                enable_ansi_support();
            }

            ~OpenCVVideoPlayer()
            {
                stop();
                std::cout << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
            }

            /**
             * @brief Play video/webcam (blocking)
             * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
             * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
             * @param stop_key Key to stop playback (default 's', '\0' to disable)
             * @return true if playback completed successfully
             */
            bool play(Shell shell = Shell::noninteractive, char pause_key = 'p', char stop_key = 's')
            {
                if (_running.exchange(true))
                    return false;

                _shell = shell;
                _pause_key = pause_key;
                _stop_key = stop_key;
                _paused = false;

                bool result = _play_internal();
                _running = false;
                return result;
            }

            void stop()
            {
                _running = false;
                _paused = false;
            }

            void toggle_pause() { _paused = !_paused; }
            bool is_paused() const { return _paused; }
            bool is_webcam() const { return _is_webcam; }

        private:
            bool _play_internal()
            {
                cv::VideoCapture cap;

                if (_is_webcam)
                {
                    int device_idx = parse_webcam_index(_source);
                    if (!cap.open(device_idx))
                    {
                        std::cerr << "Error: Cannot open webcam device " << device_idx << "\n";
                        return false;
                    }
                }
                else
                {
                    if (!cap.open(_source))
                    {
                        // OpenCV failed to open file
                        return false;
                    }
                }

                double video_fps = cap.get(cv::CAP_PROP_FPS);
                if (video_fps <= 0)
                    video_fps = 30;

                // Use user-specified FPS or video's native FPS
                double target_fps = (_fps > 0) ? _fps : video_fps;
                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                // Seek to start time if specified (not for webcam)
                double total_duration = 0;
                if (!_is_webcam && _start_time >= 0)
                {
                    double start_ms = _start_time * 1000.0;
                    cap.set(cv::CAP_PROP_POS_MSEC, start_ms);
                    total_duration = cap.get(cv::CAP_PROP_FRAME_COUNT) / video_fps;
                }

                // Calculate end time in milliseconds for checking
                double end_ms = -1;
                if (!_is_webcam && _end_time >= 0)
                {
                    end_ms = _end_time * 1000.0;
                }

                cv::Mat frame, rgb, resized, gray;

                // Calculate output dimensions
                int cap_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
                int cap_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

                double scale;
                bool uses_braille = (_mode == Mode::bw_dot || _mode == Mode::colored_dot ||
                                     _mode == Mode::bw_dithered || _mode == Mode::grayscale_dot ||
                                     _mode == Mode::flood_dot || _mode == Mode::flood_dot_colored ||
                                     _mode == Mode::colored_dithered);
                if (uses_braille)
                    scale = static_cast<double>(_width * 2) / cap_width;
                else
                    scale = static_cast<double>(_width) / cap_width;

                int out_w = static_cast<int>(cap_width * scale);
                int out_h = static_cast<int>(cap_height * scale);

                // Initialize canvases for all modes
                BrailleCanvas braille_canvas;
                BWBlockCanvas bw_canvas;
                ColorCanvas color_canvas;
                ColoredBrailleCanvas color_dot_canvas;

                switch (_mode)
                {
                case Mode::bw_dot:
                case Mode::bw_dithered:
                case Mode::grayscale_dot:
                case Mode::flood_dot:
                    braille_canvas = BrailleCanvas::from_pixels(out_w, out_h);
                    break;
                case Mode::bw:
                    bw_canvas = BWBlockCanvas::from_pixels(out_w, out_h);
                    break;
                case Mode::colored:
                    color_canvas = ColorCanvas::from_pixels(out_w, out_h);
                    break;
                case Mode::colored_dot:
                case Mode::flood_dot_colored:
                case Mode::colored_dithered:
                    color_dot_canvas = ColoredBrailleCanvas::from_pixels(out_w, out_h);
                    break;
                }

                TerminalStateGuard term_guard;

                // Only enable keyboard input in interactive mode
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                std::setvbuf(stdout, nullptr, _IOFBF, 1 << 19); // 512KB fully-buffered for atomic frame output
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                auto next_frame_deadline = start_time + frame_duration; // Absolute deadline for next frame
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

                std::string frame_output;
                frame_output.reserve(out_w * out_h * 40);

                while (_running && !term_guard.was_interrupted() && !user_stopped)
                {
                    // Check for keyboard input (pause/stop) - only in interactive mode
                    if (keyboard)
                    {
                        int key = keyboard->get_key();
                        if (key != -1)
                        {
                            if (_stop_key != '\0' && key == _stop_key)
                            {
                                user_stopped = true;
                                break;
                            }
                            if (_pause_key != '\0' && key == _pause_key)
                            {
                                _paused = !_paused;
                                if (_paused)
                                {
                                    pause_start = std::chrono::steady_clock::now();
                                    std::cout << ansi::CURSOR_HOME << "[PAUSED - Press '" << _pause_key << "' to resume]" << std::flush;
                                }
                                else
                                {
                                    total_pause_time += std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::steady_clock::now() - pause_start);
                                    // Reset absolute deadline after pause to prevent catch-up burst
                                    next_frame_deadline = std::chrono::steady_clock::now() + frame_duration;
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    // Check if we've reached end_time
                    if (end_ms >= 0)
                    {
                        double current_ms = cap.get(cv::CAP_PROP_POS_MSEC);
                        if (current_ms >= end_ms)
                            break;
                    }

                    if (!cap.read(frame))
                        break;

                    // Resize and convert
                    cv::resize(frame, resized, cv::Size(out_w, out_h), 0, 0, cv::INTER_AREA);
                    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

                    frame_output = ansi::CURSOR_HOME;

                    switch (_mode)
                    {
                    case Mode::bw_dot:
                        braille_canvas.load_frame_rgb_fast(rgb.data, out_w, out_h, _threshold);
                        frame_output += braille_canvas.render();
                        break;
                    case Mode::bw:
                        bw_canvas.load_frame_rgb(rgb.data, out_w, out_h, _threshold);
                        frame_output += bw_canvas.render();
                        break;
                    case Mode::colored:
                        color_canvas.load_frame_rgb(rgb.data, out_w, out_h);
                        frame_output += color_canvas.render();
                        break;
                    case Mode::colored_dot:
                        color_dot_canvas.load_frame_rgb(rgb.data, out_w, out_h, _threshold);
                        frame_output += color_dot_canvas.render();
                        break;
                    case Mode::bw_dithered:
                        // Convert to grayscale and use ordered dithering
                        cv::cvtColor(resized, gray, cv::COLOR_BGR2GRAY);
                        braille_canvas.load_frame_ordered_dithered(gray.data, out_w, out_h);
                        frame_output += braille_canvas.render();
                        break;
                    case Mode::grayscale_dot:
                        // Grayscale with dithering and brightness coloring
                        cv::cvtColor(resized, gray, cv::COLOR_BGR2GRAY);
                        {
                            size_t char_w = (out_w + 1) / 2;
                            size_t char_h = (out_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < out_w && y < out_h)
                                                grays[row * 2 + col] = gray.at<uint8_t>(y, x);
                                        }
                                    }
                                    braille_canvas.set_block_gray_dithered_with_brightness(cx, cy, grays);
                                }
                            }
                        }
                        frame_output += braille_canvas.render_grayscale();
                        break;
                    case Mode::flood_dot:
                        // All dots on, colored by average brightness
                        cv::cvtColor(resized, gray, cv::COLOR_BGR2GRAY);
                        {
                            size_t char_w = (out_w + 1) / 2;
                            size_t char_h = (out_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t grays[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < out_w && y < out_h)
                                                grays[row * 2 + col] = gray.at<uint8_t>(y, x);
                                        }
                                    }
                                    braille_canvas.set_block_flood_fill(cx, cy, grays);
                                }
                            }
                        }
                        frame_output += braille_canvas.render_grayscale();
                        break;
                    case Mode::flood_dot_colored:
                        // All dots on, colored by average RGB
                        {
                            size_t char_w = (out_w + 1) / 2;
                            size_t char_h = (out_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    int sum_r = 0, sum_g = 0, sum_b = 0;
                                    int count = 0;
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < out_w && y < out_h)
                                            {
                                                cv::Vec3b color = rgb.at<cv::Vec3b>(y, x);
                                                sum_r += color[0];
                                                sum_g += color[1];
                                                sum_b += color[2];
                                                count++;
                                            }
                                        }
                                    }
                                    if (count > 0)
                                    {
                                        color_dot_canvas.set_pattern(cx, cy, 0xFF);
                                        color_dot_canvas.set_color(cx, cy, sum_r / count, sum_g / count, sum_b / count);
                                    }
                                }
                            }
                        }
                        frame_output += color_dot_canvas.render();
                        break;
                    case Mode::colored_dithered:
                        // Dithered dots, colored by average RGB
                        {
                            static const int bayer2x2[2][2] = {{0, 2}, {3, 1}};
                            static const int dot_map[4][2] = {{0, 3}, {1, 4}, {2, 5}, {6, 7}};
                            size_t char_w = (out_w + 1) / 2;
                            size_t char_h = (out_h + 3) / 4;
                            for (size_t cy = 0; cy < char_h; ++cy)
                            {
                                for (size_t cx = 0; cx < char_w; ++cx)
                                {
                                    uint8_t pattern = 0;
                                    int sum_r = 0, sum_g = 0, sum_b = 0;
                                    int count = 0;
                                    int px = cx * 2, py = cy * 4;
                                    for (int row = 0; row < 4; ++row)
                                    {
                                        for (int col = 0; col < 2; ++col)
                                        {
                                            int x = px + col, y = py + row;
                                            if (x < out_w && y < out_h)
                                            {
                                                cv::Vec3b color = rgb.at<cv::Vec3b>(y, x);
                                                sum_r += color[0];
                                                sum_g += color[1];
                                                sum_b += color[2];
                                                count++;

                                                uint8_t g = (color[0] * 77 + color[1] * 150 + color[2] * 29) >> 8;
                                                int bayer_x = col & 1;
                                                int bayer_y = row & 1;
                                                int threshold_val = ((bayer2x2[bayer_y][bayer_x] + 1) * 255) / 5;
                                                if (g > threshold_val)
                                                {
                                                    pattern |= (1 << dot_map[row][col]);
                                                }
                                            }
                                        }
                                    }
                                    if (count > 0)
                                    {
                                        color_dot_canvas.set_pattern(cx, cy, pattern);
                                        color_dot_canvas.set_color(cx, cy, sum_r / count, sum_g / count, sum_b / count);
                                    }
                                }
                            }
                        }
                        frame_output += color_dot_canvas.render();
                        break;
                    }

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    // Absolute deadline frame pacing (no drift, no jitter)
                    std::this_thread::sleep_until(next_frame_deadline);
                    next_frame_deadline += frame_duration;
                    // If we fell behind (render took longer than frame_duration),
                    // snap the deadline forward to prevent a burst of catch-up frames
                    {
                        auto now_tp = std::chrono::steady_clock::now();
                        if (next_frame_deadline < now_tp)
                        {
                            auto behind = now_tp - next_frame_deadline;
                            auto frames_behind = behind / frame_duration;
                            next_frame_deadline += frame_duration * (frames_behind + 1);
                        }
                    }
                }

                cap.release();
                term_guard.restore();

                auto total_time = std::chrono::steady_clock::now() - start_time - total_pause_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback " << (user_stopped ? "stopped" : "finished") << ": "
                          << frame_num << " frames, "
                          << std::fixed << std::setprecision(1) << actual_fps << " fps average\n";

                return !user_stopped;
            }
        };

        /**
         * @brief Play video using OpenCV with fallback to FFmpeg
         * @param source Path to video file or webcam source
         * @param width Terminal width in characters
         * @param mode Rendering mode
         * @param threshold Brightness threshold for BW modes
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop playback (default 's', '\0' to disable)
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_opencv(const std::string &source, int width = 80,
                                      Mode mode = Mode::bw_dot, int threshold = 128,
                                      Shell shell = Shell::noninteractive,
                                      char pause_key = 'p', char stop_key = 's',
                                      double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            OpenCVVideoPlayer player(source, width, mode, threshold, fps, start_time, end_time);
            if (!player.play(shell, pause_key, stop_key))
            {
                // Webcam failed - throw exception
                if (player.is_webcam())
                {
                    throw std::runtime_error("Failed to open webcam. OpenCV required for webcam support.");
                }

                // Video file failed - fall back to FFmpeg with all modes properly supported
                std::cerr << "Warning: OpenCV failed, falling back to FFmpeg\n";
                play_video_with_mode(source, width, mode, threshold, shell, pause_key, stop_key, fps, start_time, end_time);
            }
        }

        /**
         * @brief Play from webcam
         * @param source Webcam source ("0", "webcam:0", "/dev/video0", etc.)
         * @param width Terminal width in characters
         * @param mode Rendering mode
         * @param threshold Brightness threshold for BW modes
         * @param shell Shell mode - interactive enables keyboard controls, noninteractive (default) disables them
         * @param pause_key Key to pause/resume playback (default 'p', '\0' to disable)
         * @param stop_key Key to stop playback (default 's', '\0' to disable)
         */
        inline void play_webcam(const std::string &source = "0", int width = 80,
                                Mode mode = Mode::bw_dot, int threshold = 128,
                                Shell shell = Shell::noninteractive,
                                char pause_key = 'p', char stop_key = 's')
        {
            if (!is_webcam_source(source))
            {
                throw std::runtime_error("Invalid webcam source: " + source);
            }

            OpenCVVideoPlayer player(source, width, mode, threshold);
            if (!player.play(shell, pause_key, stop_key))
            {
                throw std::runtime_error("Failed to open webcam. Is OpenCV installed with video capture support?");
            }
        }

#if defined(PYTHONIC_ENABLE_SDL2_AUDIO) || defined(PYTHONIC_ENABLE_PORTAUDIO)
        /**
         * @brief Play video using OpenCV with synchronized audio via FFmpeg
         *
         * Combines OpenCV's video rendering with FFmpeg audio decoding.
         * For webcam sources, audio is disabled (no audio source).
         *
         * @param source Path to video file or webcam source
         * @param width Terminal width in characters
         * @param mode Rendering mode
         * @param threshold Brightness threshold for BW modes
         * @param fps Target FPS (0 = use video's native FPS)
         * @param start_time Start time in seconds (-1 = from beginning)
         * @param end_time End time in seconds (-1 = to end)
         */
        inline void play_video_opencv_audio(const std::string &source, int width = 80,
                                            Mode mode = Mode::bw_dot, int threshold = 128,
                                            double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            // For webcams, just use regular OpenCV (no audio source)
            if (is_webcam_source(source))
            {
                play_video_opencv(source, width, mode, threshold, Shell::noninteractive, 'p', 's', fps, start_time, end_time);
                return;
            }

            // For video files, we use AudioVideoPlayer which combines FFmpeg audio with video
            // But we want to use OpenCV for video rendering. The AudioVideoPlayer already
            // uses the same rendering pipeline, so we can use it directly.
            AudioVideoPlayer player(source, width, mode, fps, start_time, end_time);
            player.play();
        }
#else
        /**
         * @brief Fallback when audio not compiled in
         */
        inline void play_video_opencv_audio(const std::string &source, int width = 80,
                                            Mode mode = Mode::bw_dot, int threshold = 128,
                                            double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            std::cerr << "Warning: Audio playback not available.\n"
                      << "Rebuild with -DPYTHONIC_ENABLE_SDL2_AUDIO=ON or -DPYTHONIC_ENABLE_PORTAUDIO=ON\n"
                      << "Falling back to silent video playback...\n\n";
            play_video_opencv(source, width, mode, threshold, Shell::noninteractive, 'p', 's', fps, start_time, end_time);
        }
#endif
#else
        // Stubs when OpenCV not available
        inline void play_video_opencv(const std::string &source, int width = 80,
                                      Mode mode = Mode::bw_dot, int threshold = 128,
                                      Shell shell = Shell::noninteractive,
                                      char pause_key = 'p', char stop_key = 's',
                                      double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            if (is_webcam_source(source))
            {
                throw std::runtime_error("Webcam requires OpenCV. Rebuild with -DPYTHONIC_ENABLE_OPENCV=ON");
            }

            std::cerr << "Warning: OpenCV not available, using FFmpeg\n";
            play_video_with_mode(source, width, mode, threshold, shell, pause_key, stop_key, fps, start_time, end_time);
        }

        inline void play_webcam(const std::string &source = "0", int width = 80,
                                Mode mode = Mode::bw_dot, int threshold = 128,
                                [[maybe_unused]] Shell shell = Shell::noninteractive,
                                [[maybe_unused]] char pause_key = 'p', [[maybe_unused]] char stop_key = 's')
        {
            (void)source;
            (void)width;
            (void)mode;
            (void)threshold;
            throw std::runtime_error("Webcam requires OpenCV. Rebuild with -DPYTHONIC_ENABLE_OPENCV=ON");
        }

        // Stub for play_video_opencv_audio when OpenCV not available
        inline void play_video_opencv_audio(const std::string &source, int width = 80,
                                            Mode mode = Mode::bw_dot, int threshold = 128,
                                            double fps = 0.0, double start_time = -1.0, double end_time = -1.0)
        {
            std::cerr << "Warning: OpenCV not available.\n";
            // Fall back to FFmpeg with audio
#if defined(PYTHONIC_ENABLE_SDL2_AUDIO) || defined(PYTHONIC_ENABLE_PORTAUDIO)
            AudioVideoPlayer player(source, width, mode, fps, start_time, end_time);
            player.play();
#else
            std::cerr << "Audio playback also not available. Playing silent video...\n";
            play_video_with_mode(source, width, mode, threshold, Shell::noninteractive, 'p', 's', fps, start_time, end_time);
#endif
        }
#endif

        // ==================== Threaded Video Playback API ====================

#if defined(PYTHONIC_ENABLE_SDL2_AUDIO) || defined(PYTHONIC_ENABLE_PORTAUDIO)
        /**
         * @brief Play video with full interactive controls using threaded architecture
         *
         * Features:
         * - Non-blocking keyboard input (doesn't halt video playback)
         * - Volume control via up/down arrows
         * - Seeking via left/right arrows
         * - Pause/Stop with configurable keys
         * - Synchronized audio playback
         *
         * @param source Path to video file
         * @param config RenderConfig with all playback options
         *
         * Example:
         * @code
         * RenderConfig config;
         * config.set_mode(Mode::colored)
         *       .set_max_width(120)
         *       .with_audio()
         *       .interactive()
         *       .set_volume(80)
         *       .set_seek_frames(150);  // 5 seconds at 30fps
         * play_video_threaded("video.mp4", config);
         * @endcode
         */
        inline void play_video_threaded(const std::string &source, const RenderConfig &config)
        {
            if (is_webcam_source(source))
            {
                // Webcams don't support seeking or audio
                play_video_opencv(source, config.max_width, config.mode, config.threshold,
                                  config.shell, config.pause_key, config.stop_key);
                return;
            }

            ThreadedAudioVideoPlayer player(source, config);
            player.play();
        }

        /**
         * @brief Convenience function with sensible defaults for interactive playback
         */
        inline void play_video_threaded(const std::string &source, int width = 80,
                                        Mode mode = Mode::bw_dot, int threshold = 128)
        {
            RenderConfig config;
            config.set_max_width(width)
                .set_mode(mode)
                .set_threshold(threshold)
                .with_audio()
                .interactive();

            play_video_threaded(source, config);
        }
#else
        /**
         * @brief Fallback when audio not compiled in
         */
        inline void play_video_threaded(const std::string &source, const RenderConfig &config)
        {
            std::cerr << "Warning: Audio playback not available.\n"
                      << "Rebuild with -DPYTHONIC_ENABLE_SDL2_AUDIO=ON or -DPYTHONIC_ENABLE_PORTAUDIO=ON\n"
                      << "Falling back to silent video playback...\n\n";

            play_video_with_mode(source, config.max_width, config.mode, config.threshold,
                                 config.shell, config.pause_key, config.stop_key,
                                 config.fps, config.start_time, config.end_time);
        }

        inline void play_video_threaded(const std::string &source, int width = 80,
                                        Mode mode = Mode::bw_dot, int threshold = 128)
        {
            std::cerr << "Warning: Audio playback not available.\n"
                      << "Rebuild with -DPYTHONIC_ENABLE_SDL2_AUDIO=ON or -DPYTHONIC_ENABLE_PORTAUDIO=ON\n"
                      << "Falling back to silent video playback...\n\n";

            play_video_with_mode(source, width, mode, threshold, Shell::interactive, 'p', 's', 0, -1, -1);
        }
#endif

    } // namespace draw
} // namespace pythonic
