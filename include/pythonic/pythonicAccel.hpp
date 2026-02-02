#pragma once
/**
 * @file pythonicAccel.hpp
 * @brief GPU acceleration and compute backend abstraction
 *
 * Provides cross-platform GPU detection and acceleration for image processing.
 * Supports OpenCL (cross-vendor), CUDA (NVIDIA), and CPU fallback.
 *
 * Usage:
 *   auto backend = pythonic::accel::get_best_backend();
 *   backend->process_batch(frames, output);
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

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace pythonic
{
    namespace accel
    {

        // ==================== GPU Information ====================

        /**
         * @brief GPU vendor identification
         */
        enum class GPUVendor
        {
            unknown,
            nvidia,
            amd,
            intel,
            apple, // Apple Silicon
            cpu    // No GPU, CPU fallback
        };

        /**
         * @brief Get vendor name as string
         */
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

        /**
         * @brief Information about detected GPU
         */
        struct GPUInfo
        {
            GPUVendor vendor = GPUVendor::unknown;
            std::string name;
            std::string driver_version;
            size_t memory_mb = 0;  // VRAM in MB
            int compute_units = 0; // CUDA cores / Stream processors / etc
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

        // ==================== GPU Detection ====================

        namespace detail
        {
            /**
             * @brief Execute a command and capture output
             */
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

            /**
             * @brief Check if a command exists
             */
            inline bool command_exists(const std::string &cmd)
            {
#ifdef _WIN32
                std::string check = "where " + cmd + " >nul 2>&1";
#else
                std::string check = "which " + cmd + " >/dev/null 2>&1";
#endif
                return std::system(check.c_str()) == 0;
            }

            /**
             * @brief Detect NVIDIA GPU using nvidia-smi
             */
            inline GPUInfo detect_nvidia()
            {
                GPUInfo info;
                info.vendor = GPUVendor::nvidia;

                if (!command_exists("nvidia-smi"))
                    return info;

                // Get GPU name
                std::string name_cmd = "nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null";
                std::string name = exec_command(name_cmd);
                if (!name.empty())
                {
                    // Trim whitespace
                    name.erase(name.find_last_not_of(" \n\r\t") + 1);
                    info.name = name;
                }

                // Get memory
                std::string mem_cmd = "nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits 2>/dev/null";
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

                // Get driver version
                std::string driver_cmd = "nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>/dev/null";
                std::string driver = exec_command(driver_cmd);
                if (!driver.empty())
                {
                    driver.erase(driver.find_last_not_of(" \n\r\t") + 1);
                    info.driver_version = driver;
                }

                info.supports_cuda = true;
                info.supports_opencl = true; // NVIDIA supports OpenCL too

                return info;
            }

            /**
             * @brief Detect AMD GPU
             */
            inline GPUInfo detect_amd()
            {
                GPUInfo info;
                info.vendor = GPUVendor::amd;

#ifdef _WIN32
                // Check Windows registry or use clinfo
#else
                // Check using lspci or clinfo
                if (command_exists("lspci"))
                {
                    std::string cmd = "lspci 2>/dev/null | grep -i 'vga.*amd\\|display.*amd\\|3d.*amd\\|vga.*radeon' | head -1";
                    std::string result = exec_command(cmd);
                    if (!result.empty())
                    {
                        // Extract name from lspci output
                        size_t bracket = result.find('[');
                        if (bracket != std::string::npos)
                        {
                            size_t end_bracket = result.find(']', bracket);
                            if (end_bracket != std::string::npos)
                            {
                                info.name = result.substr(bracket + 1, end_bracket - bracket - 1);
                            }
                        }
                        info.supports_opencl = true;
                    }
                }
#endif
                return info;
            }

            /**
             * @brief Detect Intel GPU
             */
            inline GPUInfo detect_intel()
            {
                GPUInfo info;
                info.vendor = GPUVendor::intel;

#ifdef _WIN32
                // Check using clinfo or registry
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
                            {
                                info.name = result.substr(bracket + 1, end_bracket - bracket - 1);
                            }
                        }
                        info.supports_opencl = true;
                    }
                }
#endif
                return info;
            }

#ifdef __APPLE__
            /**
             * @brief Detect Apple GPU (Metal)
             */
            inline GPUInfo detect_apple()
            {
                GPUInfo info;
                info.vendor = GPUVendor::apple;
                info.supports_metal = true;

                // Use system_profiler to get GPU info
                std::string cmd = "system_profiler SPDisplaysDataType 2>/dev/null | grep 'Chipset Model' | head -1";
                std::string result = exec_command(cmd);
                if (!result.empty())
                {
                    size_t colon = result.find(':');
                    if (colon != std::string::npos)
                    {
                        info.name = result.substr(colon + 1);
                        // Trim whitespace
                        info.name.erase(0, info.name.find_first_not_of(" \n\r\t"));
                        info.name.erase(info.name.find_last_not_of(" \n\r\t") + 1);
                    }
                }

                return info;
            }
#endif

        } // namespace detail

        /**
         * @brief Detect all available GPUs on the system
         * @return Vector of detected GPU information
         */
        inline std::vector<GPUInfo> detect_gpus()
        {
            std::vector<GPUInfo> gpus;

            // Try NVIDIA first (most common for compute)
            auto nvidia = detail::detect_nvidia();
            if (!nvidia.name.empty())
                gpus.push_back(nvidia);

            // Try AMD
            auto amd = detail::detect_amd();
            if (!amd.name.empty())
                gpus.push_back(amd);

            // Try Intel
            auto intel = detail::detect_intel();
            if (!intel.name.empty())
                gpus.push_back(intel);

#ifdef __APPLE__
            // Try Apple Silicon
            auto apple = detail::detect_apple();
            if (!apple.name.empty())
                gpus.push_back(apple);
#endif

            return gpus;
        }

        /**
         * @brief Get the best available GPU for compute
         * Priority: NVIDIA > AMD > Intel > Apple > None
         */
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

            // Prioritize by vendor
            for (const auto &gpu : gpus)
            {
                if (gpu.vendor == GPUVendor::nvidia)
                    return gpu;
            }
            for (const auto &gpu : gpus)
            {
                if (gpu.vendor == GPUVendor::amd)
                    return gpu;
            }
            for (const auto &gpu : gpus)
            {
                if (gpu.vendor == GPUVendor::intel)
                    return gpu;
            }

            return gpus[0];
        }

        // ==================== Compute Backend Interface ====================

        /**
         * @brief Abstract interface for compute backends
         */
        class ComputeBackend
        {
        public:
            virtual ~ComputeBackend() = default;

            /**
             * @brief Get the name of this backend
             */
            virtual std::string name() const = 0;

            /**
             * @brief Check if the backend is available and initialized
             */
            virtual bool is_available() const = 0;

            /**
             * @brief Process an image: convert RGB to grayscale
             * @param rgb_data Input RGB data (3 bytes per pixel)
             * @param width Image width
             * @param height Image height
             * @param output Output grayscale data (1 byte per pixel)
             */
            virtual void rgb_to_grayscale(const uint8_t *rgb_data, int width, int height,
                                          uint8_t *output) = 0;

            /**
             * @brief Process a batch of images
             */
            virtual void rgb_to_grayscale_batch(const std::vector<const uint8_t *> &inputs,
                                                const std::vector<int> &widths,
                                                const std::vector<int> &heights,
                                                std::vector<uint8_t *> &outputs) = 0;

            /**
             * @brief Get estimated speedup factor over CPU
             */
            virtual float speedup_factor() const = 0;
        };

        // ==================== CPU Backend (Fallback) ====================

        /**
         * @brief CPU-based image processing (fallback when no GPU available)
         */
        class CPUBackend : public ComputeBackend
        {
        private:
            int _num_threads;

        public:
            CPUBackend()
            {
                _num_threads = std::thread::hardware_concurrency();
                if (_num_threads == 0)
                    _num_threads = 4; // Fallback
            }

            std::string name() const override { return "CPU (multi-threaded)"; }
            bool is_available() const override { return true; }
            float speedup_factor() const override { return 1.0f; }

            void rgb_to_grayscale(const uint8_t *rgb_data, int width, int height,
                                  uint8_t *output) override
            {
                // Parallel processing using standard threads
                const int num_pixels = width * height;
                const int chunk_size = (num_pixels + _num_threads - 1) / _num_threads;

                std::vector<std::thread> threads;
                for (int t = 0; t < _num_threads; t++)
                {
                    int start = t * chunk_size;
                    int end = std::min(start + chunk_size, num_pixels);

                    threads.emplace_back([rgb_data, output, start, end]()
                                         {
                        for (int i = start; i < end; i++)
                        {
                            const uint8_t *pixel = rgb_data + i * 3;
                            // ITU-R BT.601 luma coefficients
                            output[i] = static_cast<uint8_t>(
                                (299 * pixel[0] + 587 * pixel[1] + 114 * pixel[2]) / 1000);
                        } });
                }

                for (auto &t : threads)
                    t.join();
            }

            void rgb_to_grayscale_batch(const std::vector<const uint8_t *> &inputs,
                                        const std::vector<int> &widths,
                                        const std::vector<int> &heights,
                                        std::vector<uint8_t *> &outputs) override
            {
                for (size_t i = 0; i < inputs.size(); i++)
                {
                    rgb_to_grayscale(inputs[i], widths[i], heights[i], outputs[i]);
                }
            }
        };

        // ==================== FFmpeg Hardware Encoder Detection ====================

        /**
         * @brief Available hardware encoders for FFmpeg
         */
        struct HWEncoders
        {
            bool h264_nvenc = false;        // NVIDIA H.264
            bool hevc_nvenc = false;        // NVIDIA HEVC
            bool h264_vaapi = false;        // Intel/AMD VAAPI
            bool hevc_vaapi = false;        // Intel/AMD VAAPI HEVC
            bool h264_qsv = false;          // Intel QuickSync
            bool hevc_qsv = false;          // Intel QuickSync HEVC
            bool h264_videotoolbox = false; // macOS VideoToolbox
            bool hevc_videotoolbox = false; // macOS VideoToolbox HEVC

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
                return "libx264"; // CPU fallback
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
                return "libx265"; // CPU fallback
            }

            bool has_hw_encoder() const
            {
                return h264_nvenc || hevc_nvenc || h264_vaapi || hevc_vaapi ||
                       h264_qsv || hevc_qsv || h264_videotoolbox || hevc_videotoolbox;
            }
        };

        /**
         * @brief Detect available FFmpeg hardware encoders
         */
        inline HWEncoders detect_hw_encoders()
        {
            HWEncoders enc;

            // Check if ffmpeg is available
            if (!detail::command_exists("ffmpeg"))
                return enc;

            // Get list of encoders
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

        // ==================== Backend Factory ====================

        /**
         * @brief Get the best available compute backend
         */
        inline std::shared_ptr<ComputeBackend> get_best_backend()
        {
            // For now, return CPU backend
            // In future, this would detect and return OpenCL/CUDA backends
            return std::make_shared<CPUBackend>();
        }

        /**
         * @brief Print GPU detection info
         */
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
                    std::cout << "\033[32m• " << gpu.to_string() << "\033[0m";
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
                std::cout << "\033[33mNo HW encoders found, using libx264\033[0m\n";
            }
        }

    } // namespace accel
} // namespace pythonic
