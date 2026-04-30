#pragma once
#include "AsciiShader.h"
#include "Matrix4.h"
#include "Mesh.h"
#include "Vector3.h"
#include <algorithm>

namespace Engine {

class Renderer {
public:
    Matrix4 projection = Matrix4::Identity();
    Vector3 lightDirection = Vector3(0.5f, 0.8f, -0.3f).Normalize();
    AsciiShader shader;

    float charAspect = 2.0f;

    void Render(const Mesh& mesh, const Matrix4& world, AsciiBuffer& buffer,
                uint8_t meshColor = Color::Default) const {
        for (const auto& face : mesh.faces) {
            Vector3 w0 = world.MultiplyVector(mesh.vertices[face[0]]);
            Vector3 w1 = world.MultiplyVector(mesh.vertices[face[1]]);
            Vector3 w2 = world.MultiplyVector(mesh.vertices[face[2]]);

            Vector3 normal = w1.Subtract(w0).Cross(w2.Subtract(w0)).Normalize();

            Vector3 viewDir = Vector3(0, 0, 1);
            if (normal.Dot(viewDir) <= 0.0f) continue;

            float brightness = std::max(0.0f, normal.Dot(lightDirection)) * 0.85f + 0.15f;
            char c = shader.GetCharFromBrightness(brightness);

            float p0[4], p1[4], p2[4];
            projection.MultiplyVectorW(w0, p0);
            projection.MultiplyVectorW(w1, p1);
            projection.MultiplyVectorW(w2, p2);

            if (p0[3] <= 0.01f || p1[3] <= 0.01f || p2[3] <= 0.01f) continue;

            auto toScreen = [&](float p[4], float& sx, float& sy, float& sz) {
                float ndcX = p[0] / p[3];
                float ndcY = p[1] / p[3];
                float ndcZ = p[2] / p[3];
                sx = (ndcX * 0.5f + 0.5f) * buffer.width;
                sy = (1.0f - (ndcY * 0.5f + 0.5f)) * buffer.height;
                sz = ndcZ;
            };

            float x0, y0, z0, x1, y1, z1, x2, y2, z2;
            toScreen(p0, x0, y0, z0);
            toScreen(p1, x1, y1, z1);
            toScreen(p2, x2, y2, z2);

            RasterizeTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2, c, meshColor, buffer);
        }
    }

private:
    static float EdgeFunction(float ax, float ay, float bx, float by, float cx, float cy) {
        return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
    }

    void RasterizeTriangle(float x0, float y0, float z0,
                           float x1, float y1, float z1,
                           float x2, float y2, float z2,
                           char c, uint8_t col, AsciiBuffer& buffer) const {
        int minX = std::max(0, (int)std::floor(std::min({x0, x1, x2})));
        int maxX = std::min(buffer.width  - 1, (int)std::ceil (std::max({x0, x1, x2})));
        int minY = std::max(0, (int)std::floor(std::min({y0, y1, y2})));
        int maxY = std::min(buffer.height - 1, (int)std::ceil (std::max({y0, y1, y2})));

        float area = EdgeFunction(x0, y0, x1, y1, x2, y2);
        if (std::abs(area) < 1e-6f) return;
        float invArea = 1.0f / area;

        for (int py = minY; py <= maxY; ++py) {
            for (int px = minX; px <= maxX; ++px) {
                float sx = px + 0.5f;
                float sy = py + 0.5f;
                float w0 = EdgeFunction(x1, y1, x2, y2, sx, sy) * invArea;
                float w1 = EdgeFunction(x2, y2, x0, y0, sx, sy) * invArea;
                float w2 = EdgeFunction(x0, y0, x1, y1, sx, sy) * invArea;

                bool inside = (w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                              (w0 <= 0 && w1 <= 0 && w2 <= 0);
                if (!inside) continue;

                float depth = w0 * z0 + w1 * z1 + w2 * z2;
                buffer.SetPixel(px, py, depth, c, col);
            }
        }
    }
};

} // namespace Engine
