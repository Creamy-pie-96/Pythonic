/**
 * @file Sound.hpp
 * @brief Short sound effect playback
 *
 * Cross-platform audio with multiple backend support:
 * - SDL2 (when PYTHONIC_ENABLE_SDL2_AUDIO is defined) - fastest, recommended
 * - System commands (fallback) - works everywhere but slower
 *
 * To enable SDL2 audio, compile with:
 *   -DPYTHONIC_ENABLE_SDL2_AUDIO -lSDL2
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Time.hpp"
#include "SoundBuffer.hpp"
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <mutex>
#include <cstring>

// SDL2 audio support (optional, but much faster)
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
#include <SDL2/SDL.h>
#endif

TG_NAMESPACE_BEGIN

//=============================================================================
// SDL2 Audio Backend (fast, low-latency) - Preload-based
//=============================================================================
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
namespace sdl_audio
{

    // Audio data ready to play (pre-converted to device format)
    struct AudioData
    {
        std::vector<Uint8> samples;
        size_t playOffset = 0;
        float volume = 1.0f;
        bool playing = false;
    };

    inline std::atomic<bool> g_sdlInitialized{false};
    inline SDL_AudioDeviceID g_audioDevice{0};
    inline SDL_AudioSpec g_audioSpec;
    inline std::vector<std::shared_ptr<AudioData>> g_playingSounds;
    inline std::mutex g_audioMutex;

    // Audio callback - mixes all playing sounds
    inline void audioCallback(void *userdata, Uint8 *stream, int len)
    {
        (void)userdata;
        std::memset(stream, 0, len);

        std::lock_guard<std::mutex> lock(g_audioMutex);

        for (auto &sound : g_playingSounds)
        {
            if (!sound->playing)
                continue;

            size_t remaining = sound->samples.size() - sound->playOffset;
            size_t toCopy = std::min(remaining, static_cast<size_t>(len));

            // Mix with volume
            const Uint8 *src = sound->samples.data() + sound->playOffset;
            Uint8 volume = static_cast<Uint8>(sound->volume * SDL_MIX_MAXVOLUME);
            SDL_MixAudioFormat(stream, src, g_audioSpec.format, toCopy, volume);

            sound->playOffset += toCopy;
            if (sound->playOffset >= sound->samples.size())
            {
                sound->playing = false;
            }
        }

        // Clean up finished sounds
        g_playingSounds.erase(
            std::remove_if(g_playingSounds.begin(), g_playingSounds.end(),
                           [](const auto &s)
                           { return !s->playing; }),
            g_playingSounds.end());
    }

    inline bool init()
    {
        if (g_sdlInitialized.load())
            return true;

        if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
            return false;
        }

        SDL_AudioSpec want;
        SDL_zero(want);
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 512; // Low latency (about 11ms at 44.1kHz)
        want.callback = audioCallback;
        want.userdata = nullptr;

        g_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &g_audioSpec, 0);
        if (g_audioDevice == 0)
        {
            SDL_Quit();
            return false;
        }

        // Start audio playback
        SDL_PauseAudioDevice(g_audioDevice, 0);

        g_sdlInitialized.store(true);

        std::atexit([]()
                    {
            // Clear playing sounds first to prevent audio callback access
            {
                std::lock_guard<std::mutex> lock(g_audioMutex);
                g_playingSounds.clear();
            }
            if (g_audioDevice != 0) {
                SDL_CloseAudioDevice(g_audioDevice);
                g_audioDevice = 0;
            }
            SDL_Quit(); });
        return true;
    }

    // Play pre-loaded samples directly (instant, no file I/O)
    inline void playSamples(const int16_t *samples, size_t sampleCount,
                            unsigned int channels, unsigned int sampleRate,
                            float volume)
    {
        if (!init() || !samples || sampleCount == 0)
            return;

        // Convert to device format if needed
        SDL_AudioCVT cvt;
        int needsConversion = SDL_BuildAudioCVT(&cvt,
                                                AUDIO_S16SYS, channels, sampleRate,
                                                g_audioSpec.format, g_audioSpec.channels, g_audioSpec.freq);

        auto audioData = std::make_shared<AudioData>();
        audioData->volume = volume;
        audioData->playOffset = 0;
        audioData->playing = true;

        if (needsConversion > 0)
        {
            size_t inputLen = sampleCount * sizeof(int16_t);
            cvt.len = static_cast<int>(inputLen);
            cvt.buf = static_cast<Uint8 *>(SDL_malloc(inputLen * cvt.len_mult));
            if (!cvt.buf)
                return;

            SDL_memcpy(cvt.buf, samples, inputLen);

            if (SDL_ConvertAudio(&cvt) == 0)
            {
                audioData->samples.assign(cvt.buf, cvt.buf + cvt.len_cvt);
            }
            SDL_free(cvt.buf);
        }
        else
        {
            // No conversion needed
            const Uint8 *data = reinterpret_cast<const Uint8 *>(samples);
            audioData->samples.assign(data, data + sampleCount * sizeof(int16_t));
        }

        std::lock_guard<std::mutex> lock(g_audioMutex);
        g_playingSounds.push_back(audioData);
    }

    // Fallback: play from file (slower, used when no sample data available)
    inline void playFile(const std::string &filePath, int volume)
    {
        if (!init())
            return;

        SDL_AudioSpec wavSpec;
        Uint8 *wavBuffer = nullptr;
        Uint32 wavLength = 0;

        if (SDL_LoadWAV(filePath.c_str(), &wavSpec, &wavBuffer, &wavLength) == nullptr)
            return;

        // Convert samples to int16_t* format for playSamples
        // This is a fallback path - prefer using sample data directly
        size_t sampleCount = wavLength / sizeof(int16_t);
        playSamples(reinterpret_cast<const int16_t *>(wavBuffer),
                    sampleCount, wavSpec.channels, wavSpec.freq,
                    volume / 100.0f);

        SDL_FreeWAV(wavBuffer);
    }

} // namespace sdl_audio
#endif

//=============================================================================
// Fallback Audio Backend (system commands)
//=============================================================================
namespace fallback_audio
{

    /**
     * @brief Play audio file using available system methods
     * @param filePath Path to audio file
     * @param volume Volume level 0-100
     */
    inline void playFile(const std::string &filePath, int volume)
    {
        if (filePath.empty())
            return;

        std::string cmd;

// Platform-specific audio playback
#if defined(_WIN32)
        // Windows - use PowerShell Media.SoundPlayer (only supports WAV)
        cmd = "powershell -c \"(New-Object Media.SoundPlayer('" +
              filePath + "')).PlaySync()\" 2>nul &";
#elif defined(__APPLE__)
        // macOS - use afplay with volume
        float vol = volume / 100.0f;
        cmd = "afplay -v " + std::to_string(vol) +
              " \"" + filePath + "\" 2>/dev/null &";
#else
        // Linux - try multiple players in order of preference
        // ffplay is usually fastest and most compatible
        cmd = "(ffplay -nodisp -autoexit -volume " + std::to_string(volume) +
              " -loglevel quiet \"" + filePath + "\" 2>/dev/null || "
                                                 "paplay \"" +
              filePath + "\" 2>/dev/null || "
                         "aplay -q \"" +
              filePath + "\" 2>/dev/null) &";
#endif

        std::system(cmd.c_str());
    }

} // namespace fallback_audio

//=============================================================================
// Unified Audio Playback
//=============================================================================
namespace audio_detail
{

    // Play from pre-loaded sample data (fast path)
    inline void playSamples(const int16_t *samples, size_t sampleCount,
                            unsigned int channels, unsigned int sampleRate,
                            float volume)
    {
#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
        sdl_audio::playSamples(samples, sampleCount, channels, sampleRate, volume);
#else
        // Fallback: can't play samples directly without SDL2
        // Would need to write to temp file - not worth it
        (void)samples;
        (void)sampleCount;
        (void)channels;
        (void)sampleRate;
        (void)volume;
#endif
    }

    // Play from file (slow path, fallback)
    inline void playAudioFile(const std::string &filePath, float volume)
    {
        int vol = static_cast<int>(std::clamp(volume, 0.0f, 100.0f));

#ifdef PYTHONIC_ENABLE_SDL2_AUDIO
        sdl_audio::playFile(filePath, vol);
#else
        fallback_audio::playFile(filePath, vol);
#endif
    }

} // namespace audio_detail

/**
 * @brief Playback status for audio
 */
enum class SoundStatus
{
    Stopped,
    Paused,
    Playing
};

/**
 * @brief Sound effect player
 *
 * Plays short sounds stored in a SoundBuffer.
 *
 * For best performance, compile with SDL2 support:
 * @code
 * g++ -std=c++20 -DPYTHONIC_ENABLE_SDL2_AUDIO -lSDL2 -lSDL2_mixer ...
 * @endcode
 *
 * Without SDL2, falls back to system commands (ffplay, paplay, etc.)
 *
 * @code
 * SoundBuffer buffer;
 * buffer.loadFromFile("click.wav");
 *
 * Sound sound(buffer);
 * sound.setVolume(50);  // 50% volume
 * sound.play();
 * @endcode
 */
class Sound
{
public:
    /**
     * @brief Default constructor
     */
    Sound()
        : m_buffer(nullptr), m_status(SoundStatus::Stopped), m_volume(100), m_pitch(1.0f), m_loop(false), m_playingOffset(0) {}

    /**
     * @brief Create a sound from a buffer
     */
    explicit Sound(const SoundBuffer &buffer)
        : m_buffer(&buffer), m_status(SoundStatus::Stopped), m_volume(100), m_pitch(1.0f), m_loop(false), m_playingOffset(0) {}

    /**
     * @brief Copy constructor
     */
    Sound(const Sound &copy)
        : m_buffer(copy.m_buffer), m_status(SoundStatus::Stopped), m_volume(copy.m_volume), m_pitch(copy.m_pitch), m_loop(copy.m_loop), m_playingOffset(0) {}

    /**
     * @brief Destructor
     */
    ~Sound()
    {
        stop();
    }

    /**
     * @brief Play the sound with optional volume
     * @param volume Volume level (0.0 to 1.0, default 1.0 = 100%)
     */
    void play(float volume = 1.0f)
    {
        if (!m_buffer)
            return;

        // Apply volume if specified
        if (volume >= 0.0f && volume <= 1.0f)
        {
            m_volume = volume * 100.0f;
        }

        stop();
        m_status = SoundStatus::Playing;

        // Use non-blocking fork for faster playback
        playAsync();
    }

    /**
     * @brief Pause the sound
     */
    void pause()
    {
        if (m_status == SoundStatus::Playing)
            m_status = SoundStatus::Paused;
    }

    /**
     * @brief Stop the sound
     */
    void stop()
    {
        m_status = SoundStatus::Stopped;
        m_playingOffset = 0;
        // Note: SDL2 audio handles playback in its own callback thread
        // so no thread management is needed here
    }

    /**
     * @brief Set the sound buffer
     */
    void setBuffer(const SoundBuffer &buffer)
    {
        stop();
        m_buffer = &buffer;
    }

    /**
     * @brief Get the sound buffer
     */
    const SoundBuffer *getBuffer() const
    {
        return m_buffer;
    }

    /**
     * @brief Set looping
     */
    void setLoop(bool loop)
    {
        m_loop = loop;
    }

    /**
     * @brief Check if looping
     */
    bool getLoop() const
    {
        return m_loop;
    }

    /**
     * @brief Set the playback position
     */
    void setPlayingOffset(Time offset)
    {
        if (m_buffer)
        {
            size_t sample = static_cast<size_t>(
                offset.asSeconds() * m_buffer->getSampleRate() *
                m_buffer->getChannelCount());
            m_playingOffset = std::min(sample, m_buffer->getSampleCount());
        }
    }

    /**
     * @brief Get the current playback position
     */
    Time getPlayingOffset() const
    {
        if (!m_buffer || m_buffer->getSampleRate() == 0)
            return Time::Zero;

        return Time::seconds(
            static_cast<float>(m_playingOffset) /
            (m_buffer->getSampleRate() * m_buffer->getChannelCount()));
    }

    /**
     * @brief Get the playback status
     */
    SoundStatus getStatus() const
    {
        return m_status;
    }

    /**
     * @brief Set volume (0-100)
     */
    void setVolume(float volume)
    {
        m_volume = std::max(0.0f, std::min(100.0f, volume));
    }

    /**
     * @brief Get volume
     */
    float getVolume() const
    {
        return m_volume;
    }

    /**
     * @brief Set pitch (playback speed multiplier)
     */
    void setPitch(float pitch)
    {
        m_pitch = std::max(0.1f, pitch);
    }

    /**
     * @brief Get pitch
     */
    float getPitch() const
    {
        return m_pitch;
    }

private:
    const SoundBuffer *m_buffer;
    std::atomic<SoundStatus> m_status;
    float m_volume;
    float m_pitch;
    bool m_loop;
    std::atomic<size_t> m_playingOffset;

    /**
     * @brief Play audio asynchronously
     */
    void playAsync()
    {
        if (!m_buffer)
        {
            m_status = SoundStatus::Stopped;
            return;
        }

        // Fast path: use pre-loaded sample data (instant playback)
        const int16_t *samples = m_buffer->getSamples();
        size_t sampleCount = m_buffer->getSampleCount();

        if (samples && sampleCount > 0)
        {
            audio_detail::playSamples(samples, sampleCount,
                                      m_buffer->getChannelCount(),
                                      m_buffer->getSampleRate(),
                                      m_volume / 100.0f);
            m_status = SoundStatus::Stopped;
            return;
        }

        // Slow path: fallback to file-based playback
        const std::string &filePath = m_buffer->getFilePath();
        if (!filePath.empty())
        {
            audio_detail::playAudioFile(filePath, m_volume);
        }
        m_status = SoundStatus::Stopped;
    }

    void playInternal()
    {
        playAsync();
    }
};

TG_NAMESPACE_END
