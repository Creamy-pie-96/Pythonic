/**
 * @file TerminalWindow.hpp
 * @brief Main window class for terminal rendering
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "../Event/Events.hpp"
#include "VideoMode.hpp"
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <iostream>
#include <signal.h>

TG_NAMESPACE_BEGIN

/**
 * @brief Window style flags
 */
namespace Style
{
    enum : unsigned int
    {
        None = 0,            ///< No decorations
        Titlebar = 1 << 0,   ///< Show title bar (terminal title)
        Close = 1 << 1,      ///< Handle Ctrl+C gracefully
        Fullscreen = 1 << 2, ///< Use entire terminal
        Default = Titlebar | Close
    };
}

/**
 * @brief Terminal window for rendering graphics
 *
 * The main entry point for terminal-based graphics applications.
 * Manages the terminal state, provides double-buffered rendering,
 * and handles input events.
 *
 * @code
 * // Create a window
 * TerminalWindow window(VideoMode::getDesktopMode(), "My Game");
 *
 * // Main loop
 * while (window.isOpen())
 * {
 *     Event event;
 *     while (window.pollEvent(event))
 *     {
 *         if (event.type == EventType::Closed)
 *             window.close();
 *     }
 *
 *     window.clear(Color::Black);
 *     // Draw things...
 *     window.display();
 * }
 * @endcode
 */
class TerminalWindow
{
public:
    /**
     * @brief Default constructor (creates an invalid window)
     */
    TerminalWindow()
        : m_isOpen(false), m_hasFocus(true), m_style(Style::Default) {}

    /**
     * @brief Create a window with the given mode and title
     * @param mode Video mode (terminal dimensions)
     * @param title Window title (shown in terminal title bar)
     * @param style Window style flags
     */
    TerminalWindow(const VideoMode &mode, const std::string &title,
                   unsigned int style = Style::Default)
    {
        create(mode, title, style);
    }

    /**
     * @brief Destructor - restores terminal state
     */
    ~TerminalWindow()
    {
        close();
    }

    // Non-copyable
    TerminalWindow(const TerminalWindow &) = delete;
    TerminalWindow &operator=(const TerminalWindow &) = delete;

    // Movable
    TerminalWindow(TerminalWindow &&other) noexcept
        : m_mode(other.m_mode), m_title(std::move(other.m_title)), m_style(other.m_style), m_isOpen(other.m_isOpen.load()), m_hasFocus(other.m_hasFocus), m_frontBuffer(std::move(other.m_frontBuffer)), m_backBuffer(std::move(other.m_backBuffer))
    {
        other.m_isOpen = false;
    }

    TerminalWindow &operator=(TerminalWindow &&other) noexcept
    {
        if (this != &other)
        {
            close();
            m_mode = other.m_mode;
            m_title = std::move(other.m_title);
            m_style = other.m_style;
            m_isOpen = other.m_isOpen.load();
            m_hasFocus = other.m_hasFocus;
            m_frontBuffer = std::move(other.m_frontBuffer);
            m_backBuffer = std::move(other.m_backBuffer);
            other.m_isOpen = false;
        }
        return *this;
    }

    /**
     * @brief Create the window with specified parameters
     */
    void create(const VideoMode &mode, const std::string &title,
                unsigned int style = Style::Default)
    {
        m_mode = mode;
        m_title = title;
        m_style = style;

        // Initialize buffers
        size_t bufferSize = static_cast<size_t>(mode.width) * mode.height;
        m_frontBuffer.resize(bufferSize);
        m_backBuffer.resize(bufferSize);

        // Clear buffers
        Cell emptyCell{' ', Color::White, Color::Black};
        std::fill(m_frontBuffer.begin(), m_frontBuffer.end(), emptyCell);
        std::fill(m_backBuffer.begin(), m_backBuffer.end(), emptyCell);

        // Setup terminal
        setupTerminal();

        // Initialize keyboard
        Keyboard::init();

        // Set window title if supported
        if (m_style & Style::Titlebar)
            setTitle(title);

        m_isOpen = true;

        // Register for resize signals
        registerResizeHandler();
    }

    /**
     * @brief Close the window and restore terminal
     */
    void close()
    {
        if (!m_isOpen)
            return;

        m_isOpen = false;

        // Shutdown keyboard
        Keyboard::shutdown();

        // Restore terminal
        restoreTerminal();

        // Clear buffers
        m_frontBuffer.clear();
        m_backBuffer.clear();
    }

    /**
     * @brief Check if the window is open
     */
    bool isOpen() const
    {
        return m_isOpen;
    }

    /**
     * @brief Poll for the next event
     * @param event Output parameter for the event
     * @return true if an event was available
     */
    bool pollEvent(Event &event)
    {
        // Check for close request
        if (Keyboard::isKeyPressed(Key::Escape) && (m_style & Style::Close))
        {
            event = Event::closed();
            return true;
        }

        // Check for key presses
        for (int i = 0; i < static_cast<int>(Key::KeyCount); ++i)
        {
            Key key = static_cast<Key>(i);
            if (Keyboard::isKeyJustPressed(key))
            {
                event = Event::keyPressed(key);
                return true;
            }
        }

        // Check for resize
        if (m_resizeRequested)
        {
            m_resizeRequested = false;
            VideoMode newMode = VideoMode::getDesktopMode();
            if (newMode != m_mode)
            {
                m_mode = newMode;
                resizeBuffers();
                event = Event::resized(m_mode.width, m_mode.height);
                return true;
            }
        }

        return m_eventQueue.poll(event);
    }

    /**
     * @brief Wait for an event (blocking)
     * @param event Output parameter for the event
     * @return true if an event was received
     */
    bool waitEvent(Event &event)
    {
        while (m_isOpen)
        {
            if (pollEvent(event))
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }

    /**
     * @brief Clear the window with a color
     * @param color Fill color (default: black)
     */
    void clear(const Color &color = Color::Black)
    {
        Cell clearCell{' ', Color::White, color};
        std::fill(m_backBuffer.begin(), m_backBuffer.end(), clearCell);
    }

    /**
     * @brief Set a character at a position
     * @param x Column (0-based)
     * @param y Row (0-based)
     * @param c Character to draw
     * @param fg Foreground color
     * @param bg Background color
     */
    void setCell(unsigned int x, unsigned int y, char c,
                 const Color &fg = Color::White,
                 const Color &bg = Color::Black)
    {
        if (x >= m_mode.width || y >= m_mode.height)
            return;

        size_t idx = static_cast<size_t>(y) * m_mode.width + x;
        m_backBuffer[idx] = Cell{c, fg, bg};
    }

    /**
     * @brief Set a wide/Unicode character at a position
     */
    void setCellWide(unsigned int x, unsigned int y, const std::string &utf8Char,
                     const Color &fg = Color::White,
                     const Color &bg = Color::Black)
    {
        if (x >= m_mode.width || y >= m_mode.height)
            return;

        size_t idx = static_cast<size_t>(y) * m_mode.width + x;
        m_backBuffer[idx] = Cell{utf8Char.empty() ? ' ' : utf8Char[0], fg, bg};
        // For now, just use first byte - full Unicode support would need
        // a string-based cell implementation
    }

    /**
     * @brief Draw text at a position
     * @param x Starting column
     * @param y Row
     * @param text Text to draw
     * @param fg Foreground color
     * @param bg Background color
     */
    void drawText(unsigned int x, unsigned int y, const std::string &text,
                  const Color &fg = Color::White,
                  const Color &bg = Color::Black)
    {
        for (size_t i = 0; i < text.size() && x + i < m_mode.width; ++i)
        {
            setCell(x + static_cast<unsigned int>(i), y, text[i], fg, bg);
        }
    }

    /**
     * @brief Display the back buffer to the terminal
     *
     * Performs double-buffered rendering - only draws cells that changed.
     */
    void display()
    {
        std::string output;
        output.reserve(m_mode.width * m_mode.height * 20); // Estimate

        // Move cursor to home
        output += ansi::CURSOR_HOME;

        Color lastFg = Color::White;
        Color lastBg = Color::Black;
        bool needsColorReset = true;

        for (unsigned int y = 0; y < m_mode.height; ++y)
        {
            for (unsigned int x = 0; x < m_mode.width; ++x)
            {
                size_t idx = static_cast<size_t>(y) * m_mode.width + x;
                const Cell &cell = m_backBuffer[idx];
                const Cell &frontCell = m_frontBuffer[idx];

                // Only update changed cells for efficiency
                bool cellChanged = (cell.ch != frontCell.ch ||
                                    cell.fg != frontCell.fg ||
                                    cell.bg != frontCell.bg);

                if (cellChanged || needsColorReset)
                {
                    // Update colors if needed
                    if (cell.fg != lastFg || cell.bg != lastBg || needsColorReset)
                    {
                        output += cell.fg.toAnsiFg();
                        output += cell.bg.toAnsiBg();
                        lastFg = cell.fg;
                        lastBg = cell.bg;
                        needsColorReset = false;
                    }
                }

                output += cell.ch;
            }

            // Don't add newline after last row
            if (y < m_mode.height - 1)
                output += "\n";
        }

        // Reset colors at end
        output += ansi::RESET;

        // Write to terminal
        write(STDOUT_FILENO, output.c_str(), output.size());

        // Swap buffers
        std::swap(m_frontBuffer, m_backBuffer);
    }

    /**
     * @brief Set the window title
     */
    void setTitle(const std::string &title)
    {
        m_title = title;
        // Set terminal title using OSC escape sequence
        std::string cmd = "\033]0;" + title + "\007";
        write(STDOUT_FILENO, cmd.c_str(), cmd.size());
    }

    /**
     * @brief Get the window size
     */
    Vector2u getSize() const
    {
        return Vector2u(m_mode.width, m_mode.height);
    }

    /**
     * @brief Get the video mode
     */
    const VideoMode &getVideoMode() const
    {
        return m_mode;
    }

    /**
     * @brief Check if window has focus
     */
    bool hasFocus() const
    {
        return m_hasFocus;
    }

    /**
     * @brief Set framerate limit
     * @param limit Target FPS (0 = unlimited)
     */
    void setFramerateLimit(unsigned int limit)
    {
        m_framerateLimit = limit;
        if (limit > 0)
            m_frameTime = Time::seconds(1.0f / static_cast<float>(limit));
        else
            m_frameTime = Time::Zero;
    }

    /**
     * @brief Enable/disable vertical sync (no-op for terminal)
     */
    void setVerticalSyncEnabled(bool /*enabled*/)
    {
        // Terminals don't have vsync, but we keep the API for compatibility
    }

    /**
     * @brief Set cursor visibility
     */
    void setMouseCursorVisible(bool visible)
    {
        m_cursorVisible = visible;
        if (visible)
            std::cout << "\033[?25h"; // Show cursor
        else
            std::cout << "\033[?25l"; // Hide cursor
    }

    /**
     * @brief Push an event to the queue (for custom events)
     */
    void pushEvent(const Event &event)
    {
        m_eventQueue.push(event);
    }

private:
    /**
     * @brief A single character cell in the buffer
     */
    struct Cell
    {
        char ch = ' ';
        Color fg = Color::White;
        Color bg = Color::Black;

        bool operator==(const Cell &other) const
        {
            return ch == other.ch && fg == other.fg && bg == other.bg;
        }

        bool operator!=(const Cell &other) const
        {
            return !(*this == other);
        }
    };

    VideoMode m_mode;
    std::string m_title;
    unsigned int m_style = Style::Default;
    std::atomic<bool> m_isOpen{false};
    bool m_hasFocus = true;
    bool m_cursorVisible = false;
    unsigned int m_framerateLimit = 0;
    Time m_frameTime = Time::Zero;

    std::vector<Cell> m_frontBuffer;
    std::vector<Cell> m_backBuffer;

    EventQueue m_eventQueue;
    static inline std::atomic<bool> m_resizeRequested{false};

#ifndef TG_PLATFORM_WINDOWS
    static inline struct termios s_originalTermios;
    static inline bool s_termiosSaved = false;
#endif

    void setupTerminal()
    {
        // Hide cursor
        setMouseCursorVisible(false);

        // Clear screen
        std::cout << ansi::CLEAR_SCREEN << ansi::CURSOR_HOME << std::flush;

        // Enable alternate screen buffer for clean restore
        std::cout << "\033[?1049h" << std::flush;

#ifndef TG_PLATFORM_WINDOWS
        // Save terminal state (for raw mode, handled by Keyboard)
#endif
    }

    void restoreTerminal()
    {
        // Disable alternate screen buffer
        std::cout << "\033[?1049l" << std::flush;

        // Show cursor
        std::cout << "\033[?25h" << std::flush;

        // Reset colors
        std::cout << ansi::RESET << std::flush;
    }

    void resizeBuffers()
    {
        size_t bufferSize = static_cast<size_t>(m_mode.width) * m_mode.height;
        m_frontBuffer.resize(bufferSize);
        m_backBuffer.resize(bufferSize);

        Cell emptyCell{' ', Color::White, Color::Black};
        std::fill(m_frontBuffer.begin(), m_frontBuffer.end(), emptyCell);
        std::fill(m_backBuffer.begin(), m_backBuffer.end(), emptyCell);
    }

    void registerResizeHandler()
    {
#ifndef TG_PLATFORM_WINDOWS
        struct sigaction sa;
        sa.sa_handler = [](int)
        { m_resizeRequested = true; };
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGWINCH, &sa, nullptr);
#endif
    }
};

TG_NAMESPACE_END
