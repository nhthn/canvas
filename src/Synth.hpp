#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

#include "simdpp/simd.h"

#include "RingBuffer.hpp"


constexpr int k_oscillatorVectorSize = 8;
using Float8 = simdpp::float32<8>;
using Int8 = simdpp::uint32<8>;

class Oscillator8 {
public:
    Oscillator8(float sampleRate, Float8 frequencies, Float8 phase);
    void processAdd(float* out1, float* out2, int blockSize);
    void setTargetAmplitudesLeft(Float8 amplitude) { m_targetAmplitudesLeft = amplitude; };
    void setTargetAmplitudesRight(Float8 amplitude) { m_targetAmplitudesRight = amplitude; };

    void setPDMode(int pdMode) { m_pdMode = pdMode; };
    void setPDDistort(float pdDistort) { m_pdDistort = pdDistort; };

private:
    const float m_sampleRate;
    Float8 m_phases;
    const Float8 m_frequencies;
    Float8 m_amplitudesLeft;
    Float8 m_targetAmplitudesLeft;
    Float8 m_amplitudesRight;
    Float8 m_targetAmplitudesRight;

    int m_pdMode = 0;
    float m_pdDistort = 0;
};

class Synth {
public:
    Synth(float sampleRate, std::mt19937& randomEngine);

    int getNumOscillators() { return m_oscillators.size() * k_oscillatorVectorSize; };

    void setPDMode(int pdMode);
    void setPDDistort(float pdDistort);
    void setOscillatorAmplitudes(float* amplitudes);

    void updateFromRingBuffer(std::shared_ptr<RingBuffer<float>>);

    void process(
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
    std::vector<std::unique_ptr<Oscillator8>> m_oscillators;
    float m_position = 0;
    float m_speedInPixelsPerSecond = 100;
};


class Oscillator {
public:
    Oscillator(float sampleRate, float frequency, float phase);
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

class SynthOriginal {
public:
    SynthOriginal(float sampleRate, std::mt19937& randomEngine);

    int getNumOscillators() { return m_oscillators.size(); };
    void setOscillatorAmplitude(int index, float amplitudeLeft, float amplitudeRight);

    void setPDMode(int pdMode);
    void setPDDistort(float pdDistort);

    void updateFromRingBuffer(std::shared_ptr<RingBuffer<float>>);

    void process(
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
