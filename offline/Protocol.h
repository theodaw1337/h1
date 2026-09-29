#pragma once
#include <stdint.h>
#define LAB_VERSION 1u
#define LAB_MAX_ACTORS 32u
#define LAB_DEVICE_TYPE 0x8337u
#define LAB_READ CTL_CODE(LAB_DEVICE_TYPE,0x800,METHOD_BUFFERED,FILE_READ_ACCESS)
#define LAB_PUBLISH CTL_CODE(LAB_DEVICE_TYPE,0x801,METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define LAB_DEVICE_PATH L"\\\\.\\H1OfflineLab"

// Fixed-size values only. No process identifiers, addresses or user pointers.
typedef struct LAB_ACTOR {
    uint32_t id;
    int32_t x_mm, y_mm, z_mm;
    uint32_t health;
    char name[24];
    uint32_t reserved;
} LAB_ACTOR;
typedef struct LAB_SNAPSHOT {
    uint32_t version, count;
    uint64_t sequence;
    LAB_ACTOR actors[LAB_MAX_ACTORS];
} LAB_SNAPSHOT;

typedef enum LAB_RESULT { LabOk, LabInvalid, LabSmallBuffer, LabUnsupported } LAB_RESULT;
static __inline void LabInitialize(LAB_SNAPSHOT* state) {
    unsigned int i;
    unsigned char* bytes = (unsigned char*)state;
    for (i=0;i<sizeof(*state);++i) bytes[i]=0;
    state->version=LAB_VERSION;
}
// Caller serializes access. Kernel caller supplies only the I/O manager's
// METHOD_BUFFERED SystemBuffer. This function never follows embedded pointers.
static __inline LAB_RESULT LabExchange(LAB_SNAPSHOT* state, uint32_t code, void* buffer,
    uint32_t inputLength, uint32_t outputLength, uint32_t* written) {
    uint32_t i,j;
    LAB_SNAPSHOT* packet=(LAB_SNAPSHOT*)buffer;
    *written=0;
    if (code==LAB_READ) {
        if (inputLength!=0) return LabInvalid;
        if (!buffer || outputLength<sizeof(*state)) return LabSmallBuffer;
        *packet=*state;
        *written=sizeof(*state);
        return LabOk;
    }
    if (code!=LAB_PUBLISH) return LabUnsupported;
    if (!buffer || inputLength!=sizeof(*state) || outputLength!=0) return LabInvalid;
    if (packet->version!=LAB_VERSION || packet->count>LAB_MAX_ACTORS) return LabInvalid;
    for (i=0;i<packet->count;++i) {
        const LAB_ACTOR* actor=&packet->actors[i];
        if (actor->health>100 || actor->reserved!=0 || actor->id==0 ||
            actor->x_mm < -1000000 || actor->x_mm > 1000000 ||
            actor->y_mm < -1000000 || actor->y_mm > 1000000 ||
            actor->z_mm < -1000000 || actor->z_mm > 1000000) return LabInvalid;
        for (j=0;j<sizeof(actor->name) && actor->name[j]!=0;++j) {}
        if (j==0 || j==sizeof(actor->name)) return LabInvalid;
        for (j=0;j<i;++j) if (packet->actors[j].id==actor->id) return LabInvalid;
    }
    // Validate the entire packet before changing the published snapshot.
    state->count=packet->count;
    ++state->sequence;
    for (i=0;i<LAB_MAX_ACTORS;++i) {
        unsigned char* bytes=(unsigned char*)&state->actors[i];
        for (j=0;j<sizeof(LAB_ACTOR);++j) bytes[j]=0;
        if (i<packet->count) {
            state->actors[i].id=packet->actors[i].id;
            state->actors[i].x_mm=packet->actors[i].x_mm;
            state->actors[i].y_mm=packet->actors[i].y_mm;
            state->actors[i].z_mm=packet->actors[i].z_mm;
            state->actors[i].health=packet->actors[i].health;
            for (j=0;j<sizeof(packet->actors[i].name);++j) {
                state->actors[i].name[j]=packet->actors[i].name[j];
                if (packet->actors[i].name[j]==0) break;
            }
        }
    }
    return LabOk;
}
