#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <lgfx/utility/lgfx_pngle.h>

namespace vu_meter {
constexpr int maskX = 94, maskY = 178, maskW = 132, maskH = 62;
// Decode the PNG once, preserving alpha instead of flattening onto the dial.
struct MaskDecode {
    const uint8_t* png;
    size_t size, position;
    uint32_t* pixels; // 0xAARRGGBB
};
inline uint32_t readMask(void* data, uint8_t* dst, uint32_t len) {
    auto& d = *static_cast<MaskDecode*>(data);
    len = std::min<size_t>(len, d.size - d.position);
    if (dst) std::memcpy(dst, d.png + d.position, len);
    d.position += len;
    return len;
}
inline void storeMask(void* data, uint32_t x, uint32_t y,
                      uint_fast8_t step, size_t len, const uint8_t* argb) {
    auto& d = *static_cast<MaskDecode*>(data);
    if (y < maskY || y >= maskY + maskH) return;
    for (size_t i = 0; i < len; ++i, x += step, argb += 4) {
        if (x < maskX || x >= maskX + maskW) continue;
        d.pixels[(y-maskY)*maskW + x-maskX] = (uint32_t(argb[0]) << 24)
            | (uint32_t(argb[1]) << 16) | (uint32_t(argb[2]) << 8) | argb[3];
    }
}
inline bool decodeMask(const uint8_t* png, size_t size, uint32_t* pixels) {
    std::memset(pixels, 0, maskW * maskH * sizeof(uint32_t));
    MaskDecode data{png, size, 0, pixels};
    auto* decoder = lgfx_pngle_new();
    if (!decoder) return false;
    const bool ok = lgfx_pngle_prepare(decoder, readMask, &data) >= 0
        && lgfx_pngle_get_width(decoder) == 320 && lgfx_pngle_get_height(decoder) == 240
        && lgfx_pngle_decomp(decoder, storeMask) >= 0;
    lgfx_pngle_destroy(decoder);
    return ok;
}
// LGFX 16-bit sprite buffers use byte-swapped RGB565. Transparent pixels
// are untouched; partially transparent edges blend with the current needle.
inline uint16_t blendMaskPixel(uint16_t swapped, uint32_t argb) {
    const unsigned a = argb >> 24;
    if (!a) return swapped;
    if (a == 255) {
        const uint16_t out = (((argb >> 19) & 31) << 11)
            | (((argb >> 10) & 63) << 5) | ((argb >> 3) & 31);
        return (out >> 8) | (out << 8);
    }
    const uint16_t rgb = (swapped >> 8) | (swapped << 8);
    const unsigned inv = 255 - a;
    const unsigned r = (((argb >> 16) & 255) * a + ((rgb >> 11) * 255 / 31) * inv + 127) / 255;
    const unsigned g = (((argb >> 8) & 255) * a + (((rgb >> 5) & 63) * 255 / 63) * inv + 127) / 255;
    const unsigned b = ((argb & 255) * a + ((rgb & 31) * 255 / 31) * inv + 127) / 255;
    const uint16_t out = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    return (out >> 8) | (out << 8);
}
inline void compositeMask(uint16_t* frame, int width, int height,
                          int x, int y, const uint32_t* pixels) {
    for (int row = 0; row < maskH; ++row) {
        if (row+y < 0 || row+y >= height) continue;
        for (int col = 0; col < maskW; ++col) {
            if (col+x < 0 || col+x >= width) continue;
            auto& dst = frame[(row+y)*width + col+x];
            dst = blendMaskPixel(dst, pixels[row*maskW+col]);
        }
    }
}
} // namespace vu_meter
