// AsciiPyramid - DirectComposition build.
// Strategy: create our own borderless full-screen HWND, parent it to WorkerW,
// bind a DComp target to our window. This avoids the cross-process
// E_ACCESSDENIED that hits when binding directly to WorkerW.

#include "../Engine/Renderer.h"
#include "../Platform/DcompRenderer.h"
#include "../Platform/DesktopWindow.h"
#include "../Platform/WorkerWManager.h"
#include "SceneSetup.h"

#include <chrono>
#include <cstdio>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <combaseapi.h>

#pragma comment(lib, "ole32.lib")

static constexpr int kBufferWidth  = 120;
static constexpr int kBufferHeight = 40;

static volatile bool g_running = true;

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT ||
        signal == CTRL_BREAK_EVENT) {
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

int main() {
    ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    HRESULT coHr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(coHr) && coHr != RPC_E_CHANGED_MODE) {
        std::printf("[diag] CoInitializeEx failed 0x%lX\n", (unsigned long)coHr);
        return 1;
    }

    HWND workerW = Platform::WorkerWManager::GetWorkerW();
    if (!workerW) {
        std::printf("Could not locate WorkerW. Is Explorer running?\n");
        return 1;
    }
    char cls[64] = {};
    ::GetClassNameA(workerW, cls, sizeof(cls));
    std::printf("[diag] WorkerW hwnd=%p class=%s\n", (void*)workerW, cls);

    Platform::DesktopWindow window;
    if (!window.Create()) {
        std::printf("DesktopWindow::Create failed.\n");
        return 1;
    }
    if (!window.AttachToWorkerW(workerW)) {
        std::printf("AttachToWorkerW failed.\n");
        return 1;
    }
    std::printf("[diag] our hwnd=%p parented to WorkerW. Size=%dx%d\n",
                (void*)window.hwnd, window.screenW, window.screenH);

    Platform::DcompRenderer dcomp;
    if (!dcomp.Initialize(window.screenW, window.screenH)) {
        std::printf("DComp init failed.\n");
        return 1;
    }
    if (!dcomp.BindToWindow(window.hwnd)) {
        std::printf("DComp BindToWindow failed.\n");
        return 1;
    }
    std::printf("[diag] DComp bound. cell=%.1fx%.1f\n", dcomp.cellW, dcomp.cellH);

    Engine::AsciiBuffer buffer(kBufferWidth, kBufferHeight);

    Engine::Renderer engineRenderer;
    float aspect = (float)kBufferWidth / (float)(kBufferHeight * 2);
    engineRenderer.projection = Engine::Matrix4::Perspective(
        60.0f * 3.14159265f / 180.0f, aspect, 0.1f, 100.0f);

    App::Scene scene;

    auto last = std::chrono::high_resolution_clock::now();
    const auto frameTime = std::chrono::milliseconds(16);

    int frames = 0;
    while (g_running) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;

        if (!window.IsTargetValid()) {
            std::printf("[diag] target invalid; re-acquiring WorkerW\n");
            HWND fresh = Platform::WorkerWManager::GetWorkerW();
            if (fresh) {
                window.AttachToWorkerW(fresh);
                dcomp.BindToWindow(window.hwnd);
            }
        }

        scene.Update(dt);
        buffer.Clear();
        engineRenderer.Render(scene.pyramid, scene.GetTransform(), buffer);
        dcomp.Render(buffer);

        if (++frames % 120 == 0) {
            std::printf("[diag] %d frames\n", frames);
            std::fflush(stdout);
        }

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        std::this_thread::sleep_for(frameTime);
    }

    if (HWND p = ::FindWindowA("Progman", nullptr)) {
        ::InvalidateRect(p, nullptr, TRUE);
        ::UpdateWindow(p);
    }
    ::CoUninitialize();
    return 0;
}