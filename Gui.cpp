#include "Gui.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);

namespace gui {
namespace {
bool initialized = false, dirty = false;
int selectedTab = 0;
bool saveSucceeded = true;
double toastUntil = 0;
ImFont* bodyFont = nullptr;
ImFont* titleFont = nullptr;
float scale = 1;
constexpr ImVec4 accent{0.43f,0.88f,0.74f,1};
constexpr ImVec4 muted{0.52f,0.58f,0.66f,1};
constexpr ImVec4 ink{0.90f,0.93f,0.97f,1};
void Record(Inspection* inspection, const char* name) {
    if (!inspection) return;
    const auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    (*inspection)[name] = {a.x,a.y,b.x,b.y};
}
void Hint(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text,muted);
    ImGui::TextWrapped("%s",text);
    ImGui::PopStyleColor();
}
void Heading(const char* title, const char* subtitle) {
    ImGui::PushFont(titleFont);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    Hint(subtitle);
    ImGui::Dummy({0,12*scale});
}
bool Toggle(const char* label, bool& value, const char* key, Inspection* inspection) {
    const auto position = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float height = 32*scale;
    ImGui::PushID(key);
    const bool clicked = ImGui::InvisibleButton("toggle",{width,height});
    if (clicked) { value = !value; dirty = true; }
    Record(inspection,key);
    auto* draw = ImGui::GetWindowDrawList();
    if (ImGui::IsItemHovered())
        draw->AddRectFilled(position,{position.x+width,position.y+height},IM_COL32(33,42,54,255),5*scale);
    draw->AddText({position.x+8*scale,position.y+8*scale},ImGui::GetColorU32(ink),label);
    const float x = position.x+width-48*scale, y = position.y+8*scale;
    draw->AddRectFilled({x,y},{x+38*scale,y+20*scale},
        value ? ImGui::GetColorU32(accent) : IM_COL32(56,66,80,255),10*scale);
    draw->AddCircleFilled({x+(value ? 28 : 10)*scale,y+10*scale},7*scale,
        value ? IM_COL32(17,33,30,255) : IM_COL32(177,188,200,255));
    if (ImGui::IsItemFocused()) draw->AddRect(position,{position.x+width,position.y+height},ImGui::GetColorU32(accent),5*scale);
    ImGui::PopID();
    return clicked;
}
void Slider(const char* label, const char* id, float& value, float low, float high,
            const char* format, Inspection* inspection) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(155*scale);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat(id,&value,low,high,format,ImGuiSliderFlags_AlwaysClamp)) dirty = true;
    Record(inspection,id);
}
}

bool Initialize(HWND window, IDirect3DDevice9* device) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    // Bound scaling so the complete panel fits comfortably on ordinary displays.
    RECT client{}; GetClientRect(window,&client);
    scale = std::clamp(static_cast<float>(GetDpiForWindow(window))/96.0f,1.0f,1.4f);
    scale = std::min(scale,std::max(0.75f,std::min((client.right-24)/820.0f,(client.bottom-24)/650.0f)));
    wchar_t windows[MAX_PATH]{};
    GetWindowsDirectoryW(windows,MAX_PATH);
    const auto font = std::filesystem::path(windows)/L"Fonts"/L"segoeui.ttf";
    const auto bold = std::filesystem::path(windows)/L"Fonts"/L"segoeuib.ttf";
    if (std::filesystem::exists(font))
        bodyFont = io.Fonts->AddFontFromFileTTF(font.string().c_str(),16*scale);
    if (!bodyFont) bodyFont = io.Fonts->AddFontDefault();
    if (std::filesystem::exists(bold))
        titleFont = io.Fonts->AddFontFromFileTTF(bold.string().c_str(),23*scale);
    if (!titleFont) titleFont = bodyFont;
    io.FontDefault = bodyFont;
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowPadding = {22,20};
    style.FramePadding = {12,8};
    style.ItemSpacing = {10,8};
    style.WindowRounding = 12;
    style.ChildRounding = 8;
    style.FrameRounding = 5;
    style.GrabRounding = 5;
    style.ScrollbarRounding = 6;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 0;
    style.ScaleAllSizes(scale);
    auto* c = style.Colors;
    c[ImGuiCol_Text] = ink;
    c[ImGuiCol_TextDisabled] = muted;
    c[ImGuiCol_WindowBg] = {0.065f,0.085f,0.12f,1};
    c[ImGuiCol_ChildBg] = {0.085f,0.11f,0.15f,1};
    c[ImGuiCol_Border] = {0.19f,0.24f,0.30f,1};
    c[ImGuiCol_FrameBg] = {0.13f,0.17f,0.22f,1};
    c[ImGuiCol_FrameBgHovered] = {0.17f,0.23f,0.29f,1};
    c[ImGuiCol_FrameBgActive] = {0.19f,0.29f,0.32f,1};
    c[ImGuiCol_CheckMark] = accent;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = {0.61f,1,0.86f,1};
    c[ImGuiCol_Button] = {0.14f,0.19f,0.24f,1};
    c[ImGuiCol_ButtonHovered] = {0.19f,0.28f,0.32f,1};
    c[ImGuiCol_ButtonActive] = {0.23f,0.34f,0.37f,1};
    c[ImGuiCol_Header] = {0.13f,0.28f,0.27f,1};
    c[ImGuiCol_HeaderHovered] = {0.18f,0.30f,0.31f,1};
    c[ImGuiCol_HeaderActive] = {0.20f,0.37f,0.34f,1};
    c[ImGuiCol_Separator] = c[ImGuiCol_Border];
    c[ImGuiCol_NavCursor] = accent;
    if (!ImGui_ImplWin32_Init(window)) { ImGui::DestroyContext(); return false; }
    if (!ImGui_ImplDX9_Init(device)) { ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); return false; }
    initialized = true;
    return true;
}
void Shutdown() {
    if (!initialized) return;
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}
void BeginFrame(bool open) {
    auto& io = ImGui::GetIO();
    io.MouseDrawCursor = open;
    if (open) io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    else io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}
void ResetInput() {
    if (!initialized) return;
    auto& io = ImGui::GetIO();
    io.ClearEventsQueue();
    io.ClearInputKeys();
    io.ClearInputMouse();
}
void MarkDirty() { dirty = true; }
void Saved(bool success) {
    saveSucceeded = success;
    toastUntil = ImGui::GetTime()+4;
    if (success) dirty = false;
}
LRESULT HandleMessage(HWND window,UINT message,WPARAM wParam,LPARAM lParam) {
    return initialized ? ImGui_ImplWin32_WndProcHandler(window,message,wParam,lParam) : 0;
}
Actions Draw(AppSettings& settings, const Status& status, Inspection* inspection) {
    Actions actions;
    if (inspection) inspection->clear();
    const auto display = ImGui::GetIO().DisplaySize;
    const ImVec2 size{std::min(820*scale,std::max(300.0f,display.x-24)),
        std::min(650*scale,std::max(300.0f,display.y-24))};
    ImGui::SetNextWindowSize(size,ImGuiCond_Always);
    ImGui::SetNextWindowPos({std::max(0.0f,(display.x-size.x)*0.5f),std::max(0.0f,(display.y-size.y)*0.5f)},ImGuiCond_Always);
    ImGui::Begin("H1 Control",nullptr,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::PushFont(titleFont);
    ImGui::TextColored(accent,"H1");
    ImGui::SameLine(0,12*scale);
    ImGui::TextUnformatted("Kontrolpanel");
    ImGui::PopFont();
    ImGui::SameLine(ImGui::GetWindowWidth()-155*scale);
    ImGui::TextColored(muted,"[ INSERT ]");
    Hint("Tilpas visning og styring. Aim er på pause, mens menuen er åben.");
    ImGui::Dummy({0,7*scale});
    ImGui::Separator();
    ImGui::Dummy({0,9*scale});
    const float bodyHeight = ImGui::GetContentRegionAvail().y-80*scale;
    ImGui::BeginChild("Navigation",{180*scale,bodyHeight},ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::TextColored(muted,"OVERSIGT");
    ImGui::Dummy({0,8*scale});
    const char* tabs[] = {"Visning","Aim","Indstillinger"};
    const char* ids[] = {"tab_display","tab_aim","tab_settings"};
    for (int i = 0; i < 3; ++i) {
        if (ImGui::Selectable(tabs[i],selectedTab == i,0,{0,34*scale})) selectedTab = i;
        Record(inspection,ids[i]);
    }
    ImGui::Dummy({0,22*scale});
    ImGui::Separator();
    ImGui::Dummy({0,12*scale});
    ImGui::TextColored(status.connected ? accent : muted,status.connected ? "FORBUNDET" : "FORHÅNDSVISNING");
    Hint(status.writable ? "Læse- og skriveadgang" : "Kun læseadgang");
    ImGui::Dummy({0,10*scale});
    ImGui::TextColored(muted,"GENVEJE");
    Hint("INSERT   Menu\nF1-F7     Funktioner\nEND        Afslut");
    ImGui::EndChild();
    ImGui::SameLine(0,16*scale);
    ImGui::BeginChild("Content",{0,bodyHeight},ImGuiChildFlags_AlwaysUseWindowPadding);
    if (selectedTab == 0) {
        Heading("Visning","Vælg, hvad du vil se i spillet.");
        Toggle("Spillere",settings.players,"players",inspection);
        Toggle("Bokse",settings.boxes,"boxes",inspection);
        Toggle("Skelet",settings.bones,"bones",inspection);
        Toggle("Linjer",settings.lines,"lines",inspection);
        Toggle("Livsbjælker",settings.health,"health",inspection);
        Toggle("Køretøjer",settings.cars,"cars",inspection);
        Toggle("Genstande",settings.items,"items",inspection);
    } else if (selectedTab == 1) {
        Heading("Aim","Aktiveres ved at holde højre museknap i spillet.");
        Toggle("Aktiver aim",settings.aim,"aim",inspection);
        ImGui::Dummy({0,4*scale});
        Slider("FOV-radius","##fov",settings.fov,1,1000,"%.0f px",inspection);
        Slider("Udjævning","##smoothing",settings.smoothing,0.01f,1,"%.2f",inspection);
        Slider("Maksimal afstand","##distance",settings.maxDistance,1,2000,"%.0f m",inspection);
        Toggle("Kompensér for bullet drop",settings.drop,"drop",inspection);
        ImGui::BeginDisabled(!settings.drop);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Tyngdekraft (m/s²)");
        ImGui::SameLine(155*scale);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputFloat("##gravity",&settings.gravity,0.1f,1,"%.3f")) {
            settings.gravity = std::isfinite(settings.gravity) ? std::clamp(settings.gravity,0.0f,100.0f) : 0;
            dirty = true;
        }
        Record(inspection,"gravity");
        ImGui::EndDisabled();
        Hint("Tyngdekraften skal måles i spillet. Ingen synlighedstest er tilføjet.");
    } else {
        Heading("Indstillinger","Våbenaflæsning og aktuel status.");
        ImGui::TextUnformatted("Metode til våbenaflæsning");
        const char* paths[] = {"Automatisk","Inventory","Weapon container"};
        settings.weaponPath = std::clamp(settings.weaponPath,0,2);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##weapon_path",&settings.weaponPath,paths,3)) dirty = true;
        Record(inspection,"weapon_path");
        Hint("Automatisk stopper aim, hvis metoderne giver forskellige våben.");
        ImGui::Dummy({0,12*scale});
        ImGui::Separator();
        ImGui::Dummy({0,8*scale});
        ImGui::TextColored(muted,"AKTUELT VÅBEN");
        ImGui::TextWrapped("%s",status.weapon.empty() ? "Ukendt våben" : status.weapon.c_str());
        ImGui::Text("Projektilhastighed: %.0f m/s",static_cast<double>(status.bulletSpeed));
        ImGui::Dummy({0,10*scale});
        if (!status.detail.empty()) Hint(status.detail.c_str());
        ImGui::Dummy({0,12*scale});
        if (ImGui::Button("Gendan standardværdier",{-1,34*scale})) {
            const bool writes = settings.allowWrites;
            settings = AppSettings{};
            settings.allowWrites = writes;
            dirty = true;
        }
        Record(inspection,"defaults");
        Hint("Gælder med det samme. Tryk Gem for at beholde ændringerne.");
    }
    ImGui::EndChild();
    ImGui::Dummy({0,8*scale});
    ImGui::Separator();
    ImGui::Dummy({0,5*scale});
    ImGui::PushStyleColor(ImGuiCol_Button,{0.24f,0.61f,0.49f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0.31f,0.73f,0.59f,1});
    ImGui::PushStyleColor(ImGuiCol_Text,{0.035f,0.08f,0.07f,1});
    actions.save = ImGui::Button("Gem indstillinger",{155*scale,34*scale});
    Record(inspection,"save");
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    actions.close = ImGui::Button("Luk menu",{100*scale,34*scale});
    Record(inspection,"close");
    ImGui::SameLine();
    if (ImGui::GetTime() < toastUntil)
        ImGui::TextColored(saveSucceeded ? accent : ImVec4{1,0.5f,0.4f,1},saveSucceeded ? "Gemt" : "Kunne ikke gemme");
    else ImGui::TextColored(muted,dirty ? "Ikke gemt" : "Klar");
    ImGui::SameLine(ImGui::GetWindowWidth()-120*scale);
    actions.quit = ImGui::Button("Afslut",{95*scale,34*scale});
    Record(inspection,"quit");
    ImGui::End();
    return actions;
}
void Render() {
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}
}
