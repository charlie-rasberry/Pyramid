#pragma once
#include <limits>
#include <string>
#include <vector>
#include <cstdint>

namespace Engine {

class AsciiBuffer {
public:
    int width = 0, height = 0;
    std::vector<char>  chars;
    std::vector<float> depth;

    AsciiBuffer() = default;
    AsciiBuffer(int w, int h) : width(w), height(h),
                                chars(w * h, ' '),
                                depth(w * h, std::numeric_limits<float>::infinity()) {}

    void Clear() {
        std::fill(chars.begin(), chars.end(), ' ');
        std::fill(depth.begin(), depth.end(), std::numeric_limits<float>::infinity());
    }

    inline void SetPixel(int x, int y, float z, char c) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int i = y * width + x;
        if (z < depth[i]) {
            depth[i] = z;
            chars[i] = c;
        }
    }

    uint64_t Hash() const {
        uint64_t h = 1469598103934665603ull;
        for (char c : chars) { h ^= (uint8_t)c; h *= 1099511628211ull; }
            return h;
    }

};

class AsciiShader {
public:
    // Dark to light. Index 0 = unlit, last = brightest.
    std::string gradient = " .:-=+*#%@";

    char GetCharFromBrightness(float b) const {
        if (b < 0.0f) b = 0.0f;
        if (b > 1.0f) b = 1.0f;
        int idx = static_cast<int>(b * (gradient.size() - 1) + 0.5f);
        return gradient[idx];
    }
};

} // namespace Engine
