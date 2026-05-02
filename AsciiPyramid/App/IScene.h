#pragma once
#include "../Engine/AsciiShader.h"

namespace App {

class IScene {
public:
    virtual ~IScene() = default;
    virtual void Init(int bufferWidth, int bufferHeight) = 0;
    virtual void Update(float dt) = 0;
    virtual void Render(Engine::AsciiBuffer& buffer) = 0;
};

} // namespace App
