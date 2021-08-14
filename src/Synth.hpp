#pragma once
#include <cmath>
#include <memory>
#include <vector>
#include "RingBuffer.hpp"


class Oscillator {
public:
    Oscillator(float sampleRate, float frequency);
    void processAdd(float* out1, float* out2, int blockSize);
    void setTargetAmplitudeLeft(float amplitude) { m_targetAmplitudeLeft = amplitude; };
    void setTargetAmplitudeRight(float amplitude) { m_targetAmplitudeRight = amplitude; };

    void setPDMode(int pdMode) { m_pdMode = pdMode; };
    void setPDDistort(float pdDistort) { m_pdDistort = pdDistort; };

private:
    const float m_sampleRate;
    float m_phase = 0;
    const float m_frequency;
    float m_amplitudeLeft = 0.0;
    float m_targetAmplitudeLeft = 0.0;
    float m_amplitudeRight = 0.0;
    float m_targetAmplitudeRight = 0.0;

    int m_pdMode = 0;
    float m_pdDistort = 0;
};

class Synth {
public:
    Synth(float sampleRate, std::shared_ptr<RingBuffer<float>>(ringBuffer));

    void updateFromRingBuffer();

    void process(
        int input_channels,
        int output_channels,
        const float** input_buffer,
        float** output_buffer,
        int frame_count
    );

    void processRealtime(
        int input_channels,
        int output_channels,
        const float** input_buffer,
        float** output_buffer,
        int frame_count
    );

private:
    const float m_sampleRate;
    std::shared_ptr<RingBuffer<float>> m_ringBuffer;
    std::unique_ptr<uint32_t[]> m_pixels;
    std::vector<std::unique_ptr<Oscillator>> m_oscillators;
    float m_position = 0;
    float m_speedInPixelsPerSecond = 100;
};
