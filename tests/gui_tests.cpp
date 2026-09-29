#include "Gui.h"
#include "Lol1.h"
#include "SettingsFile.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

int screen_width = 960, screen_height = 720;
LPDIRECT3D9 d3d = nullptr;
LPDIRECT3DDEVICE9 d3ddev = nullptr;
LPD3DXFONT pFont = nullptr, pFont2 = nullptr;
ID3DXLine* d3dLine = nullptr;
HWND hWnd = nullptr, tWnd = nullptr;
int checks = 0;
void Check(bool condition, const char* message) {
    ++checks;
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
AppSettings settings;
gui::Status status;
gui::Inspection inspection;
gui::Actions Tick(float x=-1000,float y=-1000,bool down=false) {
    // Synthetic ImGui events only: never moves the real mouse or accesses a game.
    auto& io = ImGui::GetIO();
    io.DisplaySize = {static_cast<float>(screen_width),static_cast<float>(screen_height)};
    io.DeltaTime = 1.0f/60;
    io.AddMousePosEvent(x,y);
    io.AddMouseButtonEvent(0,down);
    ImGui_ImplDX9_NewFrame();
    ImGui::NewFrame();
    d3ddev->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_XRGB(8,11,18),1,0);
    Check(SUCCEEDED(d3ddev->BeginScene()),"begin hidden test frame");
    const auto actions = gui::Draw(settings,status,&inspection);
    gui::Render();
    d3ddev->EndScene();
    return actions;
}
gui::Actions Click(const char* id, float fraction = 0.5f) {
    Check(inspection.count(id) == 1,"control exists");
    const auto r = inspection.at(id);
    const float x = r.left+(r.right-r.left)*fraction, y = (r.top+r.bottom)*0.5f;
    Tick(x,y,false);
    Tick(x,y,true);
    return Tick(x,y,false);
}
void Capture(const std::filesystem::path& path) {
    IDirect3DSurface9* surface = nullptr;
    Check(SUCCEEDED(d3ddev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&surface)),"obtain own render surface");
    const auto result = D3DXSaveSurfaceToFileW(path.c_str(),D3DXIFF_PNG,surface,nullptr,nullptr);
    surface->Release();
    Check(SUCCEEDED(result),"save GUI render image");
}
int main(int argc, char** argv) {
    Check(argc == 2,"output directory supplied");
    const std::filesystem::path output = argv[1];
    status.detail = "Menuen testes uden spilforbindelse. Ingen museinput sendes til Windows.";
    hWnd = CreateWindowExW(0,L"STATIC",L"H1 GUI hidden test",WS_POPUP,0,0,
        screen_width,screen_height,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Check(hWnd != nullptr,"create hidden render window");
    Check(init(hWnd),"initialize Direct3D");
    Check(gui::Initialize(hWnd,d3ddev),"initialize GUI");
    Tick(); Tick();
    Capture(output/L"gui-display.png");
    Check(settings.boxes,"boxes initially enabled");
    Click("boxes");
    Check(!settings.boxes,"checkbox click updates application settings");
    Click("boxes");
    Check(settings.boxes,"checkbox toggles back");
    Click("tab_aim");
    Check(inspection.count("aim") == 1,"aim tab exposes aim controls");
    Click("aim");
    Check(settings.aim,"aim toggle updates state");
    Click("##fov",0.7f);
    Check(settings.fov > 500 && settings.fov < 900,"slider click changes FOV within bounds");
    Tick();
    Capture(output/L"gui-aim.png");
    Click("tab_settings");
    Check(inspection.count("weapon_path") == 1,"weapon selector visible");
    settings.weaponPath = 2;
    Click("defaults");
    Check(settings.weaponPath == 0 && !settings.aim && settings.fov == 80,"defaults reset UI settings");
    Check(Click("save").save,"save button returns save action");
    const auto file = output/L"gui-test-settings.ini";
    { std::ofstream seed(file); seed << "; preserve this note\n[Other]\nValue=keep\n"; }
    settings.smoothing = 0.73f;
    settings.cars = true;
    Check(SaveAppSettings(file,settings),"save configuration transaction");
    Check(GetPrivateProfileIntW(L"Display",L"Cars",0,file.c_str()) == 1,"saved checkbox survives disk roundtrip");
    wchar_t buffer[64]{};
    GetPrivateProfileStringW(L"Aim",L"Smoothing",L"",buffer,64,file.c_str());
    Check(std::fabs(std::wcstof(buffer,nullptr)-0.73f)<0.001f,"saved slider value roundtrip");
    GetPrivateProfileStringW(L"Other",L"Value",L"",buffer,64,file.c_str());
    Check(std::wstring(buffer) == L"keep","unrelated settings preserved");
    gui::Saved(true);
    Tick();
    Capture(output/L"gui-settings.png");
    Check(Click("close").close,"close button returns close action");
    Check(Click("quit").quit,"quit button returns quit action");
    // Exercise the real renderer's lost-object/reset path used for game resizing.
    screen_width = 800; screen_height = 600;
    SetWindowPos(hWnd,nullptr,0,0,800,600,SWP_NOZORDER|SWP_NOACTIVATE);
    Check(ResizeRenderer(),"resize recreates DX9 and GUI resources");
    Tick(); Tick();
    const auto close = inspection.at("close");
    Check(close.right <= 800 && close.bottom <= 600,"footer stays inside smaller viewport");
    Check(Click("save").save,"controls still work after resize");
    DeleteFileW(file.c_str());
    gui::Shutdown(); ReleaseRenderer(); DestroyWindow(hWnd);
    std::cout << checks << " GUI/render/settings checks passed. Hidden window, synthetic input only.\n";
}
