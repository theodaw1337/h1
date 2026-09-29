#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>

namespace runtime {
struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3 operator+(const Vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};
inline bool Finite(const Vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
inline bool ValidPosition(const Vec3& v) {
    return Finite(v) && std::fabs(v.x) < 15000 && std::fabs(v.y) < 15000 &&
        std::fabs(v.z) < 15000 && (v.x != 0 || v.y != 0 || v.z != 0);
}
inline float Distance(const Vec3& a, const Vec3& b) {
    return std::hypot(std::hypot(a.x - b.x, a.y - b.y), a.z - b.z);
}
struct Matrix { float m[4][4]{}; };
struct CameraCache {
    Matrix matrix{};
    float halfWidth = 0, halfHeight = 0;
    bool valid = false;
};
inline CameraCache MakeCamera(const Matrix& raw, int width, int height) {
    CameraCache camera;
    if (width <= 0 || height <= 0) return camera;
    bool nonzero = false;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float value = raw.m[col][row]; // The supplied matrix is transposed.
            if (!std::isfinite(value)) return {};
            if (row == 1) value = -value;
            camera.matrix.m[row][col] = value;
            nonzero = nonzero || value != 0;
        }
    }
    if (!nonzero) return {};
    camera.halfWidth = width * 0.5f;
    camera.halfHeight = height * 0.5f;
    camera.valid = true;
    return camera;
}
inline bool Project(const Vec3& world, const CameraCache& camera, Vec3& screen) {
    screen = {};
    if (!camera.valid || !Finite(world)) return false;
    const auto& m = camera.matrix.m;
    const float w = m[3][0]*world.x + m[3][1]*world.y + m[3][2]*world.z + m[3][3];
    if (!std::isfinite(w) || w < 0.098f) return false;
    const float x = m[0][0]*world.x + m[0][1]*world.y + m[0][2]*world.z + m[0][3];
    const float y = m[1][0]*world.x + m[1][1]*world.y + m[1][2]*world.z + m[1][3];
    screen = {camera.halfWidth*(1 + x/w), camera.halfHeight*(1 - y/w), 0};
    if (!Finite(screen)) { screen = {}; return false; }
    return true;
}
inline Vec3 BoneWorld(const Vec3& local, const Vec3& feet, float yaw) {
    const float s = std::sin(yaw), c = std::cos(yaw);
    return {feet.x + local.x*c - local.z*s, feet.y + local.y,
        feet.z + local.x*s + local.z*c};
}
inline constexpr std::size_t HeadBone = 26;
inline constexpr std::array<std::array<std::size_t, 2>, 16> BoneLinks{{
    {12,22}, {22,23}, {23,24}, {24,26},
    {23,52}, {52,54}, {54,55}, {23,76}, {76,78}, {78,79},
    {12,13}, {13,14}, {14,16}, {12,110}, {110,111}, {111,113}
}};
struct WeaponProfile {
    std::string name;
    float speed = 0;
    bool recognized = false;
    bool usable = false;
};
inline WeaponProfile ClassifyWeapon(std::string_view name) {
    struct Entry { std::string_view name; float speed; };
    // Forum values, not independently verified against a running game.
    static constexpr Entry entries[] = {
        {"Weapons_AR15_3P.adr",375}, {"Weapons_AK47_3P.adr",375},
        {"Weapons_M40Sniper_3P.adr",659}, {"Weapons_RanchRifle_3P.adr",350},
        {"Weapons_MK46_3P.adr",250}, {"Weapons_RiotShotgun_3P.adr",175},
        {"Weapons_M9_3P.adr",251}, {"Weapon_Pistol_45Auto_3P.adr",251},
        {"Weapons_Magnum_3P.adr",251}, {"Weapons_Crossbow01_3P.adr",120},
        {"Weapons_Bow01_3P.adr",75}, {"Weapon_Empty.adr",0},
        {"Weapon_Binoculars_3P.adr",0}, {"Weapons_Grenades_SmokeGrenade_3P.adr",0},
        {"Weapons_MolotovCocktail_3P.adr",0}, {"Weapons_Grenades_FlashBang_3P.adr",0},
        {"Weapons_Grenades_HEGrenade_3P.adr",0}, {"Weapons_Grenades_GasGrenade_3P.adr",0}
    };
    const auto slash = name.find_last_of("/\\");
    if (slash != std::string_view::npos) name.remove_prefix(slash + 1);
    for (const auto& entry : entries)
        if (name == entry.name) return {std::string(name), entry.speed, true, entry.speed > 0};
    return {std::string(name), 0, false, false};
}
inline WeaponProfile ResolveWeapon(const WeaponProfile& inventory, const WeaponProfile& container, int mode) {
    if (mode == 1) return inventory;
    if (mode == 2) return container;
    if (inventory.recognized && container.recognized && inventory.name != container.name)
        return {"Conflicting weapon paths", 0, false, false};
    if (inventory.recognized) return inventory;
    if (container.recognized) return container;
    return {"Unknown weapon", 0, false, false};
}
inline bool Predict(const Vec3& head, const Vec3& velocity, float distance,
                    float speed, float gravity, Vec3& predicted) {
    predicted = {};
    if (!Finite(head) || !Finite(velocity) || !std::isfinite(distance) || distance <= 0 ||
        !std::isfinite(speed) || speed <= 0 || !std::isfinite(gravity) || gravity < 0) return false;
    const float time = distance / speed;
    predicted = head + velocity*time;
    predicted.y += 0.5f*gravity*time*time;
    if (!Finite(predicted)) { predicted = {}; return false; }
    return true;
}
inline bool ValidHealth(int health) { return health > 0 && health <= 100; }
inline bool DisplayPlayer(bool inVehicle, bool healthRead, int health) {
    return inVehicle || (healthRead && ValidHealth(health));
}
inline bool DeadPlayer(bool inVehicle, bool healthRead, int health) {
    return !inVehicle && healthRead && health == 0;
}
struct AimCandidate {
    bool valid = false;
    float radius = std::numeric_limits<float>::max();
    Vec3 head{}, velocity{};
    float distance = 0;
};
inline void Consider(AimCandidate& best, const Vec3& screen, const Vec3& head, const Vec3& velocity,
                     float distance, const CameraCache& camera, float fov) {
    if (!Finite(screen) || !Finite(head) || !Finite(velocity) || !camera.valid ||
        !std::isfinite(distance) || distance <= 0) return;
    const float radius = std::hypot(screen.x-camera.halfWidth, screen.y-camera.halfHeight);
    if (radius < fov && (!best.valid || radius < best.radius))
        best = {true, radius, head, velocity, distance};
}
} // namespace runtime
