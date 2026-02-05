[⬅ Back to Graphics Index](../index.md) | [⬅ Back to Sprite](../Sprite/sprite.md)

# Audio Module

Sound effects playback using SDL2 backend.

> **Note**: Requires SDL2 and the compile flag `-DPYTHONIC_ENABLE_SDL2_AUDIO -lSDL2`

---

## Quick Start

```cpp
#include <pythonic/TerminalGraphics/Audio/Sound.hpp>
using namespace Pythonic::TG;

SoundBuffer buffer;
if (buffer.loadFromFile("jump.wav"))
{
    Sound sound;
    sound.setBuffer(buffer);
    sound.play();
}
```

---

## Compilation

The audio module uses SDL2 for cross-platform audio playback. Enable it with:

```bash
g++ -std=c++20 -Iinclude \
    -DPYTHONIC_ENABLE_SDL2_AUDIO \
    -o game game.cpp \
    -lSDL2 -pthread
```

Without `-DPYTHONIC_ENABLE_SDL2_AUDIO`, audio calls compile but do nothing.

---

## SoundBuffer

Container for audio sample data. Loads WAV files via SDL2.

### Loading

| Method               | Return | Description   |
| -------------------- | ------ | ------------- |
| `loadFromFile(path)` | `bool` | Load WAV file |

```cpp
SoundBuffer jumpBuffer;
if (!jumpBuffer.loadFromFile("sounds/jump.wav"))
{
    // Handle error
}
```

### Properties

| Method              | Return           | Description                 |
| ------------------- | ---------------- | --------------------------- |
| `getSamples()`      | `const int16_t*` | Raw sample data             |
| `getSampleCount()`  | `size_t`         | Number of samples           |
| `getSampleRate()`   | `unsigned`       | Sample rate (Hz)            |
| `getChannelCount()` | `unsigned`       | Channels (1=mono, 2=stereo) |
| `getDuration()`     | `float`          | Audio duration in seconds   |

```cpp
float duration = buffer.getDuration();
std::cout << "Sound is " << duration << " seconds\n";
```

---

## Sound

Playable sound instance. Multiple sounds can play simultaneously.

### Constructor

| Constructor     | Description           |
| --------------- | --------------------- |
| `Sound()`       | Create without buffer |
| `Sound(buffer)` | Create with buffer    |

### Buffer

| Method              | Description        |
| ------------------- | ------------------ |
| `setBuffer(buffer)` | Set sound buffer   |
| `getBuffer()`       | Get buffer pointer |

```cpp
Sound sound;
sound.setBuffer(jumpBuffer);
```

### Playback

| Method        | Description            |
| ------------- | ---------------------- |
| `play()`      | Start/restart playback |
| `pause()`     | Pause playback         |
| `stop()`      | Stop playback          |
| `getStatus()` | Get current state      |

| Status           | Description       |
| ---------------- | ----------------- |
| `Sound::Stopped` | Not playing       |
| `Sound::Playing` | Currently playing |
| `Sound::Paused`  | Paused            |

```cpp
sound.play();

// Check status
if (sound.getStatus() == Sound::Playing)
{
    // Still playing
}
```

### Volume

| Method              | Description               |
| ------------------- | ------------------------- |
| `setVolume(volume)` | Set volume (0.0 to 100.0) |
| `getVolume()`       | Get current volume        |

```cpp
sound.setVolume(50.0f);  // 50% volume
```

### Loop

| Method          | Description            |
| --------------- | ---------------------- |
| `setLoop(bool)` | Enable/disable looping |
| `getLoop()`     | Check if looping       |

```cpp
Sound bgm(musicBuffer);
bgm.setLoop(true);  // Loop forever
bgm.play();
```

### Pitch

| Method            | Description              |
| ----------------- | ------------------------ |
| `setPitch(pitch)` | Set pitch (1.0 = normal) |
| `getPitch()`      | Get current pitch        |

```cpp
sound.setPitch(0.5f);   // Half speed, lower pitch
sound.setPitch(2.0f);   // Double speed, higher pitch
```

---

## Multiple Sounds

Multiple sounds can play simultaneously via callback-based mixing:

```cpp
SoundBuffer jumpBuf, coinBuf, hitBuf;
jumpBuf.loadFromFile("jump.wav");
coinBuf.loadFromFile("coin.wav");
hitBuf.loadFromFile("hit.wav");

Sound jump(jumpBuf);
Sound coin(coinBuf);
Sound hit(hitBuf);

// All can play at the same time
jump.play();
coin.play();  // Overlaps with jump
// hit.play() later overlaps with both
```

---

## Preloading Sounds

For latency-free playback, load all sounds at startup:

```cpp
// At initialization
struct GameSounds
{
    SoundBuffer jumpBuffer;
    SoundBuffer coinBuffer;
    SoundBuffer hitBuffer;
    SoundBuffer gameOverBuffer;

    Sound jump;
    Sound coin;
    Sound hit;
    Sound gameOver;

    bool load()
    {
        if (!jumpBuffer.loadFromFile("jump.wav")) return false;
        if (!coinBuffer.loadFromFile("coin.wav")) return false;
        if (!hitBuffer.loadFromFile("hit.wav")) return false;
        if (!gameOverBuffer.loadFromFile("gameover.wav")) return false;

        jump.setBuffer(jumpBuffer);
        coin.setBuffer(coinBuffer);
        hit.setBuffer(hitBuffer);
        gameOver.setBuffer(gameOverBuffer);

        return true;
    }
} sounds;

// In game code
sounds.jump.play();  // Instant playback
```

---

## Error Handling

```cpp
SoundBuffer buffer;
if (!buffer.loadFromFile("missing.wav"))
{
    std::cerr << "Failed to load sound\n";
    // Continue without sound, or use fallback
}
```

---

## Complete Example: Game with Sound Effects

```cpp
#include <pythonic/TerminalGraphics/TerminalGraphics.hpp>
using namespace Pythonic::TG;

int main()
{
    Canvas canvas(160, 80, RenderMode::Braille);

    // Load sounds
    SoundBuffer jumpBuf, scoreBuf;
    jumpBuf.loadFromFile("sounds/jump.wav");
    scoreBuf.loadFromFile("sounds/score.wav");

    Sound jumpSound(jumpBuf);
    Sound scoreSound(scoreBuf);
    jumpSound.setVolume(80.0f);

    // Game state
    float playerY = 40;
    float velocityY = 0;
    int score = 0;
    bool wasSpacePressed = false;

    Clock clock;

    while (!Keyboard::isKeyPressed(Key::Escape))
    {
        float dt = clock.restart().asSeconds();

        // Jump on space (one-shot)
        bool spacePressed = Keyboard::isKeyPressed(Key::Space);
        if (spacePressed && !wasSpacePressed)
        {
            velocityY = -200;
            jumpSound.play();
        }
        wasSpacePressed = spacePressed;

        // Physics
        velocityY += 500 * dt;  // Gravity
        playerY += velocityY * dt;

        // Bounds
        if (playerY > 70)
        {
            playerY = 70;
            velocityY = 0;
        }

        // Score (example trigger)
        static float scoreTimer = 0;
        scoreTimer += dt;
        if (scoreTimer >= 2.0f)
        {
            score++;
            scoreSound.play();
            scoreTimer = 0;
        }

        // Draw
        canvas.clear();
        canvas.fillCircle(80, playerY, 5, Color::Yellow);
        Text::draw(canvas, 5, 2, "Score: " + std::to_string(score));
        canvas.display();

        sleep(Time::milliseconds(16));
    }

    return 0;
}
```

---

## Technical Notes

### SDL2 Backend Details

- Uses callback-based audio mixing for low latency
- All playing sounds are mixed in real-time in `audioCallback()`
- Audio device is initialized on first `Sound::play()`
- Cleanup is automatic via `atexit()` handler
- Thread-safe: uses mutex for sound list access

### Supported Formats

- WAV (via SDL2_LoadWAV)
- Other formats require SDL2_mixer extension

---

[⬅ Back to Sprite](../Sprite/sprite.md) | [Next: Text Module →](../Text/text.md)
