#pragma once
#include <iostream>
#include <functional>
#include "portaudio.h"

using AudioCallback = std::function<void(
    int, int, const float**, float**, int
)>;


class PortAudioBackend {
public:
    PortAudioBackend(AudioCallback callback);

    void run();
    void end();
    void process(
        const float** input_buffer,
        float** output_buffer,
        int frame_count
    );

private:
    AudioCallback m_callback;
    PaStream* m_stream;
    const float m_sample_rate = 48000.0f;
    const int m_block_size = 256;

    void handle_error(PaError error);
    int find_device();
    static int stream_callback(
        const void *inputBuffer,
        void *outputBuffer,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData
    );
};
