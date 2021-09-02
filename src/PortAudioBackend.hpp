#pragma once
#include <iostream>
#include <functional>
#include <tuple>
#include "portaudio.h"

using AudioCallback = std::function<
    void(int, float**, int)
>;


class PortAudioBackend {
public:
    PortAudioBackend(
        AudioCallback callback = [](int, float**, int) { }
    );

    float getSampleRate() { return m_sample_rate; };

    void run();
    void end();
    void process(
        const float** input_buffer,
        float** output_buffer,
        int frame_count
    );

    void setCallback(AudioCallback callback) { m_callback = callback; };

private:
    AudioCallback m_callback;
    PaStream* m_stream;
    const float m_requested_sample_rate = 48000.0f;
    float m_sample_rate = 48000.0f;
    const int m_block_size = 256;

    void handle_error(PaError error);
    std::tuple<int, int> find_device();
    static int stream_callback(
        const void *inputBuffer,
        void *outputBuffer,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData
    );
};
