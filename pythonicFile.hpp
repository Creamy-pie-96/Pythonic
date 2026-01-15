#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <functional>
#include "pythonicVars.hpp"

namespace pythonic
{
    namespace file
    {

        using namespace pythonic::vars;

        // File modes (like Python's open modes)
        enum class FileMode
        {
            READ,         // "r" - read only
            WRITE,        // "w" - write only (truncate)
            APPEND,       // "a" - append only
            READ_WRITE,   // "r+" - read and write
            WRITE_READ,   // "w+" - write and read (truncate)
            APPEND_READ,  // "a+" - append and read
            READ_BINARY,  // "rb" - read binary
            WRITE_BINARY, // "wb" - write binary
            APPEND_BINARY // "ab" - append binary
        };

        // Parse mode string to FileMode
        inline FileMode parse_mode(const std::string &mode)
        {
            if (mode == "r")
                return FileMode::READ;
            if (mode == "w")
                return FileMode::WRITE;
            if (mode == "a")
                return FileMode::APPEND;
            if (mode == "r+")
                return FileMode::READ_WRITE;
            if (mode == "w+")
                return FileMode::WRITE_READ;
            if (mode == "a+")
                return FileMode::APPEND_READ;
            if (mode == "rb")
                return FileMode::READ_BINARY;
            if (mode == "wb")
                return FileMode::WRITE_BINARY;
            if (mode == "ab")
                return FileMode::APPEND_BINARY;
            throw std::invalid_argument("Invalid file mode: " + mode);
        }

        // File class - Python-like file handle
        class File
        {
        private:
            std::string filename_;
            FileMode mode_;
            std::fstream stream_;
            bool is_open_;
            bool is_binary_;

            std::ios_base::openmode get_openmode() const
            {
                std::ios_base::openmode m = std::ios_base::in; // default

                switch (mode_)
                {
                case FileMode::READ:
                    m = std::ios_base::in;
                    break;
                case FileMode::WRITE:
                    m = std::ios_base::out | std::ios_base::trunc;
                    break;
                case FileMode::APPEND:
                    m = std::ios_base::out | std::ios_base::app;
                    break;
                case FileMode::READ_WRITE:
                    m = std::ios_base::in | std::ios_base::out;
                    break;
                case FileMode::WRITE_READ:
                    m = std::ios_base::in | std::ios_base::out | std::ios_base::trunc;
                    break;
                case FileMode::APPEND_READ:
                    m = std::ios_base::in | std::ios_base::out | std::ios_base::app;
                    break;
                case FileMode::READ_BINARY:
                    m = std::ios_base::in | std::ios_base::binary;
                    break;
                case FileMode::WRITE_BINARY:
                    m = std::ios_base::out | std::ios_base::trunc | std::ios_base::binary;
                    break;
                case FileMode::APPEND_BINARY:
                    m = std::ios_base::out | std::ios_base::app | std::ios_base::binary;
                    break;
                }
                return m;
            }

        public:
            // Constructor
            File(const std::string &filename, const std::string &mode = "r")
                : filename_(filename), mode_(parse_mode(mode)), is_open_(false)
            {
                is_binary_ = (mode.find('b') != std::string::npos);
                open();
            }

            // const char* overload to prevent ambiguity
            File(const char *filename, const char *mode = "r")
                : File(std::string(filename), std::string(mode)) {}

            File(const var &filename, const var &mode = var("r"))
                : File(filename.get<std::string>(), mode.get<std::string>()) {}

            // Destructor - auto close
            ~File()
            {
                close();
            }

            // Prevent copying
            File(const File &) = delete;
            File &operator=(const File &) = delete;

            // Allow moving
            File(File &&other) noexcept
                : filename_(std::move(other.filename_)),
                  mode_(other.mode_),
                  stream_(std::move(other.stream_)),
                  is_open_(other.is_open_),
                  is_binary_(other.is_binary_)
            {
                other.is_open_ = false;
            }

            File &operator=(File &&other) noexcept
            {
                if (this != &other)
                {
                    close();
                    filename_ = std::move(other.filename_);
                    mode_ = other.mode_;
                    stream_ = std::move(other.stream_);
                    is_open_ = other.is_open_;
                    is_binary_ = other.is_binary_;
                    other.is_open_ = false;
                }
                return *this;
            }

            // Open the file
            void open()
            {
                if (is_open_)
                    return;
                stream_.open(filename_, get_openmode());
                if (!stream_.is_open())
                {
                    throw std::runtime_error("Could not open file: " + filename_);
                }
                is_open_ = true;
            }

            // Close the file
            void close()
            {
                if (is_open_)
                {
                    stream_.close();
                    is_open_ = false;
                }
            }

            // Check if file is open
            bool is_open() const
            {
                return is_open_;
            }

            // Read entire file contents
            var read()
            {
                if (!is_open_)
                {
                    throw std::runtime_error("File is not open");
                }

                // Seek to beginning
                stream_.seekg(0, std::ios::beg);

                std::stringstream ss;
                ss << stream_.rdbuf();
                return var(ss.str());
            }

            // Read specific number of characters
            var read(size_t n)
            {
                if (!is_open_)
                {
                    throw std::runtime_error("File is not open");
                }

                std::string buffer(n, '\0');
                stream_.read(&buffer[0], n);
                buffer.resize(stream_.gcount());
                return var(buffer);
            }

            // Read a single line
            var readline()
            {
                if (!is_open_)
                {
                    throw std::runtime_error("File is not open");
                }

                std::string line;
                if (std::getline(stream_, line))
                {
                    return var(line);
                }
                return var("");
            }

            // Read all lines into a list
            var readlines()
            {
                if (!is_open_)
                {
                    throw std::runtime_error("File is not open");
                }

                // Seek to beginning
                stream_.seekg(0, std::ios::beg);

                List lines;
                std::string line;
                while (std::getline(stream_, line))
                {
                    lines.push_back(var(line));
                }
                return var(lines);
            }

            // Write string to file
            void write(const var &content)
            {
                if (!is_open_)
                {
                    throw std::runtime_error("File is not open");
                }

                stream_ << content.get<std::string>();
            }

            // Write string with newline
            void writeln(const var &content)
            {
                write(content);
                stream_ << '\n';
            }

            // Write multiple lines from list
            void writelines(const var &lines)
            {
                if (!is_open_)
                {
                    throw std::runtime_error("File is not open");
                }

                if (auto *lst = std::get_if<List>(&lines.getValue()))
                {
                    for (const auto &line : *lst)
                    {
                        stream_ << line.get<std::string>() << '\n';
                    }
                }
                else
                {
                    throw std::runtime_error("writelines() requires a list");
                }
            }

            // Flush buffer
            void flush()
            {
                if (is_open_)
                {
                    stream_.flush();
                }
            }

            // Seek to position
            void seek(std::streampos pos)
            {
                if (is_open_)
                {
                    stream_.seekg(pos);
                    stream_.seekp(pos);
                }
            }

            // Get current position
            var tell()
            {
                if (is_open_)
                {
                    return var(static_cast<long long>(stream_.tellg()));
                }
                return var(-1);
            }

            // Check if at end of file
            bool eof() const
            {
                return stream_.eof();
            }

            // Get filename
            var name() const
            {
                return var(filename_);
            }

            // Get mode as string
            var mode() const
            {
                switch (mode_)
                {
                case FileMode::READ:
                    return var("r");
                case FileMode::WRITE:
                    return var("w");
                case FileMode::APPEND:
                    return var("a");
                case FileMode::READ_WRITE:
                    return var("r+");
                case FileMode::WRITE_READ:
                    return var("w+");
                case FileMode::APPEND_READ:
                    return var("a+");
                case FileMode::READ_BINARY:
                    return var("rb");
                case FileMode::WRITE_BINARY:
                    return var("wb");
                case FileMode::APPEND_BINARY:
                    return var("ab");
                }
                return var("unknown");
            }

            // Bool conversion - true if file is open
            explicit operator bool() const
            {
                return is_open_;
            }

            // Iterator support for reading lines
            class line_iterator
            {
            private:
                File *file_;
                std::string current_line_;
                bool at_end_;

            public:
                line_iterator(File *f, bool end = false) : file_(f), at_end_(end)
                {
                    if (!at_end_ && file_)
                    {
                        ++(*this); // Read first line
                    }
                }

                var operator*() const
                {
                    return var(current_line_);
                }

                line_iterator &operator++()
                {
                    if (file_ && !file_->eof())
                    {
                        if (!std::getline(file_->stream_, current_line_))
                        {
                            at_end_ = true;
                        }
                    }
                    else
                    {
                        at_end_ = true;
                    }
                    return *this;
                }

                bool operator!=(const line_iterator &other) const
                {
                    return at_end_ != other.at_end_;
                }
            };

            line_iterator begin()
            {
                stream_.seekg(0, std::ios::beg);
                return line_iterator(this, false);
            }

            line_iterator end()
            {
                return line_iterator(this, true);
            }
        };

        // Global open() function - like Python's open()
        inline File open(const std::string &filename, const std::string &mode = "r")
        {
            return File(filename, mode);
        }

        inline File open(const char *filename, const char *mode = "r")
        {
            return File(std::string(filename), std::string(mode));
        }

        inline File open(const var &filename, const var &mode = var("r"))
        {
            return File(filename.get<std::string>(), mode.get<std::string>());
        }

        // with() style file handling using RAII
        // Usage: with_file("file.txt", "r", [](File& f) { ... });
        template <typename Func>
        void with_file(const std::string &filename, const std::string &mode, Func func)
        {
            File f(filename, mode);
            func(f);
            // File automatically closes when going out of scope
        }

        template <typename Func>
        void with_file(const var &filename, const var &mode, Func func)
        {
            with_file(filename.get<std::string>(), mode.get<std::string>(), func);
        }

// Macro for Python-like with statement
// Usage: with_open("file.txt", "r", f) { f.read(); }
#define with_open(filename, mode, var_name) \
    if (pythonic::file::File var_name(filename, mode); true)

        // Quick file reading helpers
        inline var read_file(const std::string &filename)
        {
            File f(filename, "r");
            return f.read();
        }

        inline var read_file(const char *filename)
        {
            return read_file(std::string(filename));
        }

        inline var read_file(const var &filename)
        {
            return read_file(filename.get<std::string>());
        }

        inline var read_lines(const std::string &filename)
        {
            File f(filename, "r");
            return f.readlines();
        }

        inline var read_lines(const char *filename)
        {
            return read_lines(std::string(filename));
        }

        inline var read_lines(const var &filename)
        {
            return read_lines(filename.get<std::string>());
        }

        // Quick file writing helpers
        inline void write_file(const std::string &filename, const var &content)
        {
            File f(filename, "w");
            f.write(content);
        }

        inline void write_file(const char *filename, const var &content)
        {
            write_file(std::string(filename), content);
        }

        inline void write_file(const var &filename, const var &content)
        {
            write_file(filename.get<std::string>(), content);
        }

        inline void append_file(const std::string &filename, const var &content)
        {
            File f(filename, "a");
            f.write(content);
        }

        inline void append_file(const char *filename, const var &content)
        {
            append_file(std::string(filename), content);
        }

        inline void append_file(const var &filename, const var &content)
        {
            append_file(filename.get<std::string>(), content);
        }

        inline void write_lines(const std::string &filename, const var &lines)
        {
            File f(filename, "w");
            f.writelines(lines);
        }

        inline void write_lines(const char *filename, const var &lines)
        {
            write_lines(std::string(filename), lines);
        }

        inline void write_lines(const var &filename, const var &lines)
        {
            write_lines(filename.get<std::string>(), lines);
        }

        // Check if file exists
        inline bool file_exists(const std::string &filename)
        {
            std::ifstream f(filename);
            return f.good();
        }

        inline bool file_exists(const char *filename)
        {
            return file_exists(std::string(filename));
        }

        inline var file_exists(const var &filename)
        {
            return var(file_exists(filename.get<std::string>()));
        }

    } // namespace file
} // namespace pythonic
