#pragma once
#include <tuple>

#include "common.hpp"

namespace io {

using Status = std::tuple<bool, std::string>;

Status loadAudio(Image image, std::string fileName);
Status renderAudio(
    Image image,
    std::string fileName,
    float sampleRate,
    float overallGain,
    float speedInPixelsPerSecond,
    float m_pdMode,
    float m_pdDistort
);
Status loadImage(Image image, std::string fileName);
Status saveImage(Image image, std::string fileName);

} // namespace io
