#pragma once

#define M_PI 3.1415926535897931

// Absolute addresses from the user's guide, claimed 2026-09-22; unverified in game.
// Do not add the process module base to these values.
#define ADDRESS_CGAME 0x14476D290
#define ADDRESS_CGRAPHICS 0x14476E748
#define ADDRESS_PHYSICS 0x1462B65A8

#define OFFSET_LocalPly 0x780
#define OFFSET_EntEntry 0x748

#define OFFSET_EntCount 0x50

#define OFFSET_YAW 0x530
#define OFFSET_PITCH 0x534
#define OFFSET_STANCE 0x954
#define OFFSET_PLAYER_BASE 0x300
#define OFFSET_NEXTVEH 0x700
#define OFFSET_NEXTITM 0x718
#define OFFSET_NEXTPLY 0x720

#define OFFSET_CAMERA 0x48
#define OFFSET_MATRIX 0x30
#define OFFSET_MATRIX_DEF 0x1B0

#define OFFSET_HEALTHBASE 0x3DC8
#define OFFSET_HEALTH 0xB0
#define OFFSET_VBASEPOS 0x3A8
#define OFFSET_VPELVISPOS 0x3F0
#define OFFSET_ITEMPOS 0x18C0
#define OFFSET_CARPOS 0x9D0
#define OFFSET_TYPE 0x3F8
#define OFFSET_NAME 0x7B8
#define OFFSET_VELOCITY 0x430
#define OFFSET_ID 0x688

#define OFFSET_SkeletonActors 0x5C0
#define OFFSET_SkeletonStarts  0x250
#define OFFSET_SkeletonInfos  0x50
#define OFFSET_BoneInfo  0x28
#define OFFSET_BONE_STRIDE 0x30
// The guide's actor pointer is ambiguous. We use entity+0x5C0; validate in game.
#define OFFSET_MOUNT_FROM_ACTOR 0x3A08
#define OFFSET_INVEHICLE_ENTITYINFO 0x18 // Different structure, not interchangeable.

#define OFFSET_SPEED 0x45D8
#define OFFSET_SPEEDVERTICAL 0x45D0

#define OFFSET_INVENTORY 0xCE8
#define OFFSET_INVENTORY_NAME 0x24
#define OFFSET_WEAPON_CONTAINER 0x688
#define OFFSET_ACTIVE_WEAPON 0xB8
#define OFFSET_WEAPON_MODEL 0x20
#define OFFSET_WEAPON_CONTEXT 0xBA8

#define TYPE_Loot 0x2E
#define TYPE_Player 0x10E
#define TYPE_Weapon 0x34
#define TYPE_Ammo 0x15
#define TYPE_Grenade 0xAE
#define TYPE_NonLethal 0xB2 
#define TYPE_OffRoader 0x11
#define TYPE_CAR_2 0x72
#define TYPE_CAR_3 0x76
#define TYPE_ATV 0xCC
#define TYPE_ITEM 0x2C
