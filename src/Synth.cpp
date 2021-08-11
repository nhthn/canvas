#include <cmath>
#include "Synth.hpp"


float k_sineTable2048[2048] = {
#include "sine_table_2048.txt"
};

Oscillator::Oscillator(float frequency)
: m_frequency(frequency)
{
}

float Oscillator::process() {
    m_phase += m_frequency / 48000.0;
    while (m_phase > 1.0) {
        m_phase -= 1.0;
    }
    int integerPhase = m_phase * 2048;
    float frac = m_phase * 2048 - integerPhase;
    int integerPhase2 = (integerPhase + 1) % 2048;
    return (
        k_sineTable2048[integerPhase] * (1 - frac)
        + k_sineTable2048[integerPhase2] * frac
    ) * m_amplitude;
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
