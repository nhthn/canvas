#include "common.hpp"

std::string getHomeDirectory() {
#ifdef _WIN32
    return std::string(std::getenv("HOMEDRIVE")) + std::getenv("HOMEPATH");
#else
    return getenv("HOME");
#endif // _WIN32
};

std::string getPathSeparator() {
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif // _WIN32
};

int nextPowerOfTwo(int x) {
    int power = 1;
    while (power < x) {
	power *= 2;
    }
    return power;
}

int getRed(int color) {
    return (color & 0xff0000) >> 16;
}

float getRedNormalized(int color) {
    return getRed(color) / 255.f;
}

int getGreen(int color) {
    return (color & 0x00ff00) >> 8;
}

float getGreenNormalized(int color) {
    return getGreen(color) / 255.f;
}

int getBlue(int color) {
    return ((color & 0x0000ff) >> 0);
}

float getBlueNormalized(int color) {
    return getBlue(color) / 255.f;
}

float clamp01(float x) {
    return std::max(std::min(x, 1.0f), 0.0f);
}

uint32_t colorFromNormalized(float red, float green, float blue)
{
    return (
        0xff000000
        + (static_cast<int>(clamp01(red) * 255) << 16)
        + (static_cast<int>(clamp01(green) * 255) << 8)
        + (static_cast<int>(clamp01(blue) * 255) << 0)
    );
}

// https://stackoverflow.com/a/874160
bool endsWith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return fullString.compare(
            fullString.length() - ending.length(), ending.length(), ending
        ) == 0;
    } else {
        return false;
    }
}
