#include "draw.hpp"

namespace draw {

void drawPixel(
    Image image, int x, int y, float red, float green, float blue, float alpha
)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    if (!((0 <= y) && (y < height))) {
        return;
    }
    if (!((0 <= x) && (x < width))) {
        return;
    }
    int index = y * width + x;
    int color = pixels[index];
    float newRed = getRedNormalized(color);
    float newGreen = getGreenNormalized(color);
    float newBlue = getBlueNormalized(color);
    newRed = newRed * (1 - alpha) + red * alpha;
    newGreen = newGreen * (1 - alpha) + green * alpha;
    newBlue = newBlue * (1 - alpha) + blue * alpha;
    pixels[index] = colorFromNormalized(newRed, newGreen, newBlue);
}

void drawFuzzyCircle(
    Image image, int x, int y, int radius, float red, float green, float blue, float alpha
)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (dx * dx + dy * dy <= radius * radius) {
                float pixelRadius = std::sqrt(dx * dx + dy * dy);
                float pixelAlpha = alpha * (1 - pixelRadius / radius);
                drawPixel(image, x + dx, y + dy, red, green, blue, pixelAlpha);
            }
        }
    }
}

void drawLine(
    Image image, int x1, int y1, int x2, int y2, int radius, float red, float green, float blue, float alpha
)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        drawFuzzyCircle(image, x1, y1, radius, red, green, blue, alpha);
        return;
    }
    if (std::abs(dx) > std::abs(dy)) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
            float y = y1 + std::round(static_cast<float>(x - x1) * dy / dx);
            drawFuzzyCircle(image, x, static_cast<int>(y), radius, red, green, blue, alpha);
        }
    } else {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
            float x = x1 + std::round(static_cast<float>(y - y1) * dx / dy);
            drawFuzzyCircle(image, static_cast<int>(x), y, radius, red, green, blue, alpha);
        }
    }
}

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
)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    std::uniform_real_distribution<> bipolar(-1.0, 1.0);
    std::uniform_real_distribution<> unipolar(0.0, 1.0);
    for (int i = 0; i < radius * radius * density; i++) {
        float dx = radius * (
            bipolar(randomEngine)
            + bipolar(randomEngine)
            + bipolar(randomEngine)
        ) / 3.0f;
        float dy = radius * (
            bipolar(randomEngine)
            + bipolar(randomEngine)
            + bipolar(randomEngine)
        ) / 3.0f;
        float pixelAlpha = alpha * unipolar(randomEngine);
        drawPixel(image, x + dx, y + dy, red, green, blue, pixelAlpha);
    }
}

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
)
{
    auto pixels = std::get<0>(image);
    auto width = std::get<1>(image);
    auto height = std::get<2>(image);

    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        spray(image, x1, y1, radius, density, red, green, blue, alpha, randomEngine);
        return;
    }
    if (std::abs(dx) > std::abs(dy)) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
            float y = y1 + std::round(static_cast<float>(x - x1) * dy / dx);
            spray(
                image, x, static_cast<int>(y), radius, density, red, green, blue, alpha, randomEngine
            );
        }
    } else {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
            float x = x1 + std::round(static_cast<float>(y - y1) * dx / dy);
            spray(
                image, static_cast<int>(x), y, radius, density, red, green, blue, alpha, randomEngine
            );
        }
    }
}

} // namespace draw
