#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <vector>
#include "RingBuffer.hpp"


class Oscillator {
public:
    Oscillator(float sampleRate, float frequency, float phase);
    void processAdd(float* out1, float* out2, int blockSize);
    void processAddOriginal(float* out1, float* out2, int blockSize);
    void setTargetAmplitudeLeft(float amplitude) { m_targetAmplitudeLeft = amplitude; };
    void setTargetAmplitudeRight(float amplitude) { m_targetAmplitudeRight = amplitude; };

    void setPDMode(int pdMode) { m_pdMode = pdMode; };
    void setPDDistort(float pdDistort) { m_pdDistort = pdDistort; };

private:
    const float m_sampleRate;
    float m_phase = 0;
    const float m_frequency;
    const float m_phaseInc;
    float m_amplitudeLeft = 0.0;
    float m_targetAmplitudeLeft = 0.0;
    float m_amplitudeRight = 0.0;
    float m_targetAmplitudeRight = 0.0;

    int m_pdMode = 0;
    float m_pdDistort = 0;
};

class Synth {
public:
    Synth(float sampleRate, std::mt19937& randomEngine);

    int getNumOscillators() { return m_oscillators.size(); };

    void setPDMode(int pdMode);
    void setPDDistort(float pdDistort);
    void setOscillatorAmplitude(int index, float amplitudeLeft, float amplitudeRight);

    void updateFromRingBuffer(std::shared_ptr<RingBuffer<float>>);

    void process(
        int output_channels,
        float** output_buffer,
        int frame_count
    );

    void processOriginal(
        int output_channels,
        float** output_buffer,
        int frame_count
    );

    void processRealtime(
        int output_channels,
        float** output_buffer,
        int frame_count,
        std::shared_ptr<RingBuffer<float>>
    );

private:
    const float m_sampleRate;
    std::unique_ptr<uint32_t[]> m_pixels;
    std::vector<std::unique_ptr<Oscillator>> m_oscillators;
    float m_position = 0;
    float m_speedInPixelsPerSecond = 100;
};
