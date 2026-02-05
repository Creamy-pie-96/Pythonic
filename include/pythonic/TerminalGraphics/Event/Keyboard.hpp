/**
 * @file Keyboard.hpp
 * @brief Keyboard input handling with non-blocking key detection
 *
 * Supports two modes:
 * 1. Terminal mode (default): Uses stdin for input, works everywhere
 * 2. Evdev mode (Linux only, opt-in): Direct hardware access for simultaneous keys
 *
 * To enable evdev mode, define PYTHONIC_USE_EVDEV before including this header.
 * Note: Evdev requires read access to /dev/input/event* (usually root or input group)
 *
 * Also handles mouse input parsing when Mouse is initialized.
 */

#pragma once

#include "../Config.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <array>
#include <chrono>
#include <cstdlib>

// Optional evdev support for direct hardware keyboard access
#ifdef PYTHONIC_USE_EVDEV
#include <linux/input.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#endif

TG_NAMESPACE_BEGIN

/**
 * @brief Keyboard key codes
 *
 * Enumeration of all supported keyboard keys for input detection.
 */
enum class Key
{
    Unknown = -1,

    // Letters
    A = 0,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    // Numbers
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,

    // Function keys (limited terminal support)
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    // Arrow keys
    Left,
    Right,
    Up,
    Down,

    // Navigation
    Home,
    End,
    PageUp,
    PageDown,
    Insert,
    Delete,

    // Modifiers
    Escape,
    Tab,
    Backspace,
    Enter,
    Space,

    // Punctuation (basic)
    Comma,
    Period,
    Semicolon,
    Quote,
    Slash,
    Backslash,
    LeftBracket,
    RightBracket,
    Minus,
    Equal,
    Grave,

    KeyCount // Number of keys
};

/**
 * @brief Thread-safe keyboard input manager
 *
 * Provides SFML-like keyboard input with isKeyPressed() for real-time queries.
 * Uses a background thread for non-blocking input capture.
 *
 * @code
 * Keyboard::init();
 * while (running) {
 *     if (Keyboard::isKeyPressed(Key::Left))
 *         player.moveLeft();
 *     if (Keyboard::isKeyPressed(Key::Space))
 *         player.jump();
 * }
 * Keyboard::shutdown();
 * @endcode
 */
class Keyboard
{
public:
    /**
     * @brief Initialize keyboard input system
     *
     * Must be called before using isKeyPressed(). Starts the background
     * input thread. On Linux with PYTHONIC_USE_EVDEV defined, tries to use
     * direct hardware access for better simultaneous key detection.
     */
    static void init()
    {
        if (s_initialized.exchange(true))
            return;

        s_running = true;
        s_keyStates.fill(false);
        s_lastPressTime.fill(std::chrono::steady_clock::time_point{});

#ifdef PYTHONIC_USE_EVDEV
        // Try evdev first for better key handling
        s_useEvdev = initEvdev();
        if (s_useEvdev)
        {
            s_inputThread = std::thread(evdevInputThread);
            return;
        }
        // Fall back to terminal mode if evdev fails
#endif

        // Set terminal to raw mode
        setRawMode();

        // Start input thread
        s_inputThread = std::thread(inputThreadFunc);
    }

    /**
     * @brief Check if evdev mode is active
     */
    static bool isEvdevMode()
    {
#ifdef PYTHONIC_USE_EVDEV
        return s_useEvdev;
#else
        return false;
#endif
    }

    /**
     * @brief Shutdown keyboard input system
     *
     * Stops the background thread and restores terminal settings.
     */
    static void shutdown()
    {
        if (!s_initialized.load())
            return;

        s_running = false;
        if (s_inputThread.joinable())
            s_inputThread.join();

#ifdef PYTHONIC_USE_EVDEV
        if (s_useEvdev)
        {
            closeEvdev();
            s_useEvdev = false;
        }
        else
#endif
        {
            restoreTerminal();
        }
        s_initialized.store(false);
    }

    /**
     * @brief Check if a key is currently pressed
     * @param key The key to check
     * @return true if the key is pressed
     *
     * On Windows: Uses GetAsyncKeyState for true simultaneous detection.
     * On Linux with evdev: Returns actual hardware key state.
     * In terminal mode (fallback): Returns true if key was pressed within last 200ms.
     */
    static bool isKeyPressed(Key key)
    {
        if (key == Key::Unknown || static_cast<int>(key) >= static_cast<int>(Key::KeyCount))
            return false;

        std::lock_guard<std::mutex> lock(s_mutex);

#ifdef TG_PLATFORM_WINDOWS
        // Windows always uses GetAsyncKeyState - return actual state
        return s_keyStates[static_cast<size_t>(key)];
#else
#ifdef PYTHONIC_USE_EVDEV
        if (s_useEvdev)
        {
            // In evdev mode, we have true key state - return it directly
            return s_keyStates[static_cast<size_t>(key)];
        }
#endif
        // Terminal mode: use sticky window approach for multi-key simulation
        auto now = std::chrono::steady_clock::now();
        auto lastPress = s_lastPressTime[static_cast<size_t>(key)];
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPress);

        return elapsed.count() < 200;
#endif
    }

    /**
     * @brief Check if a key was just pressed this frame
     * @param key The key to check
     * @return true if the key was just pressed
     *
     * Returns true only once per key press event.
     */
    static bool isKeyJustPressed(Key key)
    {
        if (key == Key::Unknown || static_cast<int>(key) >= static_cast<int>(Key::KeyCount))
            return false;

        std::lock_guard<std::mutex> lock(s_mutex);

        // Check if key was pressed very recently (within 16ms ~= 1 frame at 60fps)
        auto now = std::chrono::steady_clock::now();
        auto lastPress = s_lastPressTime[static_cast<size_t>(key)];
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPress);

        if (elapsed.count() < 16 && !s_keyConsumed[static_cast<size_t>(key)])
        {
            s_keyConsumed[static_cast<size_t>(key)] = true;
            return true;
        }

        return false;
    }

    /**
     * @brief Clear all key states
     *
     * Useful when transitioning between game states.
     */
    static void clearStates()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_keyStates.fill(false);
        s_keyConsumed.fill(false);
    }

private:
    static inline std::atomic<bool> s_initialized{false};
    static inline std::atomic<bool> s_running{false};
    static inline std::thread s_inputThread;
    static inline std::mutex s_mutex;

    static inline std::array<bool, static_cast<size_t>(Key::KeyCount)> s_keyStates{};
    static inline std::array<bool, static_cast<size_t>(Key::KeyCount)> s_keyConsumed{};
    static inline std::array<std::chrono::steady_clock::time_point,
                             static_cast<size_t>(Key::KeyCount)>
        s_lastPressTime{};

#ifdef PYTHONIC_USE_EVDEV
    static inline bool s_useEvdev = false;
#endif

#ifdef PYTHONIC_USE_EVDEV
    // Evdev direct hardware access (Linux only)
    static inline int s_evdevFd = -1;

    static bool initEvdev()
    {
        // Try to find a keyboard in /dev/input/
        DIR *dir = opendir("/dev/input");
        if (!dir)
            return false;

        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            if (strncmp(entry->d_name, "event", 5) != 0)
                continue;

            char path[256];
            snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd < 0)
                continue;

            // Check if this device has keyboard capabilities
            unsigned long evbit = 0;
            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) >= 0)
            {
                if (evbit & (1 << EV_KEY))
                {
                    // Check for letter keys to confirm it's a keyboard
                    unsigned long keybit[KEY_MAX / 8 / sizeof(long) + 1] = {0};
                    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0)
                    {
                        // Check if 'A' key exists
                        if (keybit[KEY_A / (8 * sizeof(long))] & (1UL << (KEY_A % (8 * sizeof(long)))))
                        {
                            s_evdevFd = fd;
                            closedir(dir);
                            return true;
                        }
                    }
                }
            }
            close(fd);
        }
        closedir(dir);
        return false;
    }

    static void closeEvdev()
    {
        if (s_evdevFd >= 0)
        {
            close(s_evdevFd);
            s_evdevFd = -1;
        }
    }

    static Key evdevToKey(int code)
    {
        // Map Linux input codes to our Key enum
        if (code >= KEY_A && code <= KEY_Z)
            return static_cast<Key>(code - KEY_A); // A=0, B=1, etc.
        if (code >= KEY_1 && code <= KEY_9)
            return static_cast<Key>(static_cast<int>(Key::Num1) + (code - KEY_1));
        if (code == KEY_0)
            return Key::Num0;

        switch (code)
        {
        case KEY_ESC:
            return Key::Escape;
        case KEY_TAB:
            return Key::Tab;
        case KEY_BACKSPACE:
            return Key::Backspace;
        case KEY_ENTER:
            return Key::Enter;
        case KEY_SPACE:
            return Key::Space;
        case KEY_UP:
            return Key::Up;
        case KEY_DOWN:
            return Key::Down;
        case KEY_LEFT:
            return Key::Left;
        case KEY_RIGHT:
            return Key::Right;
        case KEY_HOME:
            return Key::Home;
        case KEY_END:
            return Key::End;
        case KEY_PAGEUP:
            return Key::PageUp;
        case KEY_PAGEDOWN:
            return Key::PageDown;
        case KEY_INSERT:
            return Key::Insert;
        case KEY_DELETE:
            return Key::Delete;
        case KEY_COMMA:
            return Key::Comma;
        case KEY_DOT:
            return Key::Period;
        case KEY_SEMICOLON:
            return Key::Semicolon;
        case KEY_APOSTROPHE:
            return Key::Quote;
        case KEY_SLASH:
            return Key::Slash;
        case KEY_BACKSLASH:
            return Key::Backslash;
        case KEY_LEFTBRACE:
            return Key::LeftBracket;
        case KEY_RIGHTBRACE:
            return Key::RightBracket;
        case KEY_MINUS:
            return Key::Minus;
        case KEY_EQUAL:
            return Key::Equal;
        case KEY_GRAVE:
            return Key::Grave;
        case KEY_F1:
            return Key::F1;
        case KEY_F2:
            return Key::F2;
        case KEY_F3:
            return Key::F3;
        case KEY_F4:
            return Key::F4;
        case KEY_F5:
            return Key::F5;
        case KEY_F6:
            return Key::F6;
        case KEY_F7:
            return Key::F7;
        case KEY_F8:
            return Key::F8;
        case KEY_F9:
            return Key::F9;
        case KEY_F10:
            return Key::F10;
        case KEY_F11:
            return Key::F11;
        case KEY_F12:
            return Key::F12;
        }
        return Key::Unknown;
    }

    static void evdevInputThread()
    {
        struct input_event ev;
        while (s_running)
        {
            while (read(s_evdevFd, &ev, sizeof(ev)) == sizeof(ev))
            {
                if (ev.type == EV_KEY)
                {
                    Key key = evdevToKey(ev.code);
                    if (key != Key::Unknown)
                    {
                        std::lock_guard<std::mutex> lock(s_mutex);
                        size_t idx = static_cast<size_t>(key);

                        if (ev.value == 1 || ev.value == 2) // Press or repeat
                        {
                            s_keyStates[idx] = true;
                            s_lastPressTime[idx] = std::chrono::steady_clock::now();
                            s_keyConsumed[idx] = false;
                        }
                        else if (ev.value == 0) // Release
                        {
                            s_keyStates[idx] = false;
                        }
                    }
                }
            }
            // 2ms sleep - good balance for responsiveness vs CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
#endif

#ifdef TG_PLATFORM_WINDOWS
    // Windows implementation with GetAsyncKeyState for true simultaneous key detection
    static void setRawMode() { /* Not needed - using GetAsyncKeyState */ }
    static void restoreTerminal() {}

    static int winKeyToVK(Key key)
    {
        // Convert our Key enum to Windows Virtual Key codes
        if (static_cast<int>(key) >= static_cast<int>(Key::A) &&
            static_cast<int>(key) <= static_cast<int>(Key::Z))
        {
            return 'A' + (static_cast<int>(key) - static_cast<int>(Key::A));
        }
        if (static_cast<int>(key) >= static_cast<int>(Key::Num0) &&
            static_cast<int>(key) <= static_cast<int>(Key::Num9))
        {
            return '0' + (static_cast<int>(key) - static_cast<int>(Key::Num0));
        }

        switch (key)
        {
        case Key::Escape:
            return VK_ESCAPE;
        case Key::Tab:
            return VK_TAB;
        case Key::Backspace:
            return VK_BACK;
        case Key::Enter:
            return VK_RETURN;
        case Key::Space:
            return VK_SPACE;
        case Key::Up:
            return VK_UP;
        case Key::Down:
            return VK_DOWN;
        case Key::Left:
            return VK_LEFT;
        case Key::Right:
            return VK_RIGHT;
        case Key::Home:
            return VK_HOME;
        case Key::End:
            return VK_END;
        case Key::PageUp:
            return VK_PRIOR;
        case Key::PageDown:
            return VK_NEXT;
        case Key::Insert:
            return VK_INSERT;
        case Key::Delete:
            return VK_DELETE;
        case Key::F1:
            return VK_F1;
        case Key::F2:
            return VK_F2;
        case Key::F3:
            return VK_F3;
        case Key::F4:
            return VK_F4;
        case Key::F5:
            return VK_F5;
        case Key::F6:
            return VK_F6;
        case Key::F7:
            return VK_F7;
        case Key::F8:
            return VK_F8;
        case Key::F9:
            return VK_F9;
        case Key::F10:
            return VK_F10;
        case Key::F11:
            return VK_F11;
        case Key::F12:
            return VK_F12;
        case Key::Comma:
            return VK_OEM_COMMA;
        case Key::Period:
            return VK_OEM_PERIOD;
        case Key::Semicolon:
            return VK_OEM_1;
        case Key::Quote:
            return VK_OEM_7;
        case Key::Slash:
            return VK_OEM_2;
        case Key::Backslash:
            return VK_OEM_5;
        case Key::LeftBracket:
            return VK_OEM_4;
        case Key::RightBracket:
            return VK_OEM_6;
        case Key::Minus:
            return VK_OEM_MINUS;
        case Key::Equal:
            return VK_OEM_PLUS;
        case Key::Grave:
            return VK_OEM_3;
        default:
            return 0;
        }
    }

    static void inputThreadFunc()
    {
        // Windows: Use GetAsyncKeyState for true simultaneous key detection
        while (s_running)
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            auto now = std::chrono::steady_clock::now();

            for (int i = 0; i < static_cast<int>(Key::KeyCount); ++i)
            {
                Key key = static_cast<Key>(i);
                int vk = winKeyToVK(key);
                if (vk != 0)
                {
                    // GetAsyncKeyState returns high bit set if key is currently down
                    bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
                    size_t idx = static_cast<size_t>(key);

                    if (isDown)
                    {
                        if (!s_keyStates[idx])
                        {
                            s_keyConsumed[idx] = false;
                        }
                        s_keyStates[idx] = true;
                        s_lastPressTime[idx] = now;
                    }
                    else
                    {
                        s_keyStates[idx] = false;
                    }
                }
            }

            // Poll at 500Hz (2ms) for good responsiveness
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

#else
    // Unix/Linux implementation
    static inline struct termios s_oldTermios;
    static inline bool s_termiosSaved = false;

    static void setRawMode()
    {
        struct termios raw;
        if (tcgetattr(STDIN_FILENO, &s_oldTermios) == 0)
        {
            s_termiosSaved = true;
            raw = s_oldTermios;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);

            // Register atexit handler for normal exit cleanup
            static bool atexitRegistered = false;
            if (!atexitRegistered)
            {
                atexitRegistered = true;
                std::atexit([]
                            { restoreTerminal(); });
            }
        }
    }

    static void restoreTerminal()
    {
        if (s_termiosSaved)
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &s_oldTermios);
            s_termiosSaved = false;
        }
    }

    static int readKeyNonBlocking()
    {
        static char buf[32] = {0}; // Larger buffer for mouse sequences
        static int bufPos = 0;
        static int bufLen = 0;

        // Try to read more data into buffer
        if (bufPos >= bufLen)
        {
            bufPos = 0;
            bufLen = 0;
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
            if (n <= 0)
                return -1;
            bufLen = static_cast<int>(n);
            buf[bufLen] = '\0';
        }

        if (bufPos >= bufLen)
            return -1;

        // Check for mouse SGR sequence: ESC [ < ...
        if (bufLen - bufPos >= 7 && buf[bufPos] == '\x1B' &&
            buf[bufPos + 1] == '[' && buf[bufPos + 2] == '<')
        {
            // Forward declaration - we need to parse mouse
            // Parse inline since Mouse.hpp may not be included yet
            int i = bufPos + 3;
            int button = 0, x = 0, y = 0;

            // Parse button
            while (i < bufLen && buf[i] >= '0' && buf[i] <= '9')
                button = button * 10 + (buf[i++] - '0');
            if (i >= bufLen || buf[i] != ';')
            {
                bufPos++;
                return -1;
            }
            i++;

            // Parse X
            while (i < bufLen && buf[i] >= '0' && buf[i] <= '9')
                x = x * 10 + (buf[i++] - '0');
            if (i >= bufLen || buf[i] != ';')
            {
                bufPos++;
                return -1;
            }
            i++;

            // Parse Y
            while (i < bufLen && buf[i] >= '0' && buf[i] <= '9')
                y = y * 10 + (buf[i++] - '0');

            // Check terminator
            if (i >= bufLen || (buf[i] != 'M' && buf[i] != 'm'))
            {
                bufPos++;
                return -1;
            }

            char term = buf[i++];

            // Store mouse data in static variables for Mouse class to read
            s_lastMouseX = x - 1; // Convert to 0-based
            s_lastMouseY = y - 1;
            s_lastMouseButton = button;
            s_lastMousePressed = (term == 'M');
            s_hasMouseEvent = true;

            bufPos = i; // Consume the mouse sequence
            return -2;  // Special return code for mouse event
        }

        // Check for escape sequence (arrow keys, function keys)
        if (bufLen - bufPos >= 3 && buf[bufPos] == '\x1B' && buf[bufPos + 1] == '[')
        {
            char c = buf[bufPos + 2];

            if (bufLen - bufPos >= 4 && c >= '1' && c <= '6' && buf[bufPos + 3] == '~')
            {
                // Navigation keys: Home(1~), Insert(2~), Delete(3~), End(4~), PgUp(5~), PgDn(6~)
                bufPos += 4;
                return (0x1B << 16) | ('[' << 8) | ((c - '0') << 4) | '~';
            }
            // Arrow keys: A=Up, B=Down, C=Right, D=Left
            bufPos += 3;
            return (0x1B << 16) | ('[' << 8) | c;
        }

        // Check for escape sequence with O (function keys)
        if (bufLen - bufPos >= 3 && buf[bufPos] == '\x1B' && buf[bufPos + 1] == 'O')
        {
            // F1-F4: OP, OQ, OR, OS
            char c = buf[bufPos + 2];
            bufPos += 3;
            return 0xF0 | (c - 'O'); // F1=0xF1, etc.
        }

        return static_cast<unsigned char>(buf[bufPos++]);
    }

    // Mouse event data (shared with Mouse class)
    inline static int s_lastMouseX = 0;
    inline static int s_lastMouseY = 0;
    inline static int s_lastMouseButton = 0;
    inline static bool s_lastMousePressed = false;
    inline static bool s_hasMouseEvent = false;

public:
    /**
     * @brief Get pending mouse event data (called by Mouse class)
     */
    static bool getMouseEvent(int &x, int &y, int &button, bool &pressed)
    {
        if (!s_hasMouseEvent)
            return false;
        x = s_lastMouseX;
        y = s_lastMouseY;
        button = s_lastMouseButton;
        pressed = s_lastMousePressed;
        s_hasMouseEvent = false;
        return true;
    }

private:
#endif

    static Key translateKey(int rawKey)
    {
        // Letters (case-insensitive)
        if ((rawKey >= 'a' && rawKey <= 'z') || (rawKey >= 'A' && rawKey <= 'Z'))
        {
            char c = (rawKey >= 'a') ? rawKey : (rawKey + 32);
            return static_cast<Key>(c - 'a'); // A=0, B=1, etc.
        }

        // Numbers
        if (rawKey >= '0' && rawKey <= '9')
            return static_cast<Key>(static_cast<int>(Key::Num0) + (rawKey - '0'));

        // Arrow keys (ESC [ A/B/C/D encoded as (0x1B << 16) | ('[' << 8) | char)
        switch (rawKey)
        {
        case (0x1B << 16) | ('[' << 8) | 'A':
            return Key::Up;
        case (0x1B << 16) | ('[' << 8) | 'B':
            return Key::Down;
        case (0x1B << 16) | ('[' << 8) | 'C':
            return Key::Right;
        case (0x1B << 16) | ('[' << 8) | 'D':
            return Key::Left;
        case (0x1B << 16) | ('[' << 8) | 'H':
            return Key::Home;
        case (0x1B << 16) | ('[' << 8) | 'F':
            return Key::End;
        }

        // Simple keys
        switch (rawKey)
        {
        case 27:
            return Key::Escape;
        case 9:
            return Key::Tab;
        case 127:
        case 8:
            return Key::Backspace;
        case 10:
        case 13:
            return Key::Enter;
        case 32:
            return Key::Space;
        case ',':
            return Key::Comma;
        case '.':
            return Key::Period;
        case ';':
            return Key::Semicolon;
        case '\'':
            return Key::Quote;
        case '/':
            return Key::Slash;
        case '\\':
            return Key::Backslash;
        case '[':
            return Key::LeftBracket;
        case ']':
            return Key::RightBracket;
        case '-':
            return Key::Minus;
        case '=':
            return Key::Equal;
        case '`':
            return Key::Grave;
        }

        return Key::Unknown;
    }

    static void inputThreadFunc()
    {
        while (s_running)
        {
            // Read all available input in a loop to catch rapid key events
            int rawKey;
            int keysRead = 0;
            while ((rawKey = readKeyNonBlocking()) != -1 && keysRead < 20)
            {
                // -2 is special return code for mouse event (already processed)
                if (rawKey == -2)
                {
                    keysRead++;
                    continue;
                }

                Key key = translateKey(rawKey);
                if (key != Key::Unknown)
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    size_t idx = static_cast<size_t>(key);
                    s_keyStates[idx] = true;
                    s_lastPressTime[idx] = std::chrono::steady_clock::now();
                    s_keyConsumed[idx] = false;
                }
                keysRead++;
            }

            // Sleep for 2ms - good balance between responsiveness and CPU usage
            // At 60fps (16ms frames), this allows ~8 polls per frame
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
};

TG_NAMESPACE_END
