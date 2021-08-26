#pragma once
#include <tuple>
#include <random>
#include <algorithm>

#include "common.hpp"

namespace filters {

void clear(Image image);
void applyInvert(Image image);
void applyScaleFilter(Image image, int root, int scaleClass);
void applyReverb(Image image, float decay, float damping, bool reverse);
void applyChorus(Image image, float rate, float depth);
void applyTremolo(Image image, float rate, float depth, int shape, float stereo);
void applyHarmonics(
    Image image,
    float amplitude2,
    float amplitude3,
    float amplitude4,
    float amplitude5,
    bool subharmonics
);

} // namespace filters
