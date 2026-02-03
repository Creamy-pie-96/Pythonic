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

            // The cleanup function that restores terminal state
            inline void restore_terminal()
            {
                // Use raw output to avoid any C++ stream buffering issues in signal context
                const char *restore = "\033[?25h\033[0m\033[H\033[J";
                // Show cursor, reset attributes, go home, clear screen
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

            // Mark playback as ended
            inline void end_playback()
            {
                playback_active() = false;
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
         */
        enum class Mode
        {
            bw,            // Black & white with block characters (current colored renderer logic, no color)
            bw_dot,        // Black & white with braille patterns (default, higher resolution)
            colored,       // True color with block characters
            colored_dot,   // True color with braille patterns (one color per cell)
            bw_dithered,   // Black & white with ordered dithering for grayscale shading
            grayscale_dot, // Grayscale-colored braille dots (dots colored by brightness)
            flood_dot      // Flood-fill: all dots lit (⣿), colored by average cell grayscale
        };

        // Legacy alias for backward compatibility
        using Render = Mode;

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

            /**
             * @brief Generate ANSI true color foreground escape code
             */
            inline std::string fg_color(uint8_t r, uint8_t g, uint8_t b)
            {
                return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            }

            /**
             * @brief Generate ANSI true color background escape code
             */
            inline std::string bg_color(uint8_t r, uint8_t g, uint8_t b)
            {
                return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
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
                return static_cast<uint8_t>((299 * r + 587 * g + 114 * b) / 1000);
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
                const char *UPPER_HALF = "\xe2\x96\x80";

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
                        if (top != prev_fg)
                        {
                            out += ansi::fg_color(top.r, top.g, top.b);
                            prev_fg = top;
                        }
                        if (bot != prev_bg)
                        {
                            out += ansi::bg_color(bot.r, bot.g, bot.b);
                            prev_bg = bot;
                        }
                        out += UPPER_HALF;
                    }
                    // Reset at end of line and newline
                    out += ansi::RESET;
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
                                uint8_t gray = static_cast<uint8_t>((299 * r + 587 * g + 114 * b) / 1000);

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
             * @brief Render to ANSI string with colored Braille characters
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
                            out += ansi::fg_color(color.r, color.g, color.b);
                            prev_color = color;
                        }

                        out += braille_to_utf8(pattern);
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
                            gray_top = static_cast<uint8_t>((299 * data[idx] + 587 * data[idx + 1] + 114 * data[idx + 2]) / 1000);
                        }

                        // Bottom pixel - convert RGB to grayscale
                        if (py_bot < static_cast<size_t>(height))
                        {
                            size_t idx = (py_bot * width + cx) * 3;
                            gray_bot = static_cast<uint8_t>((299 * data[idx] + 587 * data[idx + 1] + 114 * data[idx + 2]) / 1000);
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
                static constexpr uint8_t dither_thresholds[8] = {
                    16,  // row 0, col 0 - lights at ~6% brightness (first)
                    144, // row 0, col 1 - lights at ~56% brightness
                    80,  // row 1, col 0 - lights at ~31% brightness
                    208, // row 1, col 1 - lights at ~81% brightness
                    112, // row 2, col 0 - lights at ~44% brightness
                    240, // row 2, col 1 - lights at ~94% brightness (last)
                    48,  // row 3, col 0 - lights at ~19% brightness
                    176  // row 3, col 1 - lights at ~69% brightness
                };

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

                // Use same proper Bayer-style thresholds as set_block_gray_dithered
                static constexpr uint8_t dither_thresholds[8] = {
                    16, 144, 80, 208, 112, 240, 48, 176};

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
                    gray[i] = static_cast<uint8_t>((299 * data[idx] + 587 * data[idx + 1] + 114 * data[idx + 2]) / 1000);
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
             * @brief Load a PGM/PPM image with ordered dithering for grayscale shading
             *
             * Unlike regular load_pgm_ppm which uses simple thresholding, this version
             * uses ordered dithering to create smooth grayscale gradients. Brighter areas
             * have more dots, darker areas have fewer dots.
             */
            bool load_pgm_ppm_dithered(const std::string &filename)
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

                // Read entire image into buffer first
                std::vector<uint8_t> grayscale(width * height);

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
                        grayscale[y * width + x] = static_cast<uint8_t>(gray);
                    }
                }

                // Apply dithered rendering using block operations
                load_frame_ordered_dithered(grayscale.data(), width, height);

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
                file.get(c);

                if (maxval > 255)
                    return false;

                // Resize canvas and grayscale storage
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0));
                _grayscale.assign(_char_height, std::vector<uint8_t>(_char_width, 0));

                // Read entire image into buffer
                std::vector<uint8_t> graybuf(width * height);

                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        int gray;
                        if (is_color)
                        {
                            unsigned char rgb[3];
                            file.read(reinterpret_cast<char *>(rgb), 3);
                            gray = (299 * rgb[0] + 587 * rgb[1] + 114 * rgb[2]) / 1000;
                        }
                        else
                        {
                            unsigned char g;
                            file.read(reinterpret_cast<char *>(&g), 1);
                            gray = g;
                        }
                        graybuf[y * width + x] = static_cast<uint8_t>(gray);
                    }
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
                file.get(c);

                if (maxval > 255)
                    return false;

                // Resize canvas and grayscale storage
                _char_width = (width + 1) / 2;
                _char_height = (height + 3) / 4;
                _pixel_width = _char_width * 2;
                _pixel_height = _char_height * 4;
                _canvas.assign(_char_height, std::vector<uint8_t>(_char_width, 0xFF)); // All dots lit
                _grayscale.assign(_char_height, std::vector<uint8_t>(_char_width, 0));

                // Read entire image into buffer
                std::vector<uint8_t> graybuf(width * height);

                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        int gray;
                        if (is_color)
                        {
                            unsigned char rgb[3];
                            file.read(reinterpret_cast<char *>(rgb), 3);
                            gray = (299 * rgb[0] + 587 * rgb[1] + 114 * rgb[2]) / 1000;
                        }
                        else
                        {
                            unsigned char g;
                            file.read(reinterpret_cast<char *>(&g), 1);
                            gray = g;
                        }
                        graybuf[y * width + x] = static_cast<uint8_t>(gray);
                    }
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
                        if (gray != prev_gray)
                        {
                            out += ansi::fg_color(gray, gray, gray);
                            prev_gray = gray;
                        }

                        out += braille_to_utf8(pattern);
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
         * @brief Render an image file with ordered dithering for smooth grayscale shading
         *
         * Unlike simple thresholding, this uses ordered dithering to create
         * proper grayscale shading. Brighter areas have more dots lit, darker areas
         * have fewer dots - creating natural-looking gradients.
         *
         * Perfect for photos and videos where you want to see detail in both
         * highlights and shadows instead of just black & white.
         */
        inline std::string render_image_dithered(const std::string &filename, int max_width = 80)
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
                if (canvas.load_pgm_ppm_dithered(ppm_file))
                {
                    std::remove(ppm_file.c_str());
                    return canvas.render();
                }
                std::remove(ppm_file.c_str());
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

            // Read PPM file manually
            std::ifstream ppm(temp_ppm, std::ios::binary);
            if (!ppm)
            {
                std::remove(temp_ppm.c_str());
                return "Error: Could not read converted image.\n";
            }

            std::string magic;
            ppm >> magic;
            if (magic != "P6")
            {
                ppm.close();
                std::remove(temp_ppm.c_str());
                return "Error: Invalid PPM format.\n";
            }

            // Skip comments
            char c;
            ppm.get(c);
            while (ppm.peek() == '#')
            {
                std::string comment;
                std::getline(ppm, comment);
            }

            int width, height, maxval;
            ppm >> width >> height >> maxval;
            ppm.get(c); // Skip whitespace

            // Read RGB data
            std::vector<uint8_t> rgb_data(width * height * 3);
            ppm.read(reinterpret_cast<char *>(rgb_data.data()), rgb_data.size());
            ppm.close();
            std::remove(temp_ppm.c_str());

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

            std::ifstream ppm(temp_ppm, std::ios::binary);
            if (!ppm)
            {
                std::remove(temp_ppm.c_str());
                return "Error: Could not read converted image.\n";
            }

            std::string magic;
            ppm >> magic;
            if (magic != "P6")
            {
                ppm.close();
                std::remove(temp_ppm.c_str());
                return "Error: Invalid PPM format.\n";
            }

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

            std::vector<uint8_t> rgb_data(width * height * 3);
            ppm.read(reinterpret_cast<char *>(rgb_data.data()), rgb_data.size());
            ppm.close();
            std::remove(temp_ppm.c_str());

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

            std::ifstream ppm(temp_ppm, std::ios::binary);
            if (!ppm)
            {
                std::remove(temp_ppm.c_str());
                return "Error: Could not read converted image.\n";
            }

            std::string magic;
            ppm >> magic;
            if (magic != "P6")
            {
                ppm.close();
                std::remove(temp_ppm.c_str());
                return "Error: Invalid PPM format.\n";
            }

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

            std::vector<uint8_t> rgb_data(width * height * 3);
            ppm.read(reinterpret_cast<char *>(rgb_data.data()), rgb_data.size());
            ppm.close();
            std::remove(temp_ppm.c_str());

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
         * @param mode Rendering mode (bw, bw_dot, colored, colored_dot, bw_dithered, grayscale_dot)
         */
        inline void print_image_with_mode(const std::string &filename, int max_width = 80,
                                          int threshold = 128, Mode mode = Mode::bw_dot)
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
                // Ordered dithering for smooth grayscale
                std::cout << render_image_dithered(filename, max_width);
                break;
            case Mode::grayscale_dot:
                // Grayscale-colored braille dots
                std::cout << render_image_grayscale(filename, max_width, threshold, true);
                break;
            case Mode::flood_dot:
                // All dots lit, colored by average brightness - smoothest
                std::cout << render_image_flood(filename, max_width);
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
                    // Mark playback as ended
                    signal_handler::end_playback();
                    // Clear screen, show cursor, reset attributes
                    std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME
                              << ansi::SHOW_CURSOR << ansi::RESET << std::flush;
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
            int _width;     // Output width in terminal characters
            int _threshold; // Binarization threshold
            double _fps;    // Target FPS (0 = use video's native FPS)
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
             */
            VideoPlayer(const std::string &filename, int width = 80, int threshold = 128, double target_fps = 0)
                : _filename(filename), _width(width), _threshold(threshold), _fps(target_fps),
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

                // Only enable keyboard input in interactive mode
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                // Clear screen and position cursor at top
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

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
         */
        inline void play_video(const std::string &filename, int width = 80, int threshold = 128,
                               Shell shell = Shell::noninteractive,
                               char pause_key = 'p', char stop_key = 's')
        {
            VideoPlayer player(filename, width, threshold);
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
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            ColorCanvas _canvas;
            Shell _shell;
            char _pause_key;
            char _stop_key;

        public:
            ColoredVideoPlayer(const std::string &filename, int width = 80, double target_fps = 0)
                : _filename(filename), _width(width), _fps(target_fps),
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

                std::string cmd = "ffmpeg -i \"" + _filename + "\" "
                                                               "-vf scale=" +
                                  std::to_string(pixel_w) + ":" + std::to_string(pixel_h) +
                                  " -pix_fmt rgb24 -f rawvideo -v quiet - 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg.\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h * 3;
                std::vector<uint8_t> frame_buffer(frame_size);

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
                std::setvbuf(stdout, nullptr, _IONBF, 0);

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
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
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    auto frame_start = std::chrono::steady_clock::now();

                    size_t bytes_read = fread(frame_buffer.data(), 1, frame_size, pipe);
                    if (bytes_read < frame_size)
                        break;

                    _canvas.load_frame_rgb(frame_buffer.data(), pixel_w, pixel_h);

                    // Build complete frame with cursor positioning
                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    // Single write for entire frame
                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    auto frame_end = std::chrono::steady_clock::now();
                    auto elapsed = frame_end - frame_start;

                    if (elapsed < frame_duration)
                        std::this_thread::sleep_for(frame_duration - elapsed);
                }

                pclose(pipe);
                term_guard.restore();

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME;

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
         */
        inline void play_video_colored(const std::string &filename, int width = 80,
                                       Shell shell = Shell::noninteractive,
                                       char pause_key = 'p', char stop_key = 's')
        {
            ColoredVideoPlayer player(filename, width);
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
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            BWBlockCanvas _canvas;

        public:
            BWBlockVideoPlayer(const std::string &filename, int width = 80, int threshold = 128, double target_fps = 0)
                : _filename(filename), _width(width), _threshold(threshold), _fps(target_fps),
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

                std::string cmd = "ffmpeg -i \"" + _filename + "\" "
                                                               "-vf scale=" +
                                  std::to_string(pixel_w) + ":" + std::to_string(pixel_h) +
                                  " -pix_fmt rgb24 -f rawvideo -v quiet - 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg.\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h * 3;
                std::vector<uint8_t> frame_buffer(frame_size);

                _canvas = BWBlockCanvas::from_pixels(pixel_w, pixel_h);

                TerminalStateGuard term_guard;

                // Only enable keyboard input in interactive mode
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                std::setvbuf(stdout, nullptr, _IONBF, 0);
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
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
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    auto frame_start = std::chrono::steady_clock::now();

                    size_t bytes_read = fread(frame_buffer.data(), 1, frame_size, pipe);
                    if (bytes_read < frame_size)
                        break;

                    _canvas.load_frame_rgb(frame_buffer.data(), pixel_w, pixel_h, _threshold);

                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    auto frame_end = std::chrono::steady_clock::now();
                    auto elapsed = frame_end - frame_start;

                    if (elapsed < frame_duration)
                        std::this_thread::sleep_for(frame_duration - elapsed);
                }

                pclose(pipe);
                term_guard.restore();

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME;

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
         */
        inline void play_video_bw_block(const std::string &filename, int width = 80, int threshold = 128,
                                        Shell shell = Shell::noninteractive,
                                        char pause_key = 'p', char stop_key = 's')
        {
            BWBlockVideoPlayer player(filename, width, threshold);
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
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            ColoredBrailleCanvas _canvas;

        public:
            ColoredBrailleVideoPlayer(const std::string &filename, int width = 80, int threshold = 128, double target_fps = 0)
                : _filename(filename), _width(width), _threshold(threshold), _fps(target_fps),
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

                std::string cmd = "ffmpeg -i \"" + _filename + "\" "
                                                               "-vf scale=" +
                                  std::to_string(pixel_w) + ":" + std::to_string(pixel_h) +
                                  " -pix_fmt rgb24 -f rawvideo -v quiet - 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg.\n";
                    return false;
                }

                size_t frame_size = pixel_w * pixel_h * 3;
                std::vector<uint8_t> frame_buffer(frame_size);

                _canvas = ColoredBrailleCanvas::from_pixels(pixel_w, pixel_h);

                TerminalStateGuard term_guard;

                // Only enable keyboard input in interactive mode
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                {
                    keyboard = std::make_unique<KeyboardInput>();
                }
                bool user_stopped = false;

                std::setvbuf(stdout, nullptr, _IONBF, 0);
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
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
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    auto frame_start = std::chrono::steady_clock::now();

                    size_t bytes_read = fread(frame_buffer.data(), 1, frame_size, pipe);
                    if (bytes_read < frame_size)
                        break;

                    _canvas.load_frame_rgb(frame_buffer.data(), pixel_w, pixel_h, _threshold);

                    frame_output = ansi::CURSOR_HOME;
                    frame_output += _canvas.render();

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    auto frame_end = std::chrono::steady_clock::now();
                    auto elapsed = frame_end - frame_start;

                    if (elapsed < frame_duration)
                        std::this_thread::sleep_for(frame_duration - elapsed);
                }

                pclose(pipe);
                term_guard.restore();

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME;

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
         */
        inline void play_video_colored_dot(const std::string &filename, int width = 80, int threshold = 128,
                                           Shell shell = Shell::noninteractive,
                                           char pause_key = 'p', char stop_key = 's')
        {
            ColoredBrailleVideoPlayer player(filename, width, threshold);
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
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            BrailleCanvas _canvas;

        public:
            DitheredVideoPlayer(const std::string &filename, int width = 80, double target_fps = 0)
                : _filename(filename), _width(width), _fps(target_fps),
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

                int w = 0, h = 0;
                double fps = 0, duration = 0;
                size_t pos = 0, comma;

                comma = result.find(',');
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

                size_t frame_size = pixel_w * pixel_h;
                std::vector<uint8_t> frame_buffer(frame_size);
                int char_height = pixel_h / 4;
                _canvas = BrailleCanvas(_width, char_height);

                TerminalStateGuard term_guard;
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                    keyboard = std::make_unique<KeyboardInput>();
                bool user_stopped = false;

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

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
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    auto frame_start = std::chrono::steady_clock::now();

                    size_t bytes_read = fread(frame_buffer.data(), 1, frame_size, pipe);
                    if (bytes_read < frame_size)
                        break;

                    // Use ordered dithering instead of threshold
                    _canvas.load_frame_ordered_dithered(frame_buffer.data(), pixel_w, pixel_h);

                    std::cout << ansi::CURSOR_HOME;
                    std::cout << _canvas.render() << std::flush;

                    ++frame_num;

                    auto frame_end = std::chrono::steady_clock::now();
                    auto elapsed = frame_end - frame_start;
                    if (elapsed < frame_duration)
                        std::this_thread::sleep_for(frame_duration - elapsed);
                }

                pclose(pipe);
                term_guard.restore();
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME;

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
         */
        inline void play_video_dithered(const std::string &filename, int width = 80,
                                        Shell shell = Shell::noninteractive,
                                        char pause_key = 'p', char stop_key = 's')
        {
            DitheredVideoPlayer player(filename, width);
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
            std::atomic<bool> _running;
            std::atomic<bool> _paused;
            std::thread _playback_thread;
            Shell _shell;
            char _pause_key;
            char _stop_key;

            BrailleCanvas _canvas;

        public:
            GrayscaleVideoPlayer(const std::string &filename, int width = 80, double target_fps = 0)
                : _filename(filename), _width(width), _fps(target_fps),
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

                int w = 0, h = 0;
                double fps = 0, duration = 0;
                size_t pos = 0, comma;

                comma = result.find(',');
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

                size_t frame_size = pixel_w * pixel_h;
                std::vector<uint8_t> frame_buffer(frame_size);
                int char_height = pixel_h / 4;
                _canvas = BrailleCanvas(_width, char_height);

                TerminalStateGuard term_guard;
                std::unique_ptr<KeyboardInput> keyboard;
                if (_shell == Shell::interactive)
                    keyboard = std::make_unique<KeyboardInput>();
                bool user_stopped = false;

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point pause_start;
                std::chrono::microseconds total_pause_time{0};

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
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    auto frame_start = std::chrono::steady_clock::now();

                    size_t bytes_read = fread(frame_buffer.data(), 1, frame_size, pipe);
                    if (bytes_read < frame_size)
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
                                        grays[row * 2 + col] = frame_buffer[y * pixel_w + x];
                                    }
                                }
                            }
                            _canvas.set_block_gray_dithered_with_brightness(cx, cy, grays);
                        }
                    }

                    std::cout << ansi::CURSOR_HOME;
                    std::cout << _canvas.render_grayscale() << std::flush;

                    ++frame_num;

                    auto frame_end = std::chrono::steady_clock::now();
                    auto elapsed = frame_end - frame_start;
                    if (elapsed < frame_duration)
                        std::this_thread::sleep_for(frame_duration - elapsed);
                }

                pclose(pipe);
                term_guard.restore();
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME;

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
         */
        inline void play_video_grayscale(const std::string &filename, int width = 80,
                                         Shell shell = Shell::noninteractive,
                                         char pause_key = 'p', char stop_key = 's')
        {
            GrayscaleVideoPlayer player(filename, width);
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
         */
        inline void play_video_with_mode(const std::string &filename, int width = 80,
                                         Mode mode = Mode::bw_dot, int threshold = 128,
                                         Shell shell = Shell::noninteractive,
                                         char pause_key = 'p', char stop_key = 's')
        {
            switch (mode)
            {
            case Mode::bw:
                play_video_bw_block(filename, width, threshold, shell, pause_key, stop_key);
                break;
            case Mode::bw_dot:
                play_video(filename, width, threshold, shell, pause_key, stop_key);
                break;
            case Mode::colored:
                play_video_colored(filename, width, shell, pause_key, stop_key);
                break;
            case Mode::colored_dot:
                play_video_colored_dot(filename, width, threshold, shell, pause_key, stop_key);
                break;
            case Mode::bw_dithered:
                play_video_dithered(filename, width, shell, pause_key, stop_key);
                break;
            case Mode::grayscale_dot:
                play_video_grayscale(filename, width, shell, pause_key, stop_key);
                break;
            case Mode::flood_dot:
                // Flood dot uses same renderer as grayscale_dot but with different loading
                // For video playback, grayscale is similar enough
                play_video_grayscale(filename, width, shell, pause_key, stop_key);
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
                             Render render_mode = Mode::bw_dot, double target_fps = 0)
                : _filename(filename), _width(width), _fps(target_fps), _render_mode(render_mode)
            {
                enable_ansi_support();
            }

            ~AudioVideoPlayer()
            {
                stop();
                cleanup_audio();
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
                pclose(pipe);

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
                // FFmpeg command to decode audio to raw PCM
                std::string cmd = "ffmpeg -i \"" + _filename + "\" "
                                                               "-f s16le -acodec pcm_s16le -ar " +
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

                pclose(pipe);
            }

            void video_playback()
            {
                auto [vid_w, vid_h, vid_fps, duration] = get_info();
                if (vid_w == 0 || vid_h == 0)
                {
                    std::cerr << "Error: Could not read video info.\n";
                    return;
                }

                int pixel_w, pixel_h;
                bool use_color = (_render_mode == Mode::colored || _render_mode == Mode::colored_dot);

                if (use_color)
                {
                    pixel_w = _width;
                    pixel_h = (int)(pixel_w * vid_h / vid_w);
                    pixel_h = ((pixel_h + 1) / 2) * 2;
                }
                else
                {
                    pixel_w = _width * 2;
                    pixel_h = (int)(pixel_w * vid_h / vid_w);
                    pixel_h = (pixel_h + 3) / 4 * 4;
                }

                double target_fps = (_fps > 0) ? _fps : vid_fps;
                if (target_fps <= 0)
                    target_fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / target_fps));

                std::string pix_fmt = use_color ? "rgb24" : "gray";
                std::string cmd = "ffmpeg -i \"" + _filename + "\" "
                                                               "-vf scale=" +
                                  std::to_string(pixel_w) + ":" + std::to_string(pixel_h) +
                                  " -pix_fmt " + pix_fmt + " -f rawvideo -v quiet - 2>/dev/null";

                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                {
                    std::cerr << "Error: Could not start FFmpeg video decoder.\n";
                    return;
                }

                size_t bytes_per_pixel = use_color ? 3 : 1;
                size_t frame_size = pixel_w * pixel_h * bytes_per_pixel;
                std::vector<uint8_t> frame_buffer(frame_size);

                BrailleCanvas bw_canvas;
                ColorCanvas color_canvas;

                if (use_color)
                    color_canvas = ColorCanvas::from_pixels(pixel_w, pixel_h);
                else
                    bw_canvas = BrailleCanvas(_width, pixel_h / 4);

                TerminalStateGuard term_guard;

                // Disable stdout buffering for smoother video
                std::setvbuf(stdout, nullptr, _IONBF, 0);

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();

                // Pre-allocate output buffer
                std::string frame_output;
                frame_output.reserve(pixel_w * (use_color ? (pixel_h / 2) : (pixel_h / 4)) * 40);

                while (_running && !term_guard.was_interrupted())
                {
                    auto frame_start = std::chrono::steady_clock::now();

                    size_t bytes_read = fread(frame_buffer.data(), 1, frame_size, pipe);
                    if (bytes_read < frame_size)
                        break;

                    // Build complete frame with cursor positioning
                    frame_output = ansi::CURSOR_HOME;

                    if (use_color)
                    {
                        color_canvas.load_frame_rgb(frame_buffer.data(), pixel_w, pixel_h);
                        frame_output += color_canvas.render();
                    }
                    else
                    {
                        bw_canvas.load_frame_fast(frame_buffer.data(), pixel_w, pixel_h, 128);
                        frame_output += bw_canvas.render();
                    }

                    // Single write for entire frame
                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    auto frame_end = std::chrono::steady_clock::now();
                    auto elapsed = frame_end - frame_start;

                    if (elapsed < frame_duration)
                        std::this_thread::sleep_for(frame_duration - elapsed);
                }

                pclose(pipe);
                term_guard.restore();

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME;

                auto total_time = std::chrono::steady_clock::now() - start_time;
                double actual_fps = frame_num / (std::chrono::duration<double>(total_time).count());

                std::cout << "Playback finished: " << frame_num << " frames, "
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
         *
         * Note: Audio playback currently doesn't support pause/stop - video continues
         * while audio may desync. For full pause/stop support, use non-audio playback.
         */
        inline void play_video_audio(const std::string &filename, int width = 80,
                                     Render render_mode = Mode::bw_dot,
                                     [[maybe_unused]] Shell shell = Shell::noninteractive,
                                     [[maybe_unused]] char pause_key = 'p',
                                     [[maybe_unused]] char stop_key = 's')
        {
            AudioVideoPlayer player(filename, width, render_mode);
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
                                     char pause_key = 'p', char stop_key = 's')
        {
            std::cerr << "Warning: Audio playback not available.\n"
                      << "Rebuild with -DPYTHONIC_ENABLE_SDL2_AUDIO=ON or -DPYTHONIC_ENABLE_PORTAUDIO=ON\n"
                      << "Falling back to silent video playback...\n\n";

            if (render_mode == Mode::colored || render_mode == Mode::colored_dot)
                play_video_colored(filename, width, shell, pause_key, stop_key);
            else
                play_video(filename, width, 128, shell, pause_key, stop_key);
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
            bool _is_webcam;
            std::atomic<bool> _running{false};
            std::atomic<bool> _paused{false};
            Shell _shell;
            char _pause_key;
            char _stop_key;

        public:
            OpenCVVideoPlayer(const std::string &source, int width = 80,
                              Mode mode = Mode::bw_dot, int threshold = 128)
                : _source(source), _width(width), _threshold(threshold), _mode(mode),
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

                double fps = cap.get(cv::CAP_PROP_FPS);
                if (fps <= 0)
                    fps = 30;

                auto frame_duration = std::chrono::microseconds((int)(1000000.0 / fps));

                cv::Mat frame, rgb, resized;

                // Calculate output dimensions
                int cap_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
                int cap_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

                double scale;
                if (_mode == Mode::bw_dot || _mode == Mode::colored_dot)
                    scale = static_cast<double>(_width * 2) / cap_width;
                else
                    scale = static_cast<double>(_width) / cap_width;

                int out_w = static_cast<int>(cap_width * scale);
                int out_h = static_cast<int>(cap_height * scale);

                // Initialize canvases
                BrailleCanvas bw_dot_canvas;
                BWBlockCanvas bw_canvas;
                ColorCanvas color_canvas;
                ColoredBrailleCanvas color_dot_canvas;

                switch (_mode)
                {
                case Mode::bw_dot:
                    bw_dot_canvas = BrailleCanvas::from_pixels(out_w, out_h);
                    break;
                case Mode::bw:
                    bw_canvas = BWBlockCanvas::from_pixels(out_w, out_h);
                    break;
                case Mode::colored:
                    color_canvas = ColorCanvas::from_pixels(out_w, out_h);
                    break;
                case Mode::colored_dot:
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

                std::setvbuf(stdout, nullptr, _IONBF, 0);
                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

                size_t frame_num = 0;
                auto start_time = std::chrono::steady_clock::now();
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
                                }
                            }
                        }
                    }

                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }

                    if (!cap.read(frame))
                        break;

                    auto frame_start = std::chrono::steady_clock::now();

                    // Resize and convert
                    cv::resize(frame, resized, cv::Size(out_w, out_h), 0, 0, cv::INTER_AREA);
                    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

                    frame_output = ansi::CURSOR_HOME;

                    switch (_mode)
                    {
                    case Mode::bw_dot:
                        bw_dot_canvas.load_frame_rgb_fast(rgb.data, out_w, out_h, _threshold);
                        frame_output += bw_dot_canvas.render();
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
                    }

                    fwrite(frame_output.c_str(), 1, frame_output.size(), stdout);
                    fflush(stdout);

                    ++frame_num;

                    auto frame_end = std::chrono::steady_clock::now();
                    auto elapsed = frame_end - frame_start;

                    if (elapsed < frame_duration)
                        std::this_thread::sleep_for(frame_duration - elapsed);
                }

                cap.release();
                term_guard.restore();

                std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME;

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
         */
        inline void play_video_opencv(const std::string &source, int width = 80,
                                      Mode mode = Mode::bw_dot, int threshold = 128,
                                      Shell shell = Shell::noninteractive,
                                      char pause_key = 'p', char stop_key = 's')
        {
            OpenCVVideoPlayer player(source, width, mode, threshold);
            if (!player.play(shell, pause_key, stop_key))
            {
                // Webcam failed - throw exception
                if (player.is_webcam())
                {
                    throw std::runtime_error("Failed to open webcam. OpenCV required for webcam support.");
                }

                // Video file failed - fall back to FFmpeg
                std::cerr << "Warning: OpenCV failed, falling back to FFmpeg\n";
                if (mode == Mode::colored || mode == Mode::colored_dot)
                    play_video_colored(source, width, shell, pause_key, stop_key);
                else
                    play_video(source, width, threshold, shell, pause_key, stop_key);
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
#else
        // Stubs when OpenCV not available
        inline void play_video_opencv(const std::string &source, int width = 80,
                                      Mode mode = Mode::bw_dot, int threshold = 128,
                                      Shell shell = Shell::noninteractive,
                                      char pause_key = 'p', char stop_key = 's')
        {
            if (is_webcam_source(source))
            {
                throw std::runtime_error("Webcam requires OpenCV. Rebuild with -DPYTHONIC_ENABLE_OPENCV=ON");
            }

            std::cerr << "Warning: OpenCV not available, using FFmpeg\n";
            if (mode == Mode::colored || mode == Mode::colored_dot)
                play_video_colored(source, width, shell, pause_key, stop_key);
            else
                play_video(source, width, threshold, shell, pause_key, stop_key);
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
#endif

    } // namespace draw
} // namespace pythonic
