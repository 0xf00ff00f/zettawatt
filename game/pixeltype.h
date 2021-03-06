#pragma once

#include <cstddef>

namespace GX {

enum class PixelType {
    Invalid,
    RGBA,
    Grayscale
};

constexpr std::size_t pixelSizeInBytes(PixelType pixelType)
{
    switch (pixelType) {
    case PixelType::RGBA:
        return 4;

    case PixelType::Grayscale:
    case PixelType::Invalid:
    default:
        return 1;
    }
}

} // namespace GX
