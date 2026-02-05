/**
 * @file Event.hpp
 * @brief Event type definitions for input and window events
 */

#pragma once

#include "../Config.hpp"
#include "Keyboard.hpp"

TG_NAMESPACE_BEGIN

// Forward declare MouseButton (defined in Mouse.hpp)
enum class MouseButton;

/**
 * @brief Event types that can be captured
 */
enum class EventType
{
    Closed,      ///< Window close request (Ctrl+C, Escape)
    Resized,     ///< Terminal resized
    KeyPressed,  ///< A key was pressed
    KeyReleased, ///< A key was released
    TextEntered, ///< Text input event
    MouseMoved,  ///< Mouse moved (if supported)
    MouseButtonPressed,
    MouseButtonReleased,
    MouseWheelScrolled,
    Count ///< Number of event types
};

// Note: MouseButton enum is defined in Mouse.hpp

/**
 * @brief Size event parameters
 */
struct SizeEvent
{
    unsigned int width;  ///< New width in characters
    unsigned int height; ///< New height in characters
};

/**
 * @brief Key event parameters
 */
struct KeyEvent
{
    Key code;     ///< Key that was pressed/released
    bool alt;     ///< Was Alt held?
    bool control; ///< Was Control held?
    bool shift;   ///< Was Shift held?
    bool system;  ///< Was System (Win/Cmd) held?
};

/**
 * @brief Text event parameters
 */
struct TextEvent
{
    uint32_t unicode; ///< Unicode code point of entered character
};

/**
 * @brief Mouse move event parameters
 */
struct MouseMoveEvent
{
    int x; ///< X position in characters
    int y; ///< Y position in characters
};

/**
 * @brief Mouse button event parameters
 */
struct MouseButtonEvent
{
    MouseButton button; ///< Button that was pressed/released
    int x;              ///< X position at time of event
    int y;              ///< Y position at time of event
};

/**
 * @brief Mouse wheel event parameters
 */
struct MouseWheelEvent
{
    float delta; ///< Wheel offset (positive=up, negative=down)
    int x;       ///< X position at time of event
    int y;       ///< Y position at time of event
};

/**
 * @brief Event union containing all event data
 *
 * Similar to SFML's sf::Event, this provides a polymorphic event structure
 * where the type field indicates which union member is active.
 *
 * @code
 * Event event;
 * while (window.pollEvent(event))
 * {
 *     if (event.type == EventType::Closed)
 *         window.close();
 *     else if (event.type == EventType::KeyPressed)
 *     {
 *         if (event.key.code == Key::Escape)
 *             window.close();
 *     }
 * }
 * @endcode
 */
struct Event
{
    EventType type; ///< Type of the event

    union
    {
        SizeEvent size;
        KeyEvent key;
        TextEvent text;
        MouseMoveEvent mouseMove;
        MouseButtonEvent mouseButton;
        MouseWheelEvent mouseWheel;
    };

    Event() : type(EventType::Count) {}

    /**
     * @brief Create a key pressed event
     */
    static Event keyPressed(Key code, bool ctrl = false, bool shift = false,
                            bool alt = false, bool system = false)
    {
        Event e;
        e.type = EventType::KeyPressed;
        e.key.code = code;
        e.key.control = ctrl;
        e.key.shift = shift;
        e.key.alt = alt;
        e.key.system = system;
        return e;
    }

    /**
     * @brief Create a closed event
     */
    static Event closed()
    {
        Event e;
        e.type = EventType::Closed;
        return e;
    }

    /**
     * @brief Create a resize event
     */
    static Event resized(unsigned int width, unsigned int height)
    {
        Event e;
        e.type = EventType::Resized;
        e.size.width = width;
        e.size.height = height;
        return e;
    }

    /**
     * @brief Create a text entered event
     */
    static Event textEntered(uint32_t unicode)
    {
        Event e;
        e.type = EventType::TextEntered;
        e.text.unicode = unicode;
        return e;
    }
};

TG_NAMESPACE_END
