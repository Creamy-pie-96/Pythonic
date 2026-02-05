/**
 * @file SoundBuffer.hpp
 * @brief Audio buffer for storing sound samples
 */

#pragma once

#include "../Config.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdint>

TG_NAMESPACE_BEGIN

/**
 * @brief Storage for audio samples
 *
 * A SoundBuffer stores audio sample data that can be played using Sound.
 * Supports loading from WAV files.
 *
 * @code
 * SoundBuffer buffer;
 * buffer.loadFromFile("explosion.wav");
 *
 * Sound sound(buffer);
 * sound.play();
 * @endcode
 */
class SoundBuffer
{
public:
    /**
     * @brief Default constructor
     */
    SoundBuffer()
        : m_sampleRate(44100), m_channelCount(1), m_bitsPerSample(16) {}

    /**
     * @brief Load audio from a WAV file
     * @param filename Path to WAV file
     * @return true if successful
     */
    bool loadFromFile(const std::string &filename)
    {
        m_filePath = filename; // Store for system playback

        std::ifstream file(filename, std::ios::binary);
        if (!file)
            return false;

        // Read WAV header
        char riff[4];
        file.read(riff, 4);
        if (std::strncmp(riff, "RIFF", 4) != 0)
            return false;

        file.seekg(8); // Skip file size
        char wave[4];
        file.read(wave, 4);
        if (std::strncmp(wave, "WAVE", 4) != 0)
            return false;

        // Read format chunk
        char fmt[4];
        file.read(fmt, 4);
        if (std::strncmp(fmt, "fmt ", 4) != 0)
            return false;

        uint32_t fmtSize;
        file.read(reinterpret_cast<char *>(&fmtSize), 4);

        uint16_t audioFormat;
        file.read(reinterpret_cast<char *>(&audioFormat), 2);
        if (audioFormat != 1) // PCM only
            return false;

        uint16_t channels;
        file.read(reinterpret_cast<char *>(&channels), 2);
        m_channelCount = channels;

        uint32_t sampleRate;
        file.read(reinterpret_cast<char *>(&sampleRate), 4);
        m_sampleRate = sampleRate;

        file.seekg(4, std::ios::cur); // Skip byte rate
        file.seekg(2, std::ios::cur); // Skip block align

        uint16_t bitsPerSample;
        file.read(reinterpret_cast<char *>(&bitsPerSample), 2);
        m_bitsPerSample = bitsPerSample;

        // Skip any extra format bytes
        if (fmtSize > 16)
            file.seekg(fmtSize - 16, std::ios::cur);

        // Find data chunk
        while (file.good())
        {
            char chunkId[4];
            file.read(chunkId, 4);

            uint32_t chunkSize;
            file.read(reinterpret_cast<char *>(&chunkSize), 4);

            if (std::strncmp(chunkId, "data", 4) == 0)
            {
                // Read sample data
                size_t sampleCount = chunkSize / (m_bitsPerSample / 8);
                m_samples.resize(sampleCount);

                if (m_bitsPerSample == 16)
                {
                    file.read(reinterpret_cast<char *>(m_samples.data()), chunkSize);
                }
                else if (m_bitsPerSample == 8)
                {
                    std::vector<uint8_t> bytes(sampleCount);
                    file.read(reinterpret_cast<char *>(bytes.data()), chunkSize);
                    for (size_t i = 0; i < sampleCount; ++i)
                    {
                        // Convert 8-bit unsigned to 16-bit signed
                        m_samples[i] = (static_cast<int16_t>(bytes[i]) - 128) * 256;
                    }
                }

                return true;
            }
            else
            {
                // Skip this chunk
                file.seekg(chunkSize, std::ios::cur);
            }
        }

        return false;
    }

    /**
     * @brief Load audio from raw samples
     * @param samples Pointer to sample data
     * @param sampleCount Number of samples
     * @param channelCount Number of audio channels
     * @param sampleRate Samples per second
     */
    bool loadFromSamples(const int16_t *samples, size_t sampleCount,
                         unsigned int channelCount, unsigned int sampleRate)
    {
        m_samples.assign(samples, samples + sampleCount);
        m_channelCount = channelCount;
        m_sampleRate = sampleRate;
        m_bitsPerSample = 16;
        return true;
    }

    /**
     * @brief Save audio to a WAV file
     */
    bool saveToFile(const std::string &filename) const
    {
        std::ofstream file(filename, std::ios::binary);
        if (!file)
            return false;

        uint32_t dataSize = static_cast<uint32_t>(m_samples.size() * 2);
        uint32_t fileSize = 36 + dataSize;
        uint32_t byteRate = m_sampleRate * m_channelCount * 2;
        uint16_t blockAlign = m_channelCount * 2;

        // RIFF header
        file.write("RIFF", 4);
        file.write(reinterpret_cast<const char *>(&fileSize), 4);
        file.write("WAVE", 4);

        // fmt chunk
        file.write("fmt ", 4);
        uint32_t fmtSize = 16;
        file.write(reinterpret_cast<const char *>(&fmtSize), 4);
        uint16_t audioFormat = 1;
        file.write(reinterpret_cast<const char *>(&audioFormat), 2);
        uint16_t channels = static_cast<uint16_t>(m_channelCount);
        file.write(reinterpret_cast<const char *>(&channels), 2);
        file.write(reinterpret_cast<const char *>(&m_sampleRate), 4);
        file.write(reinterpret_cast<const char *>(&byteRate), 4);
        file.write(reinterpret_cast<const char *>(&blockAlign), 2);
        uint16_t bps = 16;
        file.write(reinterpret_cast<const char *>(&bps), 2);

        // data chunk
        file.write("data", 4);
        file.write(reinterpret_cast<const char *>(&dataSize), 4);
        file.write(reinterpret_cast<const char *>(m_samples.data()), dataSize);

        return true;
    }

    /**
     * @brief Get the sample data
     */
    const int16_t *getSamples() const
    {
        return m_samples.data();
    }

    /**
     * @brief Get the number of samples
     */
    size_t getSampleCount() const
    {
        return m_samples.size();
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

    /**
     * @brief Get the duration in seconds
     */
    float getDuration() const
    {
        if (m_sampleRate == 0 || m_channelCount == 0)
            return 0;
        return static_cast<float>(m_samples.size()) /
               (m_sampleRate * m_channelCount);
    }

    /**
     * @brief Get the file path (for system audio playback)
     */
    const std::string &getFilePath() const
    {
        return m_filePath;
    }

private:
    std::vector<int16_t> m_samples;
    unsigned int m_sampleRate;
    unsigned int m_channelCount;
    unsigned int m_bitsPerSample;
    std::string m_filePath;
};

TG_NAMESPACE_END
