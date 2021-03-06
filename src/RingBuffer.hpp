#pragma once
#include <iostream>
#include <memory>
#include "pa_ringbuffer.h"

template <class T>
class RingBuffer {
public:
    RingBuffer(int size);
    ~RingBuffer();

    void write(T* inputBuffer, int count);
    int read();

    T* getOutputBuffer() { return m_outputBuffer; };
    int getOutputBufferSize() { return m_outputBufferSize; };
    int getWritePos() { return m_writePos; };

private:
    const int m_ringBufferSize;
    const int m_outputBufferSize;

    int m_writePos = 0;

    std::unique_ptr<PaUtilRingBuffer> m_ringBuffer;
    T* m_ringBufferData;
    T* m_outputBuffer;
};

template <class T>
RingBuffer<T>::RingBuffer(int size)
    : m_ringBufferSize(size)
    , m_outputBufferSize(size)
    , m_ringBuffer(new PaUtilRingBuffer)
    , m_ringBufferData(new T[m_ringBufferSize])
    , m_outputBuffer(new T[m_outputBufferSize])
{
    ring_buffer_size_t receivedRingBufferSize = PaUtil_InitializeRingBuffer(
        m_ringBuffer.get(), sizeof(T), m_ringBufferSize, m_ringBufferData
    );
    if (receivedRingBufferSize < 0) {
        std::cerr << "RingBuffer initialization failed." << std::endl;
        exit(1);
    }

    for (int i = 0; i < m_ringBufferSize; i++) {
        m_ringBufferData[i] = 0;
    }
    for (int i = 0; i < m_outputBufferSize; i++) {
        m_outputBuffer[i] = 0;
    }
}

template <class T>
RingBuffer<T>::~RingBuffer()
{
    delete[] m_ringBufferData;
    delete[] m_outputBuffer;
}

template <class T>
void RingBuffer<T>::write(T* inputBuffer, int count)
{
    auto writeAvailable = PaUtil_GetRingBufferWriteAvailable(m_ringBuffer.get());
    if (count > writeAvailable) {
        PaUtil_AdvanceRingBufferReadIndex(m_ringBuffer.get(), count);
    }
    PaUtil_WriteRingBuffer(m_ringBuffer.get(), inputBuffer, count);
}

template <class T>
int RingBuffer<T>::read()
{
    int availableFrames = PaUtil_GetRingBufferReadAvailable(m_ringBuffer.get());
    int readSamples = std::min(availableFrames, m_outputBufferSize);
    int count = PaUtil_ReadRingBuffer(m_ringBuffer.get(), m_outputBuffer, readSamples);
    return count;
}
