#pragma once
#include "../Engine/AsciiShader.h"
#include "IScene.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace App {

// Pink Floyd "Dark Side of the Moon" album cover, ASCII edition.
// White beam from the left, dark prism in the middle, rainbow fanning to the
// lower right. Rainbow shimmers; prism wobbles slightly.
class PrismScene : public IScene {
public:
    void Init(int w, int h) override {
        bw = w;
        bh = h;
        time = 0.0f;
    }

    void Update(float dt) override {
        time += dt;
    }

    void Render(Engine::AsciiBuffer& buffer) override {
        // Wobble: vertical offset of the prism centre, in cells.
        float wobble = std::sin(time * 0.8f) * 1.0f;

        // Prism geometry. We treat the buffer as a 2D canvas. Cell aspect
        // ratio is ~2 (cells are twice as tall as wide), so a visually
        // equilateral triangle needs width ~= 2 * height in cells.
        float cx = bw * 0.45f;             // a bit left of centre
        float cy = bh * 0.5f + wobble;     // vertical wobble
        float halfH = bh * 0.32f;
        float halfW = halfH * 1.7f;        // accounts for cell aspect

        // Triangle vertices (apex at top, base at bottom).
        float ax = cx,         ay = cy - halfH;          // apex
        float bx = cx - halfW, by = cy + halfH;          // bottom-left
        float cx2 = cx + halfW, cy2 = cy + halfH;        // bottom-right

        // Where the beam hits the left face (about 40% down from apex).
        float hitT = 0.45f;
        float hitX = ax + (bx - ax) * hitT;
        float hitY = ay + (by - ay) * hitT;

        // Where each rainbow band exits the right face. We spread the exit
        // points across the right face and have them fan out at slightly
        // different angles to sell the dispersion.
        // Right face goes from apex (ax, ay) to bottom-right (cx2, cy2).
        const int bandCount = 6;
        const uint8_t bandColors[bandCount] = {
            Engine::Color::Red,
            Engine::Color::Orange,
            Engine::Color::Yellow,
            Engine::Color::Green,
            Engine::Color::Blue,
            Engine::Color::Violet,
        };

        // 1. Background stays cleared (transparent).

        // 2. Incoming white beam: from left edge to hit point.
        DrawLine(buffer, 0, hitY, hitX, hitY, '-', Engine::Color::White);

        // 3. Rainbow bands fanning out from the right face (drawn first so
        //    the prism outline sits on top of them).
        for (int i = 0; i < bandCount; ++i) {
            float t = 0.30f + (i / (float)(bandCount - 1)) * 0.40f;  // 0.30..0.70 along right face
            float exitX = ax + (cx2 - ax) * t;
            float exitY = ay + (cy2 - ay) * t;

            // Each band exits at a slightly different angle to fan out.
            // Top band (red) goes more horizontal; bottom (violet) dips down.
            float angleDeg = -2.0f + i * 4.0f;  // roughly -2..+18 degrees from horizontal
            float angleRad = angleDeg * 3.14159265f / 180.0f;

            // Project all the way to the right edge of the buffer.
            float dx = std::cos(angleRad);
            float dy = std::sin(angleRad);
            float reach = (bw - exitX) / dx;
            float endX = exitX + dx * reach;
            float endY = exitY + dy * reach;

            DrawShimmerLine(buffer, exitX, exitY, endX, endY, bandColors[i], i);
        }

        // 4. Prism outline last so it overdraws any rainbow that strayed inside.
        DrawLine(buffer, ax, ay, bx, by,   '/', Engine::Color::White);
        DrawLine(buffer, bx, by, cx2, cy2, '_', Engine::Color::White);
        DrawLine(buffer, cx2, cy2, ax, ay, '\\', Engine::Color::White);
    }

private:
    int   bw = 0, bh = 0;
    float time = 0.0f;

    // Bresenham-ish line, single character.
    static void DrawLine(Engine::AsciiBuffer& buf,
                         float x0, float y0, float x1, float y1,
                         char ch, uint8_t col) {
        float dx = x1 - x0, dy = y1 - y0;
        float steps = std::max(std::abs(dx), std::abs(dy));
        if (steps < 1.0f) return;
        for (int i = 0; i <= (int)steps; ++i) {
            float t = i / steps;
            int px = (int)(x0 + dx * t + 0.5f);
            int py = (int)(y0 + dy * t + 0.5f);
            buf.Plot(px, py, ch, col);
        }
    }

    // Rainbow band: thicker line with shimmer.
    void DrawShimmerLine(Engine::AsciiBuffer& buf,
                         float x0, float y0, float x1, float y1,
                         uint8_t col, int bandIdx) {
        // Density characters from heavy to light. Rainbow shimmers between these.
        const char* dense  = "##@%*=";
        const char* medium = "+=*o.";
        const char* sparse = "-:.,";

        float dx = x1 - x0, dy = y1 - y0;
        float steps = std::max(std::abs(dx), std::abs(dy));
        if (steps < 1.0f) return;

        for (int i = 0; i <= (int)steps; ++i) {
            float t = i / steps;
            float fx = x0 + dx * t;
            float fy = y0 + dy * t;

            // Draw a 2-cell-thick band (current row + one below).
            for (int thick = 0; thick < 2; ++thick) {
                int px = (int)(fx + 0.5f);
                int py = (int)(fy + 0.5f) + thick;

                // Shimmer: pick a character based on a hash of position + time.
                // Density falls off near the start (closer to prism = denser).
                float density = 1.0f - t * 0.7f;
                float n = Hash(px, py, bandIdx, (int)(time * 12.0f));

                const char* set;
                if (n < density * 0.5f)       set = dense;
                else if (n < density)         set = medium;
                else                          set = sparse;

                size_t setLen = std::strlen(set);
                char ch = set[(int)(n * 1000.0f) % setLen];

                buf.Plot(px, py, ch, col);
            }
        }
    }

    // Cheap hash → [0,1).
    static float Hash(int x, int y, int z, int t) {
        uint32_t h = (uint32_t)x * 374761393u
                   + (uint32_t)y * 668265263u
                   + (uint32_t)z * 2147483647u
                   + (uint32_t)t * 1274126177u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h = h ^ (h >> 16);
        return (h & 0xFFFFFF) / (float)0x1000000;
    }
};

} // namespace App
