#include "filters.hpp"

namespace filters {

void clear(Image image)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    for (int i = 0; i < height * width; i++) {
        pixels[i] = 0xff000000;
    }
}

void applyInvert(Image image)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    for (int i = 0; i < height * width; i++) {
        pixels[i] ^= 0x00ffffff;
    }
}

void applyReverb(Image image, float decay, float damping, bool reverse)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    float baseDecayLength = 1 + (decay * width * 2);
    for (int row = 0; row < height; row++) {
        float decayLength = (
            baseDecayLength * std::pow(static_cast<float>(row) / height, damping)
        );
        float k = std::pow(0.001f, 1.0f / decayLength);
        float lastRed = 0;
        float lastGreen = 0;
        float lastBlue = 0;
        for (int column = 0; column < width; column++) {
            int index = (
                row * width
                + (reverse ? width - 1 - column : column)
            );
            int color = pixels[index];
            float red = getRedNormalized(color);
            float green = getGreenNormalized(color);
            float blue = getBlueNormalized(color);
            red = std::max(lastRed * k, red);
            green = std::max(lastGreen * k, green);
            blue = std::max(lastBlue * k, blue);
            pixels[index] = colorFromNormalized(red, green, blue);
            lastRed = red;
            lastGreen = green;
            lastBlue = blue;
        }
    }
}


class RandomLFO {
public:
    RandomLFO(std::mt19937& rng, int period)
        : m_rng(rng), m_period(period), m_distribution(0.0, 1.0)
    {
        m_current = m_distribution(rng);
        m_target = m_distribution(rng);
        m_t = 0;
    }

    float process()
    {
        float value = (
            m_current * static_cast<float>(m_period - m_t) / m_period
            + m_target * static_cast<float>(m_t) / m_period
        );
        m_t += 1;
        if (m_t >= m_period) {
            m_t = 0;
            m_current = m_target;
            m_target = m_distribution(m_rng);
        }
        return value;
    }

private:
    std::mt19937 m_rng;
    int m_period;
    std::uniform_real_distribution<> m_distribution;
    float m_current;
    float m_target;
    int m_t;
};


void applyChorus(Image image, float rate, float depth)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    for (int row = 0; row < height; row++) {
        std::random_device randomDevice;
        std::mt19937 rng(randomDevice());
        int lfoPeriod = 1000.f / (height - 1 - row) / (0.05 + rate);
        RandomLFO lfoRed(rng, lfoPeriod);
        RandomLFO lfoGreen(rng, lfoPeriod);
        RandomLFO lfoBlue(rng, lfoPeriod);
        for (int column = 0; column < width; column++) {
            int index = row * width + column;
            int color = pixels[index];
            float red = getRedNormalized(color);
            float green = getGreenNormalized(color);
            float blue = getBlueNormalized(color);

            red *= 1 - lfoRed.process() * depth;
            green *= 1 - lfoGreen.process() * depth;
            blue *= 1 - lfoBlue.process() * depth;

            color = colorFromNormalized(red, green, blue);
            pixels[index] = color;
        }
    }
}


void applyScaleFilter(Image image, int root, int scaleClass)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    int scale[][12] = {
        { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 }, // Major
        { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 }, // Minor
        { 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0 }, // Acoustic
        { 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1 }, // Harmonic Major
        { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1 }, // Harmonic Minor
        { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // Whole Tone
        { 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 }, // Octatonic
        { 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1 }, // Hexatonic (Messiaen)
    };
    for (int row = 0; row < height; row++) {
        // Subtract 6 because lowest frequency is A.
        int stepsIn24EDO = height - 1 - row - 6;
        int offsetFromRoot = ((stepsIn24EDO / 2 - root) % 12 + 12) % 12;
        if (stepsIn24EDO % 2 != 0 || scale[scaleClass][offsetFromRoot] == 0) {
            for (int column = 0; column < width; column++) {
                pixels[row * width + column] = 0;
            }
        }
    }
}

static float tremoloLFO(float phase, int shape)
{
    switch (shape) {
    case 0: // Sine
        return std::cos(phase * 3.14159265358979 * 2) * 0.5 + 0.5;
    case 1: // Triangle
        return phase >= 0.5 ? 2 - 2 * phase : 2 * phase;
    case 2: // Square
        return phase < 0.5;
    case 3: // Saw Down -- squaring the signal sounds better
        return (1 - phase) * (1 - phase);
    case 4: // Saw Up
        return phase;
    }
    return 0;
}

void applyTremolo(Image image, float rate, float depth, int shape, float stereo)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    for (int row = 0; row < height; row++) {
        float lfoPhase = 0;
        float lfoPeriod = std::pow(width, 1 - rate);
        float phaseIncrement = 1 / lfoPeriod;
        for (int column = 0; column < width; column++) {
            float lfo1 = tremoloLFO(lfoPhase, shape);
            float lfo2 = tremoloLFO(lfoPhase + 0.5 * stereo, shape);
            lfoPhase += phaseIncrement;
            while (lfoPhase > 1.0) {
                lfoPhase -= 1.0;
            }

            int index = row * width + column;
            int color = pixels[index];
            float red = getRedNormalized(color);
            float green = getGreenNormalized(color);
            float blue = getBlueNormalized(color);

            red *= 1 - (1 - lfo1) * depth;
            green *= 1 - (1 - (lfo1 + lfo2) * 0.5) * depth;
            blue *= 1 - (1 - lfo2) * depth;

            color = colorFromNormalized(red, green, blue);
            pixels[index] = color;
        }
    }
}

void applyHarmonics(
    Image image,
    float amplitude2,
    float amplitude3,
    float amplitude4,
    float amplitude5,
    bool subharmonics
)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++) {
            float red2 = 0, green2 = 0, blue2 = 0;
            float red3 = 0, green3 = 0, blue3 = 0;
            float red4 = 0, green4 = 0, blue4 = 0;
            float red5 = 0, green5 = 0, blue5 = 0;

            int sign = subharmonics ? -1 : 1;

            int index2 = (row + 12 * 2 * sign) * width + column;
            if (0 <= index2 && index2 < width * height) {
                int color2 = pixels[index2];
                red2 = getRedNormalized(color2);
                green2 = getGreenNormalized(color2);
                blue2 = getBlueNormalized(color2);
            }

            int index3 = (row + (12 + 7) * 2 * sign) * width + column;
            if (0 <= index3 && index3 < width * height) {
                int color3 = pixels[index3];
                red3 = getRedNormalized(color3);
                green3 = getGreenNormalized(color3);
                blue3 = getBlueNormalized(color3);
            }

            int index4 = (row + 24 * 2 * sign) * width + column;
            if (0 <= index4 && index4 < width * height) {
                int color4 = pixels[index4];
                red4 = getRedNormalized(color4);
                green4 = getGreenNormalized(color4);
                blue4 = getBlueNormalized(color4);
            }

            int index5 = (row + (24 + 4) * 2 * sign) * width + column;
            if (0 <= index5 && index5 < width * height) {
                int color5 = pixels[index5];
                red5 = getRedNormalized(color5);
                green5 = getGreenNormalized(color5);
                blue5 = getBlueNormalized(color5);
            }

            int index = row * width + column;
            int color = pixels[index];
            float red = getRedNormalized(color);
            float green = getGreenNormalized(color);
            float blue = getBlueNormalized(color);

            red += (
                red2 * amplitude2
                + red3 * amplitude3
                + red4 * amplitude4
                + red5 * amplitude5
            );
            green += (
                green2 * amplitude2
                + green3 * amplitude3
                + green4 * amplitude4
                + green5 * amplitude5
            );
            blue += (
                blue2 * amplitude2
                + blue3 * amplitude3
                + blue4 * amplitude4
                + blue5 * amplitude5
            );

            color = colorFromNormalized(red, green, blue);
            pixels[index] = color;
        }
    }
}

} // namespace filters
