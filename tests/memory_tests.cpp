#include "Lol2.h"
#include <array>
#include <cstdlib>

int checks = 0;
void Check(bool condition, const char* message) {
    ++checks;
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
int main() {
    ProcessMemory memory;
    // Only our own test process; no game access.
    memory.process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,FALSE,GetCurrentProcessId());
    Check(memory.process != nullptr,"open own test process");
    int source = 12345, result = 0;
    const auto address = reinterpret_cast<std::uint64_t>(&source);
    Check(memory.Read(address,result) && result == source,"typed read");
    Check(!memory.Read(0,result) && result == 0,"null read returns failure and clears output");
    const char text[] = "Weapons_AR15_3P.adr";
    std::string read;
    Check(memory.ReadString(reinterpret_cast<std::uint64_t>(text),read) && read == text,"bounded string read");
    std::array<char,8> unterminated{};
    unterminated.fill('x');
    Check(!memory.ReadString(reinterpret_cast<std::uint64_t>(unterminated.data()),read,unterminated.size()) && read.empty(),"unterminated string rejected");
    const char empty[] = "";
    Check(!memory.ReadString(reinterpret_cast<std::uint64_t>(empty),read),"empty asset name rejected");
    std::uint64_t nullPointer = 0, pointer = 1;
    Check(!memory.ReadPointer(reinterpret_cast<std::uint64_t>(&nullPointer),pointer),"null pointer-chain link rejected");
    Check(!memory.Write(address,42) && source == 12345,"read-only mode refuses writes");
    Check(!ProcessMemory::Pointer(0x800000000000ull),"kernel/noncanonical address rejected");
    SYSTEM_INFO info{}; GetSystemInfo(&info);
    auto pages = static_cast<char*>(VirtualAlloc(nullptr,info.dwPageSize*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Check(pages != nullptr,"allocate boundary fixture");
    DWORD oldProtection = 0;
    Check(VirtualProtect(pages+info.dwPageSize,info.dwPageSize,PAGE_NOACCESS,&oldProtection) != FALSE,"protect second page");
    std::array<char,8> buffer; buffer.fill('y');
    Check(!memory.Read(reinterpret_cast<std::uint64_t>(pages+info.dwPageSize-4),buffer),"partial cross-page read rejected");
    Check(std::all_of(buffer.begin(),buffer.end(),[](char c){return c == 0;}),"failed partial read clears entire output");
    VirtualFree(pages,0,MEM_RELEASE);
    std::cout << checks << " memory checks passed using only this test process.\n";
}
