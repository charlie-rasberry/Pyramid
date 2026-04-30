#pragma once
#include "../Engine/Matrix4.h"
#include "../Engine/Mesh.h"
#include "../Engine/Renderer.h"
#include "IScene.h"

namespace App {

class PyramidScene : public IScene {
public:
    void Init(int bufferWidth, int bufferHeight) override {
        pyramid = Engine::Mesh::CreatePyramid();
        rotation = 0.0f;
        float aspect = (float)bufferWidth / (float)(bufferHeight * 2);
        renderer.projection = Engine::Matrix4::Perspective(
            60.0f * 3.14159265f / 180.0f, aspect, 0.1f, 100.0f);
    }

    void Update(float dt) override {
        rotation += dt;
    }

    void Render(Engine::AsciiBuffer& buffer) override {
        Engine::Matrix4 ry = Engine::Matrix4::RotationY(rotation);
        Engine::Matrix4 rx = Engine::Matrix4::RotationX(rotation * 0.5f);
        Engine::Matrix4 t  = Engine::Matrix4::Translation(0, 0, -5);
        Engine::Matrix4 transform = t.MultiplyMatrix(ry.MultiplyMatrix(rx));
        renderer.Render(pyramid, transform, buffer, Engine::Color::Default);
    }

private:
    Engine::Mesh     pyramid;
    Engine::Renderer renderer;
    float            rotation = 0.0f;
};

} // namespace App
