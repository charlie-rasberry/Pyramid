#pragma once
#include "Vector3.h"
#include <array>
#include <vector>

namespace Engine {

struct Mesh {
    std::vector<Vector3> vertices;
    std::vector<std::array<int, 3>> faces;

    static Mesh CreatePyramid() {
        Mesh m;
        m.vertices = {
            {-1, -1, -1}, { 1, -1, -1}, { 1, -1,  1}, {-1, -1,  1},
            { 0,  1,  0}
        };
        m.faces = {
            {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4},
            // base — wind it so the outward normal points down (-Y).
            {0, 2, 1}, {0, 3, 2}
        };
        return m;
    }
};

} // namespace Engine
