#pragma once
#include "raylib.h"

// Sum-of-Gerstner wave math. Pure logic: no window/GPU needed (unit-tested in tests/test_ocean.cpp).
// ---------------------------------------------------------------------------------------------
// CPU/GPU SYNC RULE
// The baked WaveField is uploaded to resources/shaders/water.vs/.fs every frame (ocean.cpp), and
// GerstnerPoint() is a line-by-line CPU mirror of gerstner() in water.vs (used by buoyancy
// through WaveHeight()). If you change one, change the other, and keep the tests passing.
// The shader additionally fades waves that are too short for the local grid spacing: inside
// kOceanFullDetailRadius of the origin every wave is at full strength, so CPU heights match the
// rendered surface there. Further out only the short chop fades (2.8 m waves are gone by ~30 m,
// 4.7 m by ~55 m with the default grid), so far props can sit a few cm off the drawn surface.
// ---------------------------------------------------------------------------------------------

constexpr int kMaxWaves = 8;                    // must match MAX_WAVES in water.vs / water.fs
constexpr float kOceanFullDetailRadius = 14.0f;   // default OceanGridSettings, waveLength 1

// One Gerstner wave, authored relative to the wind.
struct GerstnerWave {
    float angle;        // radians, direction relative to the wind
    float wavelength;   // world units (before WaveParams::lengthScale)
    float amplitude;    // world units (before amplitude/length scale)
    float steepness;    // 0..1 share of the horizontal pinch (scaled by WaveParams::choppiness)
    float phase;        // initial phase offset, radians
};

struct WaveParams {
    GerstnerWave waves[kMaxWaves];
    int count;
    float amplitudeScale;   // WeatherPreset::waveScale
    float lengthScale;      // WeatherPreset::waveLength (amplitudes scale with it: same steepness)
    float choppiness;       // WeatherPreset::choppiness
    Vector2 windDir;        // WeatherPreset::windDir (normalized)
    float gravity;          // dispersion: omega = sqrt(gravity * k) * speedScale
    float speedScale;
    float maxPinch;         // cap on sum(k * horizontal): < 1 keeps crests from looping
};

WaveParams DefaultWaveParams();

// Baked, ready-to-evaluate waves. Phases are integrated over time (time + boat scroll), so
// weather blends can change wavelengths/directions without the far field "rushing".
struct WaveComponent {
    Vector2 dir;        // unit travel direction
    float k;            // wavenumber 2*pi/L
    float omega;        // angular frequency
    float amplitude;    // vertical
    float horizontal;   // horizontal displacement amplitude (Gerstner Q*A)
    float phase;        // current phase, radians
};

struct WaveField {
    WaveComponent waves[kMaxWaves];
    int count;
    float maxHeight;    // sum of amplitudes (for shading normalization)
};

WaveField WaveFieldInit(const WaveParams& p);           // bake + initial phases
void WaveFieldBake(WaveField& f, const WaveParams& p);   // re-bake shape from params, keep phases
// Advance phases by dt seconds while the boat travels `scrollDelta` along +Z (the wave field
// scrolls past the boat, which stays near the origin).
void WaveFieldAdvance(WaveField& f, float dt, float scrollDelta);

// Forward Gerstner: world position of the surface point whose rest position is (x0, 0, z0).
Vector3 GerstnerPoint(const WaveField& f, float x0, float z0);
// Surface normal at the point displaced from rest position (x0, z0).
Vector3 GerstnerNormal(const WaveField& f, float x0, float z0);
// Water height at world (x, z): inverts the horizontal displacement with a few Newton steps.
float WaveHeight(const WaveField& f, float x, float z);
// Rest position (x0, z0) whose displaced point lands on world (x, z). Used by WaveHeight.
Vector2 GerstnerInvert(const WaveField& f, float x, float z);
