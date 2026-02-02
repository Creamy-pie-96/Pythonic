#pragma once
/**
 * @file pythonicMedia.hpp
 * @brief Proprietary media format conversion for Pythonic
 *
 * This module provides functions to convert images and videos into
 * Pythonic's proprietary encrypted formats (.pi for images, .pv for videos).
 * These formats can only be read by this library.
 *
 * File formats:
 *   .pi - Pythonic Image: Encrypted/obfuscated image data
 *   .pv - Pythonic Video: Encrypted/obfuscated video data
 *
 * Features:
 *   - XOR-based encryption with rotating key
 *   - Header verification with magic bytes
 *   - Original format preservation for lossless round-trip
 *   - Metadata storage (original extension, dimensions)
 *
 * Example usage:
 *   // Convert image to Pythonic format
 *   pythonic::media::convert("photo.jpg");  // Creates photo.pi
 *
 *   // Convert video to Pythonic format
 *   pythonic::media::convert("video.mp4");  // Creates video.pv
 *
 *   // Revert back to original format
 *   pythonic::media::revert("photo.pi");    // Creates photo_restored.jpg
 *   pythonic::media::revert("video.pv");    // Creates video_restored.mp4
 */

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <chrono>

namespace pythonic
{
    namespace media
    {
        // ==================== Format Detection ====================

        /**
         * @brief Media type for conversion
         */
        enum class Type
        {
            auto_detect, // Detect from file extension
            image,       // Force treat as image
            video        // Force treat as video
        };

        // ==================== Magic Bytes and Constants ====================

        // Magic bytes for Pythonic formats (helps identify our files)
        constexpr uint8_t PYTHONIC_IMAGE_MAGIC[8] = {'P', 'Y', 'T', 'H', 'I', 'M', 'G', 0x01};
        constexpr uint8_t PYTHONIC_VIDEO_MAGIC[8] = {'P', 'Y', 'T', 'H', 'V', 'I', 'D', 0x01};

        // Version number for format compatibility
        constexpr uint8_t FORMAT_VERSION = 1;

        // Maximum extension length to store
        constexpr size_t MAX_EXT_LENGTH = 16;

        // Encryption key (XOR with rotating key for obfuscation)
        // Not cryptographically secure - just obfuscation
        constexpr uint8_t ENCRYPT_KEY[32] = {
            0x50, 0x79, 0x74, 0x68, 0x6F, 0x6E, 0x69, 0x63, // "Pythonic"
            0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
            0x13, 0x37, 0x42, 0x69, 0x88, 0x99, 0xAA, 0xBB,
            0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44};

        // ==================== File Header Structure ====================

        /**
         * @brief Header structure for Pythonic media files
         *
         * Layout (total 64 bytes):
         *   0-7:   Magic bytes (8 bytes)
         *   8:     Format version (1 byte)
         *   9:     Original extension length (1 byte)
         *   10-25: Original extension, null-padded (16 bytes)
         *   26-29: Random salt for encryption (4 bytes)
         *   30-33: Reserved (4 bytes)
         *   34-41: Original file size (8 bytes, little-endian)
         *   42-63: Reserved for future use (22 bytes, zero-filled)
         */
#pragma pack(push, 1)
        struct PythonicMediaHeader
        {
            uint8_t magic[8];       // 0-7   (8 bytes)
            uint8_t version;        // 8     (1 byte)
            uint8_t ext_length;     // 9     (1 byte)
            char original_ext[16];  // 10-25 (16 bytes)
            uint32_t salt;          // 26-29 (4 bytes)
            uint32_t reserved1;     // 30-33 (4 bytes)
            uint64_t original_size; // 34-41 (8 bytes)
            uint8_t reserved[22];   // 42-63 (22 bytes)

            PythonicMediaHeader()
            {
                std::memset(this, 0, sizeof(*this));
                version = FORMAT_VERSION;
            }

            void set_magic_image()
            {
                std::memcpy(magic, PYTHONIC_IMAGE_MAGIC, 8);
            }

            void set_magic_video()
            {
                std::memcpy(magic, PYTHONIC_VIDEO_MAGIC, 8);
            }

            bool is_image() const
            {
                return std::memcmp(magic, PYTHONIC_IMAGE_MAGIC, 8) == 0;
            }

            bool is_video() const
            {
                return std::memcmp(magic, PYTHONIC_VIDEO_MAGIC, 8) == 0;
            }

            bool is_valid() const
            {
                return (is_image() || is_video()) && version == FORMAT_VERSION;
            }

            void set_extension(const std::string &ext)
            {
                ext_length = static_cast<uint8_t>(std::min(ext.size(), size_t(15)));
                std::memcpy(original_ext, ext.c_str(), ext_length);
                original_ext[ext_length] = '\0';
            }

            std::string get_extension() const
            {
                return std::string(original_ext, ext_length);
            }
        };
#pragma pack(pop)

        static_assert(sizeof(PythonicMediaHeader) == 64, "Header must be exactly 64 bytes");

        // ==================== Encryption/Decryption ====================

        /**
         * @brief XOR encrypt/decrypt data with rotating key and salt
         *
         * Same function for both encryption and decryption (XOR is symmetric).
         *
         * @param data Data to transform (modified in place)
         * @param salt Random salt to make each file's encryption unique
         */
        inline void xor_transform(std::vector<uint8_t> &data, uint32_t salt)
        {
            // Combine salt with base key for this file's unique key
            uint8_t file_key[32];
            for (size_t i = 0; i < 32; ++i)
            {
                file_key[i] = ENCRYPT_KEY[i] ^ static_cast<uint8_t>((salt >> (i % 4 * 8)) & 0xFF);
            }

            // XOR with rotating key
            for (size_t i = 0; i < data.size(); ++i)
            {
                // Use position-dependent rotation for better obfuscation
                size_t key_idx = (i + (i / 32)) % 32;
                data[i] ^= file_key[key_idx];

                // Additional byte-level scrambling
                data[i] = ((data[i] << 3) | (data[i] >> 5)); // Rotate bits
            }
        }

        /**
         * @brief Reverse the XOR transformation
         */
        inline void xor_untransform(std::vector<uint8_t> &data, uint32_t salt)
        {
            uint8_t file_key[32];
            for (size_t i = 0; i < 32; ++i)
            {
                file_key[i] = ENCRYPT_KEY[i] ^ static_cast<uint8_t>((salt >> (i % 4 * 8)) & 0xFF);
            }

            // Reverse the transformation (order matters!)
            for (size_t i = 0; i < data.size(); ++i)
            {
                // Reverse bit rotation first
                data[i] = ((data[i] >> 3) | (data[i] << 5));

                // Then XOR
                size_t key_idx = (i + (i / 32)) % 32;
                data[i] ^= file_key[key_idx];
            }
        }

        // ==================== File Type Detection ====================

        /**
         * @brief Check if file is an image by extension
         */
        inline bool is_image_extension(const std::string &ext)
        {
            std::string lower_ext = ext;
            for (auto &c : lower_ext)
                c = std::tolower(c);

            return lower_ext == ".png" || lower_ext == ".jpg" || lower_ext == ".jpeg" ||
                   lower_ext == ".gif" || lower_ext == ".bmp" || lower_ext == ".ppm" ||
                   lower_ext == ".pgm" || lower_ext == ".pbm" || lower_ext == ".tiff" ||
                   lower_ext == ".tif" || lower_ext == ".webp";
        }

        /**
         * @brief Check if file is a video by extension
         */
        inline bool is_video_extension(const std::string &ext)
        {
            std::string lower_ext = ext;
            for (auto &c : lower_ext)
                c = std::tolower(c);

            return lower_ext == ".mp4" || lower_ext == ".avi" || lower_ext == ".mkv" ||
                   lower_ext == ".mov" || lower_ext == ".webm" || lower_ext == ".flv" ||
                   lower_ext == ".wmv" || lower_ext == ".m4v" || lower_ext == ".gif" ||
                   lower_ext == ".mpeg" || lower_ext == ".mpg" || lower_ext == ".3gp";
        }

        /**
         * @brief Check if file is a Pythonic image (.pi)
         */
        inline bool is_pythonic_image(const std::string &filename)
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
         * @brief Check if file is a Pythonic video (.pv)
         */
        inline bool is_pythonic_video(const std::string &filename)
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
         * @brief Check if file is any Pythonic format (.pi or .pv)
         */
        inline bool is_pythonic_format(const std::string &filename)
        {
            return is_pythonic_image(filename) || is_pythonic_video(filename);
        }

        /**
         * @brief Get file extension
         */
        inline std::string get_extension(const std::string &filename)
        {
            size_t dot = filename.rfind('.');
            if (dot == std::string::npos)
                return "";
            return filename.substr(dot);
        }

        /**
         * @brief Get filename without extension
         */
        inline std::string get_basename(const std::string &filename)
        {
            size_t dot = filename.rfind('.');
            if (dot == std::string::npos)
                return filename;
            return filename.substr(0, dot);
        }

        /**
         * @brief Generate random salt
         */
        inline uint32_t generate_salt()
        {
            auto seed = static_cast<unsigned int>(
                std::chrono::steady_clock::now().time_since_epoch().count());
            std::mt19937 gen(seed);
            std::uniform_int_distribution<uint32_t> dist;
            return dist(gen);
        }

        // ==================== Conversion Functions ====================

        /**
         * @brief Convert a media file to Pythonic format
         *
         * Reads the source file, encrypts it, and writes to a .pi or .pv file.
         *
         * @param filepath Path to the source media file
         * @param type Force type (auto_detect by default)
         * @return Path to the created Pythonic file, or empty string on failure
         *
         * @throws std::runtime_error if file cannot be read or type cannot be determined
         */
        inline std::string convert(const std::string &filepath, Type type = Type::auto_detect)
        {
            // Read source file
            std::ifstream infile(filepath, std::ios::binary | std::ios::ate);
            if (!infile)
            {
                throw std::runtime_error("Cannot open file: " + filepath);
            }

            std::streamsize file_size = infile.tellg();
            infile.seekg(0, std::ios::beg);

            std::vector<uint8_t> data(file_size);
            if (!infile.read(reinterpret_cast<char *>(data.data()), file_size))
            {
                throw std::runtime_error("Cannot read file: " + filepath);
            }
            infile.close();

            // Get extension and determine type
            std::string ext = get_extension(filepath);
            bool is_image = false;
            bool is_video = false;

            switch (type)
            {
            case Type::image:
                is_image = true;
                break;
            case Type::video:
                is_video = true;
                break;
            case Type::auto_detect:
            default:
                is_image = is_image_extension(ext);
                is_video = is_video_extension(ext);
                break;
            }

            if (!is_image && !is_video)
            {
                throw std::runtime_error("Cannot determine media type for: " + filepath);
            }

            // Create header
            PythonicMediaHeader header;
            if (is_image)
                header.set_magic_image();
            else
                header.set_magic_video();

            header.set_extension(ext);
            header.original_size = static_cast<uint64_t>(file_size);
            header.salt = generate_salt();

            // Encrypt data
            xor_transform(data, header.salt);

            // Create output filename
            std::string output_path = get_basename(filepath) + (is_image ? ".pi" : ".pv");

            // Write output file
            std::ofstream outfile(output_path, std::ios::binary);
            if (!outfile)
            {
                throw std::runtime_error("Cannot create output file: " + output_path);
            }

            // Write header
            outfile.write(reinterpret_cast<const char *>(&header), sizeof(header));

            // Write encrypted data
            outfile.write(reinterpret_cast<const char *>(data.data()), data.size());
            outfile.close();

            return output_path;
        }

        /**
         * @brief Revert a Pythonic format file back to original format
         *
         * Reads the .pi or .pv file, decrypts it, and writes to the original format.
         *
         * @param filepath Path to the Pythonic file (.pi or .pv)
         * @return Path to the restored file, or empty string on failure
         *
         * @throws std::runtime_error if file is invalid or cannot be read
         */
        inline std::string revert(const std::string &filepath)
        {
            // Read source file
            std::ifstream infile(filepath, std::ios::binary | std::ios::ate);
            if (!infile)
            {
                throw std::runtime_error("Cannot open file: " + filepath);
            }

            std::streamsize file_size = infile.tellg();
            if (file_size < static_cast<std::streamsize>(sizeof(PythonicMediaHeader)))
            {
                throw std::runtime_error("File too small to be valid Pythonic format: " + filepath);
            }

            infile.seekg(0, std::ios::beg);

            // Read header
            PythonicMediaHeader header;
            if (!infile.read(reinterpret_cast<char *>(&header), sizeof(header)))
            {
                throw std::runtime_error("Cannot read header: " + filepath);
            }

            // Validate header
            if (!header.is_valid())
            {
                throw std::runtime_error("Invalid Pythonic format header: " + filepath);
            }

            // Read encrypted data
            std::streamsize data_size = file_size - sizeof(PythonicMediaHeader);
            if (data_size != static_cast<std::streamsize>(header.original_size))
            {
                throw std::runtime_error("Data size mismatch in: " + filepath);
            }

            std::vector<uint8_t> data(data_size);
            if (!infile.read(reinterpret_cast<char *>(data.data()), data_size))
            {
                throw std::runtime_error("Cannot read data: " + filepath);
            }
            infile.close();

            // Decrypt data
            xor_untransform(data, header.salt);

            // Create output filename
            std::string original_ext = header.get_extension();
            std::string output_path = get_basename(filepath) + "_restored" + original_ext;

            // Write output file
            std::ofstream outfile(output_path, std::ios::binary);
            if (!outfile)
            {
                throw std::runtime_error("Cannot create output file: " + output_path);
            }

            outfile.write(reinterpret_cast<const char *>(data.data()), data.size());
            outfile.close();

            return output_path;
        }

        /**
         * @brief Read a Pythonic format file into memory (for internal use)
         *
         * Decrypts and returns the original data without writing to disk.
         *
         * @param filepath Path to the Pythonic file (.pi or .pv)
         * @param[out] original_ext Returns the original file extension
         * @return Decrypted file data
         *
         * @throws std::runtime_error if file is invalid
         */
        inline std::vector<uint8_t> read_pythonic(const std::string &filepath, std::string &original_ext)
        {
            std::ifstream infile(filepath, std::ios::binary | std::ios::ate);
            if (!infile)
            {
                throw std::runtime_error("Cannot open file: " + filepath);
            }

            std::streamsize file_size = infile.tellg();
            if (file_size < static_cast<std::streamsize>(sizeof(PythonicMediaHeader)))
            {
                throw std::runtime_error("File too small: " + filepath);
            }

            infile.seekg(0, std::ios::beg);

            PythonicMediaHeader header;
            if (!infile.read(reinterpret_cast<char *>(&header), sizeof(header)))
            {
                throw std::runtime_error("Cannot read header: " + filepath);
            }

            if (!header.is_valid())
            {
                throw std::runtime_error("Invalid format: " + filepath);
            }

            std::streamsize data_size = file_size - sizeof(PythonicMediaHeader);
            std::vector<uint8_t> data(data_size);
            if (!infile.read(reinterpret_cast<char *>(data.data()), data_size))
            {
                throw std::runtime_error("Cannot read data: " + filepath);
            }
            infile.close();

            xor_untransform(data, header.salt);
            original_ext = header.get_extension();

            return data;
        }

        /**
         * @brief Extract Pythonic file to a temporary file for processing
         *
         * Creates a temporary file with the original format that can be
         * passed to FFmpeg or ImageMagick.
         *
         * @param filepath Path to the Pythonic file
         * @return Path to temporary file (caller must delete when done)
         */
        inline std::string extract_to_temp(const std::string &filepath)
        {
            std::string original_ext;
            std::vector<uint8_t> data = read_pythonic(filepath, original_ext);

            // Create temp file
            std::string temp_path = "/tmp/pythonic_temp_" +
                                    std::to_string(std::hash<std::string>{}(filepath)) +
                                    original_ext;

            std::ofstream outfile(temp_path, std::ios::binary);
            if (!outfile)
            {
                throw std::runtime_error("Cannot create temp file: " + temp_path);
            }

            outfile.write(reinterpret_cast<const char *>(data.data()), data.size());
            outfile.close();

            return temp_path;
        }

        /**
         * @brief Get information about a Pythonic file
         *
         * @param filepath Path to the Pythonic file
         * @return Tuple of (is_image, original_extension, original_size)
         */
        inline std::tuple<bool, std::string, uint64_t> get_info(const std::string &filepath)
        {
            std::ifstream infile(filepath, std::ios::binary);
            if (!infile)
            {
                throw std::runtime_error("Cannot open file: " + filepath);
            }

            PythonicMediaHeader header;
            if (!infile.read(reinterpret_cast<char *>(&header), sizeof(header)))
            {
                throw std::runtime_error("Cannot read header: " + filepath);
            }

            if (!header.is_valid())
            {
                throw std::runtime_error("Invalid format: " + filepath);
            }

            return {header.is_image(), header.get_extension(), header.original_size};
        }

    } // namespace media
} // namespace pythonic
