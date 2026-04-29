# AsciiPyramid (DirectComposition build)

Rotating ASCII pyramid rendered onto the Windows desktop wallpaper layer using
DirectComposition + D3D11 + DirectWrite, parented to WorkerW.

## Why this version exists

The simpler GDI-onto-WorkerW approach stops working on Windows 11 24H2 LTSC.
DWM no longer samples GDI pixels from the WorkerW HDC for the visible
wallpaper, so direct draws disappear into a void. This build participates in
DWM's composition pipeline directly via DirectComposition: a D3D11/DXGI
swapchain feeds a DComp visual that's bound to a DComp target on WorkerW.
DWM picks up the composition surface and draws it into the wallpaper layer.

## Architecture

```
Engine/        Math + software ASCII rasterizer (unchanged)
  Vector3, Matrix4, Mesh, AsciiShader, Renderer
Platform/      Win32 + DComp glue
  Win32Interop      windows.h wrapper
  WorkerWManager    locates a usable WorkerW (Win10 sibling + Win11 child layouts)
  DcompRenderer     D3D11 device, DXGI swap chain, D2D + DWrite text drawing,
                    DComp device/target/visual
  DesktopWindow     thin façade matching the original spec
App/
  SceneSetup        rotating pyramid state
  Program.cpp       entry point, render loop, COM init
```

## Build

Requires Visual Studio with the C++ workload + Windows 10/11 SDK. From an
**x64 Native Tools Command Prompt for VS**:

```
build.bat msvc
```

MinGW won't work cleanly here — DComp/D2D import libs and headers are only
guaranteed via the MSVC + SDK install.

## Run

```
AsciiPyramid.exe
```

Run from a non-elevated cmd (medium IL). Ctrl+C exits cleanly. If WorkerW
gets recycled (theme change, explorer restart) the loop reacquires.

## Honest caveats

- **Microsoft can break this any time.** WorkerW + 0x052C is undocumented;
  hosting a custom DComp visual on it isn't a supported scenario. It works
  on Win11 24H2 today.
- **Multi-monitor:** sized to the primary monitor only.
- **HiDPI:** font cell metrics are approximated; on >100% scaling the grid
  may center slightly off. Easy to extend with `IDWriteTextLayout` for exact
  metrics if it bothers you.
- **No GPU shader pipeline.** The triangle rasterizer is still software,
  drawing into an ASCII char buffer that DirectWrite then renders to the
  D2D target. The grid is small (120×40) so this is fine. Pushing the grid
  much larger would be the moment to move rasterization onto the GPU.
- **First-run cost:** D3D + DComp init takes ~100ms. Frame loop is steady
  60Hz after that on any modern GPU.

## If the pyramid still doesn't appear

You'll see `[diag]` lines in the console. Useful failure modes:

- `target hwnd=... class=Progman` — we fell back to Progman because no
  WorkerW could be located. Means the 0x052C trick has been patched on
  your build. Possible workaround: try setting a different wallpaper
  (solid color, then back), which forces Explorer to recreate WorkerW.
- `CreateTargetForHwnd failed: 0x...` — WorkerW exists but DComp refused
  to bind to it. Some hardened builds disallow this; would need a
  different parent strategy.
- `D3D11CreateDevice hardware failed` — no GPU or drivers missing; falls
  back to WARP automatically and should still work, just slower.
