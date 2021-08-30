#pragma once
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using Image = std::tuple<uint32_t*, int, int>;

std::string getHomeDirectory();
std::string getPathSeparator();

int nextPowerOfTwo(int x);

int getRed(int color);
float getRedNormalized(int color);
int getGreen(int color);
float getGreenNormalized(int color);
int getBlue(int color);
float getBlueNormalized(int color);
float clamp01(float x);
uint32_t colorFromNormalized(float red, float green, float blue);

template <class T>
T clamp(T x, T min, T max)
{
    return std::max(std::min(x, max), min);
}

bool endsWith(std::string const &fullString, std::string const &ending);
bool startsWith(std::string const &fullString, std::string const &beginning);
std::vector<std::string> split(std::string text, char delimiter);
std::string trim(const std::string& str);
