#pragma once
#include <memory>
#include "pa_ringbuffer.h"

template <class T>
class RingBuffer {
public:
    RingBuffer(int fftSize);

    void write(T* inputBuffer, int count);
    void read();

    std::shared_ptr<T[]> getOutputBuffer() { return m_outputBuffer; };
    int getNumChannels() { return m_numChannels; };
    int getOutputBufferSize() { return m_outputBufferSize; };
    int getWritePos() { return m_writePos; };

private:
    const int m_numChannels;
    const int m_ringBufferSize;
    const int m_scratchBufferSize;
    const int m_outputBufferSize;

    int m_writePos = 0;

    std::unique_ptr<PaUtilRingBuffer> m_ringBuffer;
    std::unique_ptr<T[]> m_ringBufferData;
    std::unique_ptr<T[]> m_scratchBuffer;
    std::shared_ptr<T[]> m_outputBuffer;
};
