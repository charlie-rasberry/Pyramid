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
        // 'topmost' arg is FALSE -- we don't want our visual to push above
        // the rest of WorkerW's content, just sit on top of the wallpaper.
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

        // The exact bounding box of our text grid
        D2D1_RECT_F textRect = {
            originX,
            originY,
            originX + gridW,
            originY + gridH + (buffer.height * 2.0f) // extra padding
        };

        d2dRT->BeginDraw();

        // This saves the GPU from filling the entire 1080p/4K background with black.
        // doesn't render redundant space
        d2dRT->PushAxisAlignedClip(&textRect, D2D1_ANTIALIAS_MODE_ALIASED);
        d2dRT->Clear(D2D1::ColorF(0, 0, 0, 0.0f)); // 0 0 0 1 = opaque

        // Build one single string with line breaks
        std::wstring fullGrid;
        fullGrid.reserve(buffer.height * (buffer.width + 1));
        
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                fullGrid.push_back((wchar_t)(unsigned char)buffer.chars[y * buffer.width + x]);
            }
            fullGrid.push_back(L'\n'); 
        }
        D2D1_RECT_F r = {
            originX,
            originY,
            originX + gridW,
            originY + gridH + (buffer.height * 2.0f) // extra padding
        };
        d2dRT->DrawText(fullGrid.c_str(), (UINT32)fullGrid.size(), //DrawTextW
                         textFormat.Get(), r, textBrush.Get(),
                         D2D1_DRAW_TEXT_OPTIONS_CLIP);

        d2dRT->PopAxisAlignedClip(); 

        HRESULT hr = d2dRT->EndDraw();

        /**
        RECT dirty = {
            std::max(0L, (LONG)(textRect.left - 2.0f)),
            std::max(0L, (LONG)(textRect.top - 2.0f)),
            std::min((LONG)screenW, (LONG)(textRect.right + 2.0f)),
            std::min((LONG)screenH, (LONG)(textRect.bottom + 2.0f))
        };
        **/
        DXGI_PRESENT_PARAMETERS pp = {};
        //pp.DirtyRectsCount = 1;
        //pp.pDirtyRects = &dirty;

        swapChain->Present1(1, 0, &pp); // 1 = 60fps 2 = 30 3 = 20 Old params => 
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
    ComPtr<ID2D1SolidColorBrush>   textBrush;

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
        if (FAILED(hr)) {
            std::printf("[diag] CreateSwapChainForComposition failed: 0x%lX\n",
                        (unsigned long)hr);
        }
        return SUCCEEDED(hr);
    }

    bool CreateD2D() {
        D2D1_FACTORY_OPTIONS opts = {};
        HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                         __uuidof(ID2D1Factory1), &opts, &d2dFactory);
        if (FAILED(hr)) return false;

        hr = d2dFactory->CreateDevice(dxgiDevice.Get(), d2dDevice.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                            d2dRT.GetAddressOf());
        if (FAILED(hr)) return false;

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

        D2D1_COLOR_F green = D2D1::ColorF(0.0f, 1.0f, 0.47f, 1.0f);
        return SUCCEEDED(d2dRT->CreateSolidColorBrush(green, textBrush.GetAddressOf()));
    }

    bool CreateDWrite() {
        HRESULT hr = ::DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(dwriteFactory.GetAddressOf()));
        if (FAILED(hr)) return false;

        const float fontSize = 18.0f;
        hr = dwriteFactory->CreateTextFormat(
            L"Consolas", nullptr, DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            fontSize, L"en-us", textFormat.GetAddressOf());
        if (FAILED(hr)) return false;

        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        cellW = fontSize * 0.55f;
        cellH = fontSize * 1.20f;
        textFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, cellH, cellH * 0.8f);
        
        return true;
    }

    bool CreateDComp() {
        HRESULT hr = ::DCompositionCreateDevice(
            dxgiDevice.Get(), __uuidof(IDCompositionDevice),
            reinterpret_cast<void**>(dcompDevice.GetAddressOf()));
        if (FAILED(hr)) {
            std::printf("[diag] DCompositionCreateDevice failed: 0x%lX\n",
                        (unsigned long)hr);
        }
        return SUCCEEDED(hr);
    }
};

} // namespace Platform