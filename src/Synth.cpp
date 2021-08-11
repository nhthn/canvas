#include <cmath>
#include "Synth.hpp"

Oscillator::Oscillator(float frequency)
: m_frequency(frequency)
{
}

float Oscillator::process() {
    m_phase += m_frequency / 48000.0;
    while (m_phase > 1.0) {
        m_phase -= 1.0;
    }
    return std::sin(m_phase * 2 * 3.141592653589793238) * m_amplitude;
}

Synth::Synth(std::shared_ptr<RingBuffer<float>> ringBuffer)
    : m_ringBuffer(ringBuffer)
{
    for (int i = 0; i < 256; i++) {
        float frequency = 55 * std::pow(2, i / 24.0);
        m_oscillators.push_back(
            std::make_unique<Oscillator>(frequency)
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
        int numOscillators = std::min(count, static_cast<int>(m_oscillators.size()));
        for (int i = 0; i < numOscillators; i++) {
            m_oscillators[i]->setAmplitude(m_ringBuffer->getOutputBuffer()[i]);
        }
    }
    m_position += m_speedInPixelsPerSecond * frame_count / 48000.f;

    for (int j = 0; j < frame_count; j++) {
        output_buffer[0][j] = 0;
        output_buffer[1][j] = 0;
    }

    int i = 0;
    for (auto& oscillator : m_oscillators) {
        for (int j = 0; j < frame_count; j++) {
            float sample = oscillator->process();
            output_buffer[0][j] += sample;
            output_buffer[1][j] += sample;
        }
        i += 1;
    }
}
