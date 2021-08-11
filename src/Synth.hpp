#pragma once
#include <cmath>
#include <memory>
#include <vector>
#include "RingBuffer.hpp"

class Oscillator {
public:
    Oscillator(float frequency);
    float process();
    void setAmplitude(float amplitude) { m_amplitude = amplitude; };

private:
    float m_phase = 0;
    const float m_frequency;
    float m_amplitude = 1.0;
};

class Synth {
public:
    Synth(std::shared_ptr<RingBuffer<float>>(ringBuffer));

    void process(
        int input_channels,
        int output_channels,
        const float** input_buffer,
        float** output_buffer,
        int frame_count
    );

private:
    std::shared_ptr<RingBuffer<float>> m_ringBuffer;
    std::unique_ptr<uint32_t[]> m_pixels;
    std::vector<std::unique_ptr<Oscillator>> m_oscillators;
    float m_position = 0;
    float m_speedInPixelsPerSecond = 100;
};
