#include <cmath>
#include <random>
#include "Synth.hpp"


float k_sineTable2048[2048] = {
#include "sine_table_2048.txt"
};

float distortPhase(float phase, int mode, float distort)
{
    switch (mode) {
    case 0: // Pulsar
        return std::min(phase * (1 + 6 * distort * distort), 1.f);
    case 1: // Saw
        {
            float breakpoint = 0.25 * (1 - distort * 0.9);
            float outerSlope = 0.25 / breakpoint;
            float innerSlope = 0.5 / (1 - breakpoint * 2);
            if (phase < breakpoint) {
                return phase * outerSlope;
            } else if (phase >= 1 - breakpoint) {
                return 1 - (1 - phase) * outerSlope;
            } else {
                return 0.5 + (phase - 0.5) * innerSlope;
            }
        }
    case 2: // Square
        {
            float cosinePhase = std::fmod(phase + 0.25f, 1.0f);

            float adjustedDistort = distort * 0.9;
            float breakpoint = adjustedDistort * 0.5;
            float slope = 1 / (1 - adjustedDistort);

            if (cosinePhase < breakpoint) {
                cosinePhase = 0;
            } else if (cosinePhase < 0.5f) {
                cosinePhase = (cosinePhase - breakpoint) * slope;
            } else if (cosinePhase < 0.5f + breakpoint) {
                cosinePhase = 0.5;
            } else {
                cosinePhase = 0.5 + (cosinePhase - (0.5 + breakpoint)) * slope;
            }

            return std::fmod(cosinePhase - 0.25f + 1.0f, 1.0f);
        }
    case 3: // PWM
        {
            float adjustedDistort = distort * 0.9;
            float breakpoint = 0.5 + 0.5 * adjustedDistort;

            if (phase < breakpoint) {
                return phase * 0.5 / breakpoint;
            } else {
                return 0.5 + (phase - breakpoint) * 0.5 / (1 - breakpoint);
            }
        }
    }
    return phase;
}

Oscillator::Oscillator(float sampleRate, float frequency, float phase)
    : m_sampleRate(sampleRate)
    , m_frequency(frequency)
    , m_phase(phase)
{
}

void Oscillator::processAdd(float* out1, float* out2, int blockSize) {
    for (int i = 0; i < blockSize; i++) {
        m_phase += m_frequency / m_sampleRate;
        while (m_phase > 1) {
            m_phase -= 1;
        }
        float distortedPhase = distortPhase(m_phase, m_pdMode, m_pdDistort);
        int integerPhase = distortedPhase * 2048;
        float frac = distortedPhase * 2048 - integerPhase;
        int integerPhase2 = (integerPhase + 1) % 2048;
        float amplitudeLeft = (
            m_amplitudeLeft * (1 - i / static_cast<float>(blockSize))
            + m_targetAmplitudeLeft * i / static_cast<float>(blockSize)
        );
        float amplitudeRight = (
            m_amplitudeRight * (1 - i / static_cast<float>(blockSize))
            + m_targetAmplitudeRight * i / static_cast<float>(blockSize)
        );
        float outSample = (
            k_sineTable2048[integerPhase] * (1 - frac)
            + k_sineTable2048[integerPhase2] * frac
        );
        out1[i] += outSample * amplitudeLeft;
        out2[i] += outSample * amplitudeRight;
    }
    m_amplitudeLeft = m_targetAmplitudeLeft;
    m_amplitudeRight = m_targetAmplitudeRight;
}

Oscillator8::Oscillator8(float sampleRate, Float8 frequencies, Float8 phases)
    : m_sampleRate(sampleRate)
    , m_frequencies(frequencies)
    , m_phases(phases)
    , m_phaseInc(m_frequencies / m_sampleRate)
    , m_amplitudesLeft(simdpp::make_float<simdpp::float32<8>>(0, 0, 0, 0, 0, 0, 0, 0))
    , m_amplitudesRight(simdpp::make_float<simdpp::float32<8>>(0, 0, 0, 0, 0, 0, 0, 0))
    , m_targetAmplitudesLeft(simdpp::make_float<simdpp::float32<8>>(0, 0, 0, 0, 0, 0, 0, 0))
    , m_targetAmplitudesRight(simdpp::make_float<simdpp::float32<8>>(0, 0, 0, 0, 0, 0, 0, 0))
{
}

void Oscillator8::processAdd(float* out1, float* out2, int blockSize) {
    float blockInc = 1.0 / blockSize;
    for (int i = 0; i < blockSize; i++) {
        SIMDPP_ALIGN(256) float frequencies[8];
        simdpp::store(frequencies, m_frequencies);

        m_phases = m_phases + m_phaseInc;
        m_phases = ((m_phases >= 1) & (m_phases - 1)) + ((m_phases < 1) & m_phases);

        SIMDPP_ALIGN(256) float phases[8];
        simdpp::store(phases, m_phases);
        SIMDPP_ALIGN(256) float amplitudesLeft[8];
        simdpp::store(amplitudesLeft, m_amplitudesLeft);
        SIMDPP_ALIGN(256) float amplitudesRight[8];
        simdpp::store(amplitudesRight, m_amplitudesRight);
        SIMDPP_ALIGN(256) float targetAmplitudesRight[8];
        simdpp::store(targetAmplitudesRight, m_targetAmplitudesRight);
        SIMDPP_ALIGN(256) float targetAmplitudesLeft[8];
        simdpp::store(targetAmplitudesLeft, m_targetAmplitudesLeft);
        for (int j = 0; j < 8; j++) {
            float phase = phases[j];
            float distortedPhase = distortPhase(phase, m_pdMode, m_pdDistort);
            int integerPhase = distortedPhase * 2048;
            float frac = distortedPhase * 2048 - integerPhase;
            int integerPhase2 = (integerPhase + 1) % 2048;
            float amplitudeLeft = (
                amplitudesLeft[j] * (1 - i * blockInc)
                + targetAmplitudesLeft[j] * i * blockInc
            );
            float amplitudeRight = (
                amplitudesRight[j] * (1 - i * blockInc)
                + targetAmplitudesRight[j] * i * blockInc
            );
            float outSample = (
                k_sineTable2048[integerPhase] * (1 - frac)
                + k_sineTable2048[integerPhase2] * frac
            );
            out1[i] += outSample * amplitudeLeft;
            out2[i] += outSample * amplitudeRight;
        }
    }
    m_amplitudesLeft = m_targetAmplitudesLeft;
    m_amplitudesRight = m_targetAmplitudesRight;
}

float oscillatorIndexToFrequency(int i) {
    return 55.0 / 2 * std::pow(2, i / 24.0);
}

Synth::Synth(float sampleRate, std::mt19937& randomEngine)
    : m_sampleRate(sampleRate)
{
    std::uniform_real_distribution<> distribution;
    for (int i = 0; i < 239 / k_oscillatorVectorSize; i++) {
        int tmp = i * k_oscillatorVectorSize;
        Float8 frequencies = simdpp::make_float<simdpp::float32<8>>(
            oscillatorIndexToFrequency(tmp + 0),
            oscillatorIndexToFrequency(tmp + 1),
            oscillatorIndexToFrequency(tmp + 2),
            oscillatorIndexToFrequency(tmp + 3),
            oscillatorIndexToFrequency(tmp + 4),
            oscillatorIndexToFrequency(tmp + 5),
            oscillatorIndexToFrequency(tmp + 6),
            oscillatorIndexToFrequency(tmp + 7)
        );
        Float8 phases = simdpp::make_float<simdpp::float32<8>>(
            distribution(randomEngine),
            distribution(randomEngine),
            distribution(randomEngine),
            distribution(randomEngine),
            distribution(randomEngine),
            distribution(randomEngine),
            distribution(randomEngine),
            distribution(randomEngine)
        );
        m_oscillators.push_back(
            std::make_unique<Oscillator8>(m_sampleRate, frequencies, phases)
        );
    }
}

void Synth::setPDMode(int pdMode)
{
    for (int i = 0; i < m_oscillators.size(); i++) {
        m_oscillators[i]->setPDMode(pdMode);
    }
}

void Synth::setPDDistort(float pdDistort)
{
    for (int i = 0; i < m_oscillators.size(); i++) {
        m_oscillators[i]->setPDDistort(pdDistort);
    }
}

void Synth::setOscillatorAmplitudes(float* amplitudes)
{
    for (int i = 0; i < m_oscillators.size(); i++) {
        int offset = i * k_oscillatorVectorSize * 2;
        m_oscillators[i]->setTargetAmplitudesLeft(
            simdpp::make_float<simdpp::float32<8>>(
                amplitudes[offset + 0],
                amplitudes[offset + 2],
                amplitudes[offset + 4],
                amplitudes[offset + 6],
                amplitudes[offset + 8],
                amplitudes[offset + 10],
                amplitudes[offset + 12],
                amplitudes[offset + 14]
            )
        );
        m_oscillators[i]->setTargetAmplitudesRight(
            simdpp::make_float<simdpp::float32<8>>(
                amplitudes[offset + 1],
                amplitudes[offset + 3],
                amplitudes[offset + 5],
                amplitudes[offset + 7],
                amplitudes[offset + 9],
                amplitudes[offset + 11],
                amplitudes[offset + 13],
                amplitudes[offset + 15]
            )
        );
    }
}

void Synth::updateFromRingBuffer(std::shared_ptr<RingBuffer<float>> ringBuffer)
{
    int amplitudeOffset = 2;
    int count = ringBuffer->read();
    if (count < amplitudeOffset + getNumOscillators() * 2) {
        return;
    }
    auto buffer = ringBuffer->getOutputBuffer();
    setPDMode(buffer[0]);
    setPDDistort(buffer[1]);
    setOscillatorAmplitudes(buffer + amplitudeOffset);
}

void Synth::process(
    int output_channels,
    float** output_buffer,
    int frame_count
) {
    if (output_channels != 2) {
        std::cout << "Output channels is not 2. This shouldn't happen!" << std::endl;
        exit(1);
    }

    for (int j = 0; j < frame_count; j++) {
        output_buffer[0][j] = 0;
        output_buffer[1][j] = 0;
    }
    for (auto& oscillator : m_oscillators) {
        oscillator->processAdd(output_buffer[0], output_buffer[1], frame_count);
    }
}

void Synth::processRealtime(
    int outputChannels,
    float** outputBuffer,
    int frameCount,
    std::shared_ptr<RingBuffer<float>> ringBuffer
) {
    updateFromRingBuffer(ringBuffer);
    process(outputChannels, outputBuffer, frameCount);
}

SynthOriginal::SynthOriginal(float sampleRate, std::mt19937& randomEngine)
    : m_sampleRate(sampleRate)
{
    std::uniform_real_distribution<> distribution;
    for (int i = 0; i < 239; i++) {
        float frequency = 55.0 / 2 * std::pow(2, i / 24.0);
        float phase = distribution(randomEngine);
        m_oscillators.push_back(
            std::make_unique<Oscillator>(m_sampleRate, frequency, phase)
        );
    }
}

void SynthOriginal::setPDMode(int pdMode)
{
    for (int i = 0; i < m_oscillators.size(); i++) {
        m_oscillators[i]->setPDMode(pdMode);
    }
}

void SynthOriginal::setPDDistort(float pdDistort)
{
    for (int i = 0; i < m_oscillators.size(); i++) {
        m_oscillators[i]->setPDDistort(pdDistort);
    }
}

void SynthOriginal::setOscillatorAmplitude(int index, float amplitudeLeft, float amplitudeRight)
{
    m_oscillators[index]->setTargetAmplitudeLeft(amplitudeLeft);
    m_oscillators[index]->setTargetAmplitudeRight(amplitudeRight);
}

void SynthOriginal::updateFromRingBuffer(std::shared_ptr<RingBuffer<float>> ringBuffer)
{
    int count = ringBuffer->read();
    if (count == 0) {
        return;
    }
    auto buffer = ringBuffer->getOutputBuffer();
    setPDMode(buffer[0]);
    setPDDistort(buffer[1]);
    int amplitudeOffset = 2;
    int numOscillators = std::min(
        (count - amplitudeOffset) / 2, static_cast<int>(m_oscillators.size())
    );
    for (int i = 0; i < numOscillators; i++) {
        setOscillatorAmplitude(
            i, buffer[amplitudeOffset + 2 * i], buffer[amplitudeOffset + 2 * i + 1]
        );
    }
}

void SynthOriginal::process(
    int output_channels,
    float** output_buffer,
    int frame_count
) {
    if (output_channels != 2) {
        std::cout << "Output channels is not 2. This shouldn't happen!" << std::endl;
        exit(1);
    }

    for (int j = 0; j < frame_count; j++) {
        output_buffer[0][j] = 0;
        output_buffer[1][j] = 0;
    }
    for (auto& oscillator : m_oscillators) {
        oscillator->processAdd(output_buffer[0], output_buffer[1], frame_count);
    }
}

void SynthOriginal::processRealtime(
    int outputChannels,
    float** outputBuffer,
    int frameCount,
    std::shared_ptr<RingBuffer<float>> ringBuffer
) {
    updateFromRingBuffer(ringBuffer);
    process(outputChannels, outputBuffer, frameCount);
}
