#include <cmath>
#include "Synth.hpp"


float k_sineTable2048[2048] = {
#include "sine_table_2048.txt"
};

float distortPhase(float phase, int mode, float distort)
{
    switch (mode) {
    case 0: // Pulsar
        return std::min(phase * (1 + 10 * distort), 1.f);
    case 1: // Saw
        {
            float breakpoint = 0.25 * (1 - distort * 0.9);
            float outerSlope = 0.25 / breakpoint;
            float innerSlope = 0.5 / (1 - breakpoint * 2);
            if (phase < breakpoint) {
                return phase * outerSlope;
            } else if (phase >= 1 - breakpoint) {
                return 1 - (1 - phase) * outerSlope;
            } else {
                return 0.5 + (phase - 0.5) * innerSlope;
            }
        }
    }
    return phase;
}

Oscillator::Oscillator(float sampleRate, float frequency)
    : m_sampleRate(sampleRate)
    , m_frequency(frequency)
{
}

void Oscillator::processAdd(float* out1, float* out2, int blockSize) {
    for (int i = 0; i < blockSize; i++) {
        m_phase = std::fmod(m_phase + m_frequency / m_sampleRate, 1.0);
        float distortedPhase = std::fmod(distortPhase(m_phase, m_pdMode, m_pdDistort), 1.0);
        int integerPhase = distortedPhase * 2048;
        float frac = distortedPhase * 2048 - integerPhase;
        int integerPhase2 = (integerPhase + 1) % 2048;
        float amplitudeLeft = (
            m_amplitudeLeft * (1 - i / static_cast<float>(blockSize))
            + m_targetAmplitudeLeft * i / static_cast<float>(blockSize)
        );
        float amplitudeRight = (
            m_amplitudeRight * (1 - i / static_cast<float>(blockSize))
            + m_targetAmplitudeRight * i / static_cast<float>(blockSize)
        );
        float outSample = (
            k_sineTable2048[integerPhase] * (1 - frac)
            + k_sineTable2048[integerPhase2] * frac
        );
        out1[i] += outSample * amplitudeLeft;
        out2[i] += outSample * amplitudeRight;
    }
    m_amplitudeLeft = m_targetAmplitudeLeft;
    m_amplitudeRight = m_targetAmplitudeRight;
}

Synth::Synth(float sampleRate, std::shared_ptr<RingBuffer<float>> ringBuffer)
    : m_sampleRate(sampleRate)
    , m_ringBuffer(ringBuffer)
{
    for (int i = 0; i < 239; i++) {
        float frequency = 55 * std::pow(2, i / 24.0);
        m_oscillators.push_back(
            std::make_unique<Oscillator>(m_sampleRate, frequency)
        );
    }
}

void Synth::process(
    int input_channels,
    int output_channels,
    const float** input_buffer,
    float** output_buffer,
    int frame_count
) {
    int count = m_ringBuffer->read();
    if (count > 0) {
        auto buffer = m_ringBuffer->getOutputBuffer();
        for (int i = 0; i < m_oscillators.size(); i++) {
            m_oscillators[i]->setPDMode(buffer[0]);
            m_oscillators[i]->setPDDistort(buffer[1]);
        }
        int amplitudeOffset = 2;
        int numOscillators = std::min(
            (count - amplitudeOffset) / 2, static_cast<int>(m_oscillators.size())
        );
        for (int i = 0; i < numOscillators; i++) {
            m_oscillators[i]->setTargetAmplitudeLeft(buffer[amplitudeOffset + 2 * i]);
            m_oscillators[i]->setTargetAmplitudeRight(buffer[amplitudeOffset + 2 * i + 1]);
        }
    }

    for (int j = 0; j < frame_count; j++) {
        output_buffer[0][j] = 0;
        output_buffer[1][j] = 0;
    }

    for (auto& oscillator : m_oscillators) {
        oscillator->processAdd(output_buffer[0], output_buffer[1], frame_count);
    }
}
