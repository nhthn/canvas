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

bool startsWith(std::string const &fullString, std::string const &beginning) {
    if (fullString.length() >= beginning.length()) {
        return fullString.compare(0, beginning.length(), beginning) == 0;
    } else {
        return false;
    }
}

// https://stackoverflow.com/a/65075284/16753552
std::vector<std::string> split(std::string string, char delimiter)
{
    std::string line;
    std::vector<std::string> result;
    std::stringstream stringStream(string);
    while (std::getline(stringStream, line, delimiter)) {
        result.push_back(line);
    }
    return result;
}

// https://stackoverflow.com/a/1798170/16753552
std::string trim(const std::string& string)
{
    const std::string whitespace = " \t";
    const auto begin = string.find_first_not_of(whitespace);
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = string.find_last_not_of(whitespace);
    const auto range = end - begin + 1;
    return string.substr(begin, range);
}
