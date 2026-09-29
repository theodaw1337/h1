#include "Client.h"
#include <iostream>
#include <cstdlib>
int main(int argc,char** argv) {
    if (argc<2 || (std::string(argv[1])!="publish" && std::string(argv[1])!="read")) {
        std::cout<<"Usage: offline_client.exe publish [seconds=60] | read\n"; return 2;
    }
    const bool publish=std::string(argv[1])=="publish";
    LabClient client;
    if (!client.Open(publish)) {
        std::cerr<<"Offline driver unavailable. Windows error "<<client.error
                 <<". Use a properly signed driver in the test VM; no automatic installation.\n";
        return 1;
    }
    if (!publish) {
        LAB_SNAPSHOT frame{};
        if (!client.Read(frame)) { std::cerr<<"Read failed: "<<client.error<<'\n'; return 1; }
        std::cout<<"Sequence "<<frame.sequence<<", actors "<<frame.count<<'\n';
        for (uint32_t i=0;i<frame.count;++i) {
            const auto& a=frame.actors[i];
            std::cout<<std::string(a.name,strnlen_s(a.name,sizeof(a.name)))<<" x="<<a.x_mm<<" z="<<a.z_mm<<" mm, HP="<<a.health<<'\n';
        }
        return 0;
    }
    const int seconds=argc>2 ? atoi(argv[2]) : 60;
    if (seconds<1 || seconds>3600) { std::cerr<<"Duration must be 1..3600 seconds.\n"; return 2; }
    const auto start=GetTickCount64();
    while (GetTickCount64()-start<static_cast<ULONGLONG>(seconds)*1000) {
        const auto frame=GenerateLabFrame(static_cast<double>(GetTickCount64()-start)/1000);
        if (!client.Publish(frame)) { std::cerr<<"Publish failed: "<<client.error<<'\n'; return 1; }
        Sleep(33);
    }
    std::cout<<"Synthetic frames published.\n";
}
