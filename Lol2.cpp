#include "Lol2.h"
#include "Gui.h"
#include "SettingsFile.h"

using runtime::Vec3;
using Address = std::uint64_t;
int screen_width = 1920, screen_height = 1080;
LPDIRECT3D9 d3d = nullptr;
LPDIRECT3DDEVICE9 d3ddev = nullptr;
LPD3DXFONT pFont = nullptr, pFont2 = nullptr;
ID3DXLine* d3dLine = nullptr;
HWND hWnd = nullptr, tWnd = nullptr;

namespace {
ProcessMemory memory;
runtime::CameraCache camera;
runtime::WeaponProfile weapon;
std::filesystem::path appDirectory;
std::ofstream logFile;
bool running = true, diagnosticOnly = false;
AppSettings settings;
bool menuOpen = false;
bool teleport = false, noclip = false;
Vec3 noclipPosition{};
bool haveNoclipPosition = false;
std::wstring frameStatus;
ULONGLONG lastStatusLog = 0;
std::wstring lastLoggedStatus;

void Log(const std::string& text) {
    std::cout << text << std::endl;
    if (logFile) { logFile << text << '\n'; logFile.flush(); }
}
std::wstring Wide(const std::string& text) {
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (!count) return std::wstring(text.begin(), text.end());
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), count);
    return result;
}
std::string Utf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    if (!count) return "[text conversion failed]";
    std::string result(static_cast<std::size_t>(count),'\0');
    WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),result.data(),count,nullptr,nullptr);
    return result;
}
void Status(const wchar_t* text) {
    frameStatus = text;
    const auto now = GetTickCount64();
    if (frameStatus != lastLoggedStatus && now-lastStatusLog > 1000) {
        Log(Utf8(frameStatus));
        lastStatusLog = now;
        lastLoggedStatus = frameStatus;
    }
}
void Error(const std::string& message, DWORD code = 0) {
    std::string text = message;
    if (code) {
        char* detail = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, reinterpret_cast<LPSTR>(&detail), 0, nullptr);
        text += "\nWindows error " + std::to_string(code);
        if (detail) { text += ": "; text += detail; LocalFree(detail); }
    }
    Log(text);
    if (!diagnosticOnly) MessageBoxW(nullptr, Wide(text).c_str(), L"H1 startup", MB_OK | MB_ICONERROR);
}
void LoadSettings() {
    const auto path = (appDirectory / L"settings.ini").wstring();
    auto number = [&](const wchar_t* section, const wchar_t* key, float fallback, float low, float high) {
        wchar_t value[64]{};
        GetPrivateProfileStringW(section, key, L"", value, _countof(value), path.c_str());
        if (!*value) return fallback;
        wchar_t* end = nullptr;
        const float parsed = std::wcstof(value, &end);
        return end != value && *end == L'\0' && std::isfinite(parsed) ? std::clamp(parsed, low, high) : fallback;
    };
    auto flag = [&](const wchar_t* section, const wchar_t* key, bool fallback) {
        return GetPrivateProfileIntW(section,key,fallback ? 1 : 0,path.c_str()) != 0;
    };
    settings.aim = flag(L"Aim",L"Enabled",false);
    settings.fov = number(L"Aim",L"FovPixels",80,1,1000);
    settings.smoothing = number(L"Aim",L"Smoothing",0.25f,0.01f,1);
    settings.maxDistance = number(L"Aim",L"MaxDistance",500,1,2000);
    settings.drop = flag(L"Aim",L"DropCompensation",false);
    settings.gravity = number(L"Aim",L"Gravity",0,0,100);
    settings.weaponPath = static_cast<int>(number(L"Weapon",L"Path",0,0,2));
    settings.allowWrites = flag(L"Legacy",L"AllowMemoryWrites",false);
    settings.players = flag(L"Display",L"Players",true);
    settings.boxes = flag(L"Display",L"Boxes",true);
    settings.bones = flag(L"Display",L"Bones",true);
    settings.lines = flag(L"Display",L"Lines",true);
    settings.health = flag(L"Display",L"Health",true);
    settings.cars = flag(L"Display",L"Cars",false);
    settings.items = flag(L"Display",L"Items",true);
}
bool EnableDebugPrivilege() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return false;
    LUID luid{};
    bool result = false;
    if (LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        TOKEN_PRIVILEGES privileges{};
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Luid = luid;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        SetLastError(ERROR_SUCCESS);
        result = AdjustTokenPrivileges(token,FALSE,&privileges,0,nullptr,nullptr) != FALSE &&
            GetLastError() == ERROR_SUCCESS;
    }
    CloseHandle(token);
    return result;
}
HWND FindGameWindow() {
    HWND window = FindWindowW(L"H1Z1 PlayClient (Live)", nullptr);
    return window ? window : FindWindowW(nullptr,L"H1Z1 PlayClient (Live)");
}
bool InitProcess() {
    Log(EnableDebugPrivilege() ? "Debug privilege available." :
        "Debug privilege unavailable; trying normal process access.");
    tWnd = FindGameWindow();
    if (!tWnd) {
        Error("Game window not found. Start H1Z1 and wait until it has loaded.");
        return false;
    }
    DWORD pid = 0;
    if (!GetWindowThreadProcessId(tWnd, &pid) || !pid) {
        Error("Could not identify the game process.", GetLastError()); return false;
    }
    const DWORD readAccess = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | SYNCHRONIZE;
    const DWORD requested = readAccess | (settings.allowWrites ? PROCESS_VM_WRITE | PROCESS_VM_OPERATION : 0);
    memory.process = OpenProcess(requested,FALSE,pid);
    memory.writable = memory.process && settings.allowWrites;
    if (!memory.process && settings.allowWrites) memory.process = OpenProcess(readAccess,FALSE,pid);
    if (!memory.process) {
        Error("OpenProcess failed. Check permissions. Windows or game protection may deny access.", GetLastError());
        return false;
    }
    Log("Connected to PID " + std::to_string(pid) + (memory.writable ? " (read/write)." : " (read only)."));
    return true;
}
bool UpdateCamera() {
    camera = {};
    Address graphics = 0, view = 0, matrixAddress = 0;
    runtime::Matrix raw{};
    if (!memory.ReadPointer(ADDRESS_CGRAPHICS,graphics) ||
        !memory.ReadPointer(graphics+OFFSET_CAMERA,view) ||
        !memory.ReadPointer(view+OFFSET_MATRIX,matrixAddress) ||
        !memory.Read(matrixAddress+OFFSET_MATRIX_DEF,raw)) return false;
    camera = runtime::MakeCamera(raw,screen_width,screen_height);
    return camera.valid;
}
runtime::WeaponProfile ReadWeaponName(Address local, bool containerPath) {
    Address first = 0, second = 0, nameAddress = 0;
    std::string name;
    if (containerPath) {
        if (!memory.ReadPointer(local+OFFSET_WEAPON_CONTAINER,first) ||
            !memory.ReadPointer(first+OFFSET_ACTIVE_WEAPON,second) ||
            !memory.ReadPointer(second+OFFSET_WEAPON_MODEL,nameAddress)) return {};
    } else {
        if (!memory.ReadPointer(local+OFFSET_INVENTORY,first) ||
            !memory.ReadPointer(first+OFFSET_INVENTORY_NAME,nameAddress)) return {};
    }
    return memory.ReadString(nameAddress,name) ? runtime::ClassifyWeapon(name) : runtime::WeaponProfile{};
}
runtime::WeaponProfile ReadWeapon(Address local) {
    const auto inventory = settings.weaponPath != 2 ? ReadWeaponName(local,false) : runtime::WeaponProfile{};
    const auto container = settings.weaponPath != 1 ? ReadWeaponName(local,true) : runtime::WeaponProfile{};
    auto selected = runtime::ResolveWeapon(inventory,container,settings.weaponPath);
    if (selected.name.empty()) selected.name = "Unknown weapon";
    return selected;
}
struct Player {
    Address entity = 0, positionBase = 0, joints = 0, mount = 0;
    Vec3 feet{}, velocity{};
    float yaw = 0;
    int stance = 0, health = 0;
    bool healthRead = false, mounted = false, mountKnown = false, velocityRead = false;
};
bool ReadPlayer(Address entity, Player& player) {
    player = {};
    player.entity = entity;
    Address actor = 0, skeleton = 0, pose = 0, healthBase = 0;
    const bool actorValid = memory.ReadPointer(entity+OFFSET_SkeletonActors,actor);
    Address mount = 0;
    if (actorValid && memory.Read(actor+OFFSET_MOUNT_FROM_ACTOR,mount)) {
        if (!mount) player.mountKnown = true;
        else if (ProcessMemory::Pointer(mount)) {
            player.mountKnown = true;
            player.mounted = true;
            player.mount = mount;
        }
    }
    if (player.mounted) {
        if (!memory.Read(player.mount+OFFSET_CARPOS,player.feet)) return false;
        player.velocityRead = memory.Read(player.mount+OFFSET_VELOCITY,player.velocity);
    } else {
        if (!memory.ReadPointer(entity+OFFSET_PLAYER_BASE,player.positionBase) ||
            !memory.Read(player.positionBase+OFFSET_VBASEPOS,player.feet)) return false;
        player.velocityRead = memory.Read(entity+OFFSET_VELOCITY,player.velocity);
    }
    if (!runtime::ValidPosition(player.feet)) return false;
    memory.Read(entity+OFFSET_YAW,player.yaw);
    memory.Read(entity+OFFSET_STANCE,player.stance);
    if (!std::isfinite(player.yaw)) return false;
    int rawHealth = 0;
    if (memory.ReadPointer(entity+OFFSET_HEALTHBASE,healthBase) &&
        memory.Read(healthBase+OFFSET_HEALTH,rawHealth) && rawHealth >= 0 && rawHealth <= 10000) {
        player.healthRead = true;
        player.health = rawHealth == 0 ? 0 : std::max(1,rawHealth/100);
    }
    if (actorValid && memory.ReadPointer(actor+OFFSET_SkeletonStarts,skeleton) &&
        memory.ReadPointer(skeleton+OFFSET_SkeletonInfos,pose))
        memory.ReadPointer(pose+OFFSET_BoneInfo,player.joints);
    return true;
}
bool GetBone(const Player& player, std::size_t index, Vec3& world) {
    world = {};
    Vec3 local{};
    if (!ProcessMemory::Pointer(player.joints) ||
        !memory.Read(player.joints+index*OFFSET_BONE_STRIDE,local) || !runtime::Finite(local) ||
        (local.x == 0 && local.y == 0 && local.z == 0) ||
        std::fabs(local.x) > 10 || std::fabs(local.y) > 10 || std::fabs(local.z) > 10) return false;
    world = runtime::BoneWorld(local,player.feet,player.yaw);
    return runtime::ValidPosition(world);
}
std::wstring ReadName(Address entity) {
    Address namePointer = 0;
    std::string name;
    if (!memory.ReadPointer(entity+OFFSET_NAME,namePointer) || !memory.ReadString(namePointer,name,128)) return L"?";
    return Wide(name);
}
void DrawBones(const Player& player) {
    // Read/project each bone at most once per player per frame.
    std::array<Vec3,114> points{};
    std::array<bool,114> checked{}, valid{};
    for (const auto& link : runtime::BoneLinks) {
        for (const auto index : link) {
            if (!checked[index]) {
                Vec3 world{};
                valid[index] = GetBone(player,index,world) && runtime::Project(world,camera,points[index]);
                checked[index] = true;
            }
        }
        if (valid[link[0]] && valid[link[1]])
            DrawLine(points[link[0]].x,points[link[0]].y,points[link[1]].x,points[link[1]].y,1.5f,WHITE(255));
    }
}
bool OnScreen(const Vec3& point) {
    return point.x >= 0 && point.x <= screen_width && point.y >= 0 && point.y <= screen_height;
}
void DrawPlayer(const Player& player, float distance, const Vec3& head, bool actualHead) {
    if (!settings.players) return;
    Vec3 feetScreen{}, headScreen{};
    if (!runtime::Project(player.feet,camera,feetScreen) || !runtime::Project(head,camera,headScreen)) return;
    if (!OnScreen(feetScreen) && !OnScreen(headScreen)) return;
    const bool usableHealth = player.healthRead && runtime::ValidHealth(player.health);
    if (player.mountKnown && runtime::DeadPlayer(player.mounted,player.healthRead,player.health)) {
        SText(static_cast<int>(headScreen.x),static_cast<int>(headScreen.y),centered,pFont,true,RED(255),BLACK(255),L"DEAD");
        return;
    }
    // Unknown mount state is displayed explicitly and never used as an aim target.
    if (player.mountKnown && !runtime::DisplayPlayer(player.mounted,player.healthRead,player.health)) return;
    const float height = std::fabs(feetScreen.y-headScreen.y);
    const float width = height/2.2f;
    const float top = std::min(feetScreen.y,headScreen.y);
    if (settings.boxes && height >= 2 && height < screen_height*2.0f)
        DrawBox(headScreen.x-width*0.5f,top,width,height,player.mounted ? YELLOW(255) : RED(255));
    if (settings.lines && distance < 250)
        DrawLine(camera.halfWidth,static_cast<float>(screen_height),feetScreen.x,feetScreen.y,1.2f,RED(255));
    if (settings.bones && actualHead) DrawBones(player);
    // Vehicle health may be plausible garbage; never draw it as measured health.
    if (settings.health && usableHealth && player.mountKnown && !player.mounted && height >= 2 && height < screen_height*2.0f) {
        const float x = headScreen.x-width*0.5f-5;
        DrawLine(x,top,x,top+height,3,BLACK(200));
        const float filled = height*player.health/100.0f;
        const DWORD color = D3DCOLOR_ARGB(255,255-player.health*2,player.health*2,0);
        DrawLine(x,top+height-filled,x,top+height,2,color);
    }
    const auto name = ReadName(player.entity);
    const wchar_t* state = player.mounted ? L" [vehicle; HP ?]" :
        !player.mountKnown ? L" [mount ?]" : !player.healthRead ? L" [HP ?]" : L"";
    SText(static_cast<int>(feetScreen.x),static_cast<int>(feetScreen.y+5),centered,pFont,true,WHITE(255),BLACK(255),
        L"%ls [%dm]%ls",name.c_str(),static_cast<int>(distance),state);
}
bool Pressed(int key) {
    static bool previous[256]{};
    const bool down = (GetAsyncKeyState(key)&0x8000) != 0;
    const bool pressed = down && !previous[key];
    previous[key] = down;
    return pressed;
}
void SetMenuOpen(bool open, bool restoreGame = true) {
    if (menuOpen == open) return;
    menuOpen = open;
    gui::ResetInput();
    LONG_PTR style = GetWindowLongPtrW(hWnd,GWL_EXSTYLE);
    if (open) {
        style &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    } else {
        style |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    }
    SetWindowLongPtrW(hWnd,GWL_EXSTYLE,style);
    SetWindowPos(hWnd,nullptr,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    if (open) {
        ShowWindow(hWnd,SW_SHOW);
        SetForegroundWindow(hWnd);
        SetFocus(hWnd);
        ClipCursor(nullptr);
        SetCursor(LoadCursorW(nullptr,IDC_ARROW));
    } else {
        ReleaseCapture();
        if (restoreGame && GetForegroundWindow() == hWnd && IsWindow(tWnd)) SetForegroundWindow(tWnd);
    }
}
void HandleKeys() {
    if (Pressed(VK_INSERT)) SetMenuOpen(!menuOpen);
    if (Pressed(VK_END)) running = false;
    if (menuOpen) {
        if (Pressed(VK_ESCAPE)) SetMenuOpen(false);
        return;
    }
    auto toggle = [](int key, bool& setting) {
        if (Pressed(key)) { setting = !setting; gui::MarkDirty(); }
    };
    toggle(VK_F1,settings.players);
    toggle(VK_F2,settings.boxes);
    toggle(VK_F3,settings.cars);
    toggle(VK_F4,settings.items);
    toggle(VK_F5,settings.aim);
    toggle(VK_F6,settings.bones);
    toggle(VK_F7,settings.lines);
    if (Pressed(VK_F8) && memory.writable) teleport = !teleport;
    if (Pressed(VK_F10) && memory.writable) { noclip = !noclip; haveNoclipPosition = false; }
}
void DrawMenu() {
    if (!menuOpen) {
        SText(18,18,lefted,pFont,true,WHITE(230),BLACK(255),L"H1  |  INSERT: menu  |  END: exit");
        return;
    }
    gui::Status status;
    status.connected = memory.process != nullptr;
    status.writable = memory.writable;
    status.weapon = weapon.name;
    status.bulletSpeed = weapon.speed;
    status.detail = Utf8(frameStatus);
    const auto actions = gui::Draw(settings,status);
    if (actions.save) {
        const bool saved = SaveAppSettings(appDirectory/L"settings.ini",settings);
        gui::Saved(saved);
        Log(saved ? "Menu settings saved." : "Could not save menu settings.");
    }
    if (actions.close) SetMenuOpen(false);
    if (actions.quit) running = false;
}
void MoveMouse(const Vec3& screen) {
    if (!OnScreen(screen)) return;
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<LONG>((screen.x-camera.halfWidth)*settings.smoothing);
    input.mi.dy = static_cast<LONG>((screen.y-camera.halfHeight)*settings.smoothing);
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    if (input.mi.dx || input.mi.dy) SendInput(1,&input,sizeof(input));
}
void LegacyMovement(const Player& local, const Vec3* nearest, float deltaSeconds) {
    if (menuOpen || !memory.writable || local.mounted || !local.positionBase) return;
    if (noclip) {
        if (!haveNoclipPosition) { noclipPosition = local.feet; haveNoclipPosition = true; }
        const float step = 4.0f*deltaSeconds;
        if (GetAsyncKeyState('S')&0x8000) noclipPosition.x += step;
        if (GetAsyncKeyState('Z')&0x8000) noclipPosition.x -= step;
        if (GetAsyncKeyState('Q')&0x8000) noclipPosition.z -= step;
        if (GetAsyncKeyState('D')&0x8000) noclipPosition.z += step;
        if (GetAsyncKeyState(VK_SPACE)&0x8000) noclipPosition.y += step;
        if (GetAsyncKeyState(VK_XBUTTON1)&0x8000) noclipPosition.y -= step;
        if (!memory.Write(local.positionBase+OFFSET_VBASEPOS,noclipPosition))
            Status(L"Legacy movement write failed.");
    } else if (teleport && nearest && (GetAsyncKeyState(VK_SPACE)&0x8000)) {
        Vec3 target = *nearest;
        target.z -= 0.3f;
        if (!memory.Write(local.positionBase+OFFSET_VBASEPOS,target)) Status(L"Legacy teleport write failed.");
    }
}
void Frame(float deltaSeconds) {
    weapon = {"Unknown weapon",0,false,false};
    camera = {};
    Address game = 0, localAddress = 0, entity = 0;
    int count = 0;
    if (!memory.ReadPointer(ADDRESS_CGAME,game) ||
        !memory.ReadPointer(game+OFFSET_LocalPly,localAddress)) {
        Status(L"Cannot read local player; offsets/access unverified."); return;
    }
    if (!UpdateCamera()) { Status(L"Cannot read a valid camera matrix."); return; }
    Player local;
    if (!ReadPlayer(localAddress,local)) { Status(L"Cannot read local position."); return; }
    weapon = ReadWeapon(localAddress);
    if (!memory.Read(game+OFFSET_EntCount,count) || count < 0 || count > 8192 ||
        (count && !memory.ReadPointer(localAddress+OFFSET_EntEntry,entity))) {
        Status(L"Invalid entity list/count; check offsets."); return;
    }
    Status(L"Reading data - offsets and vehicle chain still require in-game validation.");
    runtime::AimCandidate best;
    std::unordered_set<Address> visited;
    Vec3 nearestPosition{};
    float nearestDistance = std::numeric_limits<float>::max();
    for (int i = 0; i < count; ++i) {
        if (entity == 0) break; // Null terminator.
        if (!ProcessMemory::Pointer(entity)) { Status(L"Invalid entity pointer."); break; }
        if (!visited.insert(entity).second) break; // Circular list/sentinel.
        Address next = 0;
        if (!memory.Read(entity+OFFSET_EntEntry,next)) {
            Status(L"Entity chain read failed."); break;
        }
        int type = 0;
        if (entity != localAddress && memory.Read(entity+OFFSET_TYPE,type)) {
            if (type == TYPE_Player) {
                Player player;
                if (ReadPlayer(entity,player)) {
                    const float distance = runtime::Distance(player.feet,local.feet);
                    if (distance > 0 && distance < settings.maxDistance) {
                        Vec3 head{};
                        const bool actualHead = GetBone(player,runtime::HeadBone,head);
                        if (!actualHead) {
                            const float height = (player.stance == 6 || player.stance == 7) ? 0.5f :
                                (player.stance == 1 || player.stance == 5) ? 1.2f : 1.8f;
                            head = player.feet + Vec3{0,height,0};
                        }
                        DrawPlayer(player,distance,head,actualHead);
                        const bool alive = player.healthRead && runtime::ValidHealth(player.health);
                        if (alive && !player.mounted && distance < nearestDistance) {
                            nearestDistance = distance; nearestPosition = player.feet;
                        }
                        Vec3 headScreen{};
                        // No visibility test is supplied by the source/guide.
                        // Unknown mount state/failed bones/failed velocity never create a target.
                        if (settings.aim && weapon.usable && actualHead && player.mountKnown && local.mountKnown &&
                            (player.mounted || alive) && player.velocityRead &&
                            runtime::Project(head,camera,headScreen) && OnScreen(headScreen))
                            runtime::Consider(best,headScreen,head,player.velocity,distance,camera,settings.fov);
                    }
                }
            } else {
                const bool car = type == TYPE_OffRoader || type == TYPE_CAR_2 || type == TYPE_CAR_3 || type == TYPE_ATV;
                const bool item = type == TYPE_ITEM || type == TYPE_Grenade || type == TYPE_Weapon ||
                    type == TYPE_Loot || type == TYPE_Ammo || type == TYPE_NonLethal;
                if ((car && settings.cars) || (item && settings.items)) {
                    Vec3 position{}, screen{};
                    if (memory.Read(entity+(car ? OFFSET_CARPOS : OFFSET_ITEMPOS),position) &&
                        runtime::ValidPosition(position) && runtime::Project(position,camera,screen) && OnScreen(screen)) {
                        const float distance = runtime::Distance(position,local.feet);
                        if (car || type == TYPE_Loot || distance < 20)
                            SText(static_cast<int>(screen.x),static_cast<int>(screen.y),centered,pFont,true,
                                car ? YELLOW(255) : LAWNGREEN(255),BLACK(255),L"%ls [%dm]",
                                ReadName(entity).c_str(),static_cast<int>(distance));
                    }
                }
            }
        }
        entity = next;
    }
    // Exactly one mouse update after all candidates have been considered.
    if (!menuOpen && settings.aim && weapon.usable && best.valid && (GetAsyncKeyState(VK_RBUTTON)&0x8000) &&
        GetForegroundWindow() == tWnd) {
        Vec3 predicted{}, screen{};
        const float gravity = settings.drop ? settings.gravity : 0;
        if (runtime::Predict(best.head,best.velocity,best.distance,weapon.speed,gravity,predicted) &&
            runtime::Project(predicted,camera,screen)) MoveMouse(screen);
    }
    LegacyMovement(local,nearestDistance < std::numeric_limits<float>::max() ? &nearestPosition : nullptr,deltaSeconds);
}
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (menuOpen && gui::HandleMessage(window,message,wParam,lParam)) return 1;
    if (message == WM_MOUSEACTIVATE) return menuOpen ? MA_ACTIVATE : MA_NOACTIVATE;
    if (message == WM_NCHITTEST && !menuOpen) return HTTRANSPARENT;
    if (message == WM_DESTROY) { running = false; PostQuitMessage(0); return 0; }
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{}; BeginPaint(window,&paint); EndPaint(window,&paint); return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}
bool AlignOverlay() {
    RECT area{};
    POINT origin{};
    if (!GetClientRect(tWnd,&area) || !ClientToScreen(tWnd,&origin)) return false;
    if (area.right <= 0 || area.bottom <= 0 || IsIconic(tWnd)) return false;
    screen_width = area.right;
    screen_height = area.bottom;
    SetWindowPos(hWnd,HWND_TOPMOST,origin.x,origin.y,screen_width,screen_height,SWP_NOACTIVATE);
    return true;
}
bool RunOverlay() {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.lpszClassName = L"H1UpdatedOverlay";
    windowClass.hCursor = LoadCursorW(nullptr,IDC_ARROW);
    if (!RegisterClassExW(&windowClass)) { Error("Window registration failed.",GetLastError()); return false; }
    hWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        windowClass.lpszClassName,L"H1 Updated",WS_POPUP,0,0,screen_width,screen_height,
        nullptr,nullptr,windowClass.hInstance,nullptr);
    if (!hWnd) { Error("Overlay creation failed.",GetLastError()); return false; }
    AlignOverlay();
    SetLayeredWindowAttributes(hWnd,RGB(0,0,0),255,LWA_COLORKEY);
    const MARGINS margins{-1,-1,-1,-1};
    DwmExtendFrameIntoClientArea(hWnd,&margins);
    if (!init(hWnd)) {
        Error("Direct3D initialization failed.");
        DestroyWindow(hWnd); return false;
    }
    if (!gui::Initialize(hWnd,d3ddev)) {
        Error("Menu initialization failed.");
        ReleaseRenderer(); DestroyWindow(hWnd); return false;
    }
    ULONGLONG previous = GetTickCount64();
    while (running && IsWindow(tWnd) && WaitForSingleObject(memory.process,0) == WAIT_TIMEOUT) {
        MSG message{};
        while (PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
            if (message.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        if (!running) break;
        const HWND foreground = GetForegroundWindow();
        if ((foreground != tWnd && foreground != hWnd) || !AlignOverlay()) {
            if (menuOpen) SetMenuOpen(false,false);
            ShowWindow(hWnd,SW_HIDE);
            previous = GetTickCount64();
            Sleep(50); continue;
        }
        HandleKeys();
        if (!running) break;
        ShowWindow(hWnd,SW_SHOWNOACTIVATE);
        if (!ResizeRenderer()) { Sleep(50); continue; }
        const auto now = GetTickCount64();
        const float delta = std::clamp(static_cast<float>(now-previous)/1000.0f,0.001f,0.1f);
        previous = now;
        d3ddev->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_ARGB(0,0,0,0),1,0);
        if (SUCCEEDED(d3ddev->BeginScene())) {
            gui::BeginFrame(menuOpen);
            Frame(delta);
            DrawMenu();
            gui::Render();
            d3ddev->EndScene();
        }
        d3ddev->Present(nullptr,nullptr,nullptr,nullptr);
        Sleep(1);
    }
    if (menuOpen) SetMenuOpen(false);
    gui::Shutdown();
    ReleaseRenderer();
    if (IsWindow(hWnd)) DestroyWindow(hWnd);
    UnregisterClassW(windowClass.lpszClassName,windowClass.hInstance);
    Log("Stopped.");
    return true;
}
} // namespace

int main(int argc, char** argv) {
    // --check-startup never opens a game process or moves input.
    bool startupCheck = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--check-startup") startupCheck = true;
        else if (std::string(argv[i]) == "--diagnose") diagnosticOnly = true;
        else { std::cerr << "Options: --check-startup, --diagnose\n"; return 2; }
    }
    wchar_t executable[32768]{};
    const DWORD length = GetModuleFileNameW(nullptr,executable,_countof(executable));
    if (!length || length == _countof(executable)) return 1;
    appDirectory = std::filesystem::path(executable).parent_path();
    logFile.open(appDirectory / L"h1.log",std::ios::app);
    LoadSettings();
    Log("H1 updated source build. Configuration: " + Utf8((appDirectory / L"settings.ini").wstring()));
    Log("Forum offsets are unverified. Drop compensation: " + std::string(settings.drop ? "on" : "off"));
    if (startupCheck) {
        Log("Startup/configuration check passed. No process attached; no input sent.");
        return 0;
    }
    SetProcessDPIAware();
    if (!InitProcess()) return 1;
    if (diagnosticOnly) {
        const bool valid = UpdateCamera();
        Log(valid ? "Camera data read; rendering/game correctness not verified." : "Camera read failed.");
        return valid ? 0 : 1;
    }
    return RunOverlay() ? 0 : 1;
}

