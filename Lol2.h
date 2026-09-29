#pragma once
#include "Lol1.h"
#include "Offs.h"
#include "RuntimeLogic.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <limits>

// Normal Windows process APIs only. No driver or mapper is integrated.
class ProcessMemory {
public:
    HANDLE process = nullptr;
    bool writable = false;
    ~ProcessMemory() { if (process) CloseHandle(process); }
    ProcessMemory() = default;
    ProcessMemory(const ProcessMemory&) = delete;
    ProcessMemory& operator=(const ProcessMemory&) = delete;
    static bool Pointer(std::uint64_t address) {
        return address >= 0x10000 && address < 0x0000800000000000ull;
    }
    template<class T> bool Read(std::uint64_t address, T& value) const {
        value = {};
        if (!process || !Pointer(address) || address > 0x0000800000000000ull-sizeof(T)) return false;
        SIZE_T bytes = 0;
        if (!ReadProcessMemory(process, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), &bytes) || bytes != sizeof(T)) {
            value = {};
            return false;
        }
        return true;
    }
    bool ReadPointer(std::uint64_t address, std::uint64_t& value) const {
        return Read(address, value) && Pointer(value);
    }
    bool ReadString(std::uint64_t address, std::string& text, std::size_t limit = 128) const {
        text.clear();
        if (!Pointer(address)) return false;
        for (std::size_t i = 0; i < limit; ++i) {
            char c = 0;
            if (!Read(address+i, c)) { text.clear(); return false; }
            if (c == 0) return !text.empty();
            text.push_back(c);
        }
        text.clear();
        return false;
    }
    template<class T> bool Write(std::uint64_t address, const T& value) const {
        if (!writable || !Pointer(address)) return false;
        SIZE_T written = 0;
        return WriteProcessMemory(process, reinterpret_cast<LPVOID>(address), &value, sizeof(T), &written) && written == sizeof(T);
    }
};
