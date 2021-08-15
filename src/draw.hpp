#pragma once
#include <random>
#include "common.hpp"

namespace draw {

void drawPixel(
    Image image, int x, int y, float red, float green, float blue, float alpha
);

void drawFuzzyCircle(
    Image image, int x, int y, int radius, float red, float green, float blue, float alpha
);

void drawLine(
    Image image, int x1, int y1, int x2, int y2, int radius, float red, float green, float blue, float alpha
);

void spray(
    Image image,
    int x,
    int y,
    float radius,
    float density,
    float red,
    float green,
    float blue,
    float alpha,
    std::mt19937& randomEngine
);

void sprayLine(
    Image image,
    int x1,
    int y1,
    int x2,
    int y2,
    int radius,
    float density,
    float red,
    float green,
    float blue,
    float alpha,
    std::mt19937& randomEngine
);

} // namespace draw
