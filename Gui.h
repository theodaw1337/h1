#pragma once
#include "AppSettings.h"
#include <windows.h>
#include <d3d9.h>
#include <string>
#include <map>

namespace gui {
struct Status {
    std::string weapon = "Ukendt våben";
    std::string detail;
    float bulletSpeed = 0;
    bool connected = false, writable = false;
};
struct Actions { bool save = false, close = false, quit = false; };
// Optional inspection used by the local interaction/render smoke test.
struct Rect { float left=0, top=0, right=0, bottom=0; };
using Inspection = std::map<std::string,Rect>;
bool Initialize(HWND window, IDirect3DDevice9* device);
void Shutdown();
void BeginFrame(bool open);
Actions Draw(AppSettings& settings, const Status& status, Inspection* inspection = nullptr);
void Render();
void MarkDirty();
void Saved(bool success);
void ResetInput();
LRESULT HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
}
