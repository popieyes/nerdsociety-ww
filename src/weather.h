#pragma once
#include "raylib.h"

// Look-and-feel tunables for one kind of weather: plain data, blended component-wise.
// Colors are 0..1 RGB (Vector3, display space) so they can be lerped and uploaded to shaders.
// The sky horizon color doubles as the fog color: water and lit objects fade into exactly the
// sky's horizon (see resources/shaders/common.glsl), so the ocean never shows an edge.
struct WeatherPreset {
    const char* name;

    // Sky
    Vector3 skyZenith;
    Vector3 skyHorizon;      // = fog color
    Vector3 sunGlowColor;    // horizon/halo tint toward the sun
    float sunGlow;           // 0..1 strength of that tint
    Vector3 sunDir;          // normalized, pointing toward the sun (or moon)
    Vector3 sunColor;        // direct light color * intensity
    float sunDisc;           // 0 = hidden, 1 = fully visible disc
    float moon;              // 0 = sun disc, 1 = moon disc (night)
    float cloudCover;        // 0..1
    Vector3 cloudLit;
    Vector3 cloudShade;
    float mountains;         // 0..1 visibility of the distant fjord ridge on the horizon
    float stars;             // 0..1
    float aurora;            // 0..1

    // Fog + ambient
    float fogNear;           // distance where fog starts
    float fogFar;            // distance where fog is total
    Vector3 ambientSky;      // hemispheric ambient, from above
    Vector3 ambientGround;   // from below (sea bounce)

    // Water
    Vector3 waterDeep;
    Vector3 waterShallow;    // also the crest/subsurface tint
    Vector3 foam;
    float foamAmount;        // 0..1 extra crest foam
    float waveScale;         // multiplies wave amplitudes (WaveParams::amplitudeScale)
    float waveLength;        // multiplies wavelengths (bigger = longer swells)
    float choppiness;        // 0..1 Gerstner horizontal pinch
    Vector2 windDir;         // XZ direction the wind (and waves) travel toward, normalized
    float windSpeed;         // world units/s: clouds, rain slant, prop drift

    // Effects
    float rain;              // 0..1 streak density
    float lightning;         // strikes per minute (random timing)

    // Color grading
    float saturation;        // 1 = unchanged
    float contrast;          // 1 = unchanged
    float exposure;          // multiplier
    Vector3 tint;            // multiplier
    float vignette;          // 0..1
};

int WeatherPresetCount();
const WeatherPreset& GetWeatherPreset(int index);   // index is clamped to a valid preset

// Component-wise blend; name comes from whichever side t is closer to.
WeatherPreset LerpWeather(const WeatherPreset& a, const WeatherPreset& b, float t);

// Lightning flash brightness (0..~1.3) `age` seconds after a strike: a double flicker + decay.
float LightningFlash(float age);

// Current weather, blending from a snapshot toward a target preset, plus lightning timing.
struct Weather {
    WeatherPreset current;
    WeatherPreset start;
    int target = 0;
    float elapsed = 0.0f;
    float duration = 0.0f;

    // Lightning (deterministic RNG so smoke runs are reproducible)
    unsigned int rng = 12345u;
    float nextStrike = 4.0f;       // seconds until the next random strike (counts down while lightning > 0)
    float strikeAge = 100.0f;      // seconds since the last strike
    int strikeCount = 0;           // increments per strike; seeds the bolt shape in vfx
    float flash = 0.0f;            // current flash brightness, 0 when idle
};

void WeatherSnap(Weather& w, int index);                       // jump immediately
void WeatherBlendTo(Weather& w, int index, float seconds);     // smooth transition from current
void WeatherTriggerLightning(Weather& w);                      // hook for story/gameplay (storm boss)
void WeatherUpdate(Weather& w, float dt);
