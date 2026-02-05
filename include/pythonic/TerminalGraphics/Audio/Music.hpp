/**
 * @file Music.hpp
 * @brief Streaming music playback
 */

#pragma once

#include "../Config.hpp"
#include "../Core/Time.hpp"
#include "SoundBuffer.hpp"
#include <string>
#include <atomic>
#include <thread>
#include <fstream>

TG_NAMESPACE_BEGIN

/**
 * @brief Streaming music player
 *
 * Unlike Sound which loads the entire audio into memory, Music
 * streams audio from a file, making it suitable for longer tracks.
 *
 * Note: Terminal graphics has limited audio support. For full audio
 * functionality, consider integrating with a proper audio library
 * like SDL_mixer, OpenAL, or miniaudio.
 *
 * @code
 * Music music;
 * music.openFromFile("background.wav");
 * music.setLoop(true);
 * music.setVolume(50);
 * music.play();
 *
 * // Later...
 * music.stop();
 * @endcode
 */
class Music
{
public:
    /**
     * @brief Default constructor
     */
    Music()
        : m_status(SoundStatus::Stopped), m_volume(100), m_pitch(1.0f), m_loop(false), m_sampleRate(44100), m_channelCount(2), m_duration(Time::Zero) {}

    /**
     * @brief Destructor
     */
    ~Music()
    {
        stop();
    }

    // Non-copyable
    Music(const Music &) = delete;
    Music &operator=(const Music &) = delete;

    /**
     * @brief Open a music file for streaming
     * @param filename Path to audio file (WAV format)
     * @return true if successful
     */
    bool openFromFile(const std::string &filename)
    {
        stop();

        m_filename = filename;

        // Read header to get format info
        std::ifstream file(filename, std::ios::binary);
        if (!file)
            return false;

        // Parse WAV header
        char riff[4];
        file.read(riff, 4);
        if (std::strncmp(riff, "RIFF", 4) != 0)
            return false;

        file.seekg(8);
        char wave[4];
        file.read(wave, 4);
        if (std::strncmp(wave, "WAVE", 4) != 0)
            return false;

        // Find fmt chunk
        while (file.good())
        {
            char chunkId[4];
            file.read(chunkId, 4);

            uint32_t chunkSize;
            file.read(reinterpret_cast<char *>(&chunkSize), 4);

            if (std::strncmp(chunkId, "fmt ", 4) == 0)
            {
                uint16_t audioFormat;
                file.read(reinterpret_cast<char *>(&audioFormat), 2);

                uint16_t channels;
                file.read(reinterpret_cast<char *>(&channels), 2);
                m_channelCount = channels;

                uint32_t sampleRate;
                file.read(reinterpret_cast<char *>(&sampleRate), 4);
                m_sampleRate = sampleRate;

                // Skip rest of fmt chunk
                file.seekg(chunkSize - 8, std::ios::cur);
            }
            else if (std::strncmp(chunkId, "data", 4) == 0)
            {
                m_dataOffset = file.tellg();
                m_dataSize = chunkSize;

                // Calculate duration
                size_t samples = chunkSize / 2; // 16-bit samples
                m_duration = Time::seconds(
                    static_cast<float>(samples) / (m_sampleRate * m_channelCount));

                return true;
            }
            else
            {
                file.seekg(chunkSize, std::ios::cur);
            }
        }

        return false;
    }

    /**
     * @brief Play the music
     */
    void play()
    {
        if (m_filename.empty())
            return;

        if (m_status == SoundStatus::Paused)
        {
            m_status = SoundStatus::Playing;
            return;
        }

        stop();
        m_status = SoundStatus::Playing;
        m_playingOffset = Time::Zero;

        m_playThread = std::thread([this]()
                                   { playInternal(); });
    }

    /**
     * @brief Pause the music
     */
    void pause()
    {
        if (m_status == SoundStatus::Playing)
            m_status = SoundStatus::Paused;
    }

    /**
     * @brief Stop the music
     */
    void stop()
    {
        m_status = SoundStatus::Stopped;
        if (m_playThread.joinable())
            m_playThread.join();
        m_playingOffset = Time::Zero;
    }

    /**
     * @brief Get the playback status
     */
    SoundStatus getStatus() const
    {
        return m_status;
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
     * @brief Set pitch
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

    /**
     * @brief Get the total duration
     */
    Time getDuration() const
    {
        return m_duration;
    }

    /**
     * @brief Get the current playback position
     */
    Time getPlayingOffset() const
    {
        return m_playingOffset;
    }

    /**
     * @brief Set the playback position
     */
    void setPlayingOffset(Time offset)
    {
        m_playingOffset = offset;
    }

    /**
     * @brief Get the sample rate
     */
    unsigned int getSampleRate() const
    {
        return m_sampleRate;
    }

    /**
     * @brief Get the number of channels
     */
    unsigned int getChannelCount() const
    {
        return m_channelCount;
    }

private:
    std::string m_filename;
    std::atomic<SoundStatus> m_status;
    float m_volume;
    float m_pitch;
    bool m_loop;
    unsigned int m_sampleRate;
    unsigned int m_channelCount;
    Time m_duration;
    std::atomic<Time> m_playingOffset;
    size_t m_dataOffset = 0;
    size_t m_dataSize = 0;
    std::thread m_playThread;

    void playInternal()
    {
        std::ifstream file(m_filename, std::ios::binary);
        if (!file)
        {
            m_status = SoundStatus::Stopped;
            return;
        }

        // Seek to data
        file.seekg(m_dataOffset);

        // Calculate sample position from offset
        size_t sampleOffset = static_cast<size_t>(
            m_playingOffset.load().asSeconds() * m_sampleRate * m_channelCount);
        file.seekg(m_dataOffset + sampleOffset * 2);

        float sampleRate = m_sampleRate * m_pitch;

        while (m_status != SoundStatus::Stopped)
        {
            if (m_status == SoundStatus::Paused)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Check if we've reached the end
            size_t currentPos = static_cast<size_t>(file.tellg()) - m_dataOffset;
            if (currentPos >= m_dataSize)
            {
                if (m_loop)
                {
                    file.seekg(m_dataOffset);
                    m_playingOffset = Time::Zero;
                }
                else
                {
                    m_status = SoundStatus::Stopped;
                    break;
                }
            }

            // Read and process a chunk of samples
            // In a real implementation, we would send these to an audio device
            constexpr size_t CHUNK_SIZE = 4096;
            int16_t buffer[CHUNK_SIZE];
            file.read(reinterpret_cast<char *>(buffer), CHUNK_SIZE * 2);
            size_t samplesRead = file.gcount() / 2;

            // Update playing offset
            m_playingOffset = Time::seconds(
                static_cast<float>(currentPos) / 2 / (m_sampleRate * m_channelCount));

            // Sleep to simulate playback timing
            float chunkDuration = static_cast<float>(samplesRead) /
                                  (sampleRate * m_channelCount);
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<long>(chunkDuration * 1000)));
        }
    }
};

TG_NAMESPACE_END
