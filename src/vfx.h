#pragma once
#include "raylib.h"
#include "boat.h"
#include "ocean.h"
#include "weather.h"

// Cheap world-space effects drawn after the opaque scene: rain streaks (camera-facing quads in a
// box that wraps around the view), lightning bolts (one per Weather strike) and bow spray.
constexpr int kMaxRain = 1400;
constexpr int kMaxSpray = 220;
constexpr int kMaxBoltPoints = 32;

struct SprayParticle {
    Vector3 pos, vel;
    float life = 0.0f;     // seconds left; <= 0 = dead
    float maxLife = 1.0f;
    float size = 0.2f;
};

struct VfxSettings {
    Vector3 rainBox = { 40.0f, 26.0f, 40.0f };   // size of the wrapping rain volume
    float rainFall = 22.0f;                     // units/s
    float rainStreak = 0.045f;                  // streak length = velocity * this
    float rainWidth = 0.022f;
    float sprayRate = 260.0f;                   // particles/s at full bow slam
    float sprayIdleRate = 10.0f;                // particles/s from just cutting through the water
};

struct Vfx {
    VfxSettings settings;
    Vector3 rain[kMaxRain] = {};       // positions inside the rain box
    SprayParticle spray[kMaxSpray];
    float sprayAccum = 0.0f;
    unsigned int rng = 4242u;
    // Current lightning bolt (regenerated when Weather::strikeCount changes)
    int boltStrike = 0;
    Vector3 bolt[kMaxBoltPoints] = {};
    int boltCount = 0;
    int boltBranchStart = 0;           // points [boltBranchStart, boltCount) are a side branch
    int boltBranchFrom = 0;            // index on the main bolt the branch starts from
    Vector3 rainCenter = {};
    Vector3 rainVel = {};
    Texture2D dot = {};                // soft round sprite for spray
};

void VfxInit(Vfx& v);
void VfxUpdate(Vfx& v, const Weather& weather, const Boat& boat, const Ocean& ocean, const Camera3D& cam, float dt);
void VfxDraw(const Vfx& v, const WeatherPreset& w, float flash, const Camera3D& cam);   // inside BeginMode3D
void VfxUnload(Vfx& v);
