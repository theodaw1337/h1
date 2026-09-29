#include "Lol1.h"
#include <cstdarg>
#include <cwchar>
#include "imgui.h"
#include "imgui_impl_dx9.h"

namespace {
D3DPRESENT_PARAMETERS presentation{};
bool resourcesLost = false;
void TextV(int x, int y, int alignment, LPD3DXFONT font, bool bordered,
           DWORD color, DWORD borderColor, const wchar_t* format, va_list args) {
    if (!font) return;
    wchar_t buffer[1024]{};
    _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, format, args);
    const DWORD flags = DT_NOCLIP | (alignment == centered ? DT_CENTER : alignment == righted ? DT_RIGHT : DT_LEFT);
    if (bordered) {
        constexpr int offsets[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        for (const auto& offset : offsets) {
            RECT rect{x+offset[0], y+offset[1], x+offset[0], y+offset[1]};
            font->DrawTextW(nullptr, buffer, -1, &rect, flags, borderColor);
        }
    }
    RECT rect{x,y,x,y};
    font->DrawTextW(nullptr, buffer, -1, &rect, flags, color);
}
}

void ReleaseRenderer() {
    if (d3dLine) { d3dLine->Release(); d3dLine = nullptr; }
    if (pFont) { pFont->Release(); pFont = nullptr; }
    if (pFont2) { pFont2->Release(); pFont2 = nullptr; }
    if (d3ddev) { d3ddev->Release(); d3ddev = nullptr; }
    if (d3d) { d3d->Release(); d3d = nullptr; }
}
bool init(HWND window) {
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return false;
    presentation = {};
    presentation.Windowed = TRUE;
    presentation.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentation.hDeviceWindow = window;
    presentation.BackBufferFormat = D3DFMT_A8R8G8B8;
    presentation.BackBufferWidth = static_cast<UINT>(screen_width);
    presentation.BackBufferHeight = static_cast<UINT>(screen_height);
    presentation.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentation, &d3ddev))) {
        ReleaseRenderer(); return false;
    }
    if (FAILED(D3DXCreateLine(d3ddev, &d3dLine)) ||
        FAILED(D3DXCreateFontW(d3ddev, 14, 0, FW_NORMAL, 1, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Verdana", &pFont)) ||
        FAILED(D3DXCreateFontW(d3ddev, 16, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Verdana", &pFont2))) {
        ReleaseRenderer(); return false;
    }
    return true;
}
bool ResizeRenderer() {
    if (!d3ddev || screen_width <= 0 || screen_height <= 0) return false;
    const HRESULT state = d3ddev->TestCooperativeLevel();
    if (state == D3DERR_DEVICELOST) return false;
    if (state == D3D_OK && !resourcesLost &&
        presentation.BackBufferWidth == static_cast<UINT>(screen_width) &&
        presentation.BackBufferHeight == static_cast<UINT>(screen_height)) return true;
    if (!resourcesLost) {
        if (ImGui::GetCurrentContext()) ImGui_ImplDX9_InvalidateDeviceObjects();
        if (pFont) pFont->OnLostDevice();
        if (pFont2) pFont2->OnLostDevice();
        if (d3dLine) d3dLine->OnLostDevice();
        resourcesLost = true;
    }
    presentation.BackBufferWidth = static_cast<UINT>(screen_width);
    presentation.BackBufferHeight = static_cast<UINT>(screen_height);
    if (FAILED(d3ddev->Reset(&presentation))) return false;
    if (pFont) pFont->OnResetDevice();
    if (pFont2) pFont2->OnResetDevice();
    if (d3dLine) d3dLine->OnResetDevice();
    if (ImGui::GetCurrentContext()) ImGui_ImplDX9_CreateDeviceObjects();
    resourcesLost = false;
    return true;
}
void SText(int x, int y, int alignment, LPD3DXFONT font, bool bordered,
           DWORD color, DWORD border, const wchar_t* format, ...) {
    va_list args; va_start(args, format);
    TextV(x,y,alignment,font,bordered,color,border,format,args);
    va_end(args);
}
void SText2(int x, int y, int alignment, LPD3DXFONT font, bool bordered,
            DWORD color, DWORD border, const wchar_t* format, ...) {
    va_list args; va_start(args, format);
    TextV(x,y,alignment,font,bordered,color,border,format,args);
    va_end(args);
}
void DrawLine(float x, float y, float xx, float yy, float width, D3DCOLOR color) {
    if (!d3dLine) return;
    const D3DXVECTOR2 points[] = {{x,y},{xx,yy}};
    d3dLine->SetWidth(width);
    d3dLine->Draw(points, 2, color);
}
void DrawBox(float x, float y, float width, float height, D3DCOLOR color) {
    if (!d3dLine) return;
    const D3DXVECTOR2 points[] = {{x,y},{x+width,y},{x+width,y+height},{x,y+height},{x,y}};
    d3dLine->SetWidth(1);
    d3dLine->Draw(points, 5, color);
}
void GetScreenSize(int* width, int* height) {
    if (width) *width = screen_width;
    if (height) *height = screen_height;
}

