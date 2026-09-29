#include "Client.h"
#include "../Gui.h"
#include "../Lol1.h"
#include "imgui.h"
#include <iostream>
#include <filesystem>

int screen_width=1080,screen_height=720;
LPDIRECT3D9 d3d=nullptr;
LPDIRECT3DDEVICE9 d3ddev=nullptr;
LPD3DXFONT pFont=nullptr,pFont2=nullptr;
ID3DXLine* d3dLine=nullptr;
HWND hWnd=nullptr,tWnd=nullptr;
bool alive=true,menu=false;
LRESULT CALLBACK WindowProc(HWND window,UINT msg,WPARAM wp,LPARAM lp) {
    if (gui::HandleMessage(window,msg,wp,lp)) return 1;
    if (msg==WM_KEYDOWN && !(lp&(1LL<<30))) {
        if (wp==VK_INSERT) menu=!menu;
        if (wp==VK_ESCAPE) menu=false;
    }
    if (msg==WM_DESTROY) { alive=false; PostQuitMessage(0); return 0; }
    return DefWindowProcW(window,msg,wp,lp);
}
int main(int argc,char** argv) {
    bool driverMode=false,connected=false,paused=false,names=true,bars=true;
    std::filesystem::path capture;
    for (int i=1;i<argc;++i) {
        if (std::string(argv[i])=="--driver") driverMode=true;
        else if (std::string(argv[i])=="--capture" && i+1<argc) capture=argv[++i];
        else { std::cerr<<"Options: --driver | --capture <png>\n"; return 2; }
    }
    if (driverMode && !capture.empty()) return 2;
    SetProcessDPIAware();
    WNDCLASSW wc{}; wc.lpfnWndProc=WindowProc; wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"H1OfflineLabView"; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if (!RegisterClassW(&wc)) return 1;
    RECT rect{0,0,screen_width,screen_height}; AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE);
    hWnd=CreateWindowW(wc.lpszClassName,L"H1 Offline Lab - synthetic data",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,wc.hInstance,nullptr);
    if (!hWnd || !init(hWnd)) return 1;
    if (!gui::Initialize(hWnd,d3ddev)) { ReleaseRenderer(); return 1; }
    if (capture.empty()) ShowWindow(hWnd,SW_SHOW);
    else menu=true;
    LabClient client;
    if (driverMode) connected=client.Open();
    LAB_SNAPSHOT store{},frame{}; LabInitialize(&store); LabInitialize(&frame);
    std::string status=driverMode ? "Venter på testdriver" : "Lokal simulation. Ingen kernel-driver indlæst.";
    ULONGLONG start=GetTickCount64(); unsigned int rendered=0;
    int exitCode=0;
    while (alive) {
        MSG message{};
        while (PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        if (!alive) break;
        RECT area{}; GetClientRect(hWnd,&area);
        if (IsIconic(hWnd) || area.right<=0 || area.bottom<=0) { Sleep(30); continue; }
        screen_width=area.right; screen_height=area.bottom;
        if (!ResizeRenderer()) { Sleep(30); continue; }
        if (!driverMode && !paused) {
            auto input=GenerateLabFrame(capture.empty() ? static_cast<double>(GetTickCount64()-start)/1000 : 2.0);
            uint32_t bytes=0;
            LabExchange(&store,LAB_PUBLISH,&input,sizeof(input),0,&bytes);
            LabExchange(&store,LAB_READ,&frame,0,sizeof(frame),&bytes);
        } else if (driverMode) {
            if (connected && client.Read(frame)) status="Data hentet via DeviceIoControl fra driverens testbuffer.";
            else { connected=false; LabInitialize(&frame); status="Testdriver ikke tilgængelig. Windows-fejl "+std::to_string(client.error); }
        }
        d3ddev->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_XRGB(13,18,26),1,0);
        if (FAILED(d3ddev->BeginScene())) continue;
        gui::BeginFrame(false);
        ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize({static_cast<float>(screen_width),static_cast<float>(screen_height)});
        ImGui::Begin("Offline",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
        ImGui::TextColored({0.43f,0.88f,0.74f,1},"H1 / OFFLINE LAB");
        ImGui::SameLine(); ImGui::TextDisabled("Kun syntetiske data");
        ImGui::TextWrapped("%s",status.c_str());
        ImGui::Text("Kilde: %s   |   Sekvens: %llu   |   Aktører: %u",driverMode ? "Kernel-testbuffer" : "Lokal testbuffer",
            static_cast<unsigned long long>(frame.sequence),frame.count);
        ImGui::TextDisabled("INSERT: indstillinger    ESC: luk indstillinger");
        ImGui::Separator();
        if (menu) {
            ImGui::BeginChild("Options",{245,0},ImGuiChildFlags_Borders);
            ImGui::TextUnformatted("Visning");
            ImGui::Checkbox("Navne",&names);
            ImGui::Checkbox("Livsbjælker",&bars);
            ImGui::BeginDisabled(driverMode); ImGui::Checkbox("Pause simulation",&paused); ImGui::EndDisabled();
            ImGui::Spacing(); ImGui::TextWrapped("Testdata produceres i et separat program, når driveren bruges.");
            if (driverMode && ImGui::Button("Forbind igen")) connected=client.Open();
            if (ImGui::Button("Luk menu")) menu=false;
            ImGui::EndChild(); ImGui::SameLine();
        }
        ImGui::BeginChild("Scene",{0,0},ImGuiChildFlags_Borders);
        const auto origin=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail();
        auto* draw=ImGui::GetWindowDrawList();
        const ImVec2 center{origin.x+size.x*0.5f,origin.y+size.y*0.5f};
        const float zoom=std::min(size.x,size.y)/100000.0f;
        for (int i=-5;i<=5;++i) {
            const float offset=i*10000.0f*zoom;
            draw->AddLine({center.x+offset,origin.y},{center.x+offset,origin.y+size.y},IM_COL32(38,47,59,255));
            draw->AddLine({origin.x,center.y+offset},{origin.x+size.x,center.y+offset},IM_COL32(38,47,59,255));
        }
        draw->AddCircleFilled(center,4,IM_COL32(125,145,165,255));
        for (uint32_t i=0;i<frame.count;++i) {
            const auto& actor=frame.actors[i];
            const ImVec2 point{center.x+actor.x_mm*zoom,center.y-actor.z_mm*zoom};
            draw->AddCircleFilled(point,8,IM_COL32(110,224,188,255));
            if (names) {
                const std::string name(actor.name,strnlen_s(actor.name,sizeof(actor.name)));
                draw->AddText({point.x+13,point.y-10},IM_COL32(226,237,244,255),name.c_str());
            }
            if (bars) {
                draw->AddRectFilled({point.x-20,point.y+15},{point.x+20,point.y+19},IM_COL32(60,68,80,255));
                draw->AddRectFilled({point.x-20,point.y+15},{point.x-20+0.4f*actor.health,point.y+19},IM_COL32(110,224,188,255));
            }
        }
        ImGui::EndChild(); ImGui::End(); gui::Render(); d3ddev->EndScene();
        if (!capture.empty() && ++rendered==3) {
            IDirect3DSurface9* surface=nullptr;
            HRESULT result=d3ddev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&surface);
            if (SUCCEEDED(result)) { result=D3DXSaveSurfaceToFileW(capture.c_str(),D3DXIFF_PNG,surface,nullptr,nullptr); surface->Release(); }
            exitCode=SUCCEEDED(result) ? 0 : 1; break;
        }
        d3ddev->Present(nullptr,nullptr,nullptr,nullptr); Sleep(16);
    }
    gui::Shutdown(); ReleaseRenderer(); DestroyWindow(hWnd); UnregisterClassW(wc.lpszClassName,wc.hInstance);
    return exitCode;
}
