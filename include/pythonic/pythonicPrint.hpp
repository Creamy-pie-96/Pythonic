#pragma once
#include <sstream>
#include <iostream>
#include "pythonicVars.hpp"
#include "pythonicDraw.hpp"

namespace pythonic
{
    namespace print
    {

        using namespace pythonic::vars;

        /**
         * @brief Media type hints for the print function
         *
         * Usage:
         *   print("file.png", Type::image);     // Force treat as image
         *   print("file.mp4", Type::video);     // Force treat as video
         *   print("info.mp4", Type::video_info); // Show video info only
         *   print("file.png", Type::auto_detect); // Auto-detect from extension
         */
        enum class Type
        {
            auto_detect, // Detect from file extension (default)
            image,       // Force treat as image
            video,       // Force treat as video (play it)
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
         * @brief Print with explicit media type hint
         *
         * Usage:
         *   print("file.png", Type::image);      // Force treat as image
         *   print("file.mp4", Type::video);      // Play video
         *   print("file.mp4", Type::video_info); // Show video metadata only
         *   print("hello", Type::text);          // Force plain text output
         *
         * @param filepath Path or text to print
         * @param type Media type hint (auto_detect, image, video, video_info, text)
         * @param max_width Terminal width for media rendering
         * @param threshold Brightness threshold for braille conversion
         */
        inline void print(const std::string &filepath, Type type, int max_width = 80, int threshold = 128)
        {
            switch (type)
            {
            case Type::image:
                pythonic::draw::print_image(filepath, max_width, threshold);
                break;
            case Type::video:
                pythonic::draw::play_video(filepath, max_width, threshold);
                break;
            case Type::video_info:
                pythonic::draw::print_video_info(filepath);
                break;
            case Type::text:
                std::cout << filepath << std::endl;
                break;
            case Type::auto_detect:
            default:
                if (pythonic::draw::is_video_file(filepath))
                    pythonic::draw::play_video(filepath, max_width, threshold);
                else if (pythonic::draw::is_image_file(filepath))
                    pythonic::draw::print_image(filepath, max_width, threshold);
                else
                    std::cout << filepath << std::endl;
                break;
            }
        }

        // Overload for const char*
        inline void print(const char *filepath, Type type, int max_width = 80, int threshold = 128)
        {
            print(std::string(filepath), type, max_width, threshold);
        }

        /**
         * @brief Specialized print for media files - detects by extension
         *
         * If the string looks like an image or video file path, renders/plays it.
         * Otherwise, prints as normal text.
         */
        inline void print(const char *filepath)
        {
            print(std::string(filepath), Type::auto_detect);
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