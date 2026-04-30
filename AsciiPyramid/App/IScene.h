#pragma once
#include "../Engine/AsciiShader.h"

namespace App {

// Every scene fills an AsciiBuffer. That's it. Scenes are free to depend on
// whatever Engine pieces they need (or none).
class IScene {
public:
    virtual ~IScene() = default;
    virtual void Init(int bufferWidth, int bufferHeight) = 0;
    virtual void Update(float dt) = 0;
    virtual void Render(Engine::AsciiBuffer& buffer) = 0;
};

} // namespace App
