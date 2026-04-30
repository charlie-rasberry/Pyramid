#pragma once
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace Engine {

// Palette indices. Scenes pick from these; the renderer maps them to brushes.
namespace Color {
    constexpr uint8_t Default = 0;  // green (matches existing pyramid look)
    constexpr uint8_t White   = 1;
    constexpr uint8_t Red     = 2;
    constexpr uint8_t Orange  = 3;
    constexpr uint8_t Yellow  = 4;
    constexpr uint8_t Green   = 5;
    constexpr uint8_t Blue    = 6;
    constexpr uint8_t Violet  = 7;
    constexpr uint8_t Count   = 8;
}

class AsciiBuffer {
public:
    int width = 0, height = 0;
    std::vector<char>    chars;
    std::vector<uint8_t> color;   // palette index per cell
    std::vector<float>   depth;

    AsciiBuffer() = default;
    AsciiBuffer(int w, int h)
        : width(w), height(h),
          chars(w * h, ' '),
          color(w * h, Color::Default),
          depth(w * h, std::numeric_limits<float>::infinity()) {}

    void Clear() {
        std::fill(chars.begin(), chars.end(), ' ');
        std::fill(color.begin(), color.end(), Color::Default);
        std::fill(depth.begin(), depth.end(), std::numeric_limits<float>::infinity());
    }

    inline void SetPixel(int x, int y, float z, char c, uint8_t col = Color::Default) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int i = y * width + x;
        if (z < depth[i]) {
            depth[i]  = z;
            chars[i]  = c;
            color[i]  = col;
        }
    }

    // 2D scenes use this to write without depth testing.
    inline void Plot(int x, int y, char c, uint8_t col = Color::Default) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int i = y * width + x;
        chars[i] = c;
        color[i] = col;
        depth[i] = -1.0f;  // ensure 3D rasterizer doesn't overwrite later
    }

    uint64_t Hash() const {
        uint64_t h = 1469598103934665603ull;
        for (size_t i = 0; i < chars.size(); ++i) {
            h ^= (uint8_t)chars[i]; h *= 1099511628211ull;
            h ^= color[i];          h *= 1099511628211ull;
        }
        return h;
    }
};

class AsciiShader {
public:
    std::string gradient = " .:-=+*#%@";

    char GetCharFromBrightness(float b) const {
        if (b < 0.0f) b = 0.0f;
        if (b > 1.0f) b = 1.0f;
        int idx = static_cast<int>(b * (gradient.size() - 1) + 0.5f);
        return gradient[idx];
    }
};

} // namespace Engine
