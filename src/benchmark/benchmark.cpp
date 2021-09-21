#include <benchmark/benchmark.h>
#include "../Synth.hpp"

static void benchSynthProcess(benchmark::State& state) {
    std::mt19937 randomEngine(0);
    Synth synth(48000, randomEngine);

    for (int i = 0; i < synth.getNumOscillators(); i++) {
        synth.setOscillatorAmplitude(i, 1, 1);
    }

    int frameCount = 64;
    float outputBufferLeft[64];
    float outputBufferRight[64];
    float* outputBuffer[2] = { outputBufferLeft, outputBufferRight };

    for (auto _ : state) {
        synth.process(2, outputBuffer, frameCount);
    }
}
// Register the function as a benchmark
BENCHMARK(benchSynthProcess);

static void benchSynthProcessOriginal(benchmark::State& state) {
    std::mt19937 randomEngine(0);
    Synth synth(48000, randomEngine);

    for (int i = 0; i < synth.getNumOscillators(); i++) {
        synth.setOscillatorAmplitude(i, 1, 1);
    }

    int frameCount = 64;
    float outputBufferLeft[64];
    float outputBufferRight[64];
    float* outputBuffer[2] = { outputBufferLeft, outputBufferRight };

    for (auto _ : state) {
        synth.processOriginal(2, outputBuffer, frameCount);
    }
}
// Register the function as a benchmark
BENCHMARK(benchSynthProcessOriginal);

BENCHMARK_MAIN();
