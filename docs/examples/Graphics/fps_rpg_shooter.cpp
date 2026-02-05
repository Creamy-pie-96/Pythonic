/**
 * @file fps_rpg_shooter.cpp
 * @brief First-Person RPG Shooter Demo
 *
 * A comprehensive demo game that showcases ALL Terminal Graphics Engine features:
 * - Canvas with Braille rendering (highest resolution)
 * - Keyboard input (WASD, arrow keys, spacebar)
 * - Mouse input (look around, shoot with left click)
 * - Dynamic terminal resizing
 * - Clock/Time for delta time
 * - Shapes (Rectangle, Circle, Convex, Line)
 * - Sprites and Textures
 * - Animated Sprites (enemies, effects)
 * - Text rendering (3x5 and 5x7 fonts)
 * - Particle effects (muzzle flash, blood, explosions, fire)
 * - Audio (sound effects, music) - if SDL2 available
 * - Collision detection (AABB, circle)
 * - Z-ordering/Layers (background, entities, projectiles, effects, UI)
 * - Bezier curves and polygon fill (for dungeon geometry)
 *
 * Controls:
 * - W/S: Move forward/backward
 * - A/D: Strafe left/right
 * - Left/Right arrows OR Mouse: Turn/Look
 * - Space OR Left Click: Shoot
 * - R OR Right Click: Reload
 * - E: Interact
 * - Tab: Show stats
 * - Q: Quit
 */

#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
#include <cmath>
#include <vector>
#include <memory>
#include <string>
#include <random>
#include <chrono>
#include <queue>
#include <algorithm>

using namespace Pythonic::TG;

//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------
// These are defaults - actual size will be determined dynamically
constexpr float FOV = 60.0f;               // Field of view in degrees
constexpr float MAX_DEPTH = 16.0f;         // Maximum render distance
constexpr int MAP_WIDTH = 24;              // Map width
constexpr int MAP_HEIGHT = 24;             // Map height
constexpr float MOUSE_SENSITIVITY = 0.15f; // Mouse look sensitivity

//-----------------------------------------------------------------------------
// Map Data - 1 = wall, 0 = floor, 2 = door, 3 = health pickup, 4 = ammo
//-----------------------------------------------------------------------------
const int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 3, 0, 0, 0, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 1, 2, 1, 1, 0, 0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 1, 1, 1, 2, 1, 0, 0, 0, 0, 1, 2, 1, 1, 1, 1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 3, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 4, 0, 1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

//-----------------------------------------------------------------------------
// Player Stats
//-----------------------------------------------------------------------------
struct Player
{
    float x = 3.0f; // Position - start in open area
    float y = 3.0f;
    float angle = 0.0f;     // Looking direction in radians
    float moveSpeed = 4.0f; // Units per second
    float rotSpeed = 2.5f;  // Radians per second

    int health = 100;
    int maxHealth = 100;
    int ammo = 30;
    int maxAmmo = 90;
    int currentClip = 10;
    int clipSize = 10;

    int kills = 0;
    int level = 1;
    int xp = 0;
    int xpToNext = 100;

    bool isReloading = false;
    float reloadTimer = 0.0f;
    float reloadTime = 1.5f;

    float shootCooldown = 0.0f;
    float shootRate = 0.15f; // Seconds between shots
};

//-----------------------------------------------------------------------------
// Enemy
//-----------------------------------------------------------------------------
struct Enemy
{
    float x, y;
    float health = 50.0f;
    float maxHealth = 50.0f;
    float speed = 2.0f;
    bool alive = true;
    float distance = 0.0f; // Distance to player (for sorting)
    int xpReward = 25;

    // Animation
    float animTimer = 0.0f;
    int animFrame = 0;

    // Pathfinding
    std::vector<std::pair<int, int>> path;
    float pathTimer = 0.0f;
    int pathIndex = 0;

    Enemy(float px, float py) : x(px), y(py) {}
};

//-----------------------------------------------------------------------------
// Projectile
//-----------------------------------------------------------------------------
struct Projectile
{
    float x, y;
    float dx, dy; // Direction
    float speed = 20.0f;
    float lifetime = 2.0f;
    bool alive = true;
    int damage = 25;
};

//-----------------------------------------------------------------------------
// Pickup
//-----------------------------------------------------------------------------
struct Pickup
{
    float x, y;
    int type; // 3 = health, 4 = ammo
    bool collected = false;

    Pickup(float px, float py, int t) : x(px), y(py), type(t) {}
};

//-----------------------------------------------------------------------------
// Game State
//-----------------------------------------------------------------------------
class FPSGame
{
public:
    FPSGame()
        : canvas(Canvas::createFullscreen(RenderMode::Braille))
    {
        // Get actual screen size from canvas
        auto size = canvas.getSize();
        screenWidth = size.x;
        screenHeight = size.y;

        Keyboard::init();
        Mouse::init(); // Initialize mouse support
        Canvas::initDisplay();
        initGame();
    }

    ~FPSGame()
    {
        Mouse::shutdown();    // Cleanup mouse first
        Keyboard::shutdown(); // Then keyboard
        Canvas::cleanupDisplay();
    }

    void run()
    {
        while (running)
        {
            float dt = clock.restart().asSeconds();
            if (dt > 0.1f)
                dt = 0.1f; // Cap delta time

            // Check for terminal resize
            checkResize();

            handleInput(dt);
            update(dt);
            render();
        }
    }

private:
    Canvas canvas;
    Clock clock;
    bool running = true;
    bool gameOver = false;
    float gameOverTimer = 0.0f;

    unsigned int screenWidth;
    unsigned int screenHeight;

    Player player;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<Pickup> pickups;

    // Particle emitters
    ParticleEmitter muzzleFlash{50};
    ParticleEmitter bloodEffect{100};
    ParticleEmitter explosionEffect{200};
    std::vector<ParticleEmitter> torchEffects;

    // HUD
    bool showStats = false;

    // Messages
    std::string messageText;
    float messageTimer = 0.0f;

    // Random
    std::mt19937 rng{static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())};

    void initGame()
    {
        // Setup particle effects
        muzzleFlash.setConfig(ParticleConfig::muzzleFlash());
        bloodEffect.setConfig(ParticleConfig::blood());
        explosionEffect.setConfig(ParticleConfig::explosion());

        // Find pickups from map
        for (int y = 0; y < MAP_HEIGHT; ++y)
        {
            for (int x = 0; x < MAP_WIDTH; ++x)
            {
                if (worldMap[y][x] == 3 || worldMap[y][x] == 4)
                {
                    pickups.emplace_back(x + 0.5f, y + 0.5f, worldMap[y][x]);
                }
            }
        }

        // Spawn initial enemies
        spawnEnemies(5);

        // Setup torch fire effects near doors
        ParticleConfig torchConfig = ParticleConfig::fire();
        torchConfig.emissionRate = 8.0f;
        torchConfig.sizeStart = 1.0f;

        // Find doors and add torches
        for (int y = 0; y < MAP_HEIGHT; ++y)
        {
            for (int x = 0; x < MAP_WIDTH; ++x)
            {
                if (worldMap[y][x] == 2) // Door
                {
                    ParticleEmitter torch{30};
                    torch.setConfig(torchConfig);
                    torch.setPosition(x + 0.5f, y + 0.5f);
                    torch.start();
                    torchEffects.push_back(std::move(torch));
                }
            }
        }

        std::string mouseMode = Mouse::isUsingEvdev() ? " [DirectMouse]" : " [TermMouse]";
        showMessage("Dungeon FPS! WASD move, Mouse aim, Click shoot" + mouseMode);
    }

    void restartGame()
    {
        // Reset player
        player.x = 3.0f;
        player.y = 3.0f;
        player.angle = 0.0f;
        player.health = player.maxHealth;
        player.ammo = 30;
        player.currentClip = player.clipSize;
        player.kills = 0;
        player.level = 1;
        player.xp = 0;
        player.xpToNext = 100;
        player.isReloading = false;
        player.shootCooldown = 0.0f;

        // Clear entities
        enemies.clear();
        projectiles.clear();

        // Reset pickups
        pickups.clear();
        for (int y = 0; y < MAP_HEIGHT; ++y)
        {
            for (int x = 0; x < MAP_WIDTH; ++x)
            {
                if (worldMap[y][x] == 3 || worldMap[y][x] == 4)
                {
                    pickups.emplace_back(x + 0.5f, y + 0.5f, worldMap[y][x]);
                }
            }
        }

        // Spawn new enemies
        spawnEnemies(5);

        // Reset game state
        gameOver = false;
        gameOverTimer = 0.0f;

        showMessage("Game Restarted! Good luck!");
    }

    void checkResize()
    {
        // Check if terminal was resized
        if (detail::terminalSizeChanged)
        {
            detail::terminalSizeChanged = false;

            // Recreate canvas with new size
            canvas = Canvas::createFullscreen(RenderMode::Braille);
            auto size = canvas.getSize();
            screenWidth = size.x;
            screenHeight = size.y;
        }
    }

    void spawnEnemies(int count)
    {
        std::uniform_real_distribution<float> dist(2.0f, MAP_WIDTH - 2.0f);

        for (int i = 0; i < count; ++i)
        {
            float ex, ey;
            do
            {
                ex = dist(rng);
                ey = dist(rng);
            } while (worldMap[static_cast<int>(ey)][static_cast<int>(ex)] != 0 ||
                     (std::abs(ex - player.x) < 5.0f && std::abs(ey - player.y) < 5.0f));

            enemies.emplace_back(ex, ey);
        }
    }

    void handleInput(float dt)
    {
        // Quit
        if (Keyboard::isKeyPressed(Key::Q) || Keyboard::isKeyPressed(Key::Escape))
        {
            running = false;
            return;
        }

        // If game over, allow restart with R
        if (gameOver)
        {
            if (Keyboard::isKeyPressed(Key::R))
            {
                restartGame();
            }
            return; // Don't process other input while dead
        }

        // Tab - show stats
        showStats = Keyboard::isKeyPressed(Key::Tab);

        float moveX = 0, moveY = 0;

        // Calculate direction vectors
        float dirX = std::cos(player.angle);
        float dirY = std::sin(player.angle);
        float perpX = -dirY; // Perpendicular for strafing
        float perpY = dirX;

        // Forward/Backward
        if (Keyboard::isKeyPressed(Key::W) || Keyboard::isKeyPressed(Key::Up))
        {
            moveX += dirX;
            moveY += dirY;
        }
        if (Keyboard::isKeyPressed(Key::S) || Keyboard::isKeyPressed(Key::Down))
        {
            moveX -= dirX;
            moveY -= dirY;
        }

        // Strafe - Fixed direction: A = left, D = right
        if (Keyboard::isKeyPressed(Key::A))
        {
            moveX -= perpX;
            moveY -= perpY;
        }
        if (Keyboard::isKeyPressed(Key::D))
        {
            moveX += perpX;
            moveY += perpY;
        }

        // Normalize and apply movement
        float len = std::sqrt(moveX * moveX + moveY * moveY);
        if (len > 0.001f)
        {
            moveX /= len;
            moveY /= len;

            float newX = player.x + moveX * player.moveSpeed * dt;
            float newY = player.y + moveY * player.moveSpeed * dt;

            // Collision detection with walls
            if (worldMap[static_cast<int>(player.y)][static_cast<int>(newX)] == 0 ||
                worldMap[static_cast<int>(player.y)][static_cast<int>(newX)] == 2)
            {
                player.x = newX;
            }
            if (worldMap[static_cast<int>(newY)][static_cast<int>(player.x)] == 0 ||
                worldMap[static_cast<int>(newY)][static_cast<int>(player.x)] == 2)
            {
                player.y = newY;
            }
        }

        // Rotation via keyboard - Fixed: Right arrow = turn right (increase angle)
        if (Keyboard::isKeyPressed(Key::Right))
            player.angle += player.rotSpeed * dt;
        if (Keyboard::isKeyPressed(Key::Left))
            player.angle -= player.rotSpeed * dt;

        // Mouse look - use raw delta for smooth precision when evdev is available
        if (Mouse::isUsingEvdev())
        {
            auto rawDelta = Mouse::getRawDelta();
            if (rawDelta.x != 0)
            {
                player.angle += rawDelta.x * MOUSE_SENSITIVITY * 0.01f;
            }
        }
        else
        {
            // Terminal mouse fallback
            auto mouseDelta = Mouse::getDelta();
            if (mouseDelta.x != 0)
            {
                player.angle += mouseDelta.x * MOUSE_SENSITIVITY;
            }
        }

        // Shoot with Space or Left mouse button
        bool wantsToShoot = Keyboard::isKeyPressed(Key::Space) ||
                            Mouse::isButtonPressed(MouseButton::Left);

        if (wantsToShoot && player.shootCooldown <= 0.0f &&
            !player.isReloading && player.currentClip > 0)
        {
            shoot();
            player.shootCooldown = player.shootRate;
            player.currentClip--;

            if (player.currentClip == 0 && player.ammo > 0)
            {
                startReload();
            }
        }

        // Reload with R or Right mouse button
        bool wantsReload = Keyboard::isKeyPressed(Key::R) ||
                           Mouse::isButtonPressed(MouseButton::Right);

        if (wantsReload && !player.isReloading &&
            player.currentClip < player.clipSize && player.ammo > 0)
        {
            startReload();
        }
    }

    void shoot()
    {
        // Create projectile
        Projectile p;
        p.x = player.x;
        p.y = player.y;
        p.dx = std::cos(player.angle);
        p.dy = std::sin(player.angle);
        projectiles.push_back(p);

        // Muzzle flash effect
        muzzleFlash.setPosition(screenWidth / 2.0f, screenHeight * 0.6f);
        muzzleFlash.start();
    }

    void startReload()
    {
        player.isReloading = true;
        player.reloadTimer = player.reloadTime;
        showMessage("Reloading...");
    }

    void update(float dt)
    {
        // Update message timer even when dead
        if (messageTimer > 0)
            messageTimer -= dt;

        // Update game over timer
        if (gameOver)
        {
            gameOverTimer += dt;
            // Still update particles for visual effect
            muzzleFlash.update(Time::milliseconds(static_cast<int>(dt * 1000)));
            bloodEffect.update(Time::milliseconds(static_cast<int>(dt * 1000)));
            explosionEffect.update(Time::milliseconds(static_cast<int>(dt * 1000)));
            return; // Don't update anything else when dead
        }

        // Update cooldowns
        if (player.shootCooldown > 0)
            player.shootCooldown -= dt;

        // Update reload
        if (player.isReloading)
        {
            player.reloadTimer -= dt;
            if (player.reloadTimer <= 0)
            {
                int needed = player.clipSize - player.currentClip;
                int available = std::min(needed, player.ammo);
                player.currentClip += available;
                player.ammo -= available;
                player.isReloading = false;
                showMessage("Reloaded!");
            }
        }

        // Update projectiles
        updateProjectiles(dt);

        // Update enemies
        updateEnemies(dt);

        // Check pickups
        checkPickups();

        // Update particles
        muzzleFlash.update(Time::milliseconds(static_cast<int>(dt * 1000)));
        bloodEffect.update(Time::milliseconds(static_cast<int>(dt * 1000)));
        explosionEffect.update(Time::milliseconds(static_cast<int>(dt * 1000)));

        for (auto &torch : torchEffects)
            torch.update(Time::milliseconds(static_cast<int>(dt * 1000)));

        // Check level up
        if (player.xp >= player.xpToNext)
        {
            player.level++;
            player.xp -= player.xpToNext;
            player.xpToNext = static_cast<int>(player.xpToNext * 1.5f);
            player.maxHealth += 20;
            player.health = player.maxHealth;
            player.moveSpeed += 0.3f;
            showMessage("Level Up! Level " + std::to_string(player.level));
        }

        // Respawn enemies
        if (enemies.empty())
        {
            spawnEnemies(5 + player.level);
            showMessage("New wave of enemies!");
        }
    }

    void updateProjectiles(float dt)
    {
        for (auto &p : projectiles)
        {
            if (!p.alive)
                continue;

            p.x += p.dx * p.speed * dt;
            p.y += p.dy * p.speed * dt;
            p.lifetime -= dt;

            if (p.lifetime <= 0)
            {
                p.alive = false;
                continue;
            }

            // Wall collision
            int mapX = static_cast<int>(p.x);
            int mapY = static_cast<int>(p.y);
            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT ||
                worldMap[mapY][mapX] == 1)
            {
                p.alive = false;
                continue;
            }

            // Enemy collision
            for (auto &e : enemies)
            {
                if (!e.alive)
                    continue;

                float dx = e.x - p.x;
                float dy = e.y - p.y;
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist < 0.5f) // Hit radius
                {
                    e.health -= p.damage;
                    p.alive = false;

                    // Blood effect
                    float screenX = (e.x - player.x) * 30.0f + screenWidth / 2.0f;
                    float screenY = screenHeight / 2.0f;
                    bloodEffect.setPosition(screenX, screenY);
                    bloodEffect.start();

                    if (e.health <= 0)
                    {
                        e.alive = false;
                        player.kills++;
                        player.xp += e.xpReward;

                        // Explosion effect
                        explosionEffect.setPosition(screenX, screenY);
                        explosionEffect.start();

                        showMessage("Enemy killed! +" + std::to_string(e.xpReward) + " XP");
                    }
                    break;
                }
            }
        }

        // Remove dead projectiles
        projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                           [](const Projectile &p)
                           { return !p.alive; }),
            projectiles.end());
    }

    //-------------------------------------------------------------------------
    // A* Pathfinding for enemies
    //-------------------------------------------------------------------------
    struct PathNode
    {
        int x, y;
        float g, h, f;
        int parentX, parentY;

        bool operator>(const PathNode &o) const { return f > o.f; }
    };

    bool isWalkable(int x, int y) const
    {
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
            return false;
        int tile = worldMap[y][x];
        return tile == 0 || tile == 2 || tile == 3 || tile == 4; // Floor, door, pickups
    }

    std::vector<std::pair<int, int>> findPath(int startX, int startY, int goalX, int goalY)
    {
        std::vector<std::pair<int, int>> result;

        // Quick check - if goal is a wall, find nearest floor
        if (!isWalkable(goalX, goalY))
        {
            // Just return empty, enemy will wait
            return result;
        }

        // A* algorithm
        std::vector<std::vector<bool>> closed(MAP_HEIGHT, std::vector<bool>(MAP_WIDTH, false));
        std::vector<std::vector<PathNode>> nodes(MAP_HEIGHT, std::vector<PathNode>(MAP_WIDTH));

        // Initialize nodes
        for (int y = 0; y < MAP_HEIGHT; ++y)
        {
            for (int x = 0; x < MAP_WIDTH; ++x)
            {
                nodes[y][x] = {x, y, 1e30f, 1e30f, 1e30f, -1, -1};
            }
        }

        // Priority queue (min-heap by f value)
        std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openSet;

        // Start node
        float h = std::abs(goalX - startX) + std::abs(goalY - startY);
        nodes[startY][startX] = {startX, startY, 0, h, h, -1, -1};
        openSet.push(nodes[startY][startX]);

        // 4-directional movement
        const int dx[] = {0, 1, 0, -1};
        const int dy[] = {-1, 0, 1, 0};

        while (!openSet.empty())
        {
            PathNode current = openSet.top();
            openSet.pop();

            int cx = current.x;
            int cy = current.y;

            if (closed[cy][cx])
                continue;
            closed[cy][cx] = true;

            // Reached goal?
            if (cx == goalX && cy == goalY)
            {
                // Reconstruct path
                int px = goalX, py = goalY;
                while (px != startX || py != startY)
                {
                    result.push_back({px, py});
                    int npx = nodes[py][px].parentX;
                    int npy = nodes[py][px].parentY;
                    px = npx;
                    py = npy;
                    if (result.size() > 100)
                        break; // Safety limit
                }
                std::reverse(result.begin(), result.end());
                return result;
            }

            // Explore neighbors
            for (int i = 0; i < 4; ++i)
            {
                int nx = cx + dx[i];
                int ny = cy + dy[i];

                if (!isWalkable(nx, ny) || closed[ny][nx])
                    continue;

                float newG = nodes[cy][cx].g + 1.0f;

                if (newG < nodes[ny][nx].g)
                {
                    float newH = std::abs(goalX - nx) + std::abs(goalY - ny);
                    nodes[ny][nx] = {nx, ny, newG, newH, newG + newH, cx, cy};
                    openSet.push(nodes[ny][nx]);
                }
            }
        }

        return result; // No path found
    }

    void updateEnemies(float dt)
    {
        for (auto &e : enemies)
        {
            if (!e.alive)
                continue;

            // Calculate distance to player
            float dx = player.x - e.x;
            float dy = player.y - e.y;
            e.distance = std::sqrt(dx * dx + dy * dy);

            // Update path periodically (every 0.5 seconds)
            e.pathTimer += dt;
            if (e.pathTimer >= 0.5f || e.path.empty())
            {
                e.pathTimer = 0.0f;
                int startX = static_cast<int>(e.x);
                int startY = static_cast<int>(e.y);
                int goalX = static_cast<int>(player.x);
                int goalY = static_cast<int>(player.y);

                e.path = findPath(startX, startY, goalX, goalY);
                e.pathIndex = 0;
            }

            // Move along path using A* pathfinding
            if (e.distance > 1.0f)
            {
                if (!e.path.empty() && e.pathIndex < static_cast<int>(e.path.size()))
                {
                    // Get next waypoint
                    int targetX = e.path[e.pathIndex].first;
                    int targetY = e.path[e.pathIndex].second;

                    // Move towards center of target cell
                    float targetCenterX = targetX + 0.5f;
                    float targetCenterY = targetY + 0.5f;

                    float toDx = targetCenterX - e.x;
                    float toDy = targetCenterY - e.y;
                    float toDist = std::sqrt(toDx * toDx + toDy * toDy);

                    if (toDist > 0.1f)
                    {
                        float moveX = (toDx / toDist) * e.speed * dt;
                        float moveY = (toDy / toDist) * e.speed * dt;

                        float newX = e.x + moveX;
                        float newY = e.y + moveY;

                        // Only move if destination is walkable
                        if (isWalkable(static_cast<int>(newX), static_cast<int>(e.y)))
                            e.x = newX;
                        if (isWalkable(static_cast<int>(e.x), static_cast<int>(newY)))
                            e.y = newY;
                    }
                    else
                    {
                        // Reached waypoint, move to next
                        e.pathIndex++;
                    }
                }
            }
            else
            {
                // Attack player when close
                player.health -= 1; // Damage over time when close
            }

            // Animation
            e.animTimer += dt;
            if (e.animTimer >= 0.2f)
            {
                e.animTimer = 0;
                e.animFrame = (e.animFrame + 1) % 4;
            }
        }

        // Remove dead enemies
        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                           [](const Enemy &e)
                           { return !e.alive; }),
            enemies.end());

        // Check game over
        if (player.health <= 0 && !gameOver)
        {
            player.health = 0;
            gameOver = true;
            gameOverTimer = 0.0f;
            showMessage("YOU DIED! Press R to restart or Q to quit");
        }
    }

    void checkPickups()
    {
        for (auto &p : pickups)
        {
            if (p.collected)
                continue;

            float dx = player.x - p.x;
            float dy = player.y - p.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 0.8f)
            {
                p.collected = true;

                if (p.type == 3) // Health
                {
                    player.health = std::min(player.health + 25, player.maxHealth);
                    showMessage("+25 Health!");
                }
                else if (p.type == 4) // Ammo
                {
                    player.ammo = std::min(player.ammo + 20, player.maxAmmo);
                    showMessage("+20 Ammo!");
                }
            }
        }
    }

    void render()
    {
        canvas.clear(Color::Black);

        // Render 3D view
        render3DView();

        // Render entities (enemies, pickups)
        renderEntities();

        // Render particle effects
        canvas.draw(muzzleFlash);
        canvas.draw(bloodEffect);
        canvas.draw(explosionEffect);

        // Render weapon
        renderWeapon();

        // Render HUD
        renderHUD();

        // Render minimap
        renderMinimap();

        // Render game over screen if dead
        if (gameOver)
        {
            renderGameOver();
        }

        // Display
        canvas.display();
    }

    void renderGameOver()
    {
        // Dark overlay
        for (unsigned int y = 0; y < screenHeight; ++y)
        {
            for (unsigned int x = 0; x < screenWidth; ++x)
            {
                Color c = canvas.getPixel(x, y);
                c.r = c.r / 3;
                c.g = c.g / 4; // More blue tint
                c.b = c.b / 3;
                canvas.setPixel(x, y, c);
            }
        }

        // Blood red border effect
        for (unsigned int x = 0; x < screenWidth; ++x)
        {
            for (int i = 0; i < 10; ++i)
            {
                canvas.setPixel(x, i, Color(150, 0, 0));
                canvas.setPixel(x, screenHeight - 1 - i, Color(150, 0, 0));
            }
        }
        for (unsigned int y = 0; y < screenHeight; ++y)
        {
            for (int i = 0; i < 5; ++i)
            {
                canvas.setPixel(i, y, Color(150, 0, 0));
                canvas.setPixel(screenWidth - 1 - i, y, Color(150, 0, 0));
            }
        }

        // Game over text box
        int boxW = 200;
        int boxH = 80;
        int boxX = (screenWidth - boxW) / 2;
        int boxY = (screenHeight - boxH) / 2;

        canvas.fillRect(boxX, boxY, boxW, boxH, Color(20, 0, 0));
        canvas.drawRect(boxX, boxY, boxW, boxH, Color(255, 0, 0));
        canvas.drawRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, Color(200, 0, 0));

        // Game over text
        Text::drawLargeCentered(canvas, "GAME OVER", screenWidth / 2, boxY + 15, Color(255, 50, 50));
        Text::drawLargeCentered(canvas, "You have fallen...", screenWidth / 2, boxY + 30, Color(200, 200, 200));

        // Stats
        std::string statsLine = "Kills: " + std::to_string(player.kills) + "  Level: " + std::to_string(player.level);
        Text::drawLargeCentered(canvas, statsLine, screenWidth / 2, boxY + 45, Color(255, 200, 0));

        // Instructions
        Text::drawLargeCentered(canvas, "Press R to Restart", screenWidth / 2, boxY + 60, Color(0, 255, 0));
        Text::drawLargeCentered(canvas, "Press Q to Quit", screenWidth / 2, boxY + 72, Color(150, 150, 150));
    }

    void render3DView()
    {
        // ASPECT RATIO FIX: Terminal chars are taller than wide (~2:1)
        // Multiply FOV by 0.5 to compensate
        float fovRad = (FOV * 0.5f) * 3.14159f / 180.0f;

        // Depth buffer for edge detection (store distance per column)
        std::vector<float> depthBuffer(screenWidth, MAX_DEPTH);

        for (unsigned int x = 0; x < screenWidth; ++x)
        {
            // Calculate ray angle
            float rayAngle = (player.angle - fovRad / 2.0f) +
                             (static_cast<float>(x) / screenWidth) * fovRad;

            float dirX = std::cos(rayAngle);
            float dirY = std::sin(rayAngle);

            // DDA raycasting
            float distance = 0;
            bool hit = false;
            int wallType = 0;
            bool side = false;

            float rayX = player.x;
            float rayY = player.y;

            int mapX = static_cast<int>(rayX);
            int mapY = static_cast<int>(rayY);

            float deltaDistX = (dirX == 0) ? 1e30f : std::abs(1.0f / dirX);
            float deltaDistY = (dirY == 0) ? 1e30f : std::abs(1.0f / dirY);

            float sideDistX, sideDistY;
            int stepX, stepY;

            if (dirX < 0)
            {
                stepX = -1;
                sideDistX = (rayX - mapX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (mapX + 1.0f - rayX) * deltaDistX;
            }
            if (dirY < 0)
            {
                stepY = -1;
                sideDistY = (rayY - mapY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (mapY + 1.0f - rayY) * deltaDistY;
            }

            // Perform DDA
            while (!hit && distance < MAX_DEPTH)
            {
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = false;
                }
                else
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = true;
                }

                if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT)
                {
                    wallType = worldMap[mapY][mapX];
                    if (wallType >= 1)
                    {
                        hit = true;
                    }
                }
                else
                {
                    hit = true;
                    wallType = 1;
                }
            }

            // Calculate perpendicular distance
            if (side)
                distance = sideDistY - deltaDistY;
            else
                distance = sideDistX - deltaDistX;

            if (distance < 0.1f)
                distance = 0.1f;

            // Store in depth buffer for edge detection
            depthBuffer[x] = distance;

            // Calculate wall height
            int wallHeight = static_cast<int>(screenHeight / distance);

            int drawStart = -wallHeight / 2 + screenHeight / 2;
            int drawEnd = wallHeight / 2 + screenHeight / 2;

            if (drawStart < 0)
                drawStart = 0;
            if (drawEnd >= static_cast<int>(screenHeight))
                drawEnd = screenHeight - 1;

            // SHADED COLOR PALETTE: Desaturate and darken with distance
            // Near walls are bright and saturated, far walls are grey and dark
            float distRatio = distance / MAX_DEPTH;
            float saturation = 1.0f - distRatio * 0.7f; // Desaturate with distance
            float brightness = 1.0f - distRatio * 0.5f; // Darken with distance
            if (brightness < 0.3f)
                brightness = 0.3f;

            Color wallColor;
            switch (wallType)
            {
            case 1: // Regular wall - Red
            {
                float r = (side ? 255 : 200) * brightness;
                float g = (side ? 50 : 30) * saturation * brightness;
                float b = (side ? 50 : 30) * saturation * brightness;
                wallColor = Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
            }
            break;
            case 2: // Door - Yellow/Gold
            {
                float r = (side ? 255 : 220) * brightness;
                float g = (side ? 220 : 180) * saturation * brightness;
                float b = (side ? 50 : 30) * saturation * brightness;
                wallColor = Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
            }
            break;
            default:
                wallColor = Color(static_cast<uint8_t>(180 * brightness),
                                  static_cast<uint8_t>(180 * brightness),
                                  static_cast<uint8_t>(200 * brightness));
            }

            // CEILING: Pure black (minimal dots = sparse)
            for (int y = 0; y < drawStart; ++y)
            {
                canvas.setPixel(x, y, Color(0, 0, 0)); // Pure black = no braille dots
            }

            // WALL: Solid color
            for (int y = drawStart; y < drawEnd; ++y)
            {
                canvas.setPixel(x, y, wallColor);
            }

            // FLOOR with DEPTH-BASED SPARSITY
            for (int y = drawEnd; y < static_cast<int>(screenHeight); ++y)
            {
                float depth = static_cast<float>(screenHeight) / (2.0f * y - screenHeight);
                float depthRatio = depth / MAX_DEPTH;

                // Checkerboard pattern
                float floorX = player.x + depth * dirX;
                float floorY = player.y + depth * dirY;
                bool checker = ((static_cast<int>(floorX) + static_cast<int>(floorY)) % 2 == 0);

                // DEPTH-BASED SPARSITY: Far floor is darker (fewer dots rendered)
                // Near floor is brighter (more dots rendered)
                float floorBright = 1.0f - depthRatio * 0.8f;
                if (floorBright < 0.1f)
                    floorBright = 0.1f;

                // Desaturate with distance
                float floorSat = 1.0f - depthRatio * 0.9f;
                if (floorSat < 0.1f)
                    floorSat = 0.1f;

                Color floorColor;
                if (checker)
                {
                    // Green tile - desaturates to grey at distance
                    float g = 100 * floorBright;
                    float r = 40 * floorBright + 60 * (1.0f - floorSat) * floorBright;
                    float b = 30 * floorBright + 60 * (1.0f - floorSat) * floorBright;
                    floorColor = Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
                }
                else
                {
                    // Brown tile - desaturates to dark grey at distance
                    float r = 60 * floorBright;
                    float g = 40 * floorBright + 20 * (1.0f - floorSat) * floorBright;
                    float b = 20 * floorBright + 40 * (1.0f - floorSat) * floorBright;
                    floorColor = Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
                }

                canvas.setPixel(x, y, floorColor);
            }
        }

        // EDGE DETECTION PASS: Draw white outlines where depth changes significantly
        for (unsigned int x = 1; x < screenWidth - 1; ++x)
        {
            float depthDiff = std::abs(depthBuffer[x] - depthBuffer[x - 1]) +
                              std::abs(depthBuffer[x] - depthBuffer[x + 1]);

            // If significant depth discontinuity, draw edge
            if (depthDiff > 1.5f)
            {
                // Calculate wall height for this column
                int wallHeight = static_cast<int>(screenHeight / depthBuffer[x]);
                int drawStart = -wallHeight / 2 + screenHeight / 2;
                int drawEnd = wallHeight / 2 + screenHeight / 2;
                if (drawStart < 0)
                    drawStart = 0;
                if (drawEnd >= static_cast<int>(screenHeight))
                    drawEnd = screenHeight - 1;

                // Draw bright white edge on wall edges
                Color edgeColor(255, 255, 255);
                canvas.setPixel(x, drawStart, edgeColor);
                canvas.setPixel(x, drawStart + 1, edgeColor);
                canvas.setPixel(x, drawEnd - 1, edgeColor);
                canvas.setPixel(x, drawEnd, edgeColor);

                // Draw vertical edge line
                for (int y = drawStart; y < drawEnd; y += 3)
                {
                    canvas.setPixel(x, y, edgeColor);
                }
            }
        }
    }

    void renderEntities()
    {
        // Sort enemies by distance (furthest first)
        std::vector<Enemy *> sortedEnemies;
        for (auto &e : enemies)
        {
            if (e.alive)
            {
                float dx = e.x - player.x;
                float dy = e.y - player.y;
                e.distance = std::sqrt(dx * dx + dy * dy);
                sortedEnemies.push_back(&e);
            }
        }

        std::sort(sortedEnemies.begin(), sortedEnemies.end(),
                  [](Enemy *a, Enemy *b)
                  { return a->distance > b->distance; });

        // Render each enemy - Use BRIGHT CYAN for maximum visibility against red walls
        for (Enemy *e : sortedEnemies)
        {
            renderSprite(e->x, e->y, Color(0, 255, 255), 0.5f); // Cyan - stands out against red
        }

        // Render pickups
        for (const auto &p : pickups)
        {
            if (!p.collected)
            {
                Color color = (p.type == 3) ? Color(0, 255, 128) : Color(255, 255, 0); // Bright green/Yellow
                renderSprite(p.x, p.y, color, 0.3f);
            }
        }
    }

    void renderSprite(float spriteX, float spriteY, const Color &color, float size)
    {
        // Transform sprite position relative to player
        float relX = spriteX - player.x;
        float relY = spriteY - player.y;

        // Rotate to player's view
        float cosA = std::cos(-player.angle);
        float sinA = std::sin(-player.angle);
        float transformX = relX * cosA - relY * sinA;
        float transformY = relX * sinA + relY * cosA;

        if (transformY <= 0.1f)
            return; // Behind player

        // Project to screen
        float fovRad = FOV * 3.14159f / 180.0f;
        float spriteScreenX = screenWidth / 2.0f + transformX / transformY * screenWidth / (2.0f * std::tan(fovRad / 2.0f));

        // Calculate sprite size on screen
        float spriteHeight = size * screenHeight / transformY;
        float spriteWidth = spriteHeight;

        int drawStartX = static_cast<int>(spriteScreenX - spriteWidth / 2);
        int drawEndX = static_cast<int>(spriteScreenX + spriteWidth / 2);
        int drawStartY = static_cast<int>(screenHeight / 2 - spriteHeight / 2);
        int drawEndY = static_cast<int>(screenHeight / 2 + spriteHeight / 2);

        // Clamp to screen
        if (drawStartX < 0)
            drawStartX = 0;
        if (drawEndX >= static_cast<int>(screenWidth))
            drawEndX = screenWidth - 1;
        if (drawStartY < 0)
            drawStartY = 0;
        if (drawEndY >= static_cast<int>(screenHeight))
            drawEndY = screenHeight - 1;

        // Distance fog
        float fog = 1.0f - (transformY / MAX_DEPTH);
        if (fog < 0.2f)
            fog = 0.2f;

        Color fogColor(
            static_cast<uint8_t>(color.r * fog),
            static_cast<uint8_t>(color.g * fog),
            static_cast<uint8_t>(color.b * fog));

        // Draw sprite as filled rectangle
        canvas.fillRect(drawStartX, drawStartY, drawEndX - drawStartX, drawEndY - drawStartY, fogColor);
    }

    void renderWeapon()
    {
        // Draw simple gun shape at bottom center
        int gunX = screenWidth / 2;
        int gunY = screenHeight - 20;
        int gunWidth = 30;
        int gunHeight = 40;

        // Gun body (dark gray)
        canvas.fillRect(gunX - gunWidth / 2, gunY, gunWidth, gunHeight, Color(60, 60, 60));

        // Gun barrel
        canvas.fillRect(gunX - 5, gunY - 20, 10, 25, Color(40, 40, 40));

        // Gun handle
        canvas.fillRect(gunX - gunWidth / 2 - 5, gunY + 15, gunWidth + 10, 20, Color(80, 60, 40));

        // Muzzle flash when recently fired
        if (player.shootCooldown > player.shootRate * 0.5f)
        {
            canvas.fillCircle(gunX, gunY - 25, 8, Color::Yellow);
            canvas.fillCircle(gunX, gunY - 30, 5, Color::White);
        }
    }

    void renderHUD()
    {
        // Health bar (left side) - with black background for contrast
        int barWidth = 80;
        int barHeight = 12; // Taller bar
        int barX = 10;
        int barY = 10;

        // Dark background behind health section
        canvas.fillRect(barX - 2, barY - 2, barWidth + 4, barHeight + 20, Color(0, 0, 0));

        // Health background
        canvas.fillRect(barX, barY, barWidth, barHeight, Color(80, 0, 0));

        // Health fill
        int healthWidth = (player.health * barWidth) / player.maxHealth;
        Color healthColor = (player.health > 30) ? Color(0, 255, 0) : Color(255, 0, 0);
        canvas.fillRect(barX, barY, healthWidth, barHeight, healthColor);

        // Health border - bright white
        canvas.drawRect(barX - 1, barY - 1, barWidth + 2, barHeight + 2, Color::White);

        // Health text - bright white on dark background
        Text::drawLarge(canvas, "HP " + std::to_string(player.health),
                        barX, barY + barHeight + 2, Color::White);

        // Ammo (right side) - with dark background
        int ammoX = screenWidth - 110;
        canvas.fillRect(ammoX - 5, barY - 2, 115, 25, Color(0, 0, 0));
        Text::drawLarge(canvas, "AMMO " + std::to_string(player.currentClip) + "/" + std::to_string(player.ammo),
                        ammoX, barY, Color(255, 255, 0)); // Bright yellow

        // Reload indicator
        if (player.isReloading)
        {
            float progress = 1.0f - (player.reloadTimer / player.reloadTime);
            canvas.fillRect(ammoX, barY + 12, static_cast<int>(80 * progress), 6, Color(0, 255, 255));
        }

        // Crosshair (center) - BIGGER and BRIGHTER
        int cx = screenWidth / 2;
        int cy = screenHeight / 2;
        // Outer white cross
        canvas.drawLine(cx - 15, cy, cx - 4, cy, Color::White);
        canvas.drawLine(cx + 4, cy, cx + 15, cy, Color::White);
        canvas.drawLine(cx, cy - 15, cx, cy - 4, Color::White);
        canvas.drawLine(cx, cy + 4, cx, cy + 15, Color::White);
        // Inner yellow cross
        canvas.drawLine(cx - 12, cy - 1, cx - 4, cy - 1, Color(255, 255, 0));
        canvas.drawLine(cx + 4, cy - 1, cx + 12, cy - 1, Color(255, 255, 0));
        canvas.drawLine(cx - 1, cy - 12, cx - 1, cy - 4, Color(255, 255, 0));
        canvas.drawLine(cx - 1, cy + 4, cx - 1, cy + 12, Color(255, 255, 0));
        // Center dot - bright red
        for (int dy = -2; dy <= 2; ++dy)
            for (int dx = -2; dx <= 2; ++dx)
                canvas.setPixel(cx + dx, cy + dy, Color(255, 0, 0));

        // Bottom bar with dark background
        canvas.fillRect(0, screenHeight - 22, screenWidth, 22, Color(0, 0, 0));

        // Level and XP - bright cyan
        Text::drawLarge(canvas, "LV" + std::to_string(player.level) + " XP:" + std::to_string(player.xp) + "/" + std::to_string(player.xpToNext),
                        10, screenHeight - 18, Color(0, 255, 255));

        // Kills - bright red
        Text::drawLarge(canvas, "KILLS " + std::to_string(player.kills),
                        screenWidth - 80, screenHeight - 18, Color(255, 100, 100));

        // Message - with dark background
        if (messageTimer > 0)
        {
            int msgY = 28;
            canvas.fillRect(screenWidth / 4, msgY - 2, screenWidth / 2, 12, Color(0, 0, 0));
            Text::drawLargeCentered(canvas, messageText,
                                    screenWidth / 2, msgY, Color(255, 255, 0));
        }

        // Stats overlay
        if (showStats)
        {
            canvas.fillRect(screenWidth / 4, screenHeight / 4,
                            screenWidth / 2, screenHeight / 2, Color(0, 0, 0, 200));

            int sy = screenHeight / 4 + 10;
            int sx = screenWidth / 4 + 10;

            Text::drawLarge(canvas, "=== PLAYER STATS ===", sx, sy, Color::Cyan);

            Text::drawLarge(canvas, "Level: " + std::to_string(player.level),
                            sx, sy + 20, Color::White);

            Text::drawLarge(canvas, "Health: " + std::to_string(player.health) + "/" + std::to_string(player.maxHealth),
                            sx, sy + 35, Color::Green);

            Text::drawLarge(canvas, "Speed: " + std::to_string(static_cast<int>(player.moveSpeed * 10)) + "/10",
                            sx, sy + 50, Color::Yellow);

            Text::drawLarge(canvas, "Kills: " + std::to_string(player.kills),
                            sx, sy + 65, Color::Red);
        }
    }

    void renderMinimap()
    {
        int mapSize = 60;
        int mapX = screenWidth - mapSize - 10;
        int mapY = screenHeight - mapSize - 20;
        int cellSize = mapSize / MAP_WIDTH;

        // Draw map background
        canvas.fillRect(mapX, mapY, mapSize, mapSize, Color(0, 0, 30, 200));

        // Draw map cells
        for (int y = 0; y < MAP_HEIGHT; ++y)
        {
            for (int x = 0; x < MAP_WIDTH; ++x)
            {
                int px = mapX + x * cellSize;
                int py = mapY + y * cellSize;

                if (worldMap[y][x] == 1)
                    canvas.setPixel(px, py, Color(100, 100, 150));
                else if (worldMap[y][x] == 2)
                    canvas.setPixel(px, py, Color(150, 100, 50));
            }
        }

        // Draw player
        int playerMapX = mapX + static_cast<int>(player.x * cellSize);
        int playerMapY = mapY + static_cast<int>(player.y * cellSize);
        canvas.setPixel(playerMapX, playerMapY, Color::Green);

        // Draw direction indicator
        int dirX = playerMapX + static_cast<int>(std::cos(player.angle) * 3);
        int dirY = playerMapY + static_cast<int>(std::sin(player.angle) * 3);
        canvas.drawLine(playerMapX, playerMapY, dirX, dirY, Color::Green);

        // Draw enemies
        for (const auto &e : enemies)
        {
            if (!e.alive)
                continue;
            int ex = mapX + static_cast<int>(e.x * cellSize);
            int ey = mapY + static_cast<int>(e.y * cellSize);
            canvas.setPixel(ex, ey, Color::Red);
        }
    }

    void showMessage(const std::string &msg)
    {
        messageText = msg;
        messageTimer = 3.0f;
    }
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
int main()
{
    try
    {
        FPSGame game;
        game.run();
    }
    catch (const std::exception &e)
    {
        Canvas::cleanupDisplay();
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
