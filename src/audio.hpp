#pragma once
#include <cmath>

class Saw {
public:
    Saw(float frequency);

    float process();

private:
    float m_phase = 0;
    const float m_frequency;
};

class Synth {
public:
    Synth();

    void process(
        int input_channels,
        int output_channels,
        const float** input_buffer,
        float** output_buffer,
        int frame_count
    );

private:
    Saw m_saw_left;
    Saw m_saw_right;
};

