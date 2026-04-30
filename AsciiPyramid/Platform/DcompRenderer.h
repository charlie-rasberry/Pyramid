#pragma once
#include "../Engine/AsciiShader.h"
#include "Win32Interop.h"

#include <cstdio>
#include <string>
#include <vector>
#include <wrl/client.h>

#include <d2d1_1.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwrite.h>
#include <dxgi1_2.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

namespace Platform {

using Microsoft::WRL::ComPtr;

class DcompRenderer {
public:
    int   screenW = 0, screenH = 0;
    float cellW = 12.0f, cellH = 22.0f;

    bool Initialize(int w, int h) {
        screenW = w;
        screenH = h;
        if (!CreateD3D())       return Fail("CreateD3D");
        if (!CreateSwapChain()) return Fail("CreateSwapChain");
        if (!CreateD2D())       return Fail("CreateD2D");
        if (!CreateDWrite())    return Fail("CreateDWrite");
        if (!CreateDComp())     return Fail("CreateDComp");
        return true;
    }

    bool BindToWindow(HWND window) {
        HRESULT hr = dcompDevice->CreateTargetForHwnd(window, TRUE,
                                                      target.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            std::printf("[diag] CreateTargetForHwnd failed: 0x%lX\n", (unsigned long)hr);
            return false;
        }
        hr = dcompDevice->CreateVisual(visual.ReleaseAndGetAddressOf());
        if (FAILED(hr)) return false;
        visual->SetContent(swapChain.Get());
        target->SetRoot(visual.Get());
        return SUCCEEDED(dcompDevice->Commit());
    }

    void Render(const Engine::AsciiBuffer& buffer) {
        if (!d2dRT || !textFormat) return;

        float gridW = buffer.width  * cellW;
        float gridH = buffer.height * cellH;
        float originX = (screenW - gridW) * 0.5f;
        float originY = (screenH - gridH) * 0.5f;

        D2D1_RECT_F textRect = {
            originX, originY,
            originX + gridW, originY + gridH + (buffer.height * 2.0f)
        };

        d2dRT->BeginDraw();
        d2dRT->PushAxisAlignedClip(&textRect, D2D1_ANTIALIAS_MODE_ALIASED);
        d2dRT->Clear(D2D1::ColorF(0, 0, 0, 0.0f));  // transparent

        // Group cells by color. For each color in use, build a grid string
        // where cells of that color show their character and all others
        // are spaces. One DrawText per color.
        const int total = buffer.width * buffer.height;
        std::wstring gridStr;
        gridStr.reserve(total + buffer.height);

        for (uint8_t cIdx = 0; cIdx < Engine::Color::Count; ++cIdx) {
            // Skip color groups with no cells.
            bool any = false;
            for (int i = 0; i < total; ++i) {
                if (buffer.color[i] == cIdx && buffer.chars[i] != ' ') {
                    any = true; break;
                }
            }
            if (!any) continue;

            gridStr.clear();
            for (int y = 0; y < buffer.height; ++y) {
                for (int x = 0; x < buffer.width; ++x) {
                    int i = y * buffer.width + x;
                    char ch = (buffer.color[i] == cIdx) ? buffer.chars[i] : ' ';
                    gridStr.push_back((wchar_t)(unsigned char)ch);
                }
                gridStr.push_back(L'\n');
            }

            d2dRT->DrawText(gridStr.c_str(), (UINT32)gridStr.size(),
                            textFormat.Get(), textRect, brushes[cIdx].Get(),
                            D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        d2dRT->PopAxisAlignedClip();
        d2dRT->EndDraw();

        DXGI_PRESENT_PARAMETERS pp = {};
        swapChain->Present1(2, 0, &pp);
        dcompDevice->Commit();
    }

private:
    ComPtr<ID3D11Device>           d3dDevice;
    ComPtr<ID3D11DeviceContext>    d3dContext;
    ComPtr<IDXGIDevice>            dxgiDevice;
    ComPtr<IDXGIFactory2>          dxgiFactory;
    ComPtr<IDXGISwapChain1>        swapChain;

    ComPtr<ID2D1Factory1>          d2dFactory;
    ComPtr<ID2D1Device>            d2dDevice;
    ComPtr<ID2D1DeviceContext>     d2dRT;
    ComPtr<ID2D1SolidColorBrush>   brushes[Engine::Color::Count];

    ComPtr<IDWriteFactory>         dwriteFactory;
    ComPtr<IDWriteTextFormat>      textFormat;

    ComPtr<IDCompositionDevice>    dcompDevice;
    ComPtr<IDCompositionTarget>    target;
    ComPtr<IDCompositionVisual>    visual;

    static bool Fail(const char* what) {
        std::printf("[diag] DcompRenderer init failed at %s\n", what);
        return false;
    }

    bool CreateD3D() {
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
        };
        D3D_FEATURE_LEVEL got;
        HRESULT hr = ::D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
            levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
            d3dDevice.GetAddressOf(), &got, d3dContext.GetAddressOf());
        if (FAILED(hr)) {
            hr = ::D3D11CreateDevice(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
                levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
                d3dDevice.GetAddressOf(), &got, d3dContext.GetAddressOf());
        }
        if (FAILED(hr)) return false;
        if (FAILED(d3dDevice.As(&dxgiDevice))) return false;
        ComPtr<IDXGIAdapter> adapter;
        if (FAILED(dxgiDevice->GetAdapter(&adapter))) return false;
        return SUCCEEDED(adapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));
    }

    bool CreateSwapChain() {
        DXGI_SWAP_CHAIN_DESC1 d = {};
        d.Width  = screenW;
        d.Height = screenH;
        d.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        d.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        d.SampleDesc.Count = 1;
        d.BufferCount = 2;
        d.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        d.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

        HRESULT hr = dxgiFactory->CreateSwapChainForComposition(
            dxgiDevice.Get(), &d, nullptr, swapChain.GetAddressOf());
        return SUCCEEDED(hr);
    }

    bool CreateD2D() {
        D2D1_FACTORY_OPTIONS opts = {};
        if (FAILED(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                       __uuidof(ID2D1Factory1), &opts, &d2dFactory))) return false;
        if (FAILED(d2dFactory->CreateDevice(dxgiDevice.Get(), d2dDevice.GetAddressOf()))) return false;
        if (FAILED(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                                  d2dRT.GetAddressOf()))) return false;

        ComPtr<IDXGISurface> surface;
        if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&surface)))) return false;

        D2D1_BITMAP_PROPERTIES1 bp = {};
        bp.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                           D2D1_ALPHA_MODE_PREMULTIPLIED);
        bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        ComPtr<ID2D1Bitmap1> bitmap;
        if (FAILED(d2dRT->CreateBitmapFromDxgiSurface(surface.Get(), &bp, &bitmap)))
            return false;
        d2dRT->SetTarget(bitmap.Get());

        // Palette: 8 brushes matching Engine::Color::* indices.
        const D2D1_COLOR_F palette[Engine::Color::Count] = {
            D2D1::ColorF(0.00f, 1.00f, 0.47f, 1.0f),  // Default (green)
            D2D1::ColorF(1.00f, 1.00f, 1.00f, 1.0f),  // White
            D2D1::ColorF(1.00f, 0.20f, 0.20f, 1.0f),  // Red
            D2D1::ColorF(1.00f, 0.55f, 0.10f, 1.0f),  // Orange
            D2D1::ColorF(1.00f, 0.95f, 0.20f, 1.0f),  // Yellow
            D2D1::ColorF(0.20f, 0.95f, 0.30f, 1.0f),  // Green
            D2D1::ColorF(0.20f, 0.55f, 1.00f, 1.0f),  // Blue
            D2D1::ColorF(0.70f, 0.30f, 1.00f, 1.0f),  // Violet
        };
        for (int i = 0; i < Engine::Color::Count; ++i) {
            if (FAILED(d2dRT->CreateSolidColorBrush(palette[i], brushes[i].GetAddressOf())))
                return false;
        }
        return true;
    }

    bool CreateDWrite() {
        HRESULT hr = ::DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(dwriteFactory.GetAddressOf()));
        if (FAILED(hr)) return false;

        const float fontSize = 18.0f;
        // Compute cell metrics BEFORE applying SetLineSpacing.
        cellW = fontSize * 0.55f;
        cellH = fontSize * 1.20f;

        hr = dwriteFactory->CreateTextFormat(
            L"Consolas", nullptr, DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            fontSize, L"en-us", textFormat.GetAddressOf());
        if (FAILED(hr)) return false;

        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        textFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, cellH, cellH * 0.8f);

        return true;
    }

    bool CreateDComp() {
        HRESULT hr = ::DCompositionCreateDevice(
            dxgiDevice.Get(), __uuidof(IDCompositionDevice),
            reinterpret_cast<void**>(dcompDevice.GetAddressOf()));
        return SUCCEEDED(hr);
    }
};

} // namespace Platform
