#pragma once
#include <windows.h>
#include <winioctl.h>
#include "Protocol.h"
#include <cmath>
#include <cstdio>
#include <string>

static_assert(sizeof(LAB_ACTOR)==48 && sizeof(LAB_SNAPSHOT)==1552,"Protocol ABI changed");
class LabClient {
    HANDLE device=INVALID_HANDLE_VALUE;
public:
    DWORD error=0;
    ~LabClient() { if (device!=INVALID_HANDLE_VALUE) CloseHandle(device); }
    bool Open(bool publisher=false) {
        if (device!=INVALID_HANDLE_VALUE) { CloseHandle(device); device=INVALID_HANDLE_VALUE; }
        device=CreateFileW(LAB_DEVICE_PATH,GENERIC_READ|(publisher ? GENERIC_WRITE : 0),
            FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
        error=device==INVALID_HANDLE_VALUE ? GetLastError() : 0;
        return !error;
    }
    bool Read(LAB_SNAPSHOT& frame) {
        frame={}; DWORD bytes=0;
        if (!DeviceIoControl(device,LAB_READ,nullptr,0,&frame,sizeof(frame),&bytes,nullptr)) {
            error=GetLastError(); return false;
        }
        if (bytes!=sizeof(frame) || frame.version!=LAB_VERSION || frame.count>LAB_MAX_ACTORS) {
            frame={}; error=ERROR_INVALID_DATA; return false;
        }
        error=0; return true;
    }
    bool Publish(const LAB_SNAPSHOT& frame) {
        DWORD bytes=0;
        if (!DeviceIoControl(device,LAB_PUBLISH,const_cast<LAB_SNAPSHOT*>(&frame),sizeof(frame),nullptr,0,&bytes,nullptr)) {
            error=GetLastError(); return false;
        }
        error=0; return true;
    }
};
inline LAB_SNAPSHOT GenerateLabFrame(double seconds) {
    LAB_SNAPSHOT frame{}; frame.version=LAB_VERSION; frame.count=6;
    for (uint32_t i=0;i<frame.count;++i) {
        auto& actor=frame.actors[i];
        actor.id=i+1;
        const double angle=seconds*0.3+i*1.047197551;
        actor.x_mm=static_cast<int32_t>(std::cos(angle)*(15000+i*3500));
        actor.z_mm=static_cast<int32_t>(std::sin(angle)*(15000+i*3500));
        actor.y_mm=1800;
        actor.health=100-i*12;
        sprintf_s(actor.name,"Test %u",i+1);
    }
    return frame;
}
