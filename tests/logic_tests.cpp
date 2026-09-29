#include "RuntimeLogic.h"
#include <iostream>
#include <limits>
#include <cstdlib>

int checks = 0;
void Check(bool result, const char* message) {
    ++checks;
    if (!result) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
bool Near(float a, float b) { return std::fabs(a-b) < 0.001f; }
int main() {
    using namespace runtime;
    Matrix matrix{};
    for (int i = 0; i < 4; ++i) matrix.m[i][i] = 1;
    const auto camera = MakeCamera(matrix,1920,1080);
    Vec3 screen{};
    Check(Project({0,0,0},camera,screen) && Near(screen.x,960) && Near(screen.y,540), "center projects to viewport center");
    Check(Project({1,1,0},camera,screen) && Near(screen.x,1920) && Near(screen.y,1080), "matrix Y inversion applied exactly once");
    Check(Project({-1,-1,0},camera,screen) && Near(screen.x,0) && Near(screen.y,0), "screen origin is a valid projection");
    auto perspective = camera;
    perspective.matrix.m[3][3] = 0;
    perspective.matrix.m[3][2] = 1;
    Check(!Project({0,0,-1},perspective,screen), "behind-camera rejected");
    Check(!Project({0,0,0.01f},perspective,screen), "near plane rejected");
    Check(Project({1,0,2},perspective,screen) && Near(screen.x,1440), "perspective divide");
    Check(!Project({0,0,0},{},screen), "invalid camera rejected");
    Check(!Project({std::numeric_limits<float>::infinity(),0,0},camera,screen), "infinite world position rejected");
    Check(!MakeCamera(matrix,0,1080).valid, "zero viewport rejected");
    Check(!MakeCamera({},1920,1080).valid, "zero matrix rejected");
    matrix.m[0][1] = std::numeric_limits<float>::quiet_NaN();
    Check(!MakeCamera(matrix,1920,1080).valid, "NaN matrix rejected");
    matrix = {};
    matrix.m[3][0] = 0.5f; matrix.m[3][3] = 1;
    const auto translated = MakeCamera(matrix,1920,1080);
    Check(Project({0,0,0},translated,screen) && Near(screen.x,1440), "raw matrix transposed before projection");

    const auto bone = BoneWorld({1,2,0},{100,10,100},1.57079632679f);
    Check(Near(bone.x,100) && Near(bone.y,12) && Near(bone.z,101), "bone rotation is in world XZ plane");
    Check(!ValidPosition({0,0,0}) && !ValidPosition({16000,0,0}) && ValidPosition({1,0,1}), "position sanity bounds");
    Check(BoneLinks.back()[0] == 111 && BoneLinks.back()[1] == 113, "right leg indices preserved");

    Vec3 predicted{};
    Check(Predict({100,10,0},{10,0,0},200,375,0,predicted) && Near(predicted.x,105.333333f) && Near(predicted.y,10), "velocity lead in world units");
    Check(Predict({100,10,0},{10,2,-3},200,375,9.81f,predicted) &&
        Near(predicted.y,12.461866f) && Near(predicted.z,-1.6f), "synthetic gravity and vertical velocity combined");
    Check(!Predict({}, {}, 100,0,0,predicted), "zero bullet speed rejected");
    Check(!Predict({}, {}, -1,375,0,predicted), "negative distance rejected");
    Check(!Predict({}, {}, 100,375,-1,predicted), "negative gravity rejected");
    Check(!Predict({}, {0,std::numeric_limits<float>::quiet_NaN(),0},100,375,0,predicted), "NaN velocity rejected");

    const auto rifle = ClassifyWeapon("Weapons_AR15_3P.adr");
    const auto sniper = ClassifyWeapon("Weapons_M40Sniper_3P.adr");
    Check(rifle.usable && rifle.speed == 375 && sniper.speed == 659, "per-weapon speeds");
    Check(ClassifyWeapon("assets/Weapons_Bow01_3P.adr").speed == 75, "bow asset path");
    Check(ClassifyWeapon("assets\\Weapons_Crossbow01_3P.adr").speed == 120, "crossbow distinct from bow");
    Check(ClassifyWeapon("Weapon_Pistol_45Auto_3P.adr").speed == 251, "M1911 model recognized");
    Check(!ClassifyWeapon("Weapons_Grenades_HEGrenade_3P.adr").usable, "grenades do not enable aim");
    Check(!ClassifyWeapon("Weapon_Empty.adr").usable && !ClassifyWeapon("Weapon_Binoculars_3P.adr").usable, "fists and binoculars do not enable aim");
    Check(!ClassifyWeapon("bad_Weapons_AR15_3P.adr").recognized, "substring garbage rejected");
    Check(!ClassifyWeapon("unknown.adr").usable, "unknown weapon has no silent rifle fallback");
    Check(ResolveWeapon(rifle,{},0).speed == 375, "auto selects sole recognized path");
    Check(ResolveWeapon({},sniper,0).speed == 659, "auto can select container path");
    Check(!ResolveWeapon(rifle,sniper,0).usable, "conflicting paths disable prediction");
    Check(ResolveWeapon(rifle,sniper,1).speed == 375 && ResolveWeapon(rifle,sniper,2).speed == 659, "explicit path selection");

    Check(DisplayPlayer(true,false,0) && !DeadPlayer(true,true,0), "vehicle occupants survive unreliable health");
    Check(!DisplayPlayer(false,false,0) && !DeadPlayer(false,false,0), "failed health read is not death");
    Check(DeadPlayer(false,true,0) && DisplayPlayer(false,true,100), "on-foot health gate");
    Check(!DisplayPlayer(false,true,101), "out-of-range health rejected");
    AimCandidate best;
    Consider(best,{1000,540,0},{1,1,1},{},100,camera,80);
    Consider(best,{970,540,0},{2,2,2},{},200,camera,80);
    Consider(best,{1020,540,0},{3,3,3},{},300,camera,80);
    Check(best.valid && best.head.x == 2 && Near(best.radius,10), "closest crosshair candidate wins regardless of traversal order");
    AimCandidate outside;
    Consider(outside,{1200,540,0},{1,1,1},{},100,camera,80);
    Check(!outside.valid, "outside FOV excluded");
    std::cout << checks << " logic checks passed. No game access or mouse input.\n";
}
