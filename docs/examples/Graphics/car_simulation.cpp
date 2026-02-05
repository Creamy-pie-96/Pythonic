/**
 * @file car_simulation.cpp
 * @brief Advanced Car Racing Game - Terminal Graphics Engine Demo
 *
 * A proper racing game with:
 * - Full-screen rendering using terminal dimensions
 * - Smooth world scrolling (player stays centered, world moves)
 * - 3 proper camera views:
 *   1. Top-Down: Classic overhead racing view
 *   2. Third-Person: Chase camera behind the car with depth
 *   3. First-Person: Cockpit view with wide road perspective
 * - AI traffic with collision detection
 * - Speedometer and minimap HUD
 *
 * Controls:
 *   W/Up     - Accelerate
 *   S/Down   - Brake/Reverse
 *   A/Left   - Steer left
 *   D/Right  - Steer right
 *   Space    - Handbrake
 *   H        - Horn
 *   C        - Toggle camera view
 *   R        - Restart after crash
 *   Q/Escape - Quit
 *
 * Compile:
 *   g++ -std=c++20 -O2 -Iinclude -o car_sim examples/car_simulation.cpp -pthread
 */

#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

using namespace Pythonic::TG;

// ============================================================================
// Utility Functions
// ============================================================================

inline float lerpF(float a, float b, float t) { return a + (b - a) * t; }
inline float clampF(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
inline int clampI(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

// ============================================================================
// Color Palette
// ============================================================================

namespace Colors
{
    // Environment - BLACK backgrounds for better braille contrast
    const Color Sky(0, 0, 0); // Pure black sky
    const Color SkyHorizon(20, 30, 50);
    const Color Road(25, 25, 28); // Very dark road
    const Color RoadDark(15, 15, 18);
    const Color RoadLine(255, 255, 255);
    const Color RoadEdge(255, 255, 255);
    const Color Grass1(0, 80, 20); // Dark green grass
    const Color Grass2(0, 60, 15);

    // Player car (red sports car) - BRIGHT colors
    const Color PlayerBody(255, 60, 70);
    const Color PlayerDark(200, 40, 50);
    const Color PlayerLight(255, 100, 110);

    // AI car colors - BRIGHT and saturated
    const Color AIBlue(60, 140, 255);
    const Color AIYellow(255, 220, 60);
    const Color AIGreen(80, 255, 120);
    const Color AIPurple(220, 100, 255);
    const Color AIOrange(255, 160, 70);

    // Common car parts
    const Color Window(20, 30, 50);
    const Color WindowShine(120, 160, 220);
    const Color Tire(10, 10, 12); // Very dark
    const Color Chrome(255, 255, 255);
    const Color Headlight(255, 255, 200);
    const Color Taillight(180, 20, 20);
    const Color TaillightBrake(255, 50, 50);

    // HUD - bright on dark
    const Color HudBg(5, 5, 10, 230);
    const Color HudText(255, 255, 255);
    const Color HudAccent(100, 220, 255);
    const Color SpeedLow(80, 255, 100);
    const Color SpeedMid(255, 220, 60);
    const Color SpeedHigh(255, 60, 60);

    // Effects
    const Color Crash(255, 80, 80);
}

// ============================================================================
// Game Configuration
// ============================================================================

class Config
{
public:
    int screenW, screenH;
    int roadWidth;
    int laneCount = 3;
    int laneWidth;

    void update()
    {
        // Use VideoMode like flappy bird for proper sizing
        VideoMode mode = VideoMode::getDesktopMode();
        screenW = mode.width * 2;        // Braille: 2 pixels per character width
        screenH = (mode.height - 2) * 4; // Braille: 4 pixels per character height, -2 for margin
        roadWidth = screenW * 2 / 5;     // 40% of screen width
        laneWidth = roadWidth / laneCount;
    }

    int roadLeft() const { return (screenW - roadWidth) / 2; }
    int roadRight() const { return roadLeft() + roadWidth; }
    float laneCenter(int lane) const
    {
        return roadLeft() + laneWidth * 0.5f + lane * laneWidth;
    }
};

// ============================================================================
// Camera System
// ============================================================================

enum class CamMode
{
    TopDown,
    ThirdPerson,
    FirstPerson
};

const char *camModeName(CamMode m)
{
    switch (m)
    {
    case CamMode::TopDown:
        return "TOP-DOWN";
    case CamMode::ThirdPerson:
        return "CHASE CAM";
    case CamMode::FirstPerson:
        return "COCKPIT";
    }
    return "";
}

// ============================================================================
// Car
// ============================================================================

struct Car
{
    float x;      // Lateral position on road
    float worldY; // Position in world space (for scrolling)
    float speed = 0;
    float maxSpeed = 300;
    Color color;
    Color colorDark;
    bool isPlayer = false;
    bool braking = false;
    bool crashed = false;

    // AI
    int currentLane = 1;
    int targetLane = 1;
    float laneChangeTimer = 0;
    float indicatorTimer = 0; // Shows indicator for 1.5s before lane change
    bool indicatorLeft = false;
    bool indicatorRight = false;
    float indicatorBlink = 0; // For blinking effect

    static constexpr float W = 16, H = 26; // Collision box

    Car(float x_, float y_, Color c, bool player = false)
        : x(x_), worldY(y_), color(c), isPlayer(player)
    {
        colorDark = Color(c.r * 0.7f, c.g * 0.7f, c.b * 0.7f);
    }

    void update(float dt, bool accel, bool brake, float steer, bool handbrake)
    {
        if (crashed)
            return;

        braking = brake || handbrake;

        if (accel)
            speed += 180 * dt;
        if (brake)
        {
            // Brake: reduce speed, and if already slow/stopped, reverse
            if (speed > 10)
                speed -= 280 * dt; // Strong braking when moving forward
            else if (speed > -50)
                speed -= 100 * dt; // Gentle reverse acceleration
        }

        // Friction only when not accelerating/braking
        if (!accel && !brake)
        {
            float friction = handbrake ? 600 : 50;
            if (speed > 0)
                speed = std::max(0.0f, speed - friction * dt);
            else if (speed < 0)
                speed = std::min(0.0f, speed + friction * dt);
        }

        // Handbrake stops completely
        if (handbrake && std::abs(speed) < 100)
        {
            speed *= 0.9f; // Rapid slowdown
        }

        speed = clampF(speed, -maxSpeed * 0.3f, maxSpeed);

        // Steering affects lateral position (reduced when reversing)
        float steerPower = 100 * (1.0f - 0.3f * std::abs(speed) / maxSpeed);
        if (speed < 0)
            steer = -steer; // Invert steering when reversing
        x += steer * steerPower * dt;

        worldY += speed * dt;

        // Update indicator blink
        indicatorBlink += dt * 6.0f; // Blink ~3 times per second
    }

    bool indicatorOn() const
    {
        return std::sin(indicatorBlink) > 0;
    }

    bool collides(const Car &other) const
    {
        float dx = std::abs(x - other.x);
        float dy = std::abs(worldY - other.worldY);
        return dx < W && dy < H;
    }
};

// ============================================================================
// Drawing: Top-Down View
// ============================================================================

void drawCarTopDown(Canvas &canvas, int cx, int cy, const Car &car)
{
    Color body = car.crashed ? Colors::Crash : car.color;
    Color dark = car.crashed ? Color(150, 50, 50) : car.colorDark;
    Color light = Color(
        std::min(255, body.r + 30),
        std::min(255, body.g + 30),
        std::min(255, body.b + 30));

    // Shadow
    canvas.fillRect(cx - 6 + 3, cy - 11 + 3, 13, 22, Color(0, 0, 0, 40));

    // Main body
    canvas.fillRect(cx - 6, cy - 11, 13, 22, body);

    // Body contour
    canvas.drawRect(cx - 6, cy - 11, 13, 22, dark);

    // Hood highlight
    canvas.fillRect(cx - 1, cy - 10, 3, 8, light);

    // Front windshield
    canvas.fillRect(cx - 4, cy - 8, 9, 3, Colors::Window);
    canvas.drawLine(cx - 3, cy - 8, cx + 2, cy - 8, Colors::WindowShine);

    // Rear windshield
    canvas.fillRect(cx - 3, cy + 4, 7, 3, Colors::Window);

    // Headlights
    canvas.fillRect(cx - 5, cy - 12, 3, 2, Colors::Headlight);
    canvas.fillRect(cx + 3, cy - 12, 3, 2, Colors::Headlight);

    // Taillights
    Color tail = car.braking ? Colors::TaillightBrake : Colors::Taillight;
    canvas.fillRect(cx - 5, cy + 9, 3, 2, tail);
    canvas.fillRect(cx + 3, cy + 9, 3, 2, tail);

    // Turn indicators (orange/amber, blinking)
    Color indicatorColor = Color(255, 180, 0); // Amber
    if (car.indicatorLeft && car.indicatorOn())
    {
        canvas.fillRect(cx - 8, cy - 10, 2, 3, indicatorColor); // Front left
        canvas.fillRect(cx - 8, cy + 7, 2, 3, indicatorColor);  // Rear left
    }
    if (car.indicatorRight && car.indicatorOn())
    {
        canvas.fillRect(cx + 7, cy - 10, 2, 3, indicatorColor); // Front right
        canvas.fillRect(cx + 7, cy + 7, 2, 3, indicatorColor);  // Rear right
    }

    // Wheels
    canvas.fillRect(cx - 8, cy - 7, 3, 5, Colors::Tire);
    canvas.fillRect(cx + 6, cy - 7, 3, 5, Colors::Tire);
    canvas.fillRect(cx - 8, cy + 3, 3, 5, Colors::Tire);
    canvas.fillRect(cx + 6, cy + 3, 3, 5, Colors::Tire);

    // Wheel rims
    canvas.setPixel(cx - 7, cy - 5, Colors::Chrome);
    canvas.setPixel(cx + 7, cy - 5, Colors::Chrome);
    canvas.setPixel(cx - 7, cy + 5, Colors::Chrome);
    canvas.setPixel(cx + 7, cy + 5, Colors::Chrome);
}

// ============================================================================
// Drawing: Third-Person Chase View
// ============================================================================

void drawCarChase(Canvas &canvas, int cx, int cy, const Car &car, float scale)
{
    // Car seen from behind at an angle
    Color body = car.crashed ? Colors::Crash : car.color;
    Color dark = car.crashed ? Color(150, 50, 50) : car.colorDark;

    int w = static_cast<int>(20 * scale);
    int h = static_cast<int>(12 * scale);
    int roofH = static_cast<int>(8 * scale);
    int roofW = static_cast<int>(14 * scale);

    // Shadow
    canvas.fillRect(cx - w / 2 + 4, cy + 4, w, h, Color(0, 0, 0, 40));

    // Lower body (rear)
    canvas.fillRect(cx - w / 2, cy, w, h, body);
    canvas.drawRect(cx - w / 2, cy, w, h, dark);

    // Roof (smaller, above)
    int roofY = cy - roofH + 2;
    canvas.fillRect(cx - roofW / 2, roofY, roofW, roofH, body);
    canvas.drawRect(cx - roofW / 2, roofY, roofW, roofH, dark);

    // Rear window
    int winW = static_cast<int>(10 * scale);
    int winH = static_cast<int>(5 * scale);
    canvas.fillRect(cx - winW / 2, roofY + 1, winW, winH, Colors::Window);

    // Taillights
    Color tail = car.braking ? Colors::TaillightBrake : Colors::Taillight;
    int lightW = static_cast<int>(4 * scale);
    int lightH = static_cast<int>(2 * scale);
    canvas.fillRect(cx - w / 2 + 1, cy + h - lightH - 1, lightW, lightH, tail);
    canvas.fillRect(cx + w / 2 - lightW - 1, cy + h - lightH - 1, lightW, lightH, tail);

    // Turn indicators (orange/amber)
    Color indicatorColor = Color(255, 180, 0);
    int indW = static_cast<int>(3 * scale);
    int indH = static_cast<int>(2 * scale);
    if (car.indicatorLeft && car.indicatorOn())
    {
        canvas.fillRect(cx - w / 2 - 1, cy + h - indH - 3, indW, indH, indicatorColor);
    }
    if (car.indicatorRight && car.indicatorOn())
    {
        canvas.fillRect(cx + w / 2 - indW + 2, cy + h - indH - 3, indW, indH, indicatorColor);
    }

    // Wheels (sides visible)
    int wheelW = static_cast<int>(3 * scale);
    int wheelH = static_cast<int>(6 * scale);
    canvas.fillRect(cx - w / 2 - wheelW + 1, cy + 3, wheelW, wheelH, Colors::Tire);
    canvas.fillRect(cx + w / 2, cy + 3, wheelW, wheelH, Colors::Tire);
}

// ============================================================================
// Drawing: First-Person Cockpit View
// ============================================================================

void drawCockpit(Canvas &canvas, const Config &cfg, float speed, bool braking)
{
    int w = cfg.screenW;
    int h = cfg.screenH;

    // Dashboard at bottom
    int dashH = h / 5;
    int dashY = h - dashH;

    // Hood shape
    for (int y = dashY; y < h; ++y)
    {
        float t = static_cast<float>(y - dashY) / dashH;
        int hoodW = static_cast<int>(w * 0.4f + w * 0.2f * t);
        Color hoodColor(40, 40, 45);
        for (int x = w / 2 - hoodW; x < w / 2 + hoodW; ++x)
        {
            if (x >= 0 && x < w)
                canvas.setPixel(x, y, hoodColor);
        }
    }

    // Dashboard panel
    canvas.fillRect(w / 4, dashY + 5, w / 2, dashH - 10, Color(30, 30, 35));
    canvas.drawRect(w / 4, dashY + 5, w / 2, dashH - 10, Color(60, 60, 70));

    // Steering wheel (simple arc at bottom)
    int wheelCx = w / 2;
    int wheelCy = h - 8;
    int wheelR = 15;
    for (float a = 3.5f; a < 5.9f; a += 0.1f)
    {
        int px = wheelCx + static_cast<int>(std::cos(a) * wheelR);
        int py = wheelCy + static_cast<int>(std::sin(a) * wheelR);
        canvas.fillCircle(px, py, 2, Color(50, 50, 55));
    }

    // Speed display
    std::ostringstream ss;
    ss << static_cast<int>(speed) << " km/h";
    Text::drawCentered(canvas, ss.str(), w / 2, dashY + dashH / 2, Colors::HudText);
}

// ============================================================================
// Drawing: Road and Environment
// ============================================================================

void drawRoadTopDown(Canvas &canvas, const Config &cfg, float scroll)
{
    int left = cfg.roadLeft();
    int right = cfg.roadRight();

    // Grass with stripes - scroll DOWN when moving forward (positive scroll)
    for (int y = 0; y < cfg.screenH; ++y)
    {
        // Negative scroll offset makes stripes move down when scroll increases
        int stripeOffset = static_cast<int>(-scroll * 0.3f) % 16;
        if (stripeOffset < 0)
            stripeOffset += 16;
        Color grass = ((y + stripeOffset) / 8 % 2) ? Colors::Grass1 : Colors::Grass2;
        for (int x = 0; x < left; ++x)
            canvas.setPixel(x, y, grass);
        for (int x = right; x < cfg.screenW; ++x)
            canvas.setPixel(x, y, grass);
    }

    // Road surface
    canvas.fillRect(left, 0, cfg.roadWidth, cfg.screenH, Colors::Road);

    // Road edges (solid white)
    canvas.fillRect(left, 0, 3, cfg.screenH, Colors::RoadEdge);
    canvas.fillRect(right - 3, 0, 3, cfg.screenH, Colors::RoadEdge);

    // Lane markings (animated dashes) - move DOWN when scroll increases
    int dashLen = 20;
    int gapLen = 12;
    int cycle = dashLen + gapLen;
    // Positive scroll should make dashes appear to come from top and move down
    int offset = static_cast<int>(scroll * 0.5f) % cycle;

    for (int lane = 1; lane < cfg.laneCount; ++lane)
    {
        int lx = left + lane * cfg.laneWidth;
        // Start from offset (dashes come from top as scroll increases)
        for (int y = offset - cycle; y < cfg.screenH; y += cycle)
        {
            int dy = std::max(0, y);
            int endY = std::min(y + dashLen, cfg.screenH);
            if (endY > dy)
                canvas.fillRect(lx - 2, dy, 4, endY - dy, Colors::RoadLine);
        }
    }
}

void drawRoadFirstPerson(Canvas &canvas, const Config &cfg, float scroll, float playerX)
{
    int w = cfg.screenW;
    int h = cfg.screenH;

    // Road parameters - gentle perspective
    float roadWidthBottom = w * 0.42f; // Half-width of road at bottom
    float roadWidthTop = w * 0.08f;    // Half-width at horizon
    int horizon = h / 4;               // Horizon line
    int roadBottom = h - h / 5;        // Leave room for cockpit

    // Calculate road center offset based on player position
    // Player at road center = no offset
    // Player at left = road shifts right, and vice versa
    float roadCenter = cfg.roadLeft() + cfg.roadWidth / 2.0f;
    float playerOffset = playerX - roadCenter;

    // Sky gradient
    for (int y = 0; y < horizon; ++y)
    {
        float t = static_cast<float>(y) / horizon;
        Color c(
            lerpF(Colors::Sky.r, Colors::SkyHorizon.r, t),
            lerpF(Colors::Sky.g, Colors::SkyHorizon.g, t),
            lerpF(Colors::Sky.b, Colors::SkyHorizon.b, t));
        for (int x = 0; x < w; ++x)
            canvas.setPixel(x, y, c);
    }

    // Render road with perspective
    for (int y = horizon; y < roadBottom; ++y)
    {
        // t = 0 at horizon, 1 at bottom
        float t = static_cast<float>(y - horizon) / (roadBottom - horizon);

        // Gentler perspective curve (power of 0.8 instead of 0.5)
        float perspT = std::pow(t, 0.75f);

        // Road width interpolates from horizon to bottom
        float projectedRoadW = lerpF(roadWidthTop, roadWidthBottom, perspT);

        // Road center shifts based on player position (more at bottom, less at horizon)
        float lateralShift = -playerOffset * perspT * 0.8f;
        float centerX = w * 0.5f + lateralShift;
        float leftEdge = centerX - projectedRoadW;
        float rightEdge = centerX + projectedRoadW;

        // Grass with scrolling stripes based on depth
        float depthScale = 0.1f + 0.9f * perspT;
        float scrollSpeed = scroll * 0.015f * depthScale;
        int stripePhase = static_cast<int>(-scrollSpeed + y * 0.25f) % 10;
        if (stripePhase < 0)
            stripePhase += 10;
        Color grass = (stripePhase < 5) ? Colors::Grass1 : Colors::Grass2;

        for (int x = 0; x < static_cast<int>(leftEdge); ++x)
            canvas.setPixel(x, y, grass);
        for (int x = static_cast<int>(rightEdge); x < w; ++x)
            canvas.setPixel(x, y, grass);

        // Road surface
        for (int x = static_cast<int>(leftEdge); x < static_cast<int>(rightEdge); ++x)
            canvas.setPixel(x, y, Colors::Road);

        // Road edges (white lines)
        int edgeW = std::max(1, static_cast<int>(1 + 2 * perspT));
        for (int i = 0; i < edgeW; ++i)
        {
            canvas.setPixel(static_cast<int>(leftEdge) + i, y, Colors::RoadEdge);
            canvas.setPixel(static_cast<int>(rightEdge) - 1 - i, y, Colors::RoadEdge);
        }

        // Lane markings (dashed)
        float dashScroll = -scroll * 0.02f * depthScale;
        int dashPattern = static_cast<int>(dashScroll + y * 0.35f) % 14;
        if (dashPattern < 0)
            dashPattern += 14;

        if (dashPattern < 8)
        {
            float roadW = rightEdge - leftEdge;
            for (int lane = 1; lane < cfg.laneCount; ++lane)
            {
                float laneX = leftEdge + roadW * lane / cfg.laneCount;
                int lineW = std::max(1, static_cast<int>(1 + perspT));
                for (int i = 0; i < lineW; ++i)
                    canvas.setPixel(static_cast<int>(laneX) + i, y, Colors::RoadLine);
            }
        }
    }
}

void drawRoadThirdPerson(Canvas &canvas, const Config &cfg, float scroll, float playerX)
{
    int w = cfg.screenW;
    int h = cfg.screenH;

    // Road parameters - gentle perspective
    float roadWidthBottom = w * 0.38f; // Half-width of road at bottom
    float roadWidthTop = w * 0.06f;    // Half-width at horizon
    int horizon = h / 5;               // Horizon line
    int roadBottom = h;                // Road goes to bottom

    // Calculate road center offset based on player position
    float roadCenter = cfg.roadLeft() + cfg.roadWidth / 2.0f;
    float playerOffset = playerX - roadCenter;

    // Sky gradient
    for (int y = 0; y < horizon; ++y)
    {
        float t = static_cast<float>(y) / horizon;
        Color c(
            lerpF(Colors::Sky.r, Colors::SkyHorizon.r, t),
            lerpF(Colors::Sky.g, Colors::SkyHorizon.g, t),
            lerpF(Colors::Sky.b, Colors::SkyHorizon.b, t));
        for (int x = 0; x < w; ++x)
            canvas.setPixel(x, y, c);
    }

    // Render road with perspective
    for (int y = horizon; y < roadBottom; ++y)
    {
        // t = 0 at horizon, 1 at bottom
        float t = static_cast<float>(y - horizon) / (roadBottom - horizon);

        // Gentler perspective curve (power of 0.8)
        float perspT = std::pow(t, 0.75f);

        // Road width interpolates from horizon to bottom
        float projectedRoadW = lerpF(roadWidthTop, roadWidthBottom, perspT);

        // Road center shifts based on player position
        float lateralShift = -playerOffset * perspT * 0.6f;
        float centerX = w * 0.5f + lateralShift;
        float leftEdge = centerX - projectedRoadW;
        float rightEdge = centerX + projectedRoadW;

        // Grass with scrolling stripes based on depth
        float depthScale = 0.1f + 0.9f * perspT;
        float scrollSpeed = scroll * 0.02f * depthScale;
        int stripePhase = static_cast<int>(-scrollSpeed + y * 0.2f) % 12;
        if (stripePhase < 0)
            stripePhase += 12;
        Color grass = (stripePhase < 6) ? Colors::Grass1 : Colors::Grass2;

        for (int x = 0; x < static_cast<int>(leftEdge); ++x)
            canvas.setPixel(x, y, grass);
        for (int x = static_cast<int>(rightEdge); x < w; ++x)
            canvas.setPixel(x, y, grass);

        // Road surface
        for (int x = static_cast<int>(leftEdge); x < static_cast<int>(rightEdge); ++x)
            canvas.setPixel(x, y, Colors::Road);

        // Road edges (white lines)
        int edgeW = std::max(1, static_cast<int>(1 + 2 * perspT));
        for (int i = 0; i < edgeW; ++i)
        {
            canvas.setPixel(static_cast<int>(leftEdge) + i, y, Colors::RoadEdge);
            canvas.setPixel(static_cast<int>(rightEdge) - 1 - i, y, Colors::RoadEdge);
        }

        // Lane markings (dashed)
        float dashScroll = -scroll * 0.025f * depthScale;
        int dashPattern = static_cast<int>(dashScroll + y * 0.3f) % 12;
        if (dashPattern < 0)
            dashPattern += 12;

        if (dashPattern < 7)
        {
            float roadW = rightEdge - leftEdge;
            for (int lane = 1; lane < cfg.laneCount; ++lane)
            {
                float laneX = leftEdge + roadW * lane / cfg.laneCount;
                int lineW = std::max(1, static_cast<int>(1 + perspT));
                for (int i = 0; i < lineW; ++i)
                    canvas.setPixel(static_cast<int>(laneX) + i, y, Colors::RoadLine);
            }
        }
    }
}

// ============================================================================
// HUD Elements
// ============================================================================

void drawSpeedometer(Canvas &canvas, float speed, int x, int y)
{
    int r = 18;

    // Background
    canvas.fillCircle(x, y, r, Colors::HudBg);
    canvas.drawCircle(x, y, r, Colors::HudAccent);

    // Speed arc
    float ratio = clampF(std::abs(speed) / 300.0f, 0.0f, 1.0f);
    for (float a = 0; a < 4.2f * ratio; a += 0.12f)
    {
        float angle = 2.4f + a;
        Color c;
        float t = a / 4.2f;
        if (t < 0.5f)
            c = Color(lerpF(Colors::SpeedLow.r, Colors::SpeedMid.r, t * 2),
                      lerpF(Colors::SpeedLow.g, Colors::SpeedMid.g, t * 2),
                      lerpF(Colors::SpeedLow.b, Colors::SpeedMid.b, t * 2));
        else
            c = Color(lerpF(Colors::SpeedMid.r, Colors::SpeedHigh.r, (t - 0.5f) * 2),
                      lerpF(Colors::SpeedMid.g, Colors::SpeedHigh.g, (t - 0.5f) * 2),
                      lerpF(Colors::SpeedMid.b, Colors::SpeedHigh.b, (t - 0.5f) * 2));

        for (int dr = r - 4; dr <= r - 2; ++dr)
        {
            int px = x + static_cast<int>(std::cos(angle) * dr);
            int py = y + static_cast<int>(std::sin(angle) * dr);
            canvas.setPixel(px, py, c);
        }
    }

    // Needle
    float needleAngle = 2.4f + 4.2f * ratio;
    for (int dr = 4; dr <= r - 5; ++dr)
    {
        int px = x + static_cast<int>(std::cos(needleAngle) * dr);
        int py = y + static_cast<int>(std::sin(needleAngle) * dr);
        canvas.setPixel(px, py, Colors::SpeedHigh);
    }

    canvas.fillCircle(x, y, 2, Colors::Chrome);
}

void drawMinimap(Canvas &canvas, const Config &cfg, const Car &player,
                 const std::vector<Car> &traffic, int mx, int my, int mw, int mh)
{
    canvas.fillRect(mx, my, mw, mh, Colors::HudBg);
    canvas.drawRect(mx, my, mw, mh, Colors::HudAccent);

    // Road strip
    canvas.fillRect(mx + mw / 3, my + 2, mw / 3, mh - 4, Colors::Road);

    // Player (bottom center)
    canvas.fillCircle(mx + mw / 2, my + mh - 6, 2, Color::Red);

    // Traffic
    for (const auto &car : traffic)
    {
        float relY = (car.worldY - player.worldY) / 200.0f;
        if (relY > -1 && relY < 1)
        {
            float relX = (car.x - cfg.laneCenter(1)) / cfg.roadWidth;
            int px = mx + mw / 2 + static_cast<int>(relX * mw * 0.8f);
            int py = my + mh / 2 - static_cast<int>(relY * mh * 0.4f);
            px = clampI(px, mx + 2, mx + mw - 3);
            py = clampI(py, my + 2, my + mh - 3);
            canvas.setPixel(px, py, Color::Yellow);
        }
    }
}

void drawHUD(Canvas &canvas, const Config &cfg, const Car &player,
             CamMode cam, float gameTime)
{
    // Top bar - make taller for large text
    canvas.fillRect(0, 0, cfg.screenW, 14, Colors::HudBg);

    // Speed (large text for visibility)
    std::ostringstream speedSS;
    speedSS << static_cast<int>(std::abs(player.speed)) << " KPH";
    Text::drawLarge(canvas, speedSS.str(), 5, 4, Colors::HudText);

    // Camera mode (large text)
    Text::drawLargeCentered(canvas, camModeName(cam), cfg.screenW / 2, 4, Colors::HudAccent);

    // Time (large text)
    int m = static_cast<int>(gameTime) / 60;
    int s = static_cast<int>(gameTime) % 60;
    std::ostringstream timeSS;
    timeSS << std::setfill('0') << std::setw(2) << m << ":"
           << std::setfill('0') << std::setw(2) << s;
    int timeW = Text::widthLarge(timeSS.str());
    Text::drawLarge(canvas, timeSS.str(), cfg.screenW - 5 - timeW, 4, Colors::HudText);
}

void drawCrash(Canvas &canvas, const Config &cfg)
{
    // Darken the screen
    for (int y = 0; y < cfg.screenH; ++y)
        for (int x = 0; x < cfg.screenW; ++x)
        {
            Color c = canvas.getPixel(x, y);
            canvas.setPixel(x, y, Color(c.r / 3, c.g / 3, c.b / 3));
        }

    int centerX = cfg.screenW / 2;
    int centerY = cfg.screenH / 2;

    // Draw a box for the message (larger for large text)
    int boxW = 100;
    int boxH = 40;
    canvas.fillRect(centerX - boxW / 2, centerY - boxH / 2, boxW, boxH, Color(40, 20, 20));
    canvas.drawRect(centerX - boxW / 2, centerY - boxH / 2, boxW, boxH, Colors::Crash);
    canvas.drawRect(centerX - boxW / 2 + 1, centerY - boxH / 2 + 1, boxW - 2, boxH - 2, Colors::Crash);

    // Draw LARGE text with shadow for better visibility
    Text::drawLargeCenteredWithShadow(canvas, "CRASH!",
                                      centerX,
                                      centerY - 10,
                                      Colors::Crash, Color(100, 30, 30));

    Text::drawLargeCenteredWithShadow(canvas, "PRESS R",
                                      centerX,
                                      centerY + 5,
                                      Colors::HudText, Color(30, 30, 30));
}

// ============================================================================
// AI Traffic - Smart behavior with indicators and collision avoidance
// ============================================================================

void updateTraffic(Car &ai, const Car &player, const std::vector<Car> &allTraffic,
                   const Config &cfg, float dt, std::mt19937 &rng)
{
    if (ai.crashed)
        return;

    // Update indicators
    if (ai.indicatorTimer > 0)
    {
        ai.indicatorTimer -= dt;
        if (ai.indicatorTimer <= 0)
        {
            // Timer expired, now actually change lane if safe
            ai.currentLane = ai.targetLane;
            ai.indicatorLeft = false;
            ai.indicatorRight = false;
        }
    }

    // Decision timer for new lane changes
    ai.laneChangeTimer -= dt;
    if (ai.laneChangeTimer <= 0 && ai.indicatorTimer <= 0)
    {
        ai.laneChangeTimer = std::uniform_real_distribution<float>(4, 8)(rng);

        // Decide if we want to change lanes
        if (std::uniform_real_distribution<float>(0, 1)(rng) < 0.3f) // 30% chance
        {
            int newLane = ai.currentLane;
            if (std::uniform_int_distribution<int>(0, 1)(rng) == 0)
                newLane = std::max(0, ai.currentLane - 1);
            else
                newLane = std::min(cfg.laneCount - 1, ai.currentLane + 1);

            if (newLane != ai.currentLane)
            {
                // Check if lane is clear (no other car within 60 units in target lane)
                float targetX = cfg.laneCenter(newLane);
                bool laneClear = true;

                for (const auto &other : allTraffic)
                {
                    if (&other == &ai)
                        continue;
                    float dx = std::abs(other.x - targetX);
                    float dy = std::abs(other.worldY - ai.worldY);
                    if (dx < 25 && dy < 60) // Within lane and too close
                    {
                        laneClear = false;
                        break;
                    }
                }

                if (laneClear)
                {
                    ai.targetLane = newLane;
                    ai.indicatorTimer = 1.5f; // Show indicator for 1.5 seconds
                    ai.indicatorLeft = (newLane < ai.currentLane);
                    ai.indicatorRight = (newLane > ai.currentLane);
                }
            }
        }
    }

    // Move toward target lane center (only when not waiting for indicator)
    float currentTargetX = cfg.laneCenter(ai.indicatorTimer > 0 ? ai.currentLane : ai.targetLane);
    float steer = clampF((currentTargetX - ai.x) * 0.02f, -0.8f, 0.8f);

    // Speed control - match traffic flow, avoid rear-ending others
    float baseSpeed = 50 + ai.currentLane * 20; // Faster lanes on right
    float targetSpeed = baseSpeed;

    // Look ahead for cars to avoid
    for (const auto &other : allTraffic)
    {
        if (&other == &ai)
            continue;
        float ahead = other.worldY - ai.worldY;
        float dx = std::abs(other.x - ai.x);

        // If car is ahead in same lane-ish area
        if (ahead > 0 && ahead < 80 && dx < 20)
        {
            // Slow down to match their speed
            targetSpeed = std::min(targetSpeed, other.speed - 5);
        }
    }

    targetSpeed = std::max(30.0f, targetSpeed);

    bool accel = ai.speed < targetSpeed;
    bool brake = ai.speed > targetSpeed + 15;
    ai.braking = brake;

    ai.update(dt, accel, brake, steer, false);
    ai.x = clampF(ai.x, cfg.roadLeft() + 10.0f, cfg.roadRight() - 10.0f);
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    // Initialize keyboard BEFORE changing terminal mode
    // (important for raw mode to work correctly with alternate screen)
    Keyboard::init();

    // Hide cursor and set up terminal (like flappy bird)
    std::cout << "\033[?25l";   // Hide cursor
    std::cout << "\033[?1049h"; // Alternate screen buffer
    std::cout << "\033[2J";     // Clear screen

    Config cfg;
    cfg.update();

    Canvas canvas(cfg.screenW, cfg.screenH, RenderMode::Braille);

    CamMode camMode = CamMode::TopDown;

    Car player(cfg.laneCenter(1), 0, Colors::PlayerBody, true);

    std::vector<Car> traffic;
    std::vector<Color> trafficColors = {
        Colors::AIBlue, Colors::AIYellow, Colors::AIGreen,
        Colors::AIPurple, Colors::AIOrange};

    auto resetGame = [&]()
    {
        player = Car(cfg.laneCenter(1), 0, Colors::PlayerBody, true);
        traffic.clear();

        std::mt19937 rng(42);
        // Spawn only 4 cars, spread out well ahead of player
        // Each car in different lane, 150 units apart
        for (int i = 0; i < 4; ++i)
        {
            int lane = i % cfg.laneCount;
            float x = cfg.laneCenter(lane);
            // Start cars well ahead: 200, 350, 500, 650 units
            float y = player.worldY + 200 + i * 150;
            traffic.emplace_back(x, y, trafficColors[i % trafficColors.size()]);
            traffic.back().speed = 40 + (i % 3) * 20; // Varying speeds
            traffic.back().currentLane = lane;
            traffic.back().targetLane = lane;
            traffic.back().laneChangeTimer = 3.0f + i * 0.5f; // Stagger decision times
        }
    };

    resetGame();

    std::mt19937 rng(std::random_device{}());
    Clock clock;
    float gameTime = 0;
    float worldScroll = 0;
    bool wasCPressed = false;

    while (true)
    {
        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.05f);

        if (Canvas::wasResized())
        {
            cfg.update();
            canvas = Canvas(cfg.screenW, cfg.screenH, RenderMode::Braille);
        }

        // Input
        if (Keyboard::isKeyPressed(Key::Escape) || Keyboard::isKeyPressed(Key::Q))
            break;

        if (Keyboard::isKeyPressed(Key::R) && player.crashed)
        {
            resetGame();
            gameTime = 0;
            worldScroll = 0;
        }

        bool cPressed = Keyboard::isKeyPressed(Key::C);
        if (cPressed && !wasCPressed)
        {
            if (camMode == CamMode::TopDown)
                camMode = CamMode::ThirdPerson;
            else if (camMode == CamMode::ThirdPerson)
                camMode = CamMode::FirstPerson;
            else
                camMode = CamMode::TopDown;
        }
        wasCPressed = cPressed;

        float steer = 0;
        if (Keyboard::isKeyPressed(Key::A) || Keyboard::isKeyPressed(Key::Left))
            steer = -1;
        if (Keyboard::isKeyPressed(Key::D) || Keyboard::isKeyPressed(Key::Right))
            steer = 1;

        bool accel = Keyboard::isKeyPressed(Key::W) || Keyboard::isKeyPressed(Key::Up);
        bool brake = Keyboard::isKeyPressed(Key::S) || Keyboard::isKeyPressed(Key::Down);
        bool handbrake = Keyboard::isKeyPressed(Key::Space);

        // Update
        if (!player.crashed)
        {
            gameTime += dt;
            player.update(dt, accel, brake, steer, handbrake);
            player.x = clampF(player.x, cfg.roadLeft() + 10.0f, cfg.roadRight() - 10.0f);

            worldScroll += player.speed * dt;

            // Update traffic relative to player
            for (auto &ai : traffic)
            {
                updateTraffic(ai, player, traffic, cfg, dt, rng);

                // Calculate visible range based on camera mode
                // For perspective views, cars AHEAD of player have POSITIVE relY
                // For top-down, cars AHEAD appear at TOP of screen
                float visibleRangeAhead, visibleRangeBehind;

                if (camMode == CamMode::TopDown)
                {
                    // Top-down: player at 60% from bottom, visible ~200 world units total
                    // Cars ahead appear at top, cars behind at bottom
                    float visibleH = cfg.screenH / 0.8f;
                    visibleRangeAhead = visibleH * 0.4f;  // Can see ~80 units ahead
                    visibleRangeBehind = visibleH * 0.6f; // Can see ~120 units behind
                }
                else if (camMode == CamMode::ThirdPerson)
                {
                    // Third person: cars visible from relY 10 to 150 (AHEAD of player)
                    // relY = ai.worldY - player.worldY, so positive = ahead
                    visibleRangeAhead = 180; // We render cars from 10-150 ahead
                    visibleRangeBehind = 20; // Small range behind (not visible anyway)
                }
                else
                { // FirstPerson
                    // First person: cars visible from relY 20 to 120 (AHEAD of player)
                    visibleRangeAhead = 150; // We render cars from 20-120 ahead
                    visibleRangeBehind = 30;
                }

                // World extends 3x the visible range in each direction
                float worldFront = visibleRangeAhead * 3; // Far ahead of player
                float worldBack = visibleRangeBehind * 3; // Behind player

                float dist = ai.worldY - player.worldY; // Positive = ahead of player

                // Check if respawn needed
                bool needsRespawn = false;
                if (dist < -worldBack)
                    needsRespawn = true; // Too far behind player
                else if (dist > worldFront)
                    needsRespawn = true; // Too far ahead of player

                if (needsRespawn)
                {
                    // Respawn at the FAR EDGE of the world, which is OUTSIDE visible range
                    // For perspective views: spawn at far ahead (positive worldY direction)
                    // New Y should be: player.worldY + (somewhere between visibleRangeAhead*2 and worldFront)
                    // This ensures car spawns OUTSIDE visible area but within world
                    float minSpawnDist = visibleRangeAhead * 2; // Safely outside visible range
                    float maxSpawnDist = worldFront - 10;
                    float newY = player.worldY + std::uniform_real_distribution<float>(minSpawnDist, maxSpawnDist)(rng);
                    int newLane = std::uniform_int_distribution<int>(0, cfg.laneCount - 1)(rng);
                    float newX = cfg.laneCenter(newLane);

                    // Check for overlap with other cars
                    bool tooClose = false;
                    for (const auto &other : traffic)
                    {
                        if (&other == &ai)
                            continue;
                        float dx = std::abs(other.x - newX);
                        float dy = std::abs(other.worldY - newY);
                        if (dx < 30 && dy < 50)
                        {
                            tooClose = true;
                            break;
                        }
                    }

                    if (!tooClose)
                    {
                        ai.worldY = newY;
                        ai.x = newX;
                        ai.crashed = false;
                        ai.speed = 40 + std::uniform_real_distribution<float>(0, 40)(rng);
                        ai.currentLane = newLane;
                        ai.targetLane = newLane;
                        ai.indicatorLeft = false;
                        ai.indicatorRight = false;
                        ai.indicatorTimer = 0;
                        ai.laneChangeTimer = 3.0f + std::uniform_real_distribution<float>(0, 2)(rng);
                    }
                }
            }

            // Collision
            for (auto &ai : traffic)
            {
                if (!ai.crashed && player.collides(ai))
                {
                    player.crashed = true;
                    player.speed = 0;
                    break;
                }
            }
        }

        // Render
        canvas.clear(Colors::Sky);

        if (camMode == CamMode::TopDown)
        {
            drawRoadTopDown(canvas, cfg, worldScroll);

            // Draw traffic
            for (const auto &ai : traffic)
            {
                float screenY = cfg.screenH * 0.6f - (ai.worldY - player.worldY) * 0.8f;
                if (screenY > -30 && screenY < cfg.screenH + 30)
                {
                    drawCarTopDown(canvas, static_cast<int>(ai.x), static_cast<int>(screenY), ai);
                }
            }

            // Draw player (centered)
            drawCarTopDown(canvas, static_cast<int>(player.x), static_cast<int>(cfg.screenH * 0.6f), player);
        }
        else if (camMode == CamMode::ThirdPerson)
        {
            drawRoadThirdPerson(canvas, cfg, worldScroll, player.x);

            // Draw traffic in distance
            // relY = distance ahead of player (positive = ahead)
            // Cars close to player (small relY) should appear LARGE at BOTTOM
            // Cars far from player (large relY) should appear SMALL at TOP (horizon)
            for (const auto &ai : traffic)
            {
                float relY = ai.worldY - player.worldY;
                if (relY > 10 && relY < 150)
                {
                    // t: 0 = far (at horizon), 1 = close (at bottom of screen)
                    float t = 1.0f - (relY - 10) / 140.0f; // Normalize 10-150 to 1-0
                    t = clampF(t, 0.0f, 1.0f);

                    // Scale: close (t=1) = large, far (t=0) = small
                    float scale = 0.3f + 0.7f * t;

                    // Screen Y: close (t=1) = bottom, far (t=0) = top (horizon)
                    int horizon = cfg.screenH / 5;
                    int bottom = cfg.screenH - 50;
                    int screenY = horizon + static_cast<int>((bottom - horizon) * t);

                    // X position: perspective narrows toward horizon
                    float relX = ai.x - player.x;
                    int screenX = cfg.screenW / 2 + static_cast<int>(relX * t * 1.5f);

                    drawCarChase(canvas, screenX, screenY, ai, scale);
                }
            }

            // Draw player car larger at bottom
            drawCarChase(canvas, cfg.screenW / 2, cfg.screenH - 35, player, 1.5f);
        }
        else
        { // FirstPerson
            drawRoadFirstPerson(canvas, cfg, worldScroll, player.x);

            // Draw traffic as shapes in distance
            // relY = distance ahead (positive = ahead)
            // Close cars: large, at bottom of road area
            // Far cars: small, near horizon
            for (const auto &ai : traffic)
            {
                float relY = ai.worldY - player.worldY;
                if (relY > 20 && relY < 120)
                {
                    // t: 0 = far, 1 = close
                    float t = 1.0f - (relY - 20) / 100.0f;
                    t = clampF(t, 0.0f, 1.0f);

                    // Screen Y: close at bottom of visible road, far near horizon
                    int horizon = cfg.screenH / 4;
                    int roadBottom = cfg.screenH - cfg.screenH / 5 - 10; // Above cockpit
                    int screenY = horizon + static_cast<int>((roadBottom - horizon) * t);

                    // X: perspective narrows toward horizon
                    float relX = ai.x - player.x;
                    int screenX = cfg.screenW / 2 + static_cast<int>(relX * t * 1.2f);

                    // Size: close = large, far = small
                    int size = 2 + static_cast<int>(t * 10);

                    // Draw with indicator support
                    canvas.fillRect(screenX - size, screenY - size / 2, size * 2, size, ai.color);

                    // Taillights
                    Color tail = ai.braking ? Colors::TaillightBrake : Colors::Taillight;
                    if (size > 3)
                    {
                        canvas.fillRect(screenX - size + 1, screenY + size / 2 - 2, 2, 2, tail);
                        canvas.fillRect(screenX + size - 3, screenY + size / 2 - 2, 2, 2, tail);
                    }

                    // Indicators
                    if (ai.indicatorLeft && ai.indicatorOn())
                        canvas.fillRect(screenX - size - 1, screenY, 2, 2, Color(255, 180, 0));
                    if (ai.indicatorRight && ai.indicatorOn())
                        canvas.fillRect(screenX + size, screenY, 2, 2, Color(255, 180, 0));
                }
            }

            drawCockpit(canvas, cfg, player.speed, player.braking);
        }

        // HUD
        if (camMode == CamMode::TopDown)
        {
            drawSpeedometer(canvas, player.speed, 28, cfg.screenH - 25);
            drawMinimap(canvas, cfg, player, traffic, cfg.screenW - 35, cfg.screenH - 40, 30, 35);
        }
        drawHUD(canvas, cfg, player, camMode, gameTime);

        if (player.crashed)
            drawCrash(canvas, cfg);

        canvas.display();
        sleep(Time::milliseconds(16));
    }

    // Cleanup (like flappy bird)
    Keyboard::shutdown();
    std::cout << "\033[?1049l"; // Exit alternate screen buffer
    std::cout << "\033[?25h";   // Show cursor
    std::cout << "\033[0m";     // Reset colors

    std::cout << "Thanks for playing! Final time: "
              << static_cast<int>(gameTime) / 60 << ":"
              << std::setfill('0') << std::setw(2) << static_cast<int>(gameTime) % 60
              << std::endl;

    return 0;
}
