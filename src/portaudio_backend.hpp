#pragma once
#include <iostream>
#include "portaudio.h"
#include "audio.hpp"

class PortAudioBackend {
public:
    PortAudioBackend(World* world);

    void run();
    void end();
    void process(
        const float** input_buffer,
        float** output_buffer,
        int frame_count
    );

private:
    PaStream* m_stream;
    const float m_sample_rate = 48000.0f;
    const int m_block_size = 256;
    World* m_world;

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
