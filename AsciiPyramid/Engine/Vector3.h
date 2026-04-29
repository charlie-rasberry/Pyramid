#pragma once
#include <cmath>

namespace Engine {

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vector3 Add(const Vector3& v)        const { return { x + v.x, y + v.y, z + v.z }; }
    Vector3 Subtract(const Vector3& v)   const { return { x - v.x, y - v.y, z - v.z }; }
    Vector3 MultiplyScalar(float s)      const { return { x * s, y * s, z * s }; }
    float   Dot(const Vector3& v)        const { return x * v.x + y * v.y + z * v.z; }

    Vector3 Cross(const Vector3& v) const {
        return { y * v.z - z * v.y,
                 z * v.x - x * v.z,
                 x * v.y - y * v.x };
    }

    Vector3 Normalize() const {
        float len = std::sqrt(x * x + y * y + z * z);
        if (len < 1e-6f) return { 0, 0, 0 };
        return { x / len, y / len, z / len };
    }
};

} // namespace Engine
