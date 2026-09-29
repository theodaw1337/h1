#pragma once
#include "AppSettings.h"
#include <filesystem>
#include <windows.h>
#include <string>

// Update a sibling copy, preserving comments and unrelated keys. Replace only
// after every value has been written successfully.
inline bool SaveAppSettings(const std::filesystem::path& path, const AppSettings& s) {
    const std::wstring target = path.wstring();
    const std::wstring temporary = target + L".pending-" + std::to_wstring(GetCurrentProcessId());
    if (!CopyFileW(target.c_str(),temporary.c_str(),FALSE) && GetLastError() != ERROR_FILE_NOT_FOUND) return false;
    bool success = true;
    auto write = [&](const wchar_t* section, const wchar_t* key, const std::wstring& value) {
        if (!WritePrivateProfileStringW(section,key,value.c_str(),temporary.c_str())) success = false;
    };
    write(L"Aim",L"Enabled",s.aim ? L"1" : L"0");
    write(L"Aim",L"FovPixels",std::to_wstring(s.fov));
    write(L"Aim",L"Smoothing",std::to_wstring(s.smoothing));
    write(L"Aim",L"MaxDistance",std::to_wstring(s.maxDistance));
    write(L"Aim",L"DropCompensation",s.drop ? L"1" : L"0");
    write(L"Aim",L"Gravity",std::to_wstring(s.gravity));
    write(L"Weapon",L"Path",std::to_wstring(s.weaponPath));
    write(L"Display",L"Players",s.players ? L"1" : L"0");
    write(L"Display",L"Boxes",s.boxes ? L"1" : L"0");
    write(L"Display",L"Bones",s.bones ? L"1" : L"0");
    write(L"Display",L"Lines",s.lines ? L"1" : L"0");
    write(L"Display",L"Health",s.health ? L"1" : L"0");
    write(L"Display",L"Cars",s.cars ? L"1" : L"0");
    write(L"Display",L"Items",s.items ? L"1" : L"0");
    write(L"Legacy",L"AllowMemoryWrites",s.allowWrites ? L"1" : L"0");
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,temporary.c_str());
    if (success) success = MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    if (!success) DeleteFileW(temporary.c_str());
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,target.c_str());
    return success;
}
