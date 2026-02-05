/**
 * @file flappy_bird.cpp
 * @brief Flappy Bird game using Terminal Graphics Engine
 *
 * Compile with:
 *   g++ -std=c++20 -Iinclude -o flappy_bird examples/flappy_bird.cpp -pthread
 *
 * Run:
 *   ./flappy_bird
 *
 * Controls:
 *   SPACE - Jump
 *   Q/ESC - Quit
 */

#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

using namespace Pythonic::TG;

// Game constants
constexpr float GRAVITY = 350.0f;        // Pixels per second squared (reduced)
constexpr float JUMP_VELOCITY = -120.0f; // Upward velocity on jump (reduced for less sensitivity)
constexpr float PIPE_SPEED = 80.0f;      // Horizontal pipe movement
constexpr float PIPE_GAP = 50.0f;        // Gap between top and bottom pipes
constexpr float PIPE_WIDTH = 50.0f;      // Pipe width (increased for better visibility)
constexpr float PIPE_SPACING = 100.0f;   // Horizontal distance between pipe pairs
constexpr float BIRD_SIZE = 16.0f;

// Pipe structure
struct Pipe
{
    float x;
    float gapY; // Center of the gap
    bool scored = false;
};

class FlappyBirdGame
{
public:
    FlappyBirdGame(unsigned int width, unsigned int height)
        : m_canvasWidth(width), m_canvasHeight(height), m_canvas(width, height, RenderMode::Braille), m_birdY(height / 2.0f), m_birdVelocity(0), m_score(0), m_gameOver(false), m_started(false)
    {
        // Load collision sound
        if (m_collisionBuffer.loadFromFile("media/collision.wav"))
        {
            m_collisionSound.setBuffer(m_collisionBuffer);
        }

        // Load jump sound
        if (m_jumpBuffer.loadFromFile("media/jump.wav"))
        {
            m_jumpSound.setBuffer(m_jumpBuffer);
        }

        // Load bird texture
        if (m_birdTexture.loadFromFile("media/bird.png"))
        {
            m_birdSprite.setTexture(m_birdTexture, true);
            // Scale bird to fit
            Vector2u texSize = m_birdTexture.getSize();
            if (texSize.x > 0 && texSize.y > 0)
            {
                m_birdSprite.setScale(BIRD_SIZE / texSize.x, BIRD_SIZE / texSize.y);
            }
            // Get opaque bounds for accurate collision detection
            m_birdOpaqueBounds = m_birdTexture.getOpaqueBounds(128);
            m_useBirdSprite = true;
        }
        else
        {
            m_useBirdSprite = false;
            // Default opaque bounds (full size for fallback circle)
            m_birdOpaqueBounds = IntRect(0, 0, static_cast<int>(BIRD_SIZE), static_cast<int>(BIRD_SIZE));
        }

        // Load pipe texture
        if (m_pipeTexture.loadFromFile("media/pipe.png"))
        {
            m_pipeSprite.setTexture(m_pipeTexture, true);
            // Scale pipe to fit width
            Vector2u texSize = m_pipeTexture.getSize();
            if (texSize.x > 0 && texSize.y > 0)
            {
                m_pipeSprite.setScale(PIPE_WIDTH / texSize.x, 1.0f);
            }
            // Get opaque bounds for accurate collision detection
            m_pipeOpaqueBounds = m_pipeTexture.getOpaqueBounds(128);
            m_usePipeSprite = true;
        }
        else
        {
            m_usePipeSprite = false;
            // Default opaque bounds (full width for fallback shapes)
            m_pipeOpaqueBounds = IntRect(0, 0, static_cast<int>(PIPE_WIDTH), 1);
        }

        // Bird position (fixed X)
        m_birdX = width / 4.0f;

        // Random number generator for pipe gaps
        std::random_device rd;
        m_rng = std::mt19937(rd());
        m_gapDist = std::uniform_real_distribution<float>(
            height * 0.25f, height * 0.75f);

        // Generate initial pipes
        for (int i = 0; i < 4; ++i)
        {
            spawnPipe(width + i * PIPE_SPACING);
        }
    }

    void update(float dt)
    {
        if (m_gameOver)
            return;

        if (!m_started)
            return;

        // Apply gravity
        m_birdVelocity += GRAVITY * dt;
        m_birdY += m_birdVelocity * dt;

        // Move pipes
        for (auto &pipe : m_pipes)
        {
            pipe.x -= PIPE_SPEED * dt;

            // Check if bird passed the pipe
            if (!pipe.scored && pipe.x + PIPE_WIDTH < m_birdX)
            {
                pipe.scored = true;
                m_score++;
            }
        }

        // Remove off-screen pipes and spawn new ones
        while (!m_pipes.empty() && m_pipes.front().x < -PIPE_WIDTH)
        {
            m_pipes.erase(m_pipes.begin());
        }

        while (m_pipes.size() < 5)
        {
            float lastX = m_pipes.empty() ? m_canvasWidth : m_pipes.back().x;
            spawnPipe(lastX + PIPE_SPACING);
        }

        // Check collisions
        checkCollisions();
    }

    void jump()
    {
        if (m_gameOver)
        {
            restart();
            return;
        }

        m_started = true;
        m_birdVelocity = JUMP_VELOCITY;
        m_jumpSound.play(); // Play jump sound
    }

    void render()
    {
        m_canvas.clear(Color(50, 150, 200)); // Sky blue background

        // Draw pipes
        for (const auto &pipe : m_pipes)
        {
            drawPipe(pipe);
        }

        // Draw bird
        drawBird();

        // Draw ground line
        for (unsigned int x = 0; x < m_canvasWidth; ++x)
        {
            m_canvas.setPixel(x, m_canvasHeight - 1, Color(139, 69, 19)); // Brown
            m_canvas.setPixel(x, m_canvasHeight - 2, Color(34, 139, 34)); // Green
        }

        // Draw score
        drawScore();

        // Draw game over or start message
        if (m_gameOver)
        {
            drawCenteredText("GAME OVER", m_canvasHeight / 2 - 10, Color::Red);
            drawCenteredText("Press SPACE to restart", m_canvasHeight / 2 + 5, Color::White);
        }
        else if (!m_started)
        {
            drawCenteredText("Press SPACE to start", m_canvasHeight / 2, Color::White);
        }

        m_canvas.display();
    }

    bool isRunning() const { return !m_quit; }
    void quit() { m_quit = true; }

    void handleInput()
    {
        // Jump on space press (not hold)
        bool spaceDown = Keyboard::isKeyPressed(Key::Space);
        if (spaceDown && !m_spaceWasPressed)
        {
            jump();
        }
        m_spaceWasPressed = spaceDown;

        if (Keyboard::isKeyPressed(Key::Q) || Keyboard::isKeyPressed(Key::Escape))
        {
            quit();
        }
    }

private:
    // Input state
    bool m_spaceWasPressed = false;
    // Canvas and rendering
    unsigned int m_canvasWidth;
    unsigned int m_canvasHeight;
    Canvas m_canvas;

    // Audio
    SoundBuffer m_collisionBuffer;
    Sound m_collisionSound;
    SoundBuffer m_jumpBuffer;
    Sound m_jumpSound;

    // Bird
    Texture m_birdTexture;
    Sprite m_birdSprite;
    bool m_useBirdSprite = false;
    IntRect m_birdOpaqueBounds; // Opaque pixel bounds for accurate collision
    float m_birdX;
    float m_birdY;
    float m_birdVelocity;

    // Pipes
    Texture m_pipeTexture;
    Sprite m_pipeSprite;
    bool m_usePipeSprite = false;
    IntRect m_pipeOpaqueBounds; // Opaque pixel bounds for accurate collision
    std::vector<Pipe> m_pipes;

    // Game state
    int m_score;
    bool m_gameOver;
    bool m_started;
    bool m_quit = false;

    // Random
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_gapDist;

    void spawnPipe(float x)
    {
        Pipe pipe;
        pipe.x = x;
        pipe.gapY = m_gapDist(m_rng);
        pipe.scored = false;
        m_pipes.push_back(pipe);
    }

    void drawBird()
    {
        if (m_useBirdSprite)
        {
            // Draw sprite pixel by pixel to canvas
            m_birdSprite.setPosition(m_birdX - BIRD_SIZE / 2, m_birdY - BIRD_SIZE / 2);

            const IntRect &rect = m_birdSprite.getTextureRect();
            for (int y = 0; y < rect.height; ++y)
            {
                for (int x = 0; x < rect.width; ++x)
                {
                    Color c = m_birdTexture.getPixel(rect.left + x, rect.top + y);
                    if (c.a > 128)
                    {
                        Vector2f pos = m_birdSprite.transformPoint(Vector2f(
                            static_cast<float>(x), static_cast<float>(y)));
                        m_canvas.setPixel(
                            static_cast<unsigned int>(pos.x),
                            static_cast<unsigned int>(pos.y),
                            c);
                    }
                }
            }
        }
        else
        {
            // Draw simple yellow circle
            m_canvas.fillCircle(
                static_cast<int>(m_birdX),
                static_cast<int>(m_birdY),
                static_cast<int>(BIRD_SIZE / 2),
                Color::Yellow);
            // Eye
            m_canvas.setPixel(
                static_cast<unsigned int>(m_birdX + 2),
                static_cast<unsigned int>(m_birdY - 2),
                Color::Black);
        }
    }

    void drawPipe(const Pipe &pipe)
    {
        int x = static_cast<int>(pipe.x);
        int gapTop = static_cast<int>(pipe.gapY - PIPE_GAP / 2);
        int gapBottom = static_cast<int>(pipe.gapY + PIPE_GAP / 2);

        if (m_usePipeSprite)
        {
            // Draw pipe using texture
            const IntRect &rect = m_pipeSprite.getTextureRect();
            float scaleX = PIPE_WIDTH / static_cast<float>(rect.width);
            float scaleY = 1.0f; // Keep vertical scale

            // Top pipe (tiled from top to gap)
            for (int py = 0; py < gapTop; ++py)
            {
                int texY = py % rect.height;
                for (int px = 0; px < static_cast<int>(PIPE_WIDTH); ++px)
                {
                    int screenX = x + px;
                    if (screenX >= 0 && screenX < static_cast<int>(m_canvasWidth))
                    {
                        int texX = static_cast<int>(px / scaleX) % rect.width;
                        Color c = m_pipeTexture.getPixel(rect.left + texX, rect.top + texY);
                        if (c.a > 128)
                        {
                            m_canvas.setPixel(screenX, py, c);
                        }
                    }
                }
            }

            // Bottom pipe (tiled from gap to bottom)
            for (int py = gapBottom; py < static_cast<int>(m_canvasHeight) - 2; ++py)
            {
                int texY = (py - gapBottom) % rect.height;
                for (int px = 0; px < static_cast<int>(PIPE_WIDTH); ++px)
                {
                    int screenX = x + px;
                    if (screenX >= 0 && screenX < static_cast<int>(m_canvasWidth))
                    {
                        int texX = static_cast<int>(px / scaleX) % rect.width;
                        Color c = m_pipeTexture.getPixel(rect.left + texX, rect.top + texY);
                        if (c.a > 128)
                        {
                            m_canvas.setPixel(screenX, py, c);
                        }
                    }
                }
            }
        }
        else
        {
            // Fallback: Draw solid color pipes
            Color pipeColor(34, 139, 34); // Green
            Color pipeEdge(0, 100, 0);    // Dark green

            // Top pipe (from top to gap)
            for (int py = 0; py < gapTop; ++py)
            {
                for (int px = 0; px < static_cast<int>(PIPE_WIDTH); ++px)
                {
                    int screenX = x + px;
                    if (screenX >= 0 && screenX < static_cast<int>(m_canvasWidth))
                    {
                        Color c = (px == 0 || px == static_cast<int>(PIPE_WIDTH) - 1)
                                      ? pipeEdge
                                      : pipeColor;
                        m_canvas.setPixel(screenX, py, c);
                    }
                }
            }

            // Pipe cap (top pipe)
            for (int py = gapTop - 5; py < gapTop; ++py)
            {
                for (int px = -3; px < static_cast<int>(PIPE_WIDTH) + 3; ++px)
                {
                    int screenX = x + px;
                    if (screenX >= 0 && screenX < static_cast<int>(m_canvasWidth) &&
                        py >= 0 && py < static_cast<int>(m_canvasHeight))
                    {
                        m_canvas.setPixel(screenX, py, pipeEdge);
                    }
                }
            }

            // Bottom pipe (from gap to bottom)
            for (int py = gapBottom; py < static_cast<int>(m_canvasHeight) - 2; ++py)
            {
                for (int px = 0; px < static_cast<int>(PIPE_WIDTH); ++px)
                {
                    int screenX = x + px;
                    if (screenX >= 0 && screenX < static_cast<int>(m_canvasWidth))
                    {
                        Color c = (px == 0 || px == static_cast<int>(PIPE_WIDTH) - 1)
                                      ? pipeEdge
                                      : pipeColor;
                        m_canvas.setPixel(screenX, py, c);
                    }
                }
            }

            // Pipe cap (bottom pipe)
            for (int py = gapBottom; py < gapBottom + 5; ++py)
            {
                for (int px = -3; px < static_cast<int>(PIPE_WIDTH) + 3; ++px)
                {
                    int screenX = x + px;
                    if (screenX >= 0 && screenX < static_cast<int>(m_canvasWidth) &&
                        py < static_cast<int>(m_canvasHeight))
                    {
                        m_canvas.setPixel(screenX, py, pipeEdge);
                    }
                }
            }
        }
    }

    void drawScore()
    {
        // Draw score using Text class with shadow for visibility
        std::string scoreStr = "Score: " + std::to_string(m_score);
        Text::drawWithShadow(m_canvas, scoreStr, 5, 5, Color::White, Color::Black);
    }

    void drawCenteredText(const std::string &text, int y, const Color &color)
    {
        // Use Text class for proper rendering with shadow
        Text::drawCenteredWithBackground(m_canvas, text, m_canvasWidth / 2, y,
                                         color, Color(0, 0, 0, 180), 2);
    }

    FloatRect getBirdBounds() const
    {
        if (m_useBirdSprite)
        {
            // Use opaque bounds scaled to actual bird size for accurate collision
            Vector2u texSize = m_birdTexture.getSize();
            float scaleX = BIRD_SIZE / texSize.x;
            float scaleY = BIRD_SIZE / texSize.y;

            // Calculate actual visible bounds based on opaque pixels
            float actualWidth = m_birdOpaqueBounds.width * scaleX;
            float actualHeight = m_birdOpaqueBounds.height * scaleY;

            // Offset from center of texture to center of opaque region
            float offsetX = (m_birdOpaqueBounds.left + m_birdOpaqueBounds.width / 2.0f - texSize.x / 2.0f) * scaleX;
            float offsetY = (m_birdOpaqueBounds.top + m_birdOpaqueBounds.height / 2.0f - texSize.y / 2.0f) * scaleY;

            return FloatRect(
                m_birdX + offsetX - actualWidth / 2,
                m_birdY + offsetY - actualHeight / 2,
                actualWidth,
                actualHeight);
        }
        else
        {
            // Fallback for circle drawing
            return FloatRect(
                m_birdX - BIRD_SIZE / 2,
                m_birdY - BIRD_SIZE / 2,
                BIRD_SIZE,
                BIRD_SIZE);
        }
    }

    void checkCollisions()
    {
        FloatRect birdBounds = getBirdBounds();

        // Check screen boundaries (top and ground)
        if (m_birdY - BIRD_SIZE / 2 < 0 ||
            m_birdY + BIRD_SIZE / 2 > m_canvasHeight - 2)
        {
            m_collisionSound.play(); // Play collision sound
            m_gameOver = true;
            return;
        }

        // Check pipe collisions
        for (const auto &pipe : m_pipes)
        {
            // Calculate actual pipe width accounting for transparent padding
            float actualPipeWidth = PIPE_WIDTH;
            float pipeOffsetX = 0;

            if (m_usePipeSprite)
            {
                Vector2u texSize = m_pipeTexture.getSize();
                float scaleX = PIPE_WIDTH / texSize.x;
                actualPipeWidth = m_pipeOpaqueBounds.width * scaleX;
                pipeOffsetX = m_pipeOpaqueBounds.left * scaleX;
            }

            // Top pipe bounds
            FloatRect topPipe(
                pipe.x + pipeOffsetX, 0,
                actualPipeWidth, pipe.gapY - PIPE_GAP / 2);

            // Bottom pipe bounds
            FloatRect bottomPipe(
                pipe.x + pipeOffsetX, pipe.gapY + PIPE_GAP / 2,
                actualPipeWidth, m_canvasHeight - (pipe.gapY + PIPE_GAP / 2));

            // Use collision detection
            if (Collision::intersects(birdBounds, topPipe) ||
                Collision::intersects(birdBounds, bottomPipe))
            {
                m_collisionSound.play(); // Play collision sound
                m_gameOver = true;
                return;
            }
        }
    }

    void restart()
    {
        m_birdY = m_canvasHeight / 2.0f;
        m_birdVelocity = 0;
        m_score = 0;
        m_gameOver = false;
        m_started = false;
        m_pipes.clear();

        for (int i = 0; i < 4; ++i)
        {
            spawnPipe(m_canvasWidth + i * PIPE_SPACING);
        }
    }
};

int main()
{
    // Initialize keyboard
    Keyboard::init();

    // Hide cursor and set up terminal
    std::cout << "\033[?25l";   // Hide cursor
    std::cout << "\033[?1049h"; // Alternate screen buffer
    std::cout << "\033[2J";     // Clear screen

    // Get terminal size
    VideoMode mode = VideoMode::getDesktopMode();

    // Canvas size in pixels (braille: 2x4 per character)
    unsigned int canvasWidth = mode.width * 2;
    unsigned int canvasHeight = (mode.height - 2) * 4; // Leave room for status

    FlappyBirdGame game(canvasWidth, canvasHeight);

    Clock clock;
    const float targetFps = 60.0f;
    const Time frameTime = Time::seconds(1.0f / targetFps);

    while (game.isRunning())
    {
        Time dt = clock.restart();

        // Handle input
        game.handleInput();

        // Update game
        game.update(dt.asSeconds());

        // Render
        game.render();

        // Frame rate limiting
        Time elapsed = clock.getElapsedTime();
        if (elapsed < frameTime)
        {
            sleep(frameTime - elapsed);
        }
    }

    // Cleanup
    Keyboard::shutdown();
    std::cout << "\033[?1049l"; // Exit alternate screen buffer
    std::cout << "\033[?25h";   // Show cursor
    std::cout << "\033[0m";     // Reset colors

    std::cout << "Thanks for playing! Final score: " << std::endl;

    return 0;
}
