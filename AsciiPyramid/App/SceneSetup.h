#pragma once
#include "../Engine/Matrix4.h"
#include "../Engine/Mesh.h"

namespace App {

class Scene {
public:
    Engine::Mesh pyramid = Engine::Mesh::CreatePyramid();
    float rotation = 0.0f;

    void Update(float dt) {
        rotation += dt;
    }

    Engine::Matrix4 GetTransform() const {
        Engine::Matrix4 ry = Engine::Matrix4::RotationY(rotation);
        Engine::Matrix4 rx = Engine::Matrix4::RotationX(rotation * 0.5f);
        Engine::Matrix4 t  = Engine::Matrix4::Translation(0, 0, -5);
        return t.MultiplyMatrix(ry.MultiplyMatrix(rx));
    }
};

} // namespace App
