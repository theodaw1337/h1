#include "Client.h"
#include <iostream>
#include <cstring>
int main() {
    int checks=0;
    auto check=[&](bool value,const char* label) { ++checks; if (!value) { std::cerr<<label<<'\n'; std::exit(1); } };
    LAB_SNAPSHOT state{},buffer{}; uint32_t written=999;
    LabInitialize(&state);
    check(LabExchange(&state,LAB_READ,&buffer,0,sizeof(buffer),&written)==LabOk && buffer.count==0 && written==sizeof(buffer),"empty initialized snapshot");
    auto input=GenerateLabFrame(1.0);
    check(LabExchange(&state,LAB_PUBLISH,&input,sizeof(input),0,&written)==LabOk && written==0,"publish");
    check(LabExchange(&state,LAB_READ,&buffer,0,sizeof(buffer),&written)==LabOk && buffer.sequence==1 && buffer.count==6 && buffer.actors[0].x_mm==input.actors[0].x_mm,"complete round trip");
    const auto unchanged=state;
    auto invalid=[&](LAB_SNAPSHOT bad,const char* label) {
        check(LabExchange(&state,LAB_PUBLISH,&bad,sizeof(bad),0,&written)==LabInvalid && written==0 &&
            std::memcmp(&state,&unchanged,sizeof(state))==0,label);
    };
    auto bad=input; bad.version=55; invalid(bad,"version mismatch");
    bad=input; bad.count=33; invalid(bad,"too many actors");
    bad=input; bad.actors[5].health=101; invalid(bad,"last actor invalid; no partial publish");
    bad=input; bad.actors[0].x_mm=INT32_MIN; invalid(bad,"invalid coordinate");
    bad=input; std::memset(bad.actors[0].name,'x',24); invalid(bad,"unterminated name");
    bad=input; bad.actors[0].reserved=1; invalid(bad,"reserved field");
    bad=input; bad.actors[1].id=bad.actors[0].id; invalid(bad,"duplicate id");
    check(LabExchange(&state,LAB_READ,&buffer,0,1,&written)==LabSmallBuffer && written==0,"short output");
    check(LabExchange(&state,LAB_READ,nullptr,0,sizeof(buffer),&written)==LabSmallBuffer,"null buffer");
    check(LabExchange(&state,LAB_PUBLISH,&input,sizeof(input)-1,0,&written)==LabInvalid,"truncated input");
    check(LabExchange(&state,LAB_PUBLISH,&input,sizeof(input),sizeof(buffer),&written)==LabInvalid,"unexpected output");
    check(LabExchange(&state,0xffffffff,&input,sizeof(input),0,&written)==LabUnsupported,"unknown IOCTL");
    input.count=1; input.sequence=9999;
    check(LabExchange(&state,LAB_PUBLISH,&input,sizeof(input),0,&written)==LabOk && state.sequence==2,"sequence belongs to driver");
    LAB_ACTOR zero{};
    check(std::memcmp(&state.actors[1],&zero,sizeof(zero))==0,"unused actor bytes zeroed");
    std::cout<<checks<<" offline protocol checks passed; kernel not loaded.\n";
}
