/**
 * @file Animation.hpp
 * @brief Sprite sheet animation support
 *
 * Provides animated sprite capabilities with frame-based animations,
 * multiple animation states, and timing control.
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "../Graphics/Drawable.hpp"
#include "../Graphics/Transformable.hpp"
#include "../Graphics/RenderTarget.hpp"
#include "Texture.hpp"
#include <vector>
#include <unordered_map>
#include <string>

TG_NAMESPACE_BEGIN

/**
 * @brief A single frame in an animation
 */
struct AnimationFrame
{
    IntRect rect;  ///< Source rectangle in texture
    Time duration; ///< How long to show this frame

    AnimationFrame() : duration(Time::milliseconds(100)) {}
    AnimationFrame(const IntRect &r, Time d = Time::milliseconds(100))
        : rect(r), duration(d) {}
};

/**
 * @brief Animation playback mode
 */
enum class AnimationMode
{
    Once,     ///< Play once and stop on last frame
    Loop,     ///< Loop continuously
    PingPong, ///< Play forward then backward, repeat
    Reverse   ///< Play in reverse order
};

/**
 * @brief A named animation sequence
 */
class Animation
{
public:
    Animation() : m_mode(AnimationMode::Loop) {}

    /**
     * @brief Create an animation with frames from a sprite sheet grid
     * @param frameWidth Width of each frame
     * @param frameHeight Height of each frame
     * @param startFrame Starting frame index (0-based)
     * @param frameCount Number of frames
     * @param sheetWidth Width of sprite sheet in pixels
     * @param frameDuration Duration per frame
     */
    static Animation fromGrid(int frameWidth, int frameHeight,
                              int startFrame, int frameCount,
                              int sheetWidth,
                              Time frameDuration = Time::milliseconds(100))
    {
        Animation anim;
        int framesPerRow = sheetWidth / frameWidth;

        for (int i = 0; i < frameCount; ++i)
        {
            int idx = startFrame + i;
            int col = idx % framesPerRow;
            int row = idx / framesPerRow;

            anim.addFrame(IntRect(
                              col * frameWidth,
                              row * frameHeight,
                              frameWidth,
                              frameHeight),
                          frameDuration);
        }

        return anim;
    }

    /**
     * @brief Create an animation from a horizontal strip
     */
    static Animation fromStrip(int y, int frameWidth, int frameHeight,
                               int frameCount, Time frameDuration = Time::milliseconds(100))
    {
        Animation anim;
        for (int i = 0; i < frameCount; ++i)
        {
            anim.addFrame(IntRect(
                              i * frameWidth, y,
                              frameWidth, frameHeight),
                          frameDuration);
        }
        return anim;
    }

    /**
     * @brief Add a frame to the animation
     */
    void addFrame(const IntRect &rect, Time duration = Time::milliseconds(100))
    {
        m_frames.emplace_back(rect, duration);
    }

    /**
     * @brief Set the playback mode
     */
    void setMode(AnimationMode mode) { m_mode = mode; }
    AnimationMode getMode() const { return m_mode; }

    /**
     * @brief Get the number of frames
     */
    size_t getFrameCount() const { return m_frames.size(); }

    /**
     * @brief Get a specific frame
     */
    const AnimationFrame &getFrame(size_t index) const
    {
        return m_frames[index % m_frames.size()];
    }

    /**
     * @brief Get total animation duration
     */
    Time getTotalDuration() const
    {
        Time total;
        for (const auto &frame : m_frames)
            total = total + frame.duration;
        return total;
    }

    /**
     * @brief Check if animation is empty
     */
    bool empty() const { return m_frames.empty(); }

private:
    std::vector<AnimationFrame> m_frames;
    AnimationMode m_mode;
};

/**
 * @brief Animated sprite with multiple animation states
 *
 * AnimatedSprite extends the basic Sprite with frame-based animation
 * support. You can define multiple named animations and switch between them.
 *
 * @code
 * Texture spriteSheet;
 * spriteSheet.loadFromFile("player.ppm");
 *
 * AnimatedSprite player(spriteSheet);
 *
 * // Create animations from sprite sheet
 * player.addAnimation("idle", Animation::fromStrip(0, 32, 32, 4));
 * player.addAnimation("run", Animation::fromStrip(32, 32, 32, 6, milliseconds(80)));
 * player.addAnimation("jump", Animation::fromStrip(64, 32, 32, 2, milliseconds(150)));
 *
 * player.play("idle");
 *
 * // In game loop
 * player.update(deltaTime);
 * canvas.draw(player);
 * @endcode
 */
class AnimatedSprite : public Drawable, public Transformable
{
public:
    /**
     * @brief Default constructor
     */
    AnimatedSprite() : m_texture(nullptr), m_currentFrame(0),
                       m_playing(false), m_forward(true) {}

    /**
     * @brief Create with a texture (sprite sheet)
     */
    explicit AnimatedSprite(const Texture &texture)
        : m_texture(&texture), m_currentFrame(0),
          m_playing(false), m_forward(true) {}

    /**
     * @brief Set the sprite sheet texture
     */
    void setTexture(const Texture &texture)
    {
        m_texture = &texture;
    }

    /**
     * @brief Get the texture
     */
    const Texture *getTexture() const { return m_texture; }

    /**
     * @brief Add a named animation
     */
    void addAnimation(const std::string &name, const Animation &animation)
    {
        m_animations[name] = animation;
    }

    /**
     * @brief Check if an animation exists
     */
    bool hasAnimation(const std::string &name) const
    {
        return m_animations.find(name) != m_animations.end();
    }

    /**
     * @brief Play an animation by name
     * @param name Animation name
     * @param restart If true, restart even if already playing
     */
    void play(const std::string &name, bool restart = true)
    {
        if (m_currentAnimation != name || restart)
        {
            auto it = m_animations.find(name);
            if (it != m_animations.end())
            {
                m_currentAnimation = name;
                m_currentAnim = &it->second;
                if (restart)
                {
                    m_currentFrame = 0;
                    m_elapsedTime = Time();
                    m_forward = true;
                }
                m_playing = true;
                m_finished = false;
            }
        }
    }

    /**
     * @brief Stop playing
     */
    void stop()
    {
        m_playing = false;
    }

    /**
     * @brief Pause playback
     */
    void pause()
    {
        m_playing = false;
    }

    /**
     * @brief Resume playback
     */
    void resume()
    {
        m_playing = true;
    }

    /**
     * @brief Check if currently playing
     */
    bool isPlaying() const { return m_playing; }

    /**
     * @brief Check if animation finished (for Once mode)
     */
    bool isFinished() const { return m_finished; }

    /**
     * @brief Get current animation name
     */
    const std::string &getCurrentAnimation() const { return m_currentAnimation; }

    /**
     * @brief Get current frame index
     */
    size_t getCurrentFrame() const { return m_currentFrame; }

    /**
     * @brief Update the animation
     * @param deltaTime Time elapsed since last update
     */
    void update(Time deltaTime)
    {
        if (!m_playing || !m_currentAnim || m_currentAnim->empty())
            return;

        m_elapsedTime = m_elapsedTime + deltaTime;

        const AnimationFrame &frame = m_currentAnim->getFrame(m_currentFrame);

        while (m_elapsedTime.asMilliseconds() >= frame.duration.asMilliseconds())
        {
            m_elapsedTime = m_elapsedTime - frame.duration;
            advanceFrame();
        }
    }

    /**
     * @brief Set a color tint
     */
    void setColor(const Color &color) { m_color = color; }
    const Color &getColor() const { return m_color; }

    /**
     * @brief Get local bounding rectangle of current frame
     */
    FloatRect getLocalBounds() const
    {
        if (!m_currentAnim || m_currentAnim->empty())
            return FloatRect();

        const AnimationFrame &frame = m_currentAnim->getFrame(m_currentFrame);
        return FloatRect(0, 0,
                         static_cast<float>(frame.rect.width),
                         static_cast<float>(frame.rect.height));
    }

    /**
     * @brief Get global bounding rectangle
     */
    FloatRect getGlobalBounds() const
    {
        FloatRect local = getLocalBounds();

        Vector2f corners[4] = {
            transformPoint(Vector2f(0, 0)),
            transformPoint(Vector2f(local.width, 0)),
            transformPoint(Vector2f(0, local.height)),
            transformPoint(Vector2f(local.width, local.height))};

        float minX = corners[0].x, maxX = corners[0].x;
        float minY = corners[0].y, maxY = corners[0].y;

        for (int i = 1; i < 4; ++i)
        {
            minX = std::min(minX, corners[i].x);
            maxX = std::max(maxX, corners[i].x);
            minY = std::min(minY, corners[i].y);
            maxY = std::max(maxY, corners[i].y);
        }

        return FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

protected:
    void draw(RenderTarget &target) const override
    {
        if (!m_texture || !m_currentAnim || m_currentAnim->empty())
            return;

        const AnimationFrame &frame = m_currentAnim->getFrame(m_currentFrame);

        // Draw each pixel of the current frame
        for (int y = 0; y < frame.rect.height; ++y)
        {
            for (int x = 0; x < frame.rect.width; ++x)
            {
                // Get texture pixel
                Color texColor = m_texture->getPixel(
                    frame.rect.left + x,
                    frame.rect.top + y);

                // Skip transparent pixels
                if (texColor.a < 128)
                    continue;

                // Apply color tint
                Color finalColor = Color(
                    static_cast<uint8_t>((texColor.r * m_color.r) / 255),
                    static_cast<uint8_t>((texColor.g * m_color.g) / 255),
                    static_cast<uint8_t>((texColor.b * m_color.b) / 255),
                    static_cast<uint8_t>((texColor.a * m_color.a) / 255));

                // Transform position
                Vector2f worldPos = transformPoint(Vector2f(
                    static_cast<float>(x), static_cast<float>(y)));

                target.setPixel(
                    static_cast<unsigned int>(worldPos.x),
                    static_cast<unsigned int>(worldPos.y),
                    finalColor);
            }
        }
    }

private:
    void advanceFrame()
    {
        if (!m_currentAnim || m_currentAnim->empty())
            return;

        size_t frameCount = m_currentAnim->getFrameCount();
        AnimationMode mode = m_currentAnim->getMode();

        switch (mode)
        {
        case AnimationMode::Once:
            if (m_currentFrame + 1 < frameCount)
            {
                m_currentFrame++;
            }
            else
            {
                m_playing = false;
                m_finished = true;
            }
            break;

        case AnimationMode::Loop:
            m_currentFrame = (m_currentFrame + 1) % frameCount;
            break;

        case AnimationMode::PingPong:
            if (m_forward)
            {
                if (m_currentFrame + 1 < frameCount)
                    m_currentFrame++;
                else
                {
                    m_forward = false;
                    if (m_currentFrame > 0)
                        m_currentFrame--;
                }
            }
            else
            {
                if (m_currentFrame > 0)
                    m_currentFrame--;
                else
                {
                    m_forward = true;
                    if (m_currentFrame + 1 < frameCount)
                        m_currentFrame++;
                }
            }
            break;

        case AnimationMode::Reverse:
            if (m_currentFrame > 0)
                m_currentFrame--;
            else
                m_currentFrame = frameCount - 1;
            break;
        }
    }

    const Texture *m_texture;
    std::unordered_map<std::string, Animation> m_animations;
    std::string m_currentAnimation;
    Animation *m_currentAnim = nullptr;
    size_t m_currentFrame;
    Time m_elapsedTime;
    Color m_color = Color::White;
    bool m_playing;
    bool m_forward;
    bool m_finished = false;
};

TG_NAMESPACE_END
