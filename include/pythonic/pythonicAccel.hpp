#pragma once
/**
 * @file pythonicAccel.hpp
 * @brief Centralized acceleration, image & video processing module
 *
 * This is the SINGLE source of truth for all pixel-level processing in the
 * pythonic library.  Every other module (Draw, Print, Plot, Export) calls
 * into the APIs exposed here instead of rolling its own loops.
 *
 * Capabilities provided:
 *   1. GPU / hardware detection  (NVIDIA, AMD, Intel, Apple, FFmpeg HW encoders)
 *   2. Pixel-processing primitives  (grayscale, dithering, color-avg, braille-cell)
 *   3. Image I/O helpers  (PPM/PGM parse, ImageMagick convert)
 *   4. Video processing helpers  (FFmpeg frame extract, encode, probe)
 *   5. A pluggable ComputeBackend interface  (CPU multi-threaded today,
 *      OpenCL / CUDA / Metal stubs ready for the future)
 *
 * Design goals:
 *   - DRY: every formula (BT.601 gray, Bayer matrix, Floyd-Steinberg, ...)
 *     lives here exactly once.
 *   - Thread-safe: CPU backend parallelises heavy loops automatically.
 *   - Extensible: add a new backend by subclassing ComputeBackend.
 *
 * Usage:
 *   using namespace pythonic::accel;
 *   auto backend = get_best_backend();
 *
 *   // Convert a whole image
 *   std::vector<uint8_t> gray(w * h);
 *   backend->rgb_to_grayscale(rgb.data(), w, h, gray.data());
 *
 *   // Or use the free-function helpers directly
 *   uint8_t g = pixel::to_gray(r, g, b);
 */

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <array>
#include <mutex>
#include <numeric>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef PYTHONIC_ENABLE_OPENCL
#include <CL/cl.h>
#endif

namespace pythonic
{
    namespace accel
    {

        // ==================================================================
        //  Section 1 - Pixel-level primitives (inline, zero-overhead)
        // ==================================================================

        namespace pixel
        {
            /**
             * @brief ITU-R BT.601 luma (integer fast-path)
             *
             * This is the ONE canonical grayscale formula.
             * All code in the library must call this instead of inlining the
             * constants.  Using integer maths avoids float rounding.
             */
            inline uint8_t to_gray(uint8_t r, uint8_t g, uint8_t b)
            {
                return static_cast<uint8_t>((299u * r + 587u * g + 114u * b) / 1000u);
            }

            /**
             * @brief Map a 0-255 grayscale value to ANSI-256 palette (232-255, 24 levels)
             */
            inline int gray_to_ansi256(uint8_t gray)
            {
                return 232 + static_cast<int>(gray) * 23 / 255;
            }

            /**
             * @brief Simple RGB struct (no alpha)
             */
            struct RGB
            {
                uint8_t r = 0, g = 0, b = 0;
                RGB() = default;
                RGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
            };

        } // namespace pixel

        // ==================================================================
        //  Section 2 - Dithering algorithms
        // ==================================================================

        namespace dither
        {
            /**
             * @brief 2x4 ordered-dither thresholds optimised for Braille cells.
             *
             * The 8 values correspond to the 8 dots in a single braille
             * character (row-major: [row0-col0, row0-col1, row1-col0, ...]).
             * They are spread across the 0-255 range so that a smooth
             * gradient lights up dots one-by-one from darkest to brightest.
             */
            static constexpr uint8_t BRAILLE_ORDERED[8] = {
                16,  // row 0, col 0 - lights at ~6 %
                144, // row 0, col 1 - lights at ~56 %
                80,  // row 1, col 0 - lights at ~31 %
                208, // row 1, col 1 - lights at ~81 %
                112, // row 2, col 0 - lights at ~44 %
                240, // row 2, col 1 - lights at ~94 %
                48,  // row 3, col 0 - lights at ~19 %
                176  // row 3, col 1 - lights at ~69 %
            };

            /**
             * @brief Classic 2x2 Bayer matrix scaled to 0-255.
             *
             * Used for coloured-braille dithering where we only have a 2x2
             * sub-block per braille column.
             */
            static constexpr int BAYER_2x2[2][2] = {
                {0, 128},
                {192, 64}};

            /**
             * @brief Apply Floyd-Steinberg error-diffusion to a grayscale buffer
             *        using serpentine (boustrophedon) scanning.
             *
             * Serpentine scanning alternates left-to-right and right-to-left
             * on each row, which eliminates the directional bias artifacts of
             * a simple left-to-right scan.
             *
             * Floyd-Steinberg is inherently serial along rows due to the
             * 7/16 right-neighbour dependency.  For parallel acceleration,
             * use floyd_steinberg_parallel() which splits the image into
             * independent blocks.
             *
             * @param gray_in   Input grayscale (1 byte per pixel, row-major)
             * @param width     Image width
             * @param height    Image height
             * @param out       Output binary image (0 or 255, same size as input)
             */
            inline void floyd_steinberg(const uint8_t *gray_in, int width, int height,
                                        uint8_t *out)
            {
                if (width <= 0 || height <= 0)
                    return;

                // Work in float so error accumulation is precise
                std::vector<float> buf(static_cast<size_t>(width) * height);
                for (int i = 0; i < width * height; ++i)
                    buf[i] = static_cast<float>(gray_in[i]);

                for (int y = 0; y < height; ++y)
                {
                    bool left_to_right = (y % 2 == 0);
                    int x_start = left_to_right ? 0 : width - 1;
                    int x_end = left_to_right ? width : -1;
                    int x_step = left_to_right ? 1 : -1;

                    for (int x = x_start; x != x_end; x += x_step)
                    {
                        int idx = y * width + x;
                        float old_px = buf[idx];
                        float new_px = (old_px >= 128.0f) ? 255.0f : 0.0f;
                        buf[idx] = new_px;
                        out[idx] = static_cast<uint8_t>(new_px);

                        float err = old_px - new_px;

                        // Diffuse error in serpentine direction
                        int next_x = x + x_step;
                        int prev_x = x - x_step;

                        // 7/16 to the next pixel in scan direction
                        if (next_x >= 0 && next_x < width)
                            buf[idx + x_step] += err * 7.0f / 16.0f;

                        if (y + 1 < height)
                        {
                            // 3/16 to below-behind (opposite scan direction)
                            if (prev_x >= 0 && prev_x < width)
                                buf[(y + 1) * width + prev_x] += err * 3.0f / 16.0f;
                            // 5/16 to directly below
                            buf[(y + 1) * width + x] += err * 5.0f / 16.0f;
                            // 1/16 to below-ahead (scan direction)
                            if (next_x >= 0 && next_x < width)
                                buf[(y + 1) * width + next_x] += err * 1.0f / 16.0f;
                        }
                    }
                }
            }

            /**
             * @brief Block-parallel Floyd-Steinberg dithering.
             *
             * Splits the image into rectangular blocks and applies serpentine
             * Floyd-Steinberg independently within each block.  This enables
             * multi-threaded execution at the cost of minor discontinuities
             * at block boundaries (typically invisible at terminal resolution).
             *
             * @param gray_in     Input grayscale
             * @param width       Image width
             * @param height      Image height
             * @param out         Output binary image
             * @param block_w     Block width  (0 = auto, ~64px)
             * @param block_h     Block height (0 = auto, ~64px)
             * @param num_threads Thread count  (0 = auto)
             */
            inline void floyd_steinberg_parallel(
                const uint8_t *gray_in, int width, int height, uint8_t *out,
                int block_w = 0, int block_h = 0, int num_threads = 0)
            {
                if (width <= 0 || height <= 0)
                    return;

                if (block_w <= 0)
                    block_w = std::max(32, std::min(64, width));
                if (block_h <= 0)
                    block_h = std::max(32, std::min(64, height));
                if (num_threads <= 0)
                {
                    num_threads = static_cast<int>(std::thread::hardware_concurrency());
                    if (num_threads <= 0)
                        num_threads = 4;
                }

                int blocks_x = (width + block_w - 1) / block_w;
                int blocks_y = (height + block_h - 1) / block_h;
                int total_blocks = blocks_x * blocks_y;

                // Each block gets its own float buffer to avoid races
                auto process_block = [&](int bx, int by)
                {
                    int x0 = bx * block_w;
                    int y0 = by * block_h;
                    int x1 = std::min(x0 + block_w, width);
                    int y1 = std::min(y0 + block_h, height);
                    int bw = x1 - x0;
                    int bh = y1 - y0;

                    std::vector<float> buf(bw * bh);
                    for (int ly = 0; ly < bh; ++ly)
                        for (int lx = 0; lx < bw; ++lx)
                            buf[ly * bw + lx] = static_cast<float>(
                                gray_in[(y0 + ly) * width + (x0 + lx)]);

                    for (int ly = 0; ly < bh; ++ly)
                    {
                        bool ltr = (ly % 2 == 0);
                        int xs = ltr ? 0 : bw - 1;
                        int xe = ltr ? bw : -1;
                        int xd = ltr ? 1 : -1;

                        for (int lx = xs; lx != xe; lx += xd)
                        {
                            int lidx = ly * bw + lx;
                            float old_px = buf[lidx];
                            float new_px = (old_px >= 128.0f) ? 255.0f : 0.0f;
                            buf[lidx] = new_px;
                            out[(y0 + ly) * width + (x0 + lx)] = static_cast<uint8_t>(new_px);

                            float err = old_px - new_px;
                            int nx = lx + xd;
                            int px = lx - xd;

                            if (nx >= 0 && nx < bw)
                                buf[lidx + xd] += err * 7.0f / 16.0f;
                            if (ly + 1 < bh)
                            {
                                if (px >= 0 && px < bw)
                                    buf[(ly + 1) * bw + px] += err * 3.0f / 16.0f;
                                buf[(ly + 1) * bw + lx] += err * 5.0f / 16.0f;
                                if (nx >= 0 && nx < bw)
                                    buf[(ly + 1) * bw + nx] += err * 1.0f / 16.0f;
                            }
                        }
                    }
                };

                // Dispatch blocks to thread pool
                int chunk = (total_blocks + num_threads - 1) / num_threads;
                std::vector<std::thread> threads;
                threads.reserve(num_threads);
                for (int t = 0; t < num_threads; ++t)
                {
                    int start = t * chunk;
                    int end = std::min(start + chunk, total_blocks);
                    if (start >= end)
                        break;
                    threads.emplace_back([&, start, end]()
                                         {
                        for (int i = start; i < end; ++i)
                        {
                            int by = i / blocks_x;
                            int bx = i % blocks_x;
                            process_block(bx, by);
                        } });
                }
                for (auto &t : threads)
                    t.join();
            }

            /**
             * @brief Apply Floyd-Steinberg on an RGB buffer.
             *
             * Internally converts to grayscale first, then dithers.
             *
             * @param rgb_in  Input RGB (3 bytes/pixel, row-major)
             * @param width   Image width
             * @param height  Image height
             * @param out     Output binary image (0 or 255)
             */
            inline void floyd_steinberg_rgb(const uint8_t *rgb_in, int width, int height,
                                            uint8_t *out)
            {
                std::vector<uint8_t> gray(static_cast<size_t>(width) * height);
                for (int i = 0; i < width * height; ++i)
                {
                    const uint8_t *p = rgb_in + i * 3;
                    gray[i] = pixel::to_gray(p[0], p[1], p[2]);
                }
                floyd_steinberg(gray.data(), width, height, out);
            }

        } // namespace dither

        // ==================================================================
        //  Section 3 - Braille cell helpers
        // ==================================================================

        namespace braille
        {
            /**
             * @brief Braille dot-bit lookup table.
             *
             *  Braille layout (Unicode standard):
             *    col 0   col 1
             *    [1]     [4]    row 0  (bits 0, 3)
             *    [2]     [5]    row 1  (bits 1, 4)
             *    [3]     [6]    row 2  (bits 2, 5)
             *    [7]     [8]    row 3  (bits 6, 7)
             *
             *  DOTS[row][col] gives the bit to OR into the pattern.
             */
            static constexpr uint8_t DOTS[4][2] = {
                {0x01, 0x08}, // row 0
                {0x02, 0x10}, // row 1
                {0x04, 0x20}, // row 2
                {0x40, 0x80}  // row 3
            };

            /// Unicode base for braille patterns
            static constexpr uint32_t BASE = 0x2800;

            /**
             * @brief Result of processing one 2x4 braille cell from an image.
             */
            struct CellResult
            {
                uint8_t pattern = 0;      ///< 8-bit braille dot pattern
                uint8_t avg_gray = 0;     ///< average brightness of active dots
                pixel::RGB avg_color;     ///< average colour of active dots
                pixel::RGB avg_all_color; ///< average colour of ALL 8 pixels
                int on_count = 0;         ///< number of lit dots
            };

            /**
             * @brief Process a single 2x4 braille cell from an RGB buffer.
             *
             * This is the CANONICAL cell-extraction routine.  Every rendering
             * mode in the library ultimately calls this (with different flags)
             * rather than duplicating the 2x4 loop.
             *
             * @param rgb_data  Full image RGB buffer (3 bytes/pixel, row-major)
             * @param img_w     Image width in pixels
             * @param img_h     Image height in pixels
             * @param cx        Cell x-index (character column)
             * @param cy        Cell y-index (character row)
             * @param threshold Brightness threshold (0-255) - pixels >= threshold light up
             * @param use_dither  If true, use ordered dithering instead of flat threshold
             * @param flood       If true, light ALL 8 dots regardless of brightness
             * @return CellResult with pattern, avg brightness, avg color, count
             */
            inline CellResult process_cell_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cx, int cy,
                uint8_t threshold = 128,
                bool use_dither = false,
                bool flood = false)
            {
                CellResult res;
                int px = cx * 2;
                int py = cy * 4;

                int r_sum = 0, g_sum = 0, b_sum = 0;    // active-dot colour sum
                int ar_sum = 0, ag_sum = 0, ab_sum = 0; // all-pixel colour sum
                int all_gray_sum = 0;
                int total_pixels = 0;

                for (int row = 0; row < 4; ++row)
                {
                    int y = py + row;
                    if (y >= img_h)
                        continue;
                    for (int col = 0; col < 2; ++col)
                    {
                        int x = px + col;
                        if (x >= img_w)
                            continue;

                        size_t idx = (static_cast<size_t>(y) * img_w + x) * 3;
                        uint8_t r = rgb_data[idx];
                        uint8_t g = rgb_data[idx + 1];
                        uint8_t b = rgb_data[idx + 2];
                        uint8_t gray = pixel::to_gray(r, g, b);

                        ar_sum += r;
                        ag_sum += g;
                        ab_sum += b;
                        all_gray_sum += gray;
                        ++total_pixels;

                        bool lit = false;
                        if (flood)
                        {
                            lit = true;
                        }
                        else if (use_dither)
                        {
                            int didx = row * 2 + col;
                            lit = (gray >= dither::BRAILLE_ORDERED[didx]);
                        }
                        else
                        {
                            lit = (gray >= threshold);
                        }

                        if (lit)
                        {
                            res.pattern |= DOTS[row][col];
                            r_sum += r;
                            g_sum += g;
                            b_sum += b;
                            res.on_count++;
                        }
                    }
                }

                if (res.on_count > 0)
                {
                    res.avg_color = pixel::RGB(
                        static_cast<uint8_t>(r_sum / res.on_count),
                        static_cast<uint8_t>(g_sum / res.on_count),
                        static_cast<uint8_t>(b_sum / res.on_count));
                    res.avg_gray = pixel::to_gray(res.avg_color.r, res.avg_color.g, res.avg_color.b);
                }
                if (total_pixels > 0)
                {
                    res.avg_all_color = pixel::RGB(
                        static_cast<uint8_t>(ar_sum / total_pixels),
                        static_cast<uint8_t>(ag_sum / total_pixels),
                        static_cast<uint8_t>(ab_sum / total_pixels));
                }

                return res;
            }

            /**
             * @brief Process a single 2x4 braille cell from a GRAYSCALE buffer.
             *
             * Same logic as the RGB variant but takes 1-byte-per-pixel input.
             */
            inline CellResult process_cell_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cx, int cy,
                uint8_t threshold = 128,
                bool use_dither = false,
                bool flood = false)
            {
                CellResult res;
                int px = cx * 2;
                int py = cy * 4;
                int gray_sum = 0;
                int all_gray_sum = 0;
                int total_pixels = 0;

                for (int row = 0; row < 4; ++row)
                {
                    int y = py + row;
                    if (y >= img_h)
                        continue;
                    for (int col = 0; col < 2; ++col)
                    {
                        int x = px + col;
                        if (x >= img_w)
                            continue;

                        uint8_t gray = gray_data[y * img_w + x];
                        all_gray_sum += gray;
                        ++total_pixels;

                        bool lit = false;
                        if (flood)
                            lit = true;
                        else if (use_dither)
                            lit = (gray >= dither::BRAILLE_ORDERED[row * 2 + col]);
                        else
                            lit = (gray >= threshold);

                        if (lit)
                        {
                            res.pattern |= DOTS[row][col];
                            gray_sum += gray;
                            res.on_count++;
                        }
                    }
                }

                if (res.on_count > 0)
                {
                    uint8_t avg = static_cast<uint8_t>(gray_sum / res.on_count);
                    res.avg_gray = avg;
                    res.avg_color = pixel::RGB(avg, avg, avg);
                }
                if (total_pixels > 0)
                {
                    uint8_t avg_all = static_cast<uint8_t>(all_gray_sum / total_pixels);
                    res.avg_all_color = pixel::RGB(avg_all, avg_all, avg_all);
                }
                return res;
            }

            /**
             * @brief Process a 2x4 braille cell using 2x2 Bayer dithering with colour.
             *
             * Used by the colored_dithered mode.  Each of the 4 rows is split
             * into 2 columns; within each 2x2 sub-block the Bayer matrix
             * decides the threshold.
             */
            inline CellResult process_cell_rgb_bayer(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cx, int cy)
            {
                CellResult res;
                int px = cx * 2;
                int py = cy * 4;
                int r_sum = 0, g_sum = 0, b_sum = 0;

                for (int row = 0; row < 4; ++row)
                {
                    int y = py + row;
                    if (y >= img_h)
                        continue;
                    for (int col = 0; col < 2; ++col)
                    {
                        int x = px + col;
                        if (x >= img_w)
                            continue;

                        size_t idx = (static_cast<size_t>(y) * img_w + x) * 3;
                        uint8_t r = rgb_data[idx];
                        uint8_t g = rgb_data[idx + 1];
                        uint8_t b = rgb_data[idx + 2];
                        uint8_t gray = pixel::to_gray(r, g, b);

                        int bayer_thresh = dither::BAYER_2x2[row % 2][col % 2];
                        if (gray >= bayer_thresh)
                        {
                            res.pattern |= DOTS[row][col];
                            r_sum += r;
                            g_sum += g;
                            b_sum += b;
                            res.on_count++;
                        }
                    }
                }

                if (res.on_count > 0)
                {
                    res.avg_color = pixel::RGB(
                        static_cast<uint8_t>(r_sum / res.on_count),
                        static_cast<uint8_t>(g_sum / res.on_count),
                        static_cast<uint8_t>(b_sum / res.on_count));
                    res.avg_gray = pixel::to_gray(res.avg_color.r, res.avg_color.g, res.avg_color.b);
                }
                return res;
            }

        } // namespace braille

        // ==================================================================
        //  Section 4 - Half-block cell helpers  (U+2580)
        // ==================================================================

        namespace halfblock
        {
            /**
             * @brief Result of processing one half-block cell (2 vertical pixels).
             */
            struct CellResult
            {
                pixel::RGB top;
                pixel::RGB bottom;
                uint8_t top_gray = 0;
                uint8_t bottom_gray = 0;
            };

            /**
             * @brief Process a single half-block cell from an RGB buffer.
             *
             * Each character cell represents 2 vertically stacked pixels.
             * The foreground colour is the top pixel, the background is the bottom.
             *
             * @param rgb_data  Full image RGB buffer (3 bytes/pixel, row-major)
             * @param img_w     Image width in pixels
             * @param img_h     Image height in pixels
             * @param cx        Cell x (character column, == pixel x)
             * @param cy        Cell y (character row; top pixel = cy*2, bottom = cy*2+1)
             */
            inline CellResult process_cell_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cx, int cy)
            {
                CellResult res;
                int top_y = cy * 2;
                int bot_y = cy * 2 + 1;

                if (cx < img_w && top_y < img_h)
                {
                    size_t idx = (static_cast<size_t>(top_y) * img_w + cx) * 3;
                    res.top = pixel::RGB(rgb_data[idx], rgb_data[idx + 1], rgb_data[idx + 2]);
                    res.top_gray = pixel::to_gray(res.top.r, res.top.g, res.top.b);
                }
                if (cx < img_w && bot_y < img_h)
                {
                    size_t idx = (static_cast<size_t>(bot_y) * img_w + cx) * 3;
                    res.bottom = pixel::RGB(rgb_data[idx], rgb_data[idx + 1], rgb_data[idx + 2]);
                    res.bottom_gray = pixel::to_gray(res.bottom.r, res.bottom.g, res.bottom.b);
                }
                return res;
            }

            /**
             * @brief Process a single half-block cell from a grayscale buffer.
             */
            inline CellResult process_cell_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cx, int cy)
            {
                CellResult res;
                int top_y = cy * 2;
                int bot_y = cy * 2 + 1;

                if (cx < img_w && top_y < img_h)
                {
                    uint8_t g = gray_data[top_y * img_w + cx];
                    res.top = pixel::RGB(g, g, g);
                    res.top_gray = g;
                }
                if (cx < img_w && bot_y < img_h)
                {
                    uint8_t g = gray_data[bot_y * img_w + cx];
                    res.bottom = pixel::RGB(g, g, g);
                    res.bottom_gray = g;
                }
                return res;
            }

        } // namespace halfblock

        // ==================================================================
        //  Section 5 - Bulk image-processing functions (CPU, multi-threaded)
        // ==================================================================

        namespace processing
        {
            /**
             * @brief Convert an entire RGB image to grayscale (multi-threaded).
             */
            inline void rgb_to_grayscale(const uint8_t *rgb, int width, int height,
                                         uint8_t *out,
                                         int num_threads = 0)
            {
                if (num_threads <= 0)
                {
                    num_threads = static_cast<int>(std::thread::hardware_concurrency());
                    if (num_threads <= 0)
                        num_threads = 4;
                }

                const int total = width * height;
                const int chunk = (total + num_threads - 1) / num_threads;

                std::vector<std::thread> threads;
                threads.reserve(num_threads);
                for (int t = 0; t < num_threads; ++t)
                {
                    int start = t * chunk;
                    int end = std::min(start + chunk, total);
                    if (start >= end)
                        break;
                    threads.emplace_back([rgb, out, start, end]()
                                         {
                        for (int i = start; i < end; ++i)
                        {
                            const uint8_t *p = rgb + i * 3;
                            out[i] = pixel::to_gray(p[0], p[1], p[2]);
                        } });
                }
                for (auto &t : threads)
                    t.join();
            }

            /**
             * @brief Process all braille cells for an image (multi-threaded).
             *
             * Fills a row-major array of braille::CellResult, one per character
             * cell.  The caller chooses threshold / dither / flood to select
             * the desired rendering mode.
             *
             * @param rgb_data   Full image (3 bytes/pixel, row-major)
             * @param img_w      Image width in pixels
             * @param img_h      Image height in pixels
             * @param cells_w    Number of character columns  (img_w / 2, rounded up)
             * @param cells_h    Number of character rows     (img_h / 4, rounded up)
             * @param out        Output array of CellResult   (size = cells_w * cells_h)
             * @param threshold  Brightness threshold (ignored if use_dither or flood)
             * @param use_dither Use ordered dithering
             * @param flood      Light all 8 dots
             * @param num_threads  Thread count (0 = auto)
             */
            inline void process_braille_cells_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h,
                braille::CellResult *out,
                uint8_t threshold = 128,
                bool use_dither = false,
                bool flood = false,
                int num_threads = 0)
            {
                if (num_threads <= 0)
                {
                    num_threads = static_cast<int>(std::thread::hardware_concurrency());
                    if (num_threads <= 0)
                        num_threads = 4;
                }

                const int total_cells = cells_w * cells_h;
                const int chunk = (total_cells + num_threads - 1) / num_threads;

                std::vector<std::thread> threads;
                threads.reserve(num_threads);
                for (int t = 0; t < num_threads; ++t)
                {
                    int start = t * chunk;
                    int end = std::min(start + chunk, total_cells);
                    if (start >= end)
                        break;
                    threads.emplace_back([=]()
                                         {
                        for (int i = start; i < end; ++i)
                        {
                            int cy = i / cells_w;
                            int cx = i % cells_w;
                            out[i] = braille::process_cell_rgb(
                                rgb_data, img_w, img_h, cx, cy,
                                threshold, use_dither, flood);
                        } });
                }
                for (auto &t : threads)
                    t.join();
            }

            /**
             * @brief Process all braille cells for a grayscale image.
             */
            inline void process_braille_cells_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cells_w, int cells_h,
                braille::CellResult *out,
                uint8_t threshold = 128,
                bool use_dither = false,
                bool flood = false,
                int num_threads = 0)
            {
                if (num_threads <= 0)
                {
                    num_threads = static_cast<int>(std::thread::hardware_concurrency());
                    if (num_threads <= 0)
                        num_threads = 4;
                }

                const int total_cells = cells_w * cells_h;
                const int chunk = (total_cells + num_threads - 1) / num_threads;

                std::vector<std::thread> threads;
                threads.reserve(num_threads);
                for (int t = 0; t < num_threads; ++t)
                {
                    int start = t * chunk;
                    int end = std::min(start + chunk, total_cells);
                    if (start >= end)
                        break;
                    threads.emplace_back([=]()
                                         {
                        for (int i = start; i < end; ++i)
                        {
                            int cy = i / cells_w;
                            int cx = i % cells_w;
                            out[i] = braille::process_cell_gray(
                                gray_data, img_w, img_h, cx, cy,
                                threshold, use_dither, flood);
                        } });
                }
                for (auto &t : threads)
                    t.join();
            }

            /**
             * @brief Process all braille cells using 2x2 Bayer dithering (colored_dithered mode).
             */
            inline void process_braille_cells_bayer(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h,
                braille::CellResult *out,
                int num_threads = 0)
            {
                if (num_threads <= 0)
                {
                    num_threads = static_cast<int>(std::thread::hardware_concurrency());
                    if (num_threads <= 0)
                        num_threads = 4;
                }

                const int total_cells = cells_w * cells_h;
                const int chunk = (total_cells + num_threads - 1) / num_threads;

                std::vector<std::thread> threads;
                threads.reserve(num_threads);
                for (int t = 0; t < num_threads; ++t)
                {
                    int start = t * chunk;
                    int end = std::min(start + chunk, total_cells);
                    if (start >= end)
                        break;
                    threads.emplace_back([=]()
                                         {
                        for (int i = start; i < end; ++i)
                        {
                            int cy = i / cells_w;
                            int cx = i % cells_w;
                            out[i] = braille::process_cell_rgb_bayer(
                                rgb_data, img_w, img_h, cx, cy);
                        } });
                }
                for (auto &t : threads)
                    t.join();
            }

            /**
             * @brief Process all half-block cells for an RGB image (multi-threaded).
             */
            inline void process_halfblock_cells_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h,
                halfblock::CellResult *out,
                int num_threads = 0)
            {
                if (num_threads <= 0)
                {
                    num_threads = static_cast<int>(std::thread::hardware_concurrency());
                    if (num_threads <= 0)
                        num_threads = 4;
                }

                const int total_cells = cells_w * cells_h;
                const int chunk = (total_cells + num_threads - 1) / num_threads;

                std::vector<std::thread> threads;
                threads.reserve(num_threads);
                for (int t = 0; t < num_threads; ++t)
                {
                    int start = t * chunk;
                    int end = std::min(start + chunk, total_cells);
                    if (start >= end)
                        break;
                    threads.emplace_back([=]()
                                         {
                        for (int i = start; i < end; ++i)
                        {
                            int cy = i / cells_w;
                            int cx = i % cells_w;
                            out[i] = halfblock::process_cell_rgb(
                                rgb_data, img_w, img_h, cx, cy);
                        } });
                }
                for (auto &t : threads)
                    t.join();
            }

            /**
             * @brief Process all half-block cells for a grayscale image.
             */
            inline void process_halfblock_cells_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cells_w, int cells_h,
                halfblock::CellResult *out,
                int num_threads = 0)
            {
                if (num_threads <= 0)
                {
                    num_threads = static_cast<int>(std::thread::hardware_concurrency());
                    if (num_threads <= 0)
                        num_threads = 4;
                }

                const int total_cells = cells_w * cells_h;
                const int chunk = (total_cells + num_threads - 1) / num_threads;

                std::vector<std::thread> threads;
                threads.reserve(num_threads);
                for (int t = 0; t < num_threads; ++t)
                {
                    int start = t * chunk;
                    int end = std::min(start + chunk, total_cells);
                    if (start >= end)
                        break;
                    threads.emplace_back([=]()
                                         {
                        for (int i = start; i < end; ++i)
                        {
                            int cy = i / cells_w;
                            int cx = i % cells_w;
                            out[i] = halfblock::process_cell_gray(
                                gray_data, img_w, img_h, cx, cy);
                        } });
                }
                for (auto &t : threads)
                    t.join();
            }

            /**
             * @brief Process a batch of RGB images to grayscale (multi-threaded).
             */
            inline void rgb_to_grayscale_batch(
                const std::vector<const uint8_t *> &inputs,
                const std::vector<int> &widths,
                const std::vector<int> &heights,
                std::vector<uint8_t *> &outputs)
            {
                for (size_t i = 0; i < inputs.size(); ++i)
                    rgb_to_grayscale(inputs[i], widths[i], heights[i], outputs[i]);
            }

        } // namespace processing

        // ==================================================================
        //  Section 6 - Image I/O  (PPM / PGM parser, ImageMagick wrapper)
        // ==================================================================

        namespace image_io
        {
            /**
             * @brief Parsed image data from a PPM/PGM file.
             */
            struct ImageData
            {
                int width = 0;
                int height = 0;
                bool is_color = false;     ///< true = PPM (P6, RGB), false = PGM (P5, gray)
                std::vector<uint8_t> data; ///< pixel data (3 bytes/pixel if color, 1 byte if gray)

                bool valid() const { return width > 0 && height > 0 && !data.empty(); }

                /// Convert colour data to grayscale in-place (or no-op if already gray)
                void to_grayscale()
                {
                    if (!is_color || data.empty())
                        return;
                    std::vector<uint8_t> gray(static_cast<size_t>(width) * height);
                    for (int i = 0; i < width * height; ++i)
                    {
                        const uint8_t *p = data.data() + i * 3;
                        gray[i] = pixel::to_gray(p[0], p[1], p[2]);
                    }
                    data = std::move(gray);
                    is_color = false;
                }
            };

            /**
             * @brief Parse a PPM (P6) or PGM (P5) file.
             *
             * This is the ONE canonical PPM/PGM parser.  All other code in the
             * library must call this instead of reimplementing the header logic.
             *
             * @param filename  Path to .ppm or .pgm file
             * @return ImageData  (check .valid())
             */
            inline ImageData load_ppm_pgm(const std::string &filename)
            {
                ImageData img;
                std::ifstream file(filename, std::ios::binary);
                if (!file)
                    return img;

                std::string magic;
                file >> magic;
                if (magic != "P5" && magic != "P6")
                    return img;

                img.is_color = (magic == "P6");

                // Skip comments
                char c;
                file.get(c);
                while (file.peek() == '#')
                {
                    std::string comment;
                    std::getline(file, comment);
                }

                int maxval;
                file >> img.width >> img.height >> maxval;
                file.get(c); // Skip one whitespace byte after header

                if (maxval > 255 || img.width <= 0 || img.height <= 0)
                {
                    img.width = img.height = 0;
                    return img;
                }

                size_t bytes_per_pixel = img.is_color ? 3 : 1;
                size_t data_size = static_cast<size_t>(img.width) * img.height * bytes_per_pixel;
                img.data.resize(data_size);
                file.read(reinterpret_cast<char *>(img.data.data()), data_size);

                if (static_cast<size_t>(file.gcount()) != data_size)
                {
                    img.width = img.height = 0;
                    img.data.clear();
                }

                return img;
            }

            /**
             * @brief Convert an image file to PPM using ImageMagick.
             *
             * @param input_path   Source image (any format ImageMagick supports)
             * @param output_path  Destination .ppm file
             * @param resize_width If > 0, resize to this width (maintaining aspect ratio)
             * @return true on success
             */
            inline bool convert_to_ppm(const std::string &input_path,
                                       const std::string &output_path,
                                       int resize_width = 0)
            {
                std::string cmd = "convert \"" + input_path + "\"";
                if (resize_width > 0)
                    cmd += " -resize " + std::to_string(resize_width) + "x";
                cmd += " \"" + output_path + "\"";
#ifdef _WIN32
                cmd += " >nul 2>&1";
#else
                cmd += " >/dev/null 2>&1";
#endif
                return std::system(cmd.c_str()) == 0;
            }

            /**
             * @brief Load any image via ImageMagick -> PPM pipeline.
             *
             * @param filepath     Path to any image
             * @param resize_width Resize width (0 = no resize)
             * @return ImageData (check .valid())
             */
            inline ImageData load_image(const std::string &filepath, int resize_width = 0)
            {
                // Try direct PPM/PGM load first
                {
                    ImageData direct = load_ppm_pgm(filepath);
                    if (direct.valid())
                        return direct;
                }

                // Otherwise, convert via ImageMagick
                std::string tmp_ppm = "/tmp/pythonic_accel_tmp_" +
                                      std::to_string(std::hash<std::string>{}(filepath)) + ".ppm";
                if (!convert_to_ppm(filepath, tmp_ppm, resize_width))
                    return ImageData{};

                ImageData img = load_ppm_pgm(tmp_ppm);
                std::remove(tmp_ppm.c_str());
                return img;
            }

            /**
             * @brief Write raw RGB/Grayscale data as a PPM/PGM file.
             *
             * @param filepath   Destination file path
             * @param data       Pixel data (RGB or grayscale)
             * @param width      Image width
             * @param height     Image height
             * @param is_color   true = P6 (RGB), false = P5 (grayscale)
             * @return true on success
             */
            inline bool write_ppm(const std::string &filepath,
                                  const uint8_t *data, int width, int height,
                                  bool is_color = true)
            {
                std::ofstream f(filepath, std::ios::binary);
                if (!f.is_open())
                    return false;

                if (is_color)
                    f << "P6\n"
                      << width << " " << height << "\n255\n";
                else
                    f << "P5\n"
                      << width << " " << height << "\n255\n";

                size_t bytes = static_cast<size_t>(width) * height * (is_color ? 3 : 1);
                f.write(reinterpret_cast<const char *>(data), bytes);
                return f.good();
            }

            /**
             * @brief Convert PPM to PNG using ImageMagick.
             *
             * @param ppm_path   Source PPM file
             * @param png_path   Destination PNG file
             * @return true on success
             */
            inline bool convert_ppm_to_png(const std::string &ppm_path,
                                           const std::string &png_path)
            {
                std::string cmd = "convert \"" + ppm_path + "\" \"" + png_path + "\"";
#ifdef _WIN32
                cmd += " >nul 2>&1";
#else
                cmd += " >/dev/null 2>&1";
#endif
                return std::system(cmd.c_str()) == 0;
            }

        } // namespace image_io

        // ==================================================================
        //  Section 6b - Text/Unicode utility helpers
        // ==================================================================

        namespace text_util
        {
            /**
             * @brief Check if a Unicode codepoint is a braille character.
             * Braille patterns occupy U+2800 – U+28FF.
             */
            inline bool is_braille_char(uint32_t codepoint)
            {
                return codepoint >= 0x2800 && codepoint <= 0x28FF;
            }

            /**
             * @brief Check if a Unicode codepoint is a block character.
             * Block elements occupy U+2580 – U+259F.
             */
            inline bool is_block_char(uint32_t codepoint)
            {
                return codepoint >= 0x2580 && codepoint <= 0x259F;
            }

            /**
             * @brief Decode one UTF-8 character from a byte iterator.
             *
             * @param it   Iterator pointing to the start of a UTF-8 sequence.
             *             Advanced past the decoded character on return.
             * @param end  End iterator (for bounds checking).
             * @return Decoded Unicode codepoint, or 0xFFFD (replacement) on error.
             */
            inline uint32_t decode_utf8(std::string::const_iterator &it,
                                        std::string::const_iterator end)
            {
                if (it == end)
                    return 0xFFFD;
                uint8_t c = static_cast<uint8_t>(*it);

                uint32_t cp = 0;
                int extra = 0;

                if (c < 0x80)
                {
                    cp = c;
                    extra = 0;
                }
                else if ((c & 0xE0) == 0xC0)
                {
                    cp = c & 0x1F;
                    extra = 1;
                }
                else if ((c & 0xF0) == 0xE0)
                {
                    cp = c & 0x0F;
                    extra = 2;
                }
                else if ((c & 0xF8) == 0xF0)
                {
                    cp = c & 0x07;
                    extra = 3;
                }
                else
                {
                    ++it;
                    return 0xFFFD;
                }

                ++it;
                for (int i = 0; i < extra; ++i)
                {
                    if (it == end || (static_cast<uint8_t>(*it) & 0xC0) != 0x80)
                        return 0xFFFD;
                    cp = (cp << 6) | (static_cast<uint8_t>(*it) & 0x3F);
                    ++it;
                }
                return cp;
            }

            /**
             * @brief Strip ANSI escape sequences from a string.
             *
             * Removes all CSI sequences (\033[...m, \033[...H, etc.) and
             * OSC sequences (\033]...ST).
             */
            inline std::string strip_ansi(const std::string &input)
            {
                std::string result;
                result.reserve(input.size());

                for (size_t i = 0; i < input.size(); ++i)
                {
                    if (input[i] == '\033')
                    {
                        ++i;
                        if (i < input.size() && input[i] == '[')
                        {
                            // CSI sequence: skip until final byte (0x40-0x7E)
                            ++i;
                            while (i < input.size() &&
                                   (input[i] < 0x40 || input[i] > 0x7E))
                                ++i;
                            // Skip the final byte too
                        }
                        else if (i < input.size() && input[i] == ']')
                        {
                            // OSC sequence: skip until ST (\033\\) or BEL (\007)
                            ++i;
                            while (i < input.size())
                            {
                                if (input[i] == '\007')
                                    break;
                                if (input[i] == '\033' && i + 1 < input.size() && input[i + 1] == '\\')
                                {
                                    ++i;
                                    break;
                                }
                                ++i;
                            }
                        }
                    }
                    else
                    {
                        result += input[i];
                    }
                }
                return result;
            }

            /**
             * @brief Inverse braille dot lookup: bit index → (row, col) in 2×4 grid.
             *
             * Given a braille pattern byte and dot index, returns the pixel
             * offset within the 2×4 cell.  Useful for rendering braille
             * patterns back to pixel grids.
             *
             * Bit layout (standard Unicode braille):
             *   bit 0 → (0,0)   bit 3 → (0,1)
             *   bit 1 → (1,0)   bit 4 → (1,1)
             *   bit 2 → (2,0)   bit 5 → (2,1)
             *   bit 6 → (3,0)   bit 7 → (3,1)
             */
            struct BrailleDotPos
            {
                int row, col;
            };

            inline constexpr BrailleDotPos BRAILLE_BIT_TO_POS[8] = {
                {0, 0}, {1, 0}, {2, 0}, {0, 1}, {1, 1}, {2, 1}, {3, 0}, {3, 1}};

        } // namespace text_util

        // ==================================================================
        //  Section 7 - Video processing helpers (FFmpeg)
        // ==================================================================

        namespace video
        {
            /**
             * @brief Video metadata from ffprobe.
             */
            struct VideoInfo
            {
                int width = 0;
                int height = 0;
                double fps = 0.0;
                double duration = 0.0; ///< seconds
                size_t estimated_frames = 0;
                bool has_audio = false;
                std::string codec;
            };

            namespace detail
            {
                inline std::string exec_command(const std::string &cmd)
                {
                    std::string result;
#ifdef _WIN32
                    FILE *pipe = _popen(cmd.c_str(), "r");
#else
                    FILE *pipe = popen(cmd.c_str(), "r");
#endif
                    if (!pipe)
                        return "";
                    char buffer[256];
                    while (fgets(buffer, sizeof(buffer), pipe))
                        result += buffer;
#ifdef _WIN32
                    _pclose(pipe);
#else
                    pclose(pipe);
#endif
                    return result;
                }

                inline bool command_exists(const std::string &cmd)
                {
#ifdef _WIN32
                    std::string check = "where " + cmd + " >nul 2>&1";
#else
                    std::string check = "which " + cmd + " >/dev/null 2>&1";
#endif
                    return std::system(check.c_str()) == 0;
                }
            } // namespace detail

            /**
             * @brief Probe a video file for metadata using ffprobe.
             */
            inline VideoInfo probe(const std::string &filepath)
            {
                VideoInfo info;
                if (!detail::command_exists("ffprobe"))
                    return info;

                // Get width, height, fps, codec using key=value format for reliable parsing
                std::string cmd =
                    "ffprobe -v error -select_streams v:0 "
                    "-show_entries stream=width,height,r_frame_rate,codec_name "
                    "-of default=noprint_wrappers=1:nokey=0 \"" +
                    filepath + "\" 2>/dev/null";
                std::string result = detail::exec_command(cmd);

                if (!result.empty())
                {
                    std::istringstream stream(result);
                    std::string kv_line;
                    while (std::getline(stream, kv_line))
                    {
                        auto eq = kv_line.find('=');
                        if (eq == std::string::npos)
                            continue;
                        std::string key = kv_line.substr(0, eq);
                        std::string val = kv_line.substr(eq + 1);
                        // Trim whitespace
                        val.erase(0, val.find_first_not_of(" \t\r\n"));
                        val.erase(val.find_last_not_of(" \t\r\n") + 1);

                        try
                        {
                            if (key == "width")
                                info.width = std::stoi(val);
                            else if (key == "height")
                                info.height = std::stoi(val);
                            else if (key == "r_frame_rate")
                            {
                                auto slash = val.find('/');
                                if (slash != std::string::npos)
                                {
                                    double num = std::stod(val.substr(0, slash));
                                    double den = std::stod(val.substr(slash + 1));
                                    if (den > 0)
                                        info.fps = num / den;
                                }
                                else
                                {
                                    info.fps = std::stod(val);
                                }
                            }
                            else if (key == "codec_name")
                                info.codec = val;
                        }
                        catch (...)
                        {
                        }
                    }
                }

                // Get duration separately (more reliable)
                {
                    std::string dur_cmd =
                        "ffprobe -v error -show_entries format=duration "
                        "-of default=noprint_wrappers=1:nokey=1 \"" +
                        filepath + "\" 2>/dev/null";
                    std::string dur_str = detail::exec_command(dur_cmd);
                    try
                    {
                        info.duration = std::stod(dur_str);
                    }
                    catch (...)
                    {
                    }
                }

                if (info.fps > 0 && info.duration > 0)
                    info.estimated_frames = static_cast<size_t>(info.fps * info.duration);

                // Check for audio stream
                std::string audio_cmd =
                    "ffprobe -v error -select_streams a:0 "
                    "-show_entries stream=codec_name -of csv=p=0 \"" +
                    filepath + "\" 2>/dev/null";
                std::string audio_result = detail::exec_command(audio_cmd);
                info.has_audio = !audio_result.empty() &&
                                 audio_result.find_first_not_of(" \t\r\n") != std::string::npos;

                return info;
            }

            /**
             * @brief Get video duration in seconds.
             */
            inline double get_duration(const std::string &filepath)
            {
                if (!detail::command_exists("ffprobe"))
                    return 0.0;
                std::string cmd =
                    "ffprobe -v error -show_entries format=duration "
                    "-of default=noprint_wrappers=1:nokey=1 \"" +
                    filepath + "\" 2>/dev/null";
                std::string result = detail::exec_command(cmd);
                try
                {
                    return std::stod(result);
                }
                catch (...)
                {
                    return 0.0;
                }
            }

            /**
             * @brief Get video FPS.
             */
            inline double get_fps(const std::string &filepath)
            {
                if (!detail::command_exists("ffprobe"))
                    return 0.0;
                std::string cmd =
                    "ffprobe -v error -select_streams v:0 "
                    "-show_entries stream=r_frame_rate "
                    "-of default=noprint_wrappers=1:nokey=1 \"" +
                    filepath + "\" 2>/dev/null";
                std::string result = detail::exec_command(cmd);
                auto slash = result.find('/');
                if (slash != std::string::npos)
                {
                    try
                    {
                        double num = std::stod(result.substr(0, slash));
                        double den = std::stod(result.substr(slash + 1));
                        return (den > 0) ? num / den : 0.0;
                    }
                    catch (...)
                    {
                        return 0.0;
                    }
                }
                try
                {
                    return std::stod(result);
                }
                catch (...)
                {
                    return 0.0;
                }
            }

            /**
             * @brief Estimate frame count from duration and fps.
             */
            inline size_t estimate_frame_count(const std::string &filepath, double fps = 0.0)
            {
                double dur = get_duration(filepath);
                if (fps <= 0.0)
                    fps = get_fps(filepath);
                if (dur > 0 && fps > 0)
                    return static_cast<size_t>(dur * fps);
                return 0;
            }

            /**
             * @brief Extract video frames to a directory using FFmpeg.
             *
             * @param input_path   Source video
             * @param output_dir   Destination directory (must exist)
             * @param fps          Extraction FPS (0 = original)
             * @param start_time   Start time in seconds (-1 = beginning)
             * @param end_time     End time in seconds (-1 = end)
             * @param progress_cb  Optional callback(current_frame, total_estimate)
             * @return true on success
             */
            inline bool extract_frames(
                const std::string &input_path,
                const std::string &output_dir,
                double fps = 0.0,
                double start_time = -1.0,
                double end_time = -1.0,
                std::function<void(size_t, size_t)> progress_cb = nullptr)
            {
                if (!detail::command_exists("ffmpeg"))
                    return false;

                std::string cmd = "ffmpeg -y ";
                if (start_time >= 0)
                    cmd += "-ss " + std::to_string(start_time) + " ";
                cmd += "-i \"" + input_path + "\" ";
                if (end_time >= 0 && start_time >= 0)
                    cmd += "-t " + std::to_string(end_time - start_time) + " ";
                if (fps > 0)
                    cmd += "-vf \"fps=" + std::to_string(fps) + "\" ";

                cmd += "\"" + output_dir + "/frame_%05d.png\"";
#ifdef _WIN32
                cmd += " >nul 2>&1";
#else
                cmd += " >/dev/null 2>&1";
#endif

                if (progress_cb)
                {
                    std::string bg_cmd = cmd;
#ifndef _WIN32
                    auto pos = bg_cmd.rfind(">/dev/null 2>&1");
                    if (pos != std::string::npos)
                        bg_cmd.replace(pos, std::string(">/dev/null 2>&1").size(),
                                       ">/dev/null 2>&1 &");
#endif
                    std::system(bg_cmd.c_str());

                    size_t total_estimate = estimate_frame_count(input_path, fps);
                    size_t last_count = 0;
                    while (true)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        size_t count = 0;
                        std::string count_cmd = "ls \"" + output_dir + "\"/frame_*.png 2>/dev/null | wc -l";
                        std::string count_str = detail::exec_command(count_cmd);
                        try
                        {
                            count = std::stoul(count_str);
                        }
                        catch (...)
                        {
                        }

                        if (count > last_count)
                        {
                            progress_cb(count, total_estimate);
                            last_count = count;
                        }

                        std::string check = "pgrep -f 'ffmpeg.*" + input_path + "' >/dev/null 2>&1";
                        if (std::system(check.c_str()) != 0)
                            break;
                    }
                    size_t final_count = 0;
                    std::string fc_cmd = "ls \"" + output_dir + "\"/frame_*.png 2>/dev/null | wc -l";
                    std::string fc_str = detail::exec_command(fc_cmd);
                    try
                    {
                        final_count = std::stoul(fc_str);
                    }
                    catch (...)
                    {
                    }
                    if (final_count > 0)
                        progress_cb(final_count, final_count);
                    return final_count > 0;
                }

                return std::system(cmd.c_str()) == 0;
            }

            /**
             * @brief Encode frames from a directory into a video file.
             *
             * @param frame_dir    Directory containing frame_NNNNN.png files
             * @param output_path  Output video file path
             * @param fps          Output FPS
             * @param encoder      Encoder name (e.g. "libx264", "h264_nvenc")
             * @param audio_path   Optional audio file to mux in
             * @return true on success
             */
            inline bool encode_video(
                const std::string &frame_dir,
                const std::string &output_path,
                double fps = 30.0,
                const std::string &encoder = "libx264",
                const std::string &audio_path = "",
                const std::string &frame_pattern = "frame_%05d.png")
            {
                if (!detail::command_exists("ffmpeg"))
                    return false;

                std::string cmd = "ffmpeg -y -framerate " + std::to_string(fps) +
                                  " -i \"" + frame_dir + "/" + frame_pattern + "\" ";
                if (!audio_path.empty())
                    cmd += "-i \"" + audio_path + "\" ";
                cmd += "-c:v " + encoder + " -pix_fmt yuv420p ";
                if (!audio_path.empty())
                    cmd += "-c:a aac -shortest ";
                cmd += "\"" + output_path + "\"";
#ifdef _WIN32
                cmd += " >nul 2>&1";
#else
                cmd += " >/dev/null 2>&1";
#endif
                return std::system(cmd.c_str()) == 0;
            }

            /**
             * @brief Extract audio from a video file.
             */
            inline bool extract_audio(
                const std::string &video_path,
                const std::string &audio_path)
            {
                if (!detail::command_exists("ffmpeg"))
                    return false;
                std::string cmd = "ffmpeg -y -i \"" + video_path +
                                  "\" -vn -acodec aac \"" + audio_path + "\"";
#ifdef _WIN32
                cmd += " >nul 2>&1";
#else
                cmd += " >/dev/null 2>&1";
#endif
                return std::system(cmd.c_str()) == 0;
            }

            /**
             * @brief Extract a single frame from a video as PPM/PNG.
             */
            inline bool extract_single_frame(
                const std::string &video_path,
                const std::string &output_path,
                double timestamp = 0.0,
                int width = 0)
            {
                if (!detail::command_exists("ffmpeg"))
                    return false;
                std::string cmd = "ffmpeg -y -ss " + std::to_string(timestamp) +
                                  " -i \"" + video_path + "\" -vframes 1 ";
                if (width > 0)
                    cmd += "-vf \"scale=" + std::to_string(width) + ":-1\" ";
                cmd += "\"" + output_path + "\"";
#ifdef _WIN32
                cmd += " >nul 2>&1";
#else
                cmd += " >/dev/null 2>&1";
#endif
                return std::system(cmd.c_str()) == 0;
            }

            /**
             * @brief Open an FFmpeg pipe for streaming decoded frames.
             *
             * Returns a FILE* pipe that yields raw pixel data.
             * Caller must call close_decode_pipe() when done.
             *
             * @param video_path   Source video
             * @param pix_fmt      Pixel format ("rgb24", "gray", etc.)
             * @param width        Output width (0 = original)
             * @param height       Output height (0 = original)
             * @param fps          Target FPS (0 = original)
             * @param start_time   Start offset in seconds (-1 = beginning)
             * @param end_time     End offset in seconds (-1 = end)
             * @return FILE* pipe, or nullptr on failure
             */
            inline FILE *open_decode_pipe(
                const std::string &video_path,
                const std::string &pix_fmt = "rgb24",
                int width = 0, int height = 0,
                double fps = 0.0,
                double start_time = -1.0,
                double end_time = -1.0)
            {
                std::string cmd = "ffmpeg ";
                if (start_time >= 0)
                    cmd += "-ss " + std::to_string(start_time) + " ";
                cmd += "-i \"" + video_path + "\" ";
                if (end_time >= 0 && start_time >= 0)
                    cmd += "-t " + std::to_string(end_time - start_time) + " ";

                std::string vf;
                if (fps > 0)
                    vf += "fps=" + std::to_string(fps);
                if (width > 0 && height > 0)
                {
                    if (!vf.empty())
                        vf += ",";
                    vf += "scale=" + std::to_string(width) + ":" + std::to_string(height);
                }
                else if (width > 0)
                {
                    if (!vf.empty())
                        vf += ",";
                    vf += "scale=" + std::to_string(width) + ":-1";
                }

                if (!vf.empty())
                    cmd += "-vf \"" + vf + "\" ";

                cmd += "-f rawvideo -pix_fmt " + pix_fmt + " pipe:1 2>/dev/null";

#ifdef _WIN32
                return _popen(cmd.c_str(), "rb");
#else
                return popen(cmd.c_str(), "r");
#endif
            }

            /**
             * @brief Close a decode pipe.
             */
            inline void close_decode_pipe(FILE *pipe)
            {
                if (pipe)
                {
#ifdef _WIN32
                    _pclose(pipe);
#else
                    pclose(pipe);
#endif
                }
            }

            /**
             * @brief Open an FFmpeg pipe for encoding (writing raw frames in).
             */
            inline FILE *open_encode_pipe(
                const std::string &output_path,
                const std::string &pix_fmt,
                int width, int height,
                double fps = 30.0,
                const std::string &encoder = "libx264")
            {
                std::string cmd = "ffmpeg -y "
                                  "-f rawvideo -pix_fmt " +
                                  pix_fmt +
                                  " -s " + std::to_string(width) + "x" + std::to_string(height) +
                                  " -r " + std::to_string(fps) +
                                  " -i pipe:0 "
                                  "-c:v " +
                                  encoder + " -pix_fmt yuv420p "
                                            "\"" +
                                  output_path + "\" 2>/dev/null";

#ifdef _WIN32
                return _popen(cmd.c_str(), "wb");
#else
                return popen(cmd.c_str(), "w");
#endif
            }

            /**
             * @brief Close an encode pipe.
             */
            inline void close_encode_pipe(FILE *pipe)
            {
                if (pipe)
                {
#ifdef _WIN32
                    _pclose(pipe);
#else
                    pclose(pipe);
#endif
                }
            }

        } // namespace video

        // ==================================================================
        //  Section 8 - GPU Information & Detection
        // ==================================================================

        enum class GPUVendor
        {
            unknown,
            nvidia,
            amd,
            intel,
            apple,
            cpu
        };

        inline std::string vendor_name(GPUVendor vendor)
        {
            switch (vendor)
            {
            case GPUVendor::nvidia:
                return "NVIDIA";
            case GPUVendor::amd:
                return "AMD";
            case GPUVendor::intel:
                return "Intel";
            case GPUVendor::apple:
                return "Apple";
            case GPUVendor::cpu:
                return "CPU";
            default:
                return "Unknown";
            }
        }

        struct GPUInfo
        {
            GPUVendor vendor = GPUVendor::unknown;
            std::string name;
            std::string driver_version;
            size_t memory_mb = 0;
            int compute_units = 0;
            bool supports_opencl = false;
            bool supports_cuda = false;
            bool supports_metal = false;

            bool is_available() const
            {
                return vendor != GPUVendor::unknown && vendor != GPUVendor::cpu;
            }

            std::string to_string() const
            {
                std::ostringstream oss;
                oss << vendor_name(vendor);
                if (!name.empty())
                    oss << " " << name;
                if (memory_mb > 0)
                    oss << " (" << memory_mb << " MB)";
                return oss.str();
            }
        };

        // GPU detection helpers (internal)
        namespace detail
        {
            inline std::string exec_command(const std::string &cmd)
            {
                std::string result;
#ifdef _WIN32
                FILE *pipe = _popen(cmd.c_str(), "r");
#else
                FILE *pipe = popen(cmd.c_str(), "r");
#endif
                if (!pipe)
                    return "";
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe))
                    result += buffer;
#ifdef _WIN32
                _pclose(pipe);
#else
                pclose(pipe);
#endif
                return result;
            }

            inline bool command_exists(const std::string &cmd)
            {
#ifdef _WIN32
                std::string check = "where " + cmd + " >nul 2>&1";
#else
                std::string check = "which " + cmd + " >/dev/null 2>&1";
#endif
                return std::system(check.c_str()) == 0;
            }

            inline GPUInfo detect_nvidia()
            {
                GPUInfo info;
                info.vendor = GPUVendor::nvidia;

                // nvidia-smi works on Linux, Windows, and anywhere the NVIDIA driver is installed
                if (!command_exists("nvidia-smi"))
                    return info;

#ifdef _WIN32
                // Windows: nvidia-smi is usually in PATH when driver is installed.
                // The output format is the same CSV as on Linux.
                std::string name_cmd = "nvidia-smi --query-gpu=name --format=csv,noheader 2>nul";
                std::string mem_cmd = "nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits 2>nul";
                std::string driver_cmd = "nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>nul";
#else
                std::string name_cmd = "nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null";
                std::string mem_cmd = "nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits 2>/dev/null";
                std::string driver_cmd = "nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>/dev/null";
#endif

                std::string name = exec_command(name_cmd);
                if (!name.empty())
                {
                    name.erase(name.find_last_not_of(" \n\r\t") + 1);
                    info.name = name;
                }

                std::string mem = exec_command(mem_cmd);
                if (!mem.empty())
                {
                    try
                    {
                        info.memory_mb = std::stoul(mem);
                    }
                    catch (...)
                    {
                    }
                }

                std::string driver = exec_command(driver_cmd);
                if (!driver.empty())
                {
                    driver.erase(driver.find_last_not_of(" \n\r\t") + 1);
                    info.driver_version = driver;
                }

                info.supports_cuda = true;
                info.supports_opencl = true;
                return info;
            }

            /**
             * @brief Helper to extract a GPU name from Windows wmic output.
             *
             * `wmic path Win32_VideoController get Name` returns lines like:
             *     Name
             *     AMD Radeon RX 5700 XT
             *
             * We skip blank lines and the "Name" header and try to match
             * `filter_fn` to find the correct adapter.
             */
            inline std::string win_wmic_gpu_name(
                [[maybe_unused]] std::function<bool(const std::string &)> filter_fn)
            {
#ifdef _WIN32
                std::string raw = exec_command("wmic path Win32_VideoController get Name 2>nul");
                if (raw.empty())
                    return {};
                std::istringstream iss(raw);
                std::string line;
                while (std::getline(iss, line))
                {
                    // Trim
                    line.erase(0, line.find_first_not_of(" \n\r\t"));
                    line.erase(line.find_last_not_of(" \n\r\t") + 1);
                    if (line.empty() || line == "Name")
                        continue;
                    if (filter_fn(line))
                        return line;
                }
#endif
                return {};
            }

            inline GPUInfo detect_amd()
            {
                GPUInfo info;
                info.vendor = GPUVendor::amd;

#ifdef _WIN32
                // Windows: use wmic to find AMD/Radeon adapters
                auto name = win_wmic_gpu_name([](const std::string &s)
                                              {
                    std::string lower = s;
                    for (auto &c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    return lower.find("amd") != std::string::npos ||
                           lower.find("radeon") != std::string::npos; });
                if (!name.empty())
                {
                    info.name = name;
                    info.supports_opencl = true;
                }
#else
                if (command_exists("lspci"))
                {
                    std::string cmd = "lspci 2>/dev/null | grep -i 'vga.*amd\\|display.*amd\\|3d.*amd\\|vga.*radeon' | head -1";
                    std::string result = exec_command(cmd);
                    if (!result.empty())
                    {
                        size_t bracket = result.find('[');
                        if (bracket != std::string::npos)
                        {
                            size_t end_bracket = result.find(']', bracket);
                            if (end_bracket != std::string::npos)
                                info.name = result.substr(bracket + 1, end_bracket - bracket - 1);
                        }
                        info.supports_opencl = true;
                    }
                }
#endif
                return info;
            }

            inline GPUInfo detect_intel()
            {
                GPUInfo info;
                info.vendor = GPUVendor::intel;

#ifdef _WIN32
                auto name = win_wmic_gpu_name([](const std::string &s)
                                              {
                    std::string lower = s;
                    for (auto &c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    return lower.find("intel") != std::string::npos; });
                if (!name.empty())
                {
                    info.name = name;
                    info.supports_opencl = true;
                }
#else
                if (command_exists("lspci"))
                {
                    std::string cmd = "lspci 2>/dev/null | grep -i 'vga.*intel\\|display.*intel\\|3d.*intel' | head -1";
                    std::string result = exec_command(cmd);
                    if (!result.empty())
                    {
                        size_t bracket = result.find('[');
                        if (bracket != std::string::npos)
                        {
                            size_t end_bracket = result.find(']', bracket);
                            if (end_bracket != std::string::npos)
                                info.name = result.substr(bracket + 1, end_bracket - bracket - 1);
                        }
                        info.supports_opencl = true;
                    }
                }
#endif
                return info;
            }

#ifdef __APPLE__
            inline GPUInfo detect_apple()
            {
                GPUInfo info;
                info.vendor = GPUVendor::apple;
                info.supports_metal = true;
                std::string cmd = "system_profiler SPDisplaysDataType 2>/dev/null | grep 'Chipset Model' | head -1";
                std::string result = exec_command(cmd);
                if (!result.empty())
                {
                    size_t colon = result.find(':');
                    if (colon != std::string::npos)
                    {
                        info.name = result.substr(colon + 1);
                        info.name.erase(0, info.name.find_first_not_of(" \n\r\t"));
                        info.name.erase(info.name.find_last_not_of(" \n\r\t") + 1);
                    }
                }
                return info;
            }
#endif
        } // namespace detail

        inline std::vector<GPUInfo> detect_gpus()
        {
            std::vector<GPUInfo> gpus;
            auto nvidia = detail::detect_nvidia();
            if (!nvidia.name.empty())
                gpus.push_back(nvidia);
            auto amd = detail::detect_amd();
            if (!amd.name.empty())
                gpus.push_back(amd);
            auto intel_gpu = detail::detect_intel();
            if (!intel_gpu.name.empty())
                gpus.push_back(intel_gpu);
#ifdef __APPLE__
            auto apple = detail::detect_apple();
            if (!apple.name.empty())
                gpus.push_back(apple);
#endif
            return gpus;
        }

        inline GPUInfo get_best_gpu()
        {
            auto gpus = detect_gpus();
            if (gpus.empty())
            {
                GPUInfo cpu;
                cpu.vendor = GPUVendor::cpu;
                cpu.name = "CPU Fallback";
                return cpu;
            }
            for (const auto &gpu : gpus)
                if (gpu.vendor == GPUVendor::nvidia)
                    return gpu;
            for (const auto &gpu : gpus)
                if (gpu.vendor == GPUVendor::amd)
                    return gpu;
            for (const auto &gpu : gpus)
                if (gpu.vendor == GPUVendor::intel)
                    return gpu;
            return gpus[0];
        }

        // ==================================================================
        //  Section 9 - FFmpeg Hardware Encoder Detection
        // ==================================================================

        struct HWEncoders
        {
            bool h264_nvenc = false;
            bool hevc_nvenc = false;
            bool h264_vaapi = false;
            bool hevc_vaapi = false;
            bool h264_qsv = false;
            bool hevc_qsv = false;
            bool h264_videotoolbox = false;
            bool hevc_videotoolbox = false;

            std::string best_h264_encoder() const
            {
                if (h264_nvenc)
                    return "h264_nvenc";
                if (h264_qsv)
                    return "h264_qsv";
                if (h264_vaapi)
                    return "h264_vaapi";
                if (h264_videotoolbox)
                    return "h264_videotoolbox";
                return "libx264";
            }

            std::string best_hevc_encoder() const
            {
                if (hevc_nvenc)
                    return "hevc_nvenc";
                if (hevc_qsv)
                    return "hevc_qsv";
                if (hevc_vaapi)
                    return "hevc_vaapi";
                if (hevc_videotoolbox)
                    return "hevc_videotoolbox";
                return "libx265";
            }

            bool has_hw_encoder() const
            {
                return h264_nvenc || hevc_nvenc || h264_vaapi || hevc_vaapi ||
                       h264_qsv || hevc_qsv || h264_videotoolbox || hevc_videotoolbox;
            }
        };

        inline HWEncoders detect_hw_encoders()
        {
            HWEncoders enc;
            if (!detail::command_exists("ffmpeg"))
                return enc;
            std::string cmd = "ffmpeg -encoders 2>/dev/null | grep -E 'h264_|hevc_'";
            std::string result = detail::exec_command(cmd);
            enc.h264_nvenc = result.find("h264_nvenc") != std::string::npos;
            enc.hevc_nvenc = result.find("hevc_nvenc") != std::string::npos;
            enc.h264_vaapi = result.find("h264_vaapi") != std::string::npos;
            enc.hevc_vaapi = result.find("hevc_vaapi") != std::string::npos;
            enc.h264_qsv = result.find("h264_qsv") != std::string::npos;
            enc.hevc_qsv = result.find("hevc_qsv") != std::string::npos;
            enc.h264_videotoolbox = result.find("h264_videotoolbox") != std::string::npos;
            enc.hevc_videotoolbox = result.find("hevc_videotoolbox") != std::string::npos;
            return enc;
        }

        // ==================================================================
        //  Section 10 - Compute Backend Interface
        // ==================================================================

        /**
         * @brief Abstract interface for compute backends.
         *
         * All heavy pixel-processing goes through this interface so that
         * we can swap CPU <-> GPU transparently.
         */
        class ComputeBackend
        {
        public:
            virtual ~ComputeBackend() = default;

            virtual std::string name() const = 0;
            virtual bool is_available() const = 0;
            virtual float speedup_factor() const = 0;

            // ---- Grayscale conversion ----
            virtual void rgb_to_grayscale(const uint8_t *rgb_data, int width, int height,
                                          uint8_t *output) = 0;

            virtual void rgb_to_grayscale_batch(const std::vector<const uint8_t *> &inputs,
                                                const std::vector<int> &widths,
                                                const std::vector<int> &heights,
                                                std::vector<uint8_t *> &outputs) = 0;

            // ---- Dithering ----
            virtual void floyd_steinberg(const uint8_t *gray_in, int width, int height,
                                         uint8_t *out) = 0;

            virtual void floyd_steinberg_rgb(const uint8_t *rgb_in, int width, int height,
                                             uint8_t *out) = 0;

            // ---- Braille cell processing (bulk) ----
            virtual void process_braille_cells_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out,
                uint8_t threshold, bool use_dither, bool flood) = 0;

            virtual void process_braille_cells_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out,
                uint8_t threshold, bool use_dither, bool flood) = 0;

            virtual void process_braille_cells_bayer(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out) = 0;

            // ---- Half-block cell processing (bulk) ----
            virtual void process_halfblock_cells_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, halfblock::CellResult *out) = 0;

            virtual void process_halfblock_cells_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cells_w, int cells_h, halfblock::CellResult *out) = 0;
        };

        // ==================================================================
        //  Section 11 - CPU Backend (multi-threaded, complete)
        // ==================================================================

        class CPUBackend : public ComputeBackend
        {
        private:
            int _num_threads;

        public:
            CPUBackend()
            {
                _num_threads = static_cast<int>(std::thread::hardware_concurrency());
                if (_num_threads <= 0)
                    _num_threads = 4;
            }

            std::string name() const override { return "CPU (multi-threaded, " + std::to_string(_num_threads) + " threads)"; }
            bool is_available() const override { return true; }
            float speedup_factor() const override { return 1.0f; }

            void rgb_to_grayscale(const uint8_t *rgb_data, int width, int height,
                                  uint8_t *output) override
            {
                processing::rgb_to_grayscale(rgb_data, width, height, output, _num_threads);
            }

            void rgb_to_grayscale_batch(const std::vector<const uint8_t *> &inputs,
                                        const std::vector<int> &widths,
                                        const std::vector<int> &heights,
                                        std::vector<uint8_t *> &outputs) override
            {
                processing::rgb_to_grayscale_batch(inputs, widths, heights, outputs);
            }

            void floyd_steinberg(const uint8_t *gray_in, int width, int height,
                                 uint8_t *out) override
            {
                dither::floyd_steinberg(gray_in, width, height, out);
            }

            void floyd_steinberg_rgb(const uint8_t *rgb_in, int width, int height,
                                     uint8_t *out) override
            {
                dither::floyd_steinberg_rgb(rgb_in, width, height, out);
            }

            void process_braille_cells_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out,
                uint8_t threshold, bool use_dither, bool flood) override
            {
                processing::process_braille_cells_rgb(
                    rgb_data, img_w, img_h, cells_w, cells_h, out,
                    threshold, use_dither, flood, _num_threads);
            }

            void process_braille_cells_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out,
                uint8_t threshold, bool use_dither, bool flood) override
            {
                processing::process_braille_cells_gray(
                    gray_data, img_w, img_h, cells_w, cells_h, out,
                    threshold, use_dither, flood, _num_threads);
            }

            void process_braille_cells_bayer(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out) override
            {
                processing::process_braille_cells_bayer(
                    rgb_data, img_w, img_h, cells_w, cells_h, out, _num_threads);
            }

            void process_halfblock_cells_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, halfblock::CellResult *out) override
            {
                processing::process_halfblock_cells_rgb(
                    rgb_data, img_w, img_h, cells_w, cells_h, out, _num_threads);
            }

            void process_halfblock_cells_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cells_w, int cells_h, halfblock::CellResult *out) override
            {
                processing::process_halfblock_cells_gray(
                    gray_data, img_w, img_h, cells_w, cells_h, out, _num_threads);
            }
        };

        // ==================================================================
        //  Section 12 - GPU Backend (OpenCL - real implementation)
        // ==================================================================

#ifdef PYTHONIC_ENABLE_OPENCL

        /**
         * @brief Embedded OpenCL kernel source code.
         *
         * All GPU kernels are compiled at runtime from these strings.
         */
        namespace ocl_kernels
        {
            // ---- RGB to Grayscale ----
            static const char *rgb_to_gray_src = R"CL(
__kernel void rgb_to_grayscale(
    __global const uchar* rgb,
    __global uchar* gray,
    const int total)
{
    int gid = get_global_id(0);
    if (gid >= total) return;
    int idx = gid * 3;
    uint r = rgb[idx];
    uint g = rgb[idx + 1];
    uint b = rgb[idx + 2];
    gray[gid] = (uchar)((299u * r + 587u * g + 114u * b) / 1000u);
}
)CL";

            // ---- Braille cell processing from RGB data ----
            static const char *braille_cell_rgb_src = R"CL(
__constant uchar BRAILLE_THRESH[8] = {16, 144, 80, 208, 112, 240, 48, 176};
__constant uchar DOT_BITS[8] = {0x01, 0x08, 0x02, 0x10, 0x04, 0x20, 0x40, 0x80};

// Output layout per cell: [pattern, on_count, avg_r, avg_g, avg_b, avg_all_r, avg_all_g, avg_all_b]
__kernel void braille_cell_rgb(
    __global const uchar* rgb,
    __global uchar* results,
    const int width, const int height,
    const int cells_w, const int cells_h,
    const uchar threshold,
    const int use_dither,
    const int flood)
{
    int gid = get_global_id(0);
    if (gid >= cells_w * cells_h) return;

    int cy = gid / cells_w;
    int cx = gid % cells_w;
    int px = cx * 2;
    int py = cy * 4;

    uchar pattern = 0;
    int on_count = 0;
    int r_sum = 0, g_sum = 0, b_sum = 0;
    int ar_sum = 0, ag_sum = 0, ab_sum = 0;
    int total_pix = 0;

    for (int row = 0; row < 4; ++row) {
        int y = py + row;
        if (y >= height) continue;
        for (int col = 0; col < 2; ++col) {
            int x = px + col;
            if (x >= width) continue;

            int idx = (y * width + x) * 3;
            uint r = rgb[idx];
            uint g = rgb[idx + 1];
            uint b = rgb[idx + 2];
            uchar gray = (uchar)((299u * r + 587u * g + 114u * b) / 1000u);

            ar_sum += r; ag_sum += g; ab_sum += b;
            total_pix++;

            int lit = 0;
            if (flood) {
                lit = 1;
            } else if (use_dither) {
                int didx = row * 2 + col;
                lit = (gray >= BRAILLE_THRESH[didx]) ? 1 : 0;
            } else {
                lit = (gray >= threshold) ? 1 : 0;
            }

            if (lit) {
                int didx = row * 2 + col;
                pattern |= DOT_BITS[didx];
                r_sum += r; g_sum += g; b_sum += b;
                on_count++;
            }
        }
    }

    int base = gid * 8;
    results[base + 0] = pattern;
    results[base + 1] = (uchar)on_count;
    results[base + 2] = on_count > 0 ? (uchar)(r_sum / on_count) : 0;
    results[base + 3] = on_count > 0 ? (uchar)(g_sum / on_count) : 0;
    results[base + 4] = on_count > 0 ? (uchar)(b_sum / on_count) : 0;
    results[base + 5] = total_pix > 0 ? (uchar)(ar_sum / total_pix) : 0;
    results[base + 6] = total_pix > 0 ? (uchar)(ag_sum / total_pix) : 0;
    results[base + 7] = total_pix > 0 ? (uchar)(ab_sum / total_pix) : 0;
}
)CL";

            // ---- Bayer dithered braille cell ----
            static const char *braille_cell_bayer_src = R"CL(
__constant int BAYER_2x2[2][2] = {{0, 128}, {192, 64}};
__constant uchar DOT_BITS[8] = {0x01, 0x08, 0x02, 0x10, 0x04, 0x20, 0x40, 0x80};

__kernel void braille_cell_bayer(
    __global const uchar* rgb,
    __global uchar* results,
    const int width, const int height,
    const int cells_w, const int cells_h)
{
    int gid = get_global_id(0);
    if (gid >= cells_w * cells_h) return;

    int cy = gid / cells_w;
    int cx = gid % cells_w;
    int px = cx * 2;
    int py = cy * 4;

    uchar pattern = 0;
    int on_count = 0;
    int r_sum = 0, g_sum = 0, b_sum = 0;

    for (int row = 0; row < 4; ++row) {
        int y = py + row;
        if (y >= height) continue;
        for (int col = 0; col < 2; ++col) {
            int x = px + col;
            if (x >= width) continue;

            int idx = (y * width + x) * 3;
            uint r = rgb[idx];
            uint g = rgb[idx + 1];
            uint b = rgb[idx + 2];
            uchar gray = (uchar)((299u * r + 587u * g + 114u * b) / 1000u);

            int bayer_thresh = BAYER_2x2[row % 2][col % 2];
            if (gray >= bayer_thresh) {
                int didx = row * 2 + col;
                pattern |= DOT_BITS[didx];
                r_sum += r; g_sum += g; b_sum += b;
                on_count++;
            }
        }
    }

    int base = gid * 8;
    results[base + 0] = pattern;
    results[base + 1] = (uchar)on_count;
    results[base + 2] = on_count > 0 ? (uchar)(r_sum / on_count) : 0;
    results[base + 3] = on_count > 0 ? (uchar)(g_sum / on_count) : 0;
    results[base + 4] = on_count > 0 ? (uchar)(b_sum / on_count) : 0;
    results[base + 5] = 0;
    results[base + 6] = 0;
    results[base + 7] = 0;
}
)CL";

            // ---- Half-block cell processing from RGB ----
            static const char *halfblock_cell_rgb_src = R"CL(
// Output: [top_r, top_g, top_b, top_gray, bot_r, bot_g, bot_b, bot_gray]
__kernel void halfblock_cell_rgb(
    __global const uchar* rgb,
    __global uchar* results,
    const int width, const int height,
    const int cells_w, const int cells_h)
{
    int gid = get_global_id(0);
    if (gid >= cells_w * cells_h) return;

    int cy = gid / cells_w;
    int cx = gid % cells_w;
    int top_y = cy * 2;
    int bot_y = cy * 2 + 1;

    int base = gid * 8;

    if (cx < width && top_y < height) {
        int idx = (top_y * width + cx) * 3;
        uchar r = rgb[idx], g = rgb[idx+1], b = rgb[idx+2];
        results[base + 0] = r;
        results[base + 1] = g;
        results[base + 2] = b;
        results[base + 3] = (uchar)((299u * r + 587u * g + 114u * b) / 1000u);
    } else {
        results[base + 0] = 0; results[base + 1] = 0;
        results[base + 2] = 0; results[base + 3] = 0;
    }

    if (cx < width && bot_y < height) {
        int idx = (bot_y * width + cx) * 3;
        uchar r = rgb[idx], g = rgb[idx+1], b = rgb[idx+2];
        results[base + 4] = r;
        results[base + 5] = g;
        results[base + 6] = b;
        results[base + 7] = (uchar)((299u * r + 587u * g + 114u * b) / 1000u);
    } else {
        results[base + 4] = 0; results[base + 5] = 0;
        results[base + 6] = 0; results[base + 7] = 0;
    }
}
)CL";
        } // namespace ocl_kernels

        /**
         * @brief OpenCL GPU-accelerated backend.
         *
         * Real implementation that initialises an OpenCL context, compiles
         * embedded kernel source at construction time, and dispatches compute
         * work to the GPU for every supported operation.
         *
         * Floyd-Steinberg is inherently serial (each pixel's quantisation error
         * propagates to its right and below neighbours), so it uses the CPU
         * block-parallel variant.  Every other operation runs on the GPU.
         */
        class OpenCLBackend : public ComputeBackend
        {
        private:
            cl_platform_id _platform = nullptr;
            cl_device_id _device = nullptr;
            cl_context _context = nullptr;
            cl_command_queue _queue = nullptr;
            bool _initialized = false;
            int _num_threads;

            // Compiled kernels
            cl_kernel _k_rgb_to_gray = nullptr;
            cl_kernel _k_braille_rgb = nullptr;
            cl_kernel _k_braille_bayer = nullptr;
            cl_kernel _k_halfblock_rgb = nullptr;

            std::string _device_name;
            size_t _max_work_group_size = 256;

            cl_kernel compile_kernel(const char *source, const char *kernel_name)
            {
                cl_int err;
                cl_program prog = clCreateProgramWithSource(_context, 1, &source, nullptr, &err);
                if (err != CL_SUCCESS)
                    return nullptr;

                err = clBuildProgram(prog, 1, &_device, nullptr, nullptr, nullptr);
                if (err != CL_SUCCESS)
                {
                    size_t log_size;
                    clGetProgramBuildInfo(prog, _device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
                    std::string log(log_size, ' ');
                    clGetProgramBuildInfo(prog, _device, CL_PROGRAM_BUILD_LOG, log_size, &log[0], nullptr);
                    std::cerr << "[pythonicAccel] OpenCL build error (" << kernel_name << "): " << log << std::endl;
                    clReleaseProgram(prog);
                    return nullptr;
                }

                cl_kernel k = clCreateKernel(prog, kernel_name, &err);
                clReleaseProgram(prog);
                return (err == CL_SUCCESS) ? k : nullptr;
            }

            void cleanup()
            {
                auto release = [](cl_kernel &k)
                { if (k) { clReleaseKernel(k); k = nullptr; } };
                release(_k_rgb_to_gray);
                release(_k_braille_rgb);
                release(_k_braille_bayer);
                release(_k_halfblock_rgb);
                if (_queue)
                {
                    clReleaseCommandQueue(_queue);
                    _queue = nullptr;
                }
                if (_context)
                {
                    clReleaseContext(_context);
                    _context = nullptr;
                }
            }

        public:
            OpenCLBackend()
            {
                _num_threads = static_cast<int>(std::thread::hardware_concurrency());
                if (_num_threads <= 0)
                    _num_threads = 4;

                cl_int err;

                // Enumerate platforms, prefer one with a GPU
                cl_uint num_platforms = 0;
                clGetPlatformIDs(0, nullptr, &num_platforms);
                if (num_platforms == 0)
                    return;

                std::vector<cl_platform_id> platforms(num_platforms);
                clGetPlatformIDs(num_platforms, platforms.data(), nullptr);

                for (auto plat : platforms)
                {
                    cl_uint num_devices = 0;
                    clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
                    if (num_devices > 0)
                    {
                        _platform = plat;
                        clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, 1, &_device, nullptr);
                        break;
                    }
                }
                if (!_device)
                    return;

                // Query device properties
                char dev_name[256] = {};
                clGetDeviceInfo(_device, CL_DEVICE_NAME, sizeof(dev_name), dev_name, nullptr);
                _device_name = dev_name;
                clGetDeviceInfo(_device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                                sizeof(_max_work_group_size), &_max_work_group_size, nullptr);

                // Create context and command queue
                _context = clCreateContext(nullptr, 1, &_device, nullptr, nullptr, &err);
                if (err != CL_SUCCESS)
                {
                    _context = nullptr;
                    return;
                }

                // Use OpenCL 2.0+ API when available, fall back to deprecated API
#if CL_TARGET_OPENCL_VERSION >= 200
                cl_queue_properties props[] = {0};
                _queue = clCreateCommandQueueWithProperties(_context, _device, props, &err);
#else
                _queue = clCreateCommandQueue(_context, _device, 0, &err);
#endif
                if (err != CL_SUCCESS)
                {
                    clReleaseContext(_context);
                    _context = nullptr;
                    _queue = nullptr;
                    return;
                }

                // Compile all kernels from embedded source
                _k_rgb_to_gray = compile_kernel(ocl_kernels::rgb_to_gray_src, "rgb_to_grayscale");
                _k_braille_rgb = compile_kernel(ocl_kernels::braille_cell_rgb_src, "braille_cell_rgb");
                _k_braille_bayer = compile_kernel(ocl_kernels::braille_cell_bayer_src, "braille_cell_bayer");
                _k_halfblock_rgb = compile_kernel(ocl_kernels::halfblock_cell_rgb_src, "halfblock_cell_rgb");

                _initialized = (_k_rgb_to_gray && _k_braille_rgb &&
                                _k_braille_bayer && _k_halfblock_rgb);
            }

            ~OpenCLBackend() { cleanup(); }
            OpenCLBackend(const OpenCLBackend &) = delete;
            OpenCLBackend &operator=(const OpenCLBackend &) = delete;

            std::string name() const override
            {
                return _initialized
                           ? "OpenCL GPU (" + _device_name + ")"
                           : "OpenCL GPU (unavailable)";
            }
            bool is_available() const override { return _initialized; }
            float speedup_factor() const override { return _initialized ? 8.0f : 0.0f; }

            // ---- rgb_to_grayscale (GPU) ----
            void rgb_to_grayscale(const uint8_t *rgb_data, int width, int height,
                                  uint8_t *output) override
            {
                if (!_initialized || !_k_rgb_to_gray)
                {
                    processing::rgb_to_grayscale(rgb_data, width, height, output, _num_threads);
                    return;
                }

                cl_int err;
                int total = width * height;
                size_t rgb_sz = static_cast<size_t>(total) * 3;
                size_t gray_sz = static_cast<size_t>(total);

                cl_mem b_rgb = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                              rgb_sz, const_cast<uint8_t *>(rgb_data), &err);
                cl_mem b_gray = clCreateBuffer(_context, CL_MEM_WRITE_ONLY, gray_sz, nullptr, &err);

                clSetKernelArg(_k_rgb_to_gray, 0, sizeof(cl_mem), &b_rgb);
                clSetKernelArg(_k_rgb_to_gray, 1, sizeof(cl_mem), &b_gray);
                clSetKernelArg(_k_rgb_to_gray, 2, sizeof(int), &total);

                size_t global = ((total + 255) / 256) * 256;
                size_t local = std::min(static_cast<size_t>(256), _max_work_group_size);

                err = clEnqueueNDRangeKernel(_queue, _k_rgb_to_gray, 1, nullptr,
                                             &global, &local, 0, nullptr, nullptr);
                if (err == CL_SUCCESS)
                    clEnqueueReadBuffer(_queue, b_gray, CL_TRUE, 0, gray_sz, output, 0, nullptr, nullptr);
                else
                    processing::rgb_to_grayscale(rgb_data, width, height, output, _num_threads);

                clReleaseMemObject(b_rgb);
                clReleaseMemObject(b_gray);
            }

            void rgb_to_grayscale_batch(const std::vector<const uint8_t *> &inputs,
                                        const std::vector<int> &widths,
                                        const std::vector<int> &heights,
                                        std::vector<uint8_t *> &outputs) override
            {
                for (size_t i = 0; i < inputs.size(); ++i)
                    rgb_to_grayscale(inputs[i], widths[i], heights[i], outputs[i]);
            }

            // ---- Floyd-Steinberg (CPU block-parallel) ----
            // Floyd-Steinberg has per-pixel data dependencies (each pixel propagates
            // quantisation error to its right and lower neighbours).  This makes it
            // fundamentally impossible to parallelise per-pixel on a GPU.  We use
            // the block-parallel CPU variant which splits the image into independent
            // blocks and processes them on multiple CPU threads.
            void floyd_steinberg(const uint8_t *gray_in, int width, int height,
                                 uint8_t *out) override
            {
                dither::floyd_steinberg_parallel(gray_in, width, height, out);
            }

            void floyd_steinberg_rgb(const uint8_t *rgb_in, int width, int height,
                                     uint8_t *out) override
            {
                std::vector<uint8_t> gray(static_cast<size_t>(width) * height);
                rgb_to_grayscale(rgb_in, width, height, gray.data());
                dither::floyd_steinberg_parallel(gray.data(), width, height, out);
            }

            // ---- Braille cell processing (GPU) ----
            void process_braille_cells_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out,
                uint8_t threshold, bool use_dither, bool flood) override
            {
                if (!_initialized || !_k_braille_rgb)
                {
                    processing::process_braille_cells_rgb(rgb_data, img_w, img_h,
                                                          cells_w, cells_h, out, threshold, use_dither, flood, _num_threads);
                    return;
                }

                cl_int err;
                int total_cells = cells_w * cells_h;
                size_t rgb_sz = static_cast<size_t>(img_w) * img_h * 3;
                size_t res_sz = static_cast<size_t>(total_cells) * 8;

                cl_mem b_rgb = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                              rgb_sz, const_cast<uint8_t *>(rgb_data), &err);
                cl_mem b_res = clCreateBuffer(_context, CL_MEM_WRITE_ONLY, res_sz, nullptr, &err);

                int dither_flag = use_dither ? 1 : 0;
                int flood_flag = flood ? 1 : 0;

                clSetKernelArg(_k_braille_rgb, 0, sizeof(cl_mem), &b_rgb);
                clSetKernelArg(_k_braille_rgb, 1, sizeof(cl_mem), &b_res);
                clSetKernelArg(_k_braille_rgb, 2, sizeof(int), &img_w);
                clSetKernelArg(_k_braille_rgb, 3, sizeof(int), &img_h);
                clSetKernelArg(_k_braille_rgb, 4, sizeof(int), &cells_w);
                clSetKernelArg(_k_braille_rgb, 5, sizeof(int), &cells_h);
                clSetKernelArg(_k_braille_rgb, 6, sizeof(cl_uchar), &threshold);
                clSetKernelArg(_k_braille_rgb, 7, sizeof(int), &dither_flag);
                clSetKernelArg(_k_braille_rgb, 8, sizeof(int), &flood_flag);

                size_t global = ((total_cells + 255) / 256) * 256;
                size_t local = std::min(static_cast<size_t>(256), _max_work_group_size);

                err = clEnqueueNDRangeKernel(_queue, _k_braille_rgb, 1, nullptr,
                                             &global, &local, 0, nullptr, nullptr);
                if (err != CL_SUCCESS)
                {
                    clReleaseMemObject(b_rgb);
                    clReleaseMemObject(b_res);
                    processing::process_braille_cells_rgb(rgb_data, img_w, img_h,
                                                          cells_w, cells_h, out, threshold, use_dither, flood, _num_threads);
                    return;
                }

                // Read packed results and unpack into CellResult structs
                std::vector<uint8_t> packed(res_sz);
                clEnqueueReadBuffer(_queue, b_res, CL_TRUE, 0, res_sz, packed.data(), 0, nullptr, nullptr);

                for (int i = 0; i < total_cells; ++i)
                {
                    int base = i * 8;
                    out[i].pattern = packed[base + 0];
                    out[i].on_count = packed[base + 1];
                    out[i].avg_color = pixel::RGB(packed[base + 2], packed[base + 3], packed[base + 4]);
                    out[i].avg_gray = pixel::to_gray(packed[base + 2], packed[base + 3], packed[base + 4]);
                    out[i].avg_all_color = pixel::RGB(packed[base + 5], packed[base + 6], packed[base + 7]);
                }

                clReleaseMemObject(b_rgb);
                clReleaseMemObject(b_res);
            }

            void process_braille_cells_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out,
                uint8_t threshold, bool use_dither, bool flood) override
            {
                // Expand to RGB and use GPU kernel (transfer cost is small vs. GPU speedup)
                std::vector<uint8_t> rgb(static_cast<size_t>(img_w) * img_h * 3);
                for (int i = 0; i < img_w * img_h; ++i)
                    rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = gray_data[i];
                process_braille_cells_rgb(rgb.data(), img_w, img_h,
                                          cells_w, cells_h, out, threshold, use_dither, flood);
            }

            void process_braille_cells_bayer(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, braille::CellResult *out) override
            {
                if (!_initialized || !_k_braille_bayer)
                {
                    processing::process_braille_cells_bayer(rgb_data, img_w, img_h,
                                                            cells_w, cells_h, out, _num_threads);
                    return;
                }

                cl_int err;
                int total_cells = cells_w * cells_h;
                size_t rgb_sz = static_cast<size_t>(img_w) * img_h * 3;
                size_t res_sz = static_cast<size_t>(total_cells) * 8;

                cl_mem b_rgb = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                              rgb_sz, const_cast<uint8_t *>(rgb_data), &err);
                cl_mem b_res = clCreateBuffer(_context, CL_MEM_WRITE_ONLY, res_sz, nullptr, &err);

                clSetKernelArg(_k_braille_bayer, 0, sizeof(cl_mem), &b_rgb);
                clSetKernelArg(_k_braille_bayer, 1, sizeof(cl_mem), &b_res);
                clSetKernelArg(_k_braille_bayer, 2, sizeof(int), &img_w);
                clSetKernelArg(_k_braille_bayer, 3, sizeof(int), &img_h);
                clSetKernelArg(_k_braille_bayer, 4, sizeof(int), &cells_w);
                clSetKernelArg(_k_braille_bayer, 5, sizeof(int), &cells_h);

                size_t global = ((total_cells + 255) / 256) * 256;
                size_t local = std::min(static_cast<size_t>(256), _max_work_group_size);

                err = clEnqueueNDRangeKernel(_queue, _k_braille_bayer, 1, nullptr,
                                             &global, &local, 0, nullptr, nullptr);
                if (err != CL_SUCCESS)
                {
                    clReleaseMemObject(b_rgb);
                    clReleaseMemObject(b_res);
                    processing::process_braille_cells_bayer(rgb_data, img_w, img_h,
                                                            cells_w, cells_h, out, _num_threads);
                    return;
                }

                std::vector<uint8_t> packed(res_sz);
                clEnqueueReadBuffer(_queue, b_res, CL_TRUE, 0, res_sz, packed.data(), 0, nullptr, nullptr);

                for (int i = 0; i < total_cells; ++i)
                {
                    int base = i * 8;
                    out[i].pattern = packed[base + 0];
                    out[i].on_count = packed[base + 1];
                    out[i].avg_color = pixel::RGB(packed[base + 2], packed[base + 3], packed[base + 4]);
                    out[i].avg_gray = pixel::to_gray(packed[base + 2], packed[base + 3], packed[base + 4]);
                    out[i].avg_all_color = pixel::RGB(packed[base + 5], packed[base + 6], packed[base + 7]);
                }

                clReleaseMemObject(b_rgb);
                clReleaseMemObject(b_res);
            }

            // ---- Half-block cell processing (GPU) ----
            void process_halfblock_cells_rgb(
                const uint8_t *rgb_data, int img_w, int img_h,
                int cells_w, int cells_h, halfblock::CellResult *out) override
            {
                if (!_initialized || !_k_halfblock_rgb)
                {
                    processing::process_halfblock_cells_rgb(rgb_data, img_w, img_h,
                                                            cells_w, cells_h, out, _num_threads);
                    return;
                }

                cl_int err;
                int total_cells = cells_w * cells_h;
                size_t rgb_sz = static_cast<size_t>(img_w) * img_h * 3;
                size_t res_sz = static_cast<size_t>(total_cells) * 8;

                cl_mem b_rgb = clCreateBuffer(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                              rgb_sz, const_cast<uint8_t *>(rgb_data), &err);
                cl_mem b_res = clCreateBuffer(_context, CL_MEM_WRITE_ONLY, res_sz, nullptr, &err);

                clSetKernelArg(_k_halfblock_rgb, 0, sizeof(cl_mem), &b_rgb);
                clSetKernelArg(_k_halfblock_rgb, 1, sizeof(cl_mem), &b_res);
                clSetKernelArg(_k_halfblock_rgb, 2, sizeof(int), &img_w);
                clSetKernelArg(_k_halfblock_rgb, 3, sizeof(int), &img_h);
                clSetKernelArg(_k_halfblock_rgb, 4, sizeof(int), &cells_w);
                clSetKernelArg(_k_halfblock_rgb, 5, sizeof(int), &cells_h);

                size_t global = ((total_cells + 255) / 256) * 256;
                size_t local = std::min(static_cast<size_t>(256), _max_work_group_size);

                err = clEnqueueNDRangeKernel(_queue, _k_halfblock_rgb, 1, nullptr,
                                             &global, &local, 0, nullptr, nullptr);
                if (err != CL_SUCCESS)
                {
                    clReleaseMemObject(b_rgb);
                    clReleaseMemObject(b_res);
                    processing::process_halfblock_cells_rgb(rgb_data, img_w, img_h,
                                                            cells_w, cells_h, out, _num_threads);
                    return;
                }

                std::vector<uint8_t> packed(res_sz);
                clEnqueueReadBuffer(_queue, b_res, CL_TRUE, 0, res_sz, packed.data(), 0, nullptr, nullptr);

                for (int i = 0; i < total_cells; ++i)
                {
                    int base = i * 8;
                    out[i].top = pixel::RGB(packed[base + 0], packed[base + 1], packed[base + 2]);
                    out[i].top_gray = packed[base + 3];
                    out[i].bottom = pixel::RGB(packed[base + 4], packed[base + 5], packed[base + 6]);
                    out[i].bottom_gray = packed[base + 7];
                }

                clReleaseMemObject(b_rgb);
                clReleaseMemObject(b_res);
            }

            void process_halfblock_cells_gray(
                const uint8_t *gray_data, int img_w, int img_h,
                int cells_w, int cells_h, halfblock::CellResult *out) override
            {
                std::vector<uint8_t> rgb(static_cast<size_t>(img_w) * img_h * 3);
                for (int i = 0; i < img_w * img_h; ++i)
                    rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = gray_data[i];
                process_halfblock_cells_rgb(rgb.data(), img_w, img_h, cells_w, cells_h, out);
            }
        };
#endif // PYTHONIC_ENABLE_OPENCL

        // ==================================================================
        //  Section 13 - Backend Factory
        // ==================================================================

        /**
         * @brief Get the best available compute backend.
         * Priority: OpenCL GPU > CPU
         */
        inline std::shared_ptr<ComputeBackend> get_best_backend()
        {
#ifdef PYTHONIC_ENABLE_OPENCL
            auto ocl = std::make_shared<OpenCLBackend>();
            if (ocl->is_available())
                return ocl;
#endif
            return std::make_shared<CPUBackend>();
        }

        // ==================================================================
        //  Section 14 - Convenience / diagnostic
        // ==================================================================

        inline void print_gpu_info()
        {
            std::cout << "\033[36m=== GPU Detection ===\033[0m\n";
            auto gpus = detect_gpus();
            if (gpus.empty())
            {
                std::cout << "\033[33mNo GPU detected, using CPU fallback\033[0m\n";
            }
            else
            {
                for (const auto &gpu : gpus)
                {
                    std::cout << "\033[32m* " << gpu.to_string() << "\033[0m";
                    if (gpu.supports_cuda)
                        std::cout << " [CUDA]";
                    if (gpu.supports_opencl)
                        std::cout << " [OpenCL]";
                    if (gpu.supports_metal)
                        std::cout << " [Metal]";
                    std::cout << "\n";
                }
            }

            auto encoders = detect_hw_encoders();
            std::cout << "\n\033[36m=== FFmpeg HW Encoders ===\033[0m\n";
            if (encoders.has_hw_encoder())
            {
                std::cout << "\033[32mBest H.264: " << encoders.best_h264_encoder() << "\033[0m\n";
                std::cout << "\033[32mBest HEVC:  " << encoders.best_hevc_encoder() << "\033[0m\n";
            }
            else
            {
                std::cout << "\033[33mNo HW encoders found, using libx264/libx265\033[0m\n";
            }

            auto backend = get_best_backend();
            std::cout << "\n\033[36m=== Compute Backend ===\033[0m\n";
            std::cout << "\033[32m" << backend->name() << "\033[0m\n";
        }

        /**
         * @brief Print a summary of all available processing capabilities.
         */
        inline void print_capabilities()
        {
            std::cout << "\033[36m=== pythonic::accel Capabilities ===\033[0m\n\n";

            std::cout << "\033[33mPixel Processing:\033[0m\n";
            std::cout << "  * to_gray (BT.601)        - pixel::to_gray(r,g,b)\n";
            std::cout << "  * gray_to_ansi256          - pixel::gray_to_ansi256(gray)\n\n";

            std::cout << "\033[33mDithering:\033[0m\n";
            std::cout << "  * Ordered (Braille 2x4)    - dither::BRAILLE_ORDERED[8]\n";
            std::cout << "  * Bayer 2x2                - dither::BAYER_2x2[2][2]\n";
            std::cout << "  * Floyd-Steinberg (gray)   - dither::floyd_steinberg()\n";
            std::cout << "  * Floyd-Steinberg (RGB)    - dither::floyd_steinberg_rgb()\n\n";

            std::cout << "\033[33mBraille Cell Processing:\033[0m\n";
            std::cout << "  * process_cell_rgb         - single cell, threshold/dither/flood\n";
            std::cout << "  * process_cell_gray        - single cell, grayscale input\n";
            std::cout << "  * process_cell_rgb_bayer   - single cell, 2x2 Bayer dithered\n\n";

            std::cout << "\033[33mHalf-Block Cell Processing:\033[0m\n";
            std::cout << "  * process_cell_rgb         - half-block, RGB input\n";
            std::cout << "  * process_cell_gray        - half-block, gray input\n\n";

            std::cout << "\033[33mBulk Processing (multi-threaded via backend):\033[0m\n";
            std::cout << "  * rgb_to_grayscale         - full image\n";
            std::cout << "  * rgb_to_grayscale_batch   - multiple images\n";
            std::cout << "  * process_braille_cells_*  - all cells in an image\n";
            std::cout << "  * process_halfblock_cells_* - all cells in an image\n\n";

            std::cout << "\033[33mImage I/O:\033[0m\n";
            std::cout << "  * load_ppm_pgm             - parse P5/P6 files\n";
            std::cout << "  * convert_to_ppm           - ImageMagick conversion\n";
            std::cout << "  * load_image               - auto PPM or ImageMagick\n\n";

            std::cout << "\033[33mVideo Processing:\033[0m\n";
            std::cout << "  * probe                    - ffprobe metadata\n";
            std::cout << "  * get_duration / get_fps   - quick queries\n";
            std::cout << "  * extract_frames           - FFmpeg frame extraction\n";
            std::cout << "  * extract_single_frame     - one frame as PPM/PNG\n";
            std::cout << "  * encode_video             - FFmpeg encoding\n";
            std::cout << "  * extract_audio            - separate audio track\n";
            std::cout << "  * open_decode_pipe         - streaming raw frames\n";
            std::cout << "  * open_encode_pipe         - streaming frame encoding\n\n";

            std::cout << "\033[33mGPU / HW Detection:\033[0m\n";
            std::cout << "  * detect_gpus              - NVIDIA/AMD/Intel/Apple\n";
            std::cout << "  * detect_hw_encoders       - FFmpeg HW encoders\n";
            std::cout << "  * get_best_backend         - auto-select compute backend\n\n";

            std::cout << "\033[33mRendering Modes Supported:\033[0m\n";
            std::cout << "  * bw               - threshold braille       (process_cell_rgb, threshold)\n";
            std::cout << "  * bw_dot           - threshold braille dots  (process_cell_gray, threshold)\n";
            std::cout << "  * colored          - RGB half-block          (process_halfblock_cells_rgb)\n";
            std::cout << "  * colored_dot      - RGB colored braille     (process_cell_rgb, threshold)\n";
            std::cout << "  * bw_dithered      - ordered dither braille  (process_cell_rgb, use_dither)\n";
            std::cout << "  * grayscale_dot    - gray ANSI braille       (process_cell_gray, use_dither)\n";
            std::cout << "  * flood_dot        - all dots, gray color    (process_cell_gray, flood)\n";
            std::cout << "  * flood_dot_colored - all dots, RGB color    (process_cell_rgb, flood)\n";
            std::cout << "  * colored_dithered - Bayer dither braille    (process_cell_rgb_bayer)\n";
        }

    } // namespace accel
} // namespace pythonic
