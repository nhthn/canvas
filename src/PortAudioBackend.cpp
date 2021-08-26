#include "PortAudioBackend.hpp"

PortAudioBackend::PortAudioBackend(AudioCallback callback)
    : m_callback(callback)
{
}

void PortAudioBackend::run() {
    handle_error(Pa_Initialize());

    auto devices = find_device();

    PaSampleFormat sample_format = paFloat32 | paNonInterleaved;

    PaStreamParameters input_parameters;
    input_parameters.device = std::get<0>(devices);
    input_parameters.channelCount = 2;
    input_parameters.sampleFormat = sample_format;
    input_parameters.suggestedLatency = 0.0;
    input_parameters.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters output_parameters;
    output_parameters.device = std::get<1>(devices);
    output_parameters.channelCount = 2;
    output_parameters.sampleFormat = sample_format;
    output_parameters.suggestedLatency = 0.0;
    output_parameters.hostApiSpecificStreamInfo = nullptr;

    PaStreamFlags stream_flags = paNoFlag;
    void* user_data = this;

    handle_error(
        Pa_OpenStream(
            &m_stream,
            &input_parameters,
            &output_parameters,
            m_sample_rate,
            m_block_size,
            stream_flags,
            stream_callback,
            user_data
        )
    );

    m_sample_rate = Pa_GetStreamInfo(m_stream)->sampleRate;

    handle_error(Pa_StartStream(m_stream));
}

void PortAudioBackend::end() {
    handle_error(Pa_StopStream(m_stream));
    handle_error(Pa_CloseStream(m_stream));
    handle_error(Pa_Terminate());
}

void PortAudioBackend::process(
const float** input_buffer,
    float** output_buffer,
    int frame_count
) {
    m_callback(2, 2, input_buffer, output_buffer, frame_count);
}

void PortAudioBackend::handle_error(PaError error) {
    if (error == paNoError) {
        return;
    }
    std::cerr << "PortAudio error, exiting :(" << std::endl;
    exit(1);
}

std::tuple<int, int> PortAudioBackend::find_device() {
#ifdef _WIN32
    return std::make_tuple(
        Pa_GetDefaultInputDevice(),
        Pa_GetDefaultOutputDevice()
    );
#endif

#ifdef __linux__
    PaHostApiIndex host_api_index = Pa_HostApiTypeIdToHostApiIndex(paJACK);
    if (host_api_index < 0) {
        std::cerr << "Couldn't find JACK" << std::endl;
        exit(1);
    }

    const PaHostApiInfo* host_api_info = Pa_GetHostApiInfo(host_api_index);
    int device_count = host_api_info->deviceCount;
    for (int i = 0; i < device_count; i++) {
        int device_index = Pa_HostApiDeviceIndexToDeviceIndex(host_api_index, i);
        const PaDeviceInfo* info = Pa_GetDeviceInfo(device_index);
        std::string name = info->name;
        if (name == "system") {
            return std::make_tuple(device_index, device_index);
        }
    }
    std::cerr << "Couldn't find device" << std::endl;
    exit(1);
#endif  // __linux__
}

int PortAudioBackend::stream_callback(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
) {
    PortAudioBackend* backend = static_cast<PortAudioBackend*>(userData);
    backend->process(
        static_cast<const float**>(nullptr),
        static_cast<float**>(outputBuffer),
        frameCount
    );
    return 0;
}
