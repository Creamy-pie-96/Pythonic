/**
 * @file Mouse.hpp
 * @brief Mouse input handling for terminal applications
 *
 * Provides mouse position tracking and button state detection.
 * Uses direct /dev/input access on Linux for reliable mouse tracking.
 * Falls back to terminal mouse mode where available.
 */

#pragma once

#include "../Config.hpp"
#include "Keyboard.hpp" // For mouse event polling from terminal
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdlib>

#ifndef _WIN32
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <cstring>
#include <sys/ioctl.h>
#endif

TG_NAMESPACE_BEGIN

/**
 * @brief Mouse button types for terminal mouse support
 */
enum class MouseButton
{
    Left = 0,
    Middle = 1,
    Right = 2,
    Count
};

/**
 * @brief Mouse input manager with direct device access
 *
 * Uses /dev/input/event* for direct mouse access on Linux.
 * This provides proper mouse delta tracking even without pressing buttons.
 */
class Mouse
{
public:
    struct Position
    {
        int x, y;
    };

    /**
     * @brief Initialize mouse tracking
     *
     * Tries to use direct device access via /dev/input.
     * Falls back to terminal mouse mode if device access fails.
     */
    static void init()
    {
        if (s_initialized.exchange(true))
            return;

#ifndef _WIN32
        // Try to open mouse device directly
        s_useEvdev = initEvdev();

        if (s_useEvdev)
        {
            s_running = true;
            s_inputThread = std::thread(evdevInputThread);
        }
        else
        {
            // Fall back to terminal mouse mode
            printf("\033[?1000h\033[?1002h\033[?1003h\033[?1006h");
            fflush(stdout);

            // Register atexit handler for normal exit cleanup
            static bool atexitRegistered = false;
            if (!atexitRegistered)
            {
                atexitRegistered = true;
                std::atexit([]
                            { 
                    // Disable terminal mouse tracking on exit
                    printf("\033[?1000l\033[?1002l\033[?1003l\033[?1006l");
                    fflush(stdout); });
            }
        }
#endif

        // Reset state
        std::lock_guard<std::mutex> lock(s_mutex);
        s_posX = 0;
        s_posY = 0;
        s_deltaX = 0;
        s_deltaY = 0;
        s_rawDeltaX = 0;
        s_rawDeltaY = 0;
        for (int i = 0; i < 3; i++)
        {
            s_buttonStates[i].store(false);
            s_buttonClicked[i].store(false);
        }
        s_wheelDelta.store(0);
    }

    /**
     * @brief Disable mouse tracking
     */
    static void shutdown()
    {
        if (!s_initialized.exchange(false))
            return;

#ifndef _WIN32
        s_running = false;

        if (s_useEvdev)
        {
            if (s_inputThread.joinable())
                s_inputThread.join();
            if (s_evdevFd >= 0)
            {
                close(s_evdevFd);
                s_evdevFd = -1;
            }
        }
        else
        {
            // Disable terminal mouse tracking
            printf("\033[?1000l\033[?1002l\033[?1003l\033[?1006l");
            fflush(stdout);
        }
#endif
    }

    /**
     * @brief Check if a mouse button is currently pressed
     */
    static bool isButtonPressed(MouseButton button)
    {
        if (!s_useEvdev)
            pollKeyboardMouseEvents();
        int idx = static_cast<int>(button);
        if (idx >= 0 && idx < 3)
            return s_buttonStates[idx].load();
        return false;
    }

    /**
     * @brief Get the current mouse position in terminal characters
     */
    static Position getPosition()
    {
        if (!s_useEvdev)
            pollKeyboardMouseEvents();
        std::lock_guard<std::mutex> lock(s_mutex);
        return {s_posX, s_posY};
    }

    /**
     * @brief Get mouse movement delta since last call (and reset)
     */
    static Position getDelta()
    {
        if (!s_useEvdev)
            pollKeyboardMouseEvents();

        std::lock_guard<std::mutex> lock(s_mutex);
        Position delta = {s_deltaX, s_deltaY};
        s_deltaX = 0;
        s_deltaY = 0;
        return delta;
    }

    /**
     * @brief Get raw mouse delta (from evdev, high resolution)
     */
    static Position getRawDelta()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        Position delta = {s_rawDeltaX, s_rawDeltaY};
        s_rawDeltaX = 0;
        s_rawDeltaY = 0;
        return delta;
    }

    /**
     * @brief Get pixel-space position (scaled for canvas coordinates)
     */
    static Position getPixelPosition(int pixelsPerChar = 2, int pixelsPerRow = 4)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        return {s_posX * pixelsPerChar, s_posY * pixelsPerRow};
    }

    /**
     * @brief Check if mouse was clicked (button went from up to down)
     */
    static bool wasClicked(MouseButton button)
    {
        int idx = static_cast<int>(button);
        if (idx >= 0 && idx < 3)
            return s_buttonClicked[idx].exchange(false);
        return false;
    }

    /**
     * @brief Get wheel scroll delta (resets after reading)
     */
    static int getWheelDelta()
    {
        return s_wheelDelta.exchange(0);
    }

    /**
     * @brief Check if using direct device access
     */
    static bool isUsingEvdev() { return s_useEvdev; }

    /**
     * @brief Check if mouse is initialized
     */
    static bool isInitialized() { return s_initialized.load(); }

    /**
     * @brief Set mouse sensitivity for evdev mode
     */
    static void setSensitivity(float sens) { s_sensitivity = sens; }

private:
#ifndef _WIN32
    static bool initEvdev()
    {
        // Find mouse device in /dev/input
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

            // Check if this is a mouse (has REL_X and REL_Y)
            unsigned long evbit = 0;
            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) >= 0)
            {
                if (evbit & (1 << EV_REL))
                {
                    unsigned long relbit = 0;
                    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), &relbit) >= 0)
                    {
                        if ((relbit & (1 << REL_X)) && (relbit & (1 << REL_Y)))
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

    static void evdevInputThread()
    {
        struct input_event ev;
        while (s_running)
        {
            ssize_t n;
            while ((n = read(s_evdevFd, &ev, sizeof(ev))) == sizeof(ev))
            {
                if (ev.type == EV_REL)
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    if (ev.code == REL_X)
                    {
                        s_rawDeltaX += ev.value;
                        // Scale to terminal-friendly units
                        s_deltaX += static_cast<int>(ev.value * s_sensitivity);
                    }
                    else if (ev.code == REL_Y)
                    {
                        s_rawDeltaY += ev.value;
                        s_deltaY += static_cast<int>(ev.value * s_sensitivity);
                    }
                    else if (ev.code == REL_WHEEL)
                    {
                        s_wheelDelta.fetch_add(ev.value);
                    }
                }
                else if (ev.type == EV_KEY)
                {
                    int btnIdx = -1;
                    if (ev.code == BTN_LEFT)
                        btnIdx = 0;
                    else if (ev.code == BTN_MIDDLE)
                        btnIdx = 1;
                    else if (ev.code == BTN_RIGHT)
                        btnIdx = 2;

                    if (btnIdx >= 0 && btnIdx < 3)
                    {
                        bool pressed = (ev.value != 0);
                        bool wasPressed = s_buttonStates[btnIdx].exchange(pressed);
                        if (pressed && !wasPressed)
                            s_buttonClicked[btnIdx].store(true);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    }

    inline static int s_evdevFd = -1;
    inline static std::thread s_inputThread;
#endif

    /**
     * @brief Poll for mouse events captured by the Keyboard input thread
     */
    static void pollKeyboardMouseEvents()
    {
        if (!s_initialized.load() || s_useEvdev)
            return;

        int x, y, button;
        bool pressed;

        while (Keyboard::getMouseEvent(x, y, button, pressed))
        {
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                s_deltaX += x - s_posX;
                s_deltaY += y - s_posY;
                s_posX = x;
                s_posY = y;
            }

            int btnIdx = button & 0x03;
            bool isMotion = (button & 32) != 0;
            bool isWheel = (button & 64) != 0;

            if (isWheel)
            {
                s_wheelDelta.fetch_add(btnIdx == 0 ? 1 : -1);
            }
            else if (!isMotion && btnIdx < 3)
            {
                bool wasPressed = s_buttonStates[btnIdx].exchange(pressed);
                if (pressed && !wasPressed)
                    s_buttonClicked[btnIdx].store(true);
            }
        }
    }

    inline static std::atomic<bool> s_initialized{false};
    inline static std::atomic<bool> s_running{false};
    inline static bool s_useEvdev = false;
    inline static float s_sensitivity = 0.1f;

    inline static std::mutex s_mutex;
    inline static int s_posX = 0;
    inline static int s_posY = 0;
    inline static int s_deltaX = 0;
    inline static int s_deltaY = 0;
    inline static int s_rawDeltaX = 0;
    inline static int s_rawDeltaY = 0;

    inline static std::atomic<bool> s_buttonStates[3] = {false, false, false};
    inline static std::atomic<bool> s_buttonClicked[3] = {false, false, false};
    inline static std::atomic<int> s_wheelDelta{0};
};

TG_NAMESPACE_END
