/**
 * @file Config.hpp
 * @brief Configuration and feature detection for TerminalGraphics
 *
 * Part of the Pythonic TerminalGraphics library - a terminal-based graphics
 * engine inspired by SFML for creating games and interactive applications.
 */

#pragma once

// Version information
#define TG_VERSION_MAJOR 1
#define TG_VERSION_MINOR 0
#define TG_VERSION_PATCH 0

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define TG_PLATFORM_WINDOWS
#elif defined(__linux__)
#define TG_PLATFORM_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
#define TG_PLATFORM_MACOS
#else
#define TG_PLATFORM_UNKNOWN
#endif

// Feature detection
#ifdef TG_PLATFORM_WINDOWS
#include <conio.h>
#include <windows.h>
#define TG_HAS_VIRTUAL_TERMINAL
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#define TG_HAS_TERMIOS
#endif

// Export/Import macros (for potential DLL support)
#define TG_API

// Namespace configuration
#define TG_NAMESPACE_BEGIN \
    namespace Pythonic     \
    {                      \
        namespace TG       \
        {
#define TG_NAMESPACE_END \
    }                    \
    }

// Helper macros
#define TG_NON_COPYABLE(ClassName)         \
    ClassName(const ClassName &) = delete; \
    ClassName &operator=(const ClassName &) = delete;

#define TG_NON_MOVABLE(ClassName)     \
    ClassName(ClassName &&) = delete; \
    ClassName &operator=(ClassName &&) = delete;

// ANSI escape codes
namespace Pythonic
{
    namespace TG
    {
        namespace ansi
        {

            // Cursor movement
            inline constexpr const char *CURSOR_HOME = "\033[H";
            inline constexpr const char *CURSOR_UP = "\033[A";
            inline constexpr const char *CURSOR_DOWN = "\033[B";
            inline constexpr const char *CURSOR_RIGHT = "\033[C";
            inline constexpr const char *CURSOR_LEFT = "\033[D";
            inline constexpr const char *CURSOR_HIDE = "\033[?25l";
            inline constexpr const char *CURSOR_SHOW = "\033[?25h";

            // Screen control
            inline constexpr const char *CLEAR_SCREEN = "\033[2J";
            inline constexpr const char *CLEAR_LINE = "\033[2K";
            inline constexpr const char *RESET = "\033[0m";

            // Alternate screen buffer
            inline constexpr const char *ALT_BUFFER_ON = "\033[?1049h";
            inline constexpr const char *ALT_BUFFER_OFF = "\033[?1049l";

            // Mouse tracking
            inline constexpr const char *MOUSE_ON = "\033[?1000h";
            inline constexpr const char *MOUSE_OFF = "\033[?1000l";

        } // namespace ansi
    } // namespace TG
} // namespace Pythonic
