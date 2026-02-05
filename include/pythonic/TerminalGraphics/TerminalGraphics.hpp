/**
 * @file TerminalGraphics.hpp
 * @brief Main header for Terminal Graphics Engine
 *
 * A terminal-based graphics engine inspired by SFML.
 * Provides game development capabilities using Unicode characters
 * for rendering instead of pixel-based graphics.
 *
 * @section features Features
 * - SFML-like API for familiarity
 * - Multiple render modes (Braille, Block, Quarter, ASCII)
 * - Window management with keyboard input
 * - Drawable shapes (Rectangle, Circle, Convex, Line)
 * - Sprite and texture support
 * - Audio playback (WAV files)
 * - Time management with Clock
 *
 * @section example Basic Example
 * @code
 * #include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
 *
 * using namespace Pythonic::TG;
 *
 * int main()
 * {
 *     // Create a window
 *     TerminalWindow window(VideoMode::getDesktopMode(), "My Game");
 *     window.setFramerateLimit(60);
 *
 *     // Create a canvas for high-res rendering
 *     Canvas canvas = Canvas::fromTerminalSize(80, 24, RenderMode::Braille);
 *
 *     // Create a shape
 *     CircleShape player(10);
 *     player.setFillColor(Color::Blue);
 *     player.setPosition(40, 20);
 *
 *     Clock clock;
 *
 *     while (window.isOpen())
 *     {
 *         // Handle events
 *         Event event;
 *         while (window.pollEvent(event))
 *         {
 *             if (event.type == EventType::Closed)
 *                 window.close();
 *         }
 *
 *         // Handle real-time input
 *         float speed = 50 * clock.restart().asSeconds();
 *         if (Keyboard::isKeyPressed(Key::Left))
 *             player.move(-speed, 0);
 *         if (Keyboard::isKeyPressed(Key::Right))
 *             player.move(speed, 0);
 *
 *         // Draw
 *         canvas.clear(Color::Black);
 *         canvas.draw(player);
 *         canvas.display();
 *     }
 *
 *     return 0;
 * }
 * @endcode
 *
 * @section modules Modules
 *
 * The engine is organized into modules:
 *
 * - **Core**: Basic types (Vector2, Color, Time, Clock, Rect)
 * - **Window**: TerminalWindow, VideoMode
 * - **Event**: Keyboard, Event, EventQueue
 * - **Graphics**: Drawable, Transformable, RenderTarget, Canvas
 * - **Shapes**: Shape, RectangleShape, CircleShape, ConvexShape, Line
 * - **Sprite**: Image, Texture, Sprite
 * - **Audio**: SoundBuffer, Sound, Music
 *
 * @section render_modes Render Modes
 *
 * The Canvas supports multiple rendering modes:
 *
 * - **Braille** (default): 2x4 pixels per character cell using Unicode braille
 *   patterns (⠀-⣿). Provides the highest resolution.
 *
 * - **Block**: 1x2 pixels per cell using half-block characters (▀▄█).
 *   Good balance of resolution and compatibility.
 *
 * - **Quarter**: 2x2 pixels per cell using quarter-block characters.
 *   Medium resolution with good visual quality.
 *
 * - **ASCII**: 1x1 pixel per cell using ASCII characters (.:-=+*#%@).
 *   Maximum compatibility but lowest resolution.
 *
 * @author Terminal Graphics Engine
 * @version 1.0
 */

#pragma once

// Configuration
#include "Config.hpp"

// Core module
#include "Core/Core.hpp"

// Window module
#include "Window/Window.hpp"

// Event module
#include "Event/Events.hpp"

// Graphics module
#include "Graphics/Graphics.hpp"

// Layer/Z-ordering module
#include "Graphics/Layer.hpp"

// Shapes module
#include "Shapes/Shapes.hpp"

// Sprite module
#include "Sprite/Sprites.hpp"

// Animation module
#include "Sprite/Animation.hpp"

// Effects module (particles, etc.)
#include "Effects/Effects.hpp"

// Audio module
#include "Audio/Audio.hpp"

// Physics module
#include "Physics/Physics.hpp"

// Text module
#include "Text/Font.hpp"
#include "Text/Text.hpp"

// Convenience namespace alias
namespace TG = Pythonic::TG;
