#include <cmath>
#include "Synth.hpp"

Oscillator::Oscillator(float frequency)
: m_frequency(frequency)
{
}

float Oscillator::process() {
    m_phase += 1.0 * m_frequency / 48000.0;
    return std::sin(m_phase * 2 * 3.141592653589793238) * m_amplitude;
}

Synth::Synth(std::shared_ptr<RingBuffer<uint32_t>> ringBuffer)
    : m_ringBuffer(ringBuffer)
    , m_pixels(std::make_unique<uint32_t[]>(640 * 480))
{
    for (int i = 0; i < 256; i++) {
        float frequency = 20 * std::pow(20e3 / 20, i / 256.0f);
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
        for (int i = 0; i < count; i++) {
            m_pixels[i] = m_ringBuffer->getOutputBuffer()[i];
        }
    }
    m_position += m_speedInPixelsPerSecond * frame_count / 48000.f;
    while (m_position > 640) {
        m_position -= 640;
    }
    int position = m_position;

    for (int j = 0; j < frame_count; j++) {
        output_buffer[0][j] = 0;
        output_buffer[1][j] = 0;
    }

    int i = 0;
    for (auto& oscillator : m_oscillators) {
        float amplitude = ((m_pixels[640 * (480 - i) + position] & 0x0000ff00) >> 8) / 256.0;
        oscillator->setAmplitude(amplitude);
        for (int j = 0; j < frame_count; j++) {
            output_buffer[0][j] += oscillator->process();
            output_buffer[1][j] += oscillator->process();
        }
        i += 1;
    }
}
