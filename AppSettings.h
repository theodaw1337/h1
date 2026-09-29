#pragma once
struct AppSettings {
    bool players = true, boxes = true, bones = true, lines = true;
    bool health = true, cars = false, items = true, aim = false;
    bool drop = false, allowWrites = false;
    float fov = 80, smoothing = 0.25f, maxDistance = 500, gravity = 0;
    int weaponPath = 0;
};
