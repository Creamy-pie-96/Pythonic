/**
 * @file Particle.hpp
 * @brief Particle system for visual effects
 *
 * Provides a high-performance particle system for creating effects like
 * explosions, fire, smoke, rain, snow, sparks, etc.
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Core.hpp"
#include "../Graphics/Drawable.hpp"
#include "../Graphics/RenderTarget.hpp"
#include <vector>
#include <functional>
#include <random>
#include <cmath>

TG_NAMESPACE_BEGIN

/**
 * @brief Individual particle data
 */
struct Particle
{
    Vector2f position;     ///< Current position
    Vector2f velocity;     ///< Movement velocity
    Vector2f acceleration; ///< Acceleration (gravity, wind, etc.)
    Color color;           ///< Current color
    Color startColor;      ///< Initial color
    Color endColor;        ///< Final color (for interpolation)
    float lifetime;        ///< Total lifetime in seconds
    float age;             ///< Current age in seconds
    float size;            ///< Particle size
    float rotation;        ///< Rotation in degrees
    float angularVelocity; ///< Rotation speed
    bool alive = false;    ///< Is particle active?

    /**
     * @brief Get normalized age (0.0 = born, 1.0 = dead)
     */
    float getNormalizedAge() const
    {
        return (lifetime > 0) ? (age / lifetime) : 1.0f;
    }
};

/**
 * @brief Configuration for particle emission
 */
struct ParticleConfig
{
    // Lifetime
    float lifetimeMin = 1.0f;
    float lifetimeMax = 2.0f;

    // Initial velocity
    float speedMin = 10.0f;
    float speedMax = 50.0f;
    float angleMin = 0.0f;   // Degrees
    float angleMax = 360.0f; // Degrees

    // Physics
    Vector2f gravity = {0.0f, 50.0f};
    float drag = 0.98f; // Velocity multiplier per frame

    // Appearance
    Color startColor = Color::White;
    Color endColor = Color(255, 255, 255, 0); // Fade out
    float sizeStart = 1.0f;
    float sizeEnd = 0.5f;

    // Rotation
    float rotationMin = 0.0f;
    float rotationMax = 0.0f;
    float angularVelMin = 0.0f;
    float angularVelMax = 0.0f;

    // Emission
    float emissionRate = 10.0f; // Particles per second
    bool burst = false;         // Emit all at once
    int burstCount = 50;        // Particles in burst

    // Spawn area
    Vector2f spawnOffset = {0, 0};
    float spawnRadius = 0.0f;

    /**
     * @brief Create config for fire effect
     */
    static ParticleConfig fire()
    {
        ParticleConfig cfg;
        cfg.lifetimeMin = 0.5f;
        cfg.lifetimeMax = 1.5f;
        cfg.speedMin = 20.0f;
        cfg.speedMax = 60.0f;
        cfg.angleMin = 250.0f; // Upward
        cfg.angleMax = 290.0f;
        cfg.gravity = {0.0f, -30.0f}; // Upward
        cfg.startColor = Color(255, 200, 50);
        cfg.endColor = Color(255, 50, 0, 0);
        cfg.sizeStart = 2.0f;
        cfg.sizeEnd = 1.0f;
        cfg.emissionRate = 30.0f;
        return cfg;
    }

    /**
     * @brief Create config for smoke effect
     */
    static ParticleConfig smoke()
    {
        ParticleConfig cfg;
        cfg.lifetimeMin = 1.0f;
        cfg.lifetimeMax = 3.0f;
        cfg.speedMin = 5.0f;
        cfg.speedMax = 20.0f;
        cfg.angleMin = 250.0f;
        cfg.angleMax = 290.0f;
        cfg.gravity = {0.0f, -10.0f};
        cfg.startColor = Color(100, 100, 100, 200);
        cfg.endColor = Color(50, 50, 50, 0);
        cfg.sizeStart = 1.0f;
        cfg.sizeEnd = 3.0f;
        cfg.emissionRate = 10.0f;
        return cfg;
    }

    /**
     * @brief Create config for explosion effect
     */
    static ParticleConfig explosion()
    {
        ParticleConfig cfg;
        cfg.lifetimeMin = 0.3f;
        cfg.lifetimeMax = 0.8f;
        cfg.speedMin = 80.0f;
        cfg.speedMax = 150.0f;
        cfg.angleMin = 0.0f;
        cfg.angleMax = 360.0f;
        cfg.gravity = {0.0f, 100.0f};
        cfg.drag = 0.95f;
        cfg.startColor = Color(255, 255, 200);
        cfg.endColor = Color(255, 100, 0, 0);
        cfg.burst = true;
        cfg.burstCount = 80;
        return cfg;
    }

    /**
     * @brief Create config for sparks effect
     */
    static ParticleConfig sparks()
    {
        ParticleConfig cfg;
        cfg.lifetimeMin = 0.2f;
        cfg.lifetimeMax = 0.6f;
        cfg.speedMin = 50.0f;
        cfg.speedMax = 120.0f;
        cfg.angleMin = 0.0f;
        cfg.angleMax = 360.0f;
        cfg.gravity = {0.0f, 150.0f};
        cfg.startColor = Color(255, 255, 150);
        cfg.endColor = Color(255, 100, 0, 0);
        cfg.burst = true;
        cfg.burstCount = 30;
        return cfg;
    }

    /**
     * @brief Create config for rain effect
     */
    static ParticleConfig rain()
    {
        ParticleConfig cfg;
        cfg.lifetimeMin = 1.0f;
        cfg.lifetimeMax = 2.0f;
        cfg.speedMin = 200.0f;
        cfg.speedMax = 300.0f;
        cfg.angleMin = 85.0f; // Slightly angled down
        cfg.angleMax = 95.0f;
        cfg.gravity = {0.0f, 500.0f};
        cfg.startColor = Color(150, 180, 255, 200);
        cfg.endColor = Color(150, 180, 255, 100);
        cfg.sizeStart = 1.0f;
        cfg.sizeEnd = 1.0f;
        cfg.emissionRate = 100.0f;
        cfg.spawnOffset = {0.0f, -10.0f};
        return cfg;
    }

    /**
     * @brief Create config for snow effect
     */
    static ParticleConfig snow()
    {
        ParticleConfig cfg;
        cfg.lifetimeMin = 3.0f;
        cfg.lifetimeMax = 5.0f;
        cfg.speedMin = 10.0f;
        cfg.speedMax = 30.0f;
        cfg.angleMin = 70.0f;
        cfg.angleMax = 110.0f;
        cfg.gravity = {0.0f, 20.0f};
        cfg.startColor = Color::White;
        cfg.endColor = Color(255, 255, 255, 200);
        cfg.emissionRate = 20.0f;
        cfg.angularVelMin = -90.0f;
        cfg.angularVelMax = 90.0f;
        return cfg;
    }

    /**
     * @brief Create config for blood/gore effect
     */
    static ParticleConfig blood()
    {
        ParticleConfig cfg;
        cfg.lifetimeMin = 0.4f;
        cfg.lifetimeMax = 1.0f;
        cfg.speedMin = 40.0f;
        cfg.speedMax = 100.0f;
        cfg.angleMin = 0.0f;
        cfg.angleMax = 360.0f;
        cfg.gravity = {0.0f, 200.0f};
        cfg.drag = 0.98f;
        cfg.startColor = Color(180, 0, 0);
        cfg.endColor = Color(80, 0, 0, 0);
        cfg.burst = true;
        cfg.burstCount = 25;
        return cfg;
    }

    /**
     * @brief Create config for muzzle flash effect
     */
    static ParticleConfig muzzleFlash()
    {
        ParticleConfig cfg;
        cfg.lifetimeMin = 0.05f;
        cfg.lifetimeMax = 0.15f;
        cfg.speedMin = 100.0f;
        cfg.speedMax = 200.0f;
        cfg.angleMin = -30.0f; // Relative to gun direction
        cfg.angleMax = 30.0f;
        cfg.gravity = {0.0f, 0.0f};
        cfg.startColor = Color(255, 255, 200);
        cfg.endColor = Color(255, 150, 0, 0);
        cfg.burst = true;
        cfg.burstCount = 15;
        return cfg;
    }
};

/**
 * @brief Particle emitter that spawns and manages particles
 *
 * @code
 * ParticleEmitter fire(100);  // Max 100 particles
 * fire.setPosition(50, 80);
 * fire.setConfig(ParticleConfig::fire());
 * fire.start();
 *
 * // In game loop
 * fire.update(deltaTime);
 * canvas.draw(fire);
 * @endcode
 */
class ParticleEmitter : public Drawable
{
public:
    /**
     * @brief Create an emitter with maximum particle count
     */
    explicit ParticleEmitter(size_t maxParticles = 500)
        : m_particles(maxParticles), m_emitting(false),
          m_accumulator(0.0f)
    {
        // Seed random generator
        std::random_device rd;
        m_rng.seed(rd());
    }

    /**
     * @brief Set emitter position
     */
    void setPosition(float x, float y)
    {
        m_position = {x, y};
    }

    void setPosition(const Vector2f &pos)
    {
        m_position = pos;
    }

    /**
     * @brief Get emitter position
     */
    Vector2f getPosition() const { return m_position; }

    /**
     * @brief Set particle configuration
     */
    void setConfig(const ParticleConfig &config)
    {
        m_config = config;
    }

    /**
     * @brief Get current configuration
     */
    const ParticleConfig &getConfig() const { return m_config; }
    ParticleConfig &getConfig() { return m_config; }

    /**
     * @brief Start emitting particles
     */
    void start()
    {
        m_emitting = true;
        if (m_config.burst)
        {
            burst(m_config.burstCount);
            m_emitting = false; // Burst is one-shot
        }
    }

    /**
     * @brief Stop emitting new particles (existing continue)
     */
    void stop()
    {
        m_emitting = false;
    }

    /**
     * @brief Clear all particles immediately
     */
    void clear()
    {
        for (auto &p : m_particles)
            p.alive = false;
    }

    /**
     * @brief Emit a burst of particles
     */
    void burst(int count)
    {
        for (int i = 0; i < count; ++i)
            emitParticle();
    }

    /**
     * @brief Emit a single particle
     */
    void emit()
    {
        emitParticle();
    }

    /**
     * @brief Check if currently emitting
     */
    bool isEmitting() const { return m_emitting; }

    /**
     * @brief Get number of active particles
     */
    size_t getActiveCount() const
    {
        size_t count = 0;
        for (const auto &p : m_particles)
            if (p.alive)
                count++;
        return count;
    }

    /**
     * @brief Update all particles
     * @param deltaTime Time elapsed since last update
     */
    void update(Time deltaTime)
    {
        float dt = deltaTime.asSeconds();

        // Emit new particles based on rate
        if (m_emitting && !m_config.burst)
        {
            m_accumulator += m_config.emissionRate * dt;
            while (m_accumulator >= 1.0f)
            {
                emitParticle();
                m_accumulator -= 1.0f;
            }
        }

        // Update existing particles
        for (auto &p : m_particles)
        {
            if (!p.alive)
                continue;

            // Age particle
            p.age += dt;
            if (p.age >= p.lifetime)
            {
                p.alive = false;
                continue;
            }

            // Apply physics
            p.velocity.x += p.acceleration.x * dt;
            p.velocity.y += p.acceleration.y * dt;
            p.velocity.x *= m_config.drag;
            p.velocity.y *= m_config.drag;
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.rotation += p.angularVelocity * dt;

            // Interpolate color
            float t = p.getNormalizedAge();
            p.color = Color::lerp(p.startColor, p.endColor, t);

            // Interpolate size
            p.size = m_config.sizeStart + (m_config.sizeEnd - m_config.sizeStart) * t;
        }
    }

    /**
     * @brief Set a custom update function
     */
    void setUpdateCallback(std::function<void(Particle &, float)> callback)
    {
        m_updateCallback = callback;
    }

protected:
    void draw(RenderTarget &target) const override
    {
        for (const auto &p : m_particles)
        {
            if (!p.alive)
                continue;

            // Draw particle as a pixel or small filled circle
            int x = static_cast<int>(p.position.x);
            int y = static_cast<int>(p.position.y);

            if (p.size <= 1.0f)
            {
                target.setPixel(x, y, p.color);
            }
            else
            {
                int r = static_cast<int>(p.size);
                target.fillCircle(x, y, r, p.color);
            }
        }
    }

private:
    void emitParticle()
    {
        // Find a dead particle to reuse
        for (auto &p : m_particles)
        {
            if (!p.alive)
            {
                initParticle(p);
                return;
            }
        }
        // No free particles available
    }

    void initParticle(Particle &p)
    {
        p.alive = true;
        p.age = 0.0f;

        // Random lifetime
        p.lifetime = randomRange(m_config.lifetimeMin, m_config.lifetimeMax);

        // Random spawn position
        float angle = randomRange(0.0f, 360.0f) * 3.14159f / 180.0f;
        float dist = randomRange(0.0f, m_config.spawnRadius);
        p.position = m_position + m_config.spawnOffset;
        p.position.x += std::cos(angle) * dist;
        p.position.y += std::sin(angle) * dist;

        // Random velocity
        float speed = randomRange(m_config.speedMin, m_config.speedMax);
        float velAngle = randomRange(m_config.angleMin, m_config.angleMax) * 3.14159f / 180.0f;
        p.velocity.x = std::cos(velAngle) * speed;
        p.velocity.y = std::sin(velAngle) * speed;

        // Set acceleration (gravity)
        p.acceleration = m_config.gravity;

        // Colors
        p.startColor = m_config.startColor;
        p.endColor = m_config.endColor;
        p.color = p.startColor;

        // Size
        p.size = m_config.sizeStart;

        // Rotation
        p.rotation = randomRange(m_config.rotationMin, m_config.rotationMax);
        p.angularVelocity = randomRange(m_config.angularVelMin, m_config.angularVelMax);
    }

    float randomRange(float min, float max)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(m_rng);
    }

    std::vector<Particle> m_particles;
    Vector2f m_position;
    ParticleConfig m_config;
    bool m_emitting;
    float m_accumulator;
    std::mt19937 m_rng;
    std::function<void(Particle &, float)> m_updateCallback;
};

TG_NAMESPACE_END
