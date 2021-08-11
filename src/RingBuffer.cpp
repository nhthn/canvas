#include <iostream>
#include "RingBuffer.hpp"

template <class T>
RingBuffer<T>::RingBuffer(int size)
    : m_ringBufferSize(size)
    , m_scratchBufferSize(size)
    , m_outputBufferSize(size)
    , m_ringBuffer(new PaUtilRingBuffer)
    , m_ringBufferData(new T[m_ringBufferSize])
    , m_scratchBuffer(new T[m_scratchBufferSize])
    , m_outputBuffer(new T[m_outputBufferSize])
{
    ring_buffer_size_t receivedRingBufferSize = PaUtil_InitializeRingBuffer(
        m_ringBuffer.get(), sizeof(T), m_ringBufferSize, m_ringBufferData.get()
    );
    if (receivedRingBufferSize < 0) {
        std::cerr << "RingBuffer initialization failed." << std::endl;
        exit(1);
    }

    for (int i = 0; i < m_ringBufferSize * m_numChannels; i++) {
        m_ringBufferData[i] = 0;
    }
    for (int i = 0; i < m_scratchBufferSize * m_numChannels; i++) {
        m_scratchBuffer[i] = 0;
    }
    for (int i = 0; i < m_outputBufferSize * m_numChannels; i++) {
        m_outputBuffer[i] = 0;
    }
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
void RingBuffer<T>::read()
{
    int availableFrames = PaUtil_GetRingBufferReadAvailable(m_ringBuffer.get());
    int readSamples = std::min(availableFrames, m_scratchBufferSize);
    int count = PaUtil_ReadRingBuffer(m_ringBuffer.get(), m_scratchBuffer.get(), readSamples);

    for (int i = 0; i < count; i++) {
        m_outputBuffer.get()[m_writePos] = m_scratchBuffer.get()[i];

        m_writePos += 1;
        if (m_writePos == m_outputBufferSize) {
            m_writePos = 0;
        }
    }
}
