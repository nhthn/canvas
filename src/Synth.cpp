#include <cmath>
#include "Synth.hpp"

Saw::Saw(float frequency)
: m_frequency(frequency)
{
}

float Saw::process() {
    m_phase += m_frequency / 48000.0;
    if (m_phase > 1) {
        m_phase -= 1;
    }

    return m_phase - 0.5;
}

Synth::Synth()
: m_saw_left(440), m_saw_right(441)
{
}

void Synth::process(
    int input_channels,
    int output_channels,
    const float** input_buffer,
    float** output_buffer,
    int frame_count
) {
    for (int i = 0; i < frame_count; i++) {
        output_buffer[0][i] = m_saw_left.process();
        output_buffer[1][i] = m_saw_right.process();
    }
}
