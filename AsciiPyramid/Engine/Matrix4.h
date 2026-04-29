#pragma once
#include "Vector3.h"
#include <cmath>

namespace Engine {

struct Matrix4 {
    float m[4][4]{};

    static Matrix4 Identity() {
        Matrix4 r{};
        for (int i = 0; i < 4; ++i) r.m[i][i] = 1.0f;
        return r;
    }

    static Matrix4 RotationY(float a) {
        Matrix4 r = Identity();
        float c = std::cos(a), s = std::sin(a);
        r.m[0][0] =  c; r.m[0][2] = s;
        r.m[2][0] = -s; r.m[2][2] = c;
        return r;
    }

    static Matrix4 RotationX(float a) {
        Matrix4 r = Identity();
        float c = std::cos(a), s = std::sin(a);
        r.m[1][1] = c; r.m[1][2] = -s;
        r.m[2][1] = s; r.m[2][2] =  c;
        return r;
    }

    static Matrix4 Translation(float tx, float ty, float tz) {
        Matrix4 r = Identity();
        r.m[0][3] = tx; r.m[1][3] = ty; r.m[2][3] = tz;
        return r;
    }

    // Right-handed perspective; produces projected x,y in clip space and depth in z.
    static Matrix4 Perspective(float fovRadians, float aspect, float nearZ, float farZ) {
        Matrix4 r{};
        float f = 1.0f / std::tan(fovRadians * 0.5f);
        r.m[0][0] = f / aspect;
        r.m[1][1] = f;
        r.m[2][2] = (farZ + nearZ) / (nearZ - farZ);
        r.m[2][3] = (2.0f * farZ * nearZ) / (nearZ - farZ);
        r.m[3][2] = -1.0f;
        return r;
    }

    Matrix4 MultiplyMatrix(const Matrix4& o) const {
        Matrix4 r{};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) sum += m[i][k] * o.m[k][j];
                r.m[i][j] = sum;
            }
        return r;
    }

    // Transforms as a point (w=1), returns NDC after perspective divide skipped here.
    Vector3 MultiplyVector(const Vector3& v) const {
        float x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3];
        float y = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3];
        float z = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3];
        return { x, y, z };
    }

    // For the projection step we need w too.
    void MultiplyVectorW(const Vector3& v, float outXYZW[4]) const {
        outXYZW[0] = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3];
        outXYZW[1] = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3];
        outXYZW[2] = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3];
        outXYZW[3] = m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3];
    }
};

} // namespace Engine
