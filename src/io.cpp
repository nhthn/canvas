#include <fftw3.h>
#include <sndfile.h>

#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "io.hpp"
#include "Synth.hpp"

namespace io {

Status loadAudio(Image image, std::string fileName)
{
    uint32_t* pixels = std::get<0>(image);
    int width = std::get<1>(image);
    int height = std::get<2>(image);

    SF_INFO sf_info;
    sf_info.format = 0;
    auto soundFile = sf_open(fileName.c_str(), SFM_READ, &sf_info);

    if (soundFile == nullptr) {
        return std::make_tuple(
            false,
            std::string("Audio loading failed: ") + sf_strerror(soundFile)
        );
    }

    if ((sf_info.channels != 1) && (sf_info.channels != 2)) {
        sf_close(soundFile);
        return std::make_tuple(false, "File must have 1 or 2 channels");
    }

    int frames = sf_info.frames;
    float* audio = new float[sf_info.frames * sf_info.channels];
    sf_readf_float(soundFile, audio, sf_info.frames);
    sf_close(soundFile);

    int fftBufferSize = 4096;
    int spectrumSize = fftBufferSize / 2 + 1;
    float* fftInBuffer = new float[fftBufferSize];
    auto fftOutBuffer = static_cast<fftwf_complex*>(
        fftwf_malloc(sizeof(fftwf_complex) * spectrumSize)
    );
    float* magnitudeSpectrum = new float[spectrumSize];
    float* imageTmp = new float[height * width * 2];

    auto fftwPlan = fftwf_plan_dft_r2c_1d(
        fftBufferSize, fftInBuffer, fftOutBuffer, FFTW_MEASURE
    );

    for (int channel = 0; channel < sf_info.channels; channel++) {
        for (int x = 0; x < width; x++) {
            int offset = x * static_cast<float>(sf_info.frames) / width;
            for (int i = 0; i < fftBufferSize; i++) {
                float window = 0.5 - 0.5 * std::cos(i * 2 * 3.141592653589 / fftBufferSize);
                if (offset + i >= sf_info.frames) {
                    fftInBuffer[i] = 0;
                } else {
                    if (sf_info.channels == 1) {
                        fftInBuffer[i] = audio[offset + i] * window;
                    } else {
                        fftInBuffer[i] = audio[(offset + i) * 2 + channel] * window;
                    }
                }
            }
            fftwf_execute(fftwPlan);

            float binToFreq = (sf_info.samplerate * 0.5f) / spectrumSize;
            float freqToBin = 1 / binToFreq;
            for (int y = 0; y < height; y++) {
                float minFreq = 27.5 * std::pow(2, (height - 1 - y - 1) / 24.f);
                float midFreq = 27.5 * std::pow(2, (height - 1 - y) / 24.f);
                float maxFreq = 27.5 * std::pow(2, (height - 1 - y + 1) / 24.f);
                int minBin = clamp<int>(static_cast<int>(freqToBin * minFreq), 0, spectrumSize - 1);
                int midBin = clamp<int>(static_cast<int>(freqToBin * midFreq), 0, spectrumSize - 1);
                int maxBin = clamp<int>(static_cast<int>(freqToBin * maxFreq), 0, spectrumSize - 1);

                float amplitude = 0;
                for (int bin = minBin; bin < midBin; bin++) {
                    float binAmplitude = std::hypot(
                        fftOutBuffer[bin][0], fftOutBuffer[bin][1]
                    );
                    float multiplier = (
                        static_cast<float>(bin - minBin) / (midBin - minBin)
                    );
                    amplitude += binAmplitude * multiplier;
                }
                for (int bin = midBin; bin <= maxBin; bin++) {
                    float binAmplitude = std::hypot(
                        fftOutBuffer[bin][0], fftOutBuffer[bin][1]
                    );
                    float multiplier = (
                        1 - static_cast<float>(bin - midBin) / (maxBin - midBin)
                    );
                    amplitude += binAmplitude * multiplier;
                }
                imageTmp[(y * width + x) * 2 + channel] = amplitude;
            };
        }
    }

    float overallMaxAmplitude = 0;
    for (int i = 0; i < height * width * 2; i++) {
        if (imageTmp[i] > overallMaxAmplitude) {
            overallMaxAmplitude = imageTmp[i];
        }
    }
    if (overallMaxAmplitude != 0) {
        for (int i = 0; i < height * width * 2; i++) {
            imageTmp[i] /= overallMaxAmplitude;
        }
    }
    for (int i = 0; i < height * width; i++) {
        float left = imageTmp[2 * i];
        float right = sf_info.channels == 1 ? imageTmp[2 * i] : imageTmp[2 * i + 1];
        pixels[i] = colorFromNormalized(right, 0, left);
    }

    fftwf_free(fftOutBuffer);
    delete[] imageTmp;
    delete[] magnitudeSpectrum;
    delete[] fftInBuffer;
    delete[] audio;

    return std::make_tuple(true, "");
}

Status renderAudio(
    Image image,
    std::string fileName,
    float sampleRate,
    float overallGain,
    float speedInPixelsPerSecond,
    float pdMode,
    float pdDistort
)
{
    uint32_t* pixels = std::get<0>(image);
    int width = std::get<1>(image);
    int height = std::get<2>(image);

    if (speedInPixelsPerSecond < 0.01) {
        return std::make_tuple(false, "Speed is too slow to render audio.");
    }

    if (!endsWith(fileName, ".wav")) {
        return std::make_tuple(false, "File name must end in .wav");
    }

    SF_INFO sf_info;
    sf_info.samplerate = sampleRate;
    sf_info.channels = 2;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    sf_info.sections = 0;
    sf_info.seekable = 0;
    auto soundFile = sf_open(fileName.c_str(), SFM_WRITE, &sf_info);

    if (soundFile == nullptr) {
        return std::make_tuple(
            false, std::string("Audio rendering failed: ") + sf_strerror(soundFile)
        );
    }

    int numFrames = (
        static_cast<float>(width) / speedInPixelsPerSecond * sampleRate
    );
    float* audio = new float[numFrames * 2];

    Synth synth(sampleRate);
    synth.setPDMode(pdMode);
    synth.setPDDistort(pdDistort);

    int inChannels = 2;
    int outChannels = 2;
    int blockSize = 64;

    float* leftInBuffer = new float[blockSize];
    float* rightInBuffer = new float[blockSize];
    const float* inBuffer[2] = { leftInBuffer, rightInBuffer };

    float* leftOutBuffer = new float[blockSize];
    float* rightOutBuffer = new float[blockSize];
    float* outBuffer[2] = { leftInBuffer, rightInBuffer };

    int sampleOffset = 0;
    while (sampleOffset <= numFrames) {
        int position = static_cast<float>(sampleOffset) * width / numFrames;
        for (int i = 0; i < height; i++) {
            int color = pixels[width * (height - 1 - i) + position];
            synth.setOscillatorAmplitude(
                i,
                getBlueNormalized(color) * overallGain,
                getRedNormalized(color) * overallGain
            );
        }
        synth.process(
            inChannels, outChannels, inBuffer, outBuffer, blockSize
        );
        for (int i = 0; i < blockSize; i++) {
            if (sampleOffset + i >= numFrames) {
                break;
            }
            audio[(sampleOffset + i) * 2] = inBuffer[0][i];
            audio[(sampleOffset + i) * 2 + 1] = inBuffer[1][i];
        }
        sampleOffset += blockSize;
    }

    sf_write_float(soundFile, audio, numFrames * 2);
    sf_close(soundFile);

    delete[] audio;
    delete[] leftInBuffer;
    delete[] rightInBuffer;
    delete[] leftOutBuffer;
    delete[] rightOutBuffer;

    return std::make_tuple(true, "");
}

Status loadImage(Image image, std::string fileName)
{
    uint32_t* pixels = std::get<0>(image);
    int width = std::get<1>(image);
    int height = std::get<2>(image);

    int loadedImageWidth;
    int loadedImageHeight;
    int unused;
    int channels = 3;
    unsigned char* imageData = stbi_load(
        fileName.c_str(), &loadedImageWidth, &loadedImageHeight, &unused, channels
    );
    if (imageData == nullptr) {
        return std::make_tuple(
            false,
            std::string("Image loading failed: ") + stbi_failure_reason()
        );
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int x = static_cast<float>(j) * loadedImageWidth / width;
            int y = static_cast<float>(i) * loadedImageHeight / height;
            int offset = (y * loadedImageWidth + x) * channels;
            unsigned char red = imageData[offset];
            unsigned char green = imageData[offset + 1];
            unsigned char blue = imageData[offset + 2];
            int color = 0xff000000 + (red << 16) + (green << 8) + blue;
            pixels[i * width + j] = color;
        }
    }

    stbi_image_free(imageData);

    return std::make_tuple(true, "");
}

Status saveImage(Image image, std::string fileName)
{
    uint32_t* pixels = std::get<0>(image);
    int width = std::get<1>(image);
    int height = std::get<2>(image);


    int channels = 4;

    if (!endsWith(fileName, ".png")) {
        return std::make_tuple(false, "File name must end in .png");
    }

    char* imageData = new char[height * width * channels];

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int color = pixels[i * width + j];
            int offset = (i * width + j) * channels;
            imageData[offset + 0] = getRed(color);
            imageData[offset + 1] = getGreen(color);
            imageData[offset + 2] = getBlue(color);
            imageData[offset + 3] = 255;
        }
    }

    int strideInBytes = width * channels;

    int success = stbi_write_png(
        fileName.c_str(), width, height, channels, imageData, strideInBytes
    );

    delete[] imageData;

    if (success == 0) {
        return std::make_tuple(false, "Image saving failed");
    }

    return std::make_tuple(true, "");
}

} // namespace io
