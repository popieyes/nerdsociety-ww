#include "weather.h"

#include "raymath.h"

#include <cmath>

namespace {

Vector3 Hex(unsigned int rgb)
{
    return { (float)((rgb >> 16) & 0xFF) / 255.0f, (float)((rgb >> 8) & 0xFF) / 255.0f, (float)(rgb & 0xFF) / 255.0f };
}

Vector2 Dir2(float x, float z) { return Vector2Normalize({ x, z }); }

// Shared defaults; each preset below only overrides what makes it different.
WeatherPreset Base()
{
    WeatherPreset p = {};
    p.name = "?";
    p.skyZenith = Hex(0x4a7fc4);
    p.skyHorizon = Hex(0xc9dbe6);
    p.sunGlowColor = Hex(0xfff1d6);
    p.sunGlow = 0.3f;
    p.sunDir = Vector3Normalize({ 0.5f, 0.6f, 0.45f });
    p.sunColor = Hex(0xfff4e0);
    p.sunDisc = 1.0f;
    p.cloudCover = 0.35f;
    p.cloudLit = Hex(0xffffff);
    p.cloudShade = Hex(0xb5c4d6);
    p.mountains = 1.0f;
    p.fogNear = 60.0f;
    p.fogFar = 420.0f;
    p.ambientSky = Hex(0x8aa6c4);
    p.ambientGround = Hex(0x3b5a66);
    p.waterDeep = Hex(0x0f4a66);
    p.waterShallow = Hex(0x2fa3a6);
    p.foam = Hex(0xf2f7fa);
    p.foamAmount = 0.3f;
    p.waveScale = 1.0f;
    p.waveLength = 1.0f;
    p.choppiness = 0.6f;
    p.windDir = Dir2(0.45f, -1.0f);   // waves run toward the camera side and the bow
    p.windSpeed = 3.0f;
    p.saturation = 1.0f;
    p.contrast = 1.0f;
    p.exposure = 1.0f;
    p.tint = { 1.0f, 1.0f, 1.0f };
    p.vignette = 0.25f;
    return p;
}

WeatherPreset CalmFjord()
{
    WeatherPreset p = Base();
    p.name = "Calm fjord";
    p.skyZenith = Hex(0x3f78c2);
    p.skyHorizon = Hex(0xcfe0ea);
    p.sunDir = Vector3Normalize({ 0.35f, 0.75f, 0.55f });
    p.sunColor = Vector3Scale(Hex(0xfff3dd), 1.05f);
    p.cloudCover = 0.38f;
    p.waveScale = 0.55f;
    p.choppiness = 0.45f;
    p.saturation = 1.1f;
    return p;
}

WeatherPreset GoldenSunset()
{
    WeatherPreset p = Base();
    p.name = "Golden sunset";
    p.skyZenith = Hex(0x3b4f8f);
    p.skyHorizon = Hex(0xf6a868);
    p.sunGlowColor = Hex(0xffc070);
    p.sunGlow = 0.85f;
    p.sunDir = Vector3Normalize({ 0.95f, 0.085f, 0.3f });   // low, in front of the side camera
    p.sunColor = Vector3Scale(Hex(0xffb070), 1.15f);
    p.cloudCover = 0.42f;
    p.cloudLit = Hex(0xffb88a);
    p.cloudShade = Hex(0x8a5a7a);
    p.mountains = 0.75f;
    p.fogNear = 50.0f;
    p.fogFar = 380.0f;
    p.ambientSky = Hex(0x8b6f9a);
    p.ambientGround = Hex(0x4a3248);
    p.waterDeep = Hex(0x1d2a52);
    p.waterShallow = Hex(0x6a5a7c);
    p.foam = Hex(0xffe6c8);
    p.waveScale = 0.65f;
    p.saturation = 1.15f;
    p.tint = { 1.04f, 0.98f, 0.94f };
    p.vignette = 0.35f;
    return p;
}

WeatherPreset Fog()
{
    WeatherPreset p = Base();
    p.name = "Fog";
    p.skyZenith = Hex(0x8e9aa2);
    p.skyHorizon = Hex(0xaab3b6);
    p.sunGlowColor = Hex(0xe4e6dc);
    p.sunGlow = 0.35f;
    p.sunDir = Vector3Normalize({ 0.85f, 0.1f, 0.3f });
    p.sunColor = Vector3Scale(Hex(0xd8dcd8), 0.55f);
    p.sunDisc = 0.22f;
    p.cloudCover = 0.0f;
    p.mountains = 0.0f;
    p.fogNear = 4.0f;
    p.fogFar = 75.0f;
    p.ambientSky = Hex(0x9aa4aa);
    p.ambientGround = Hex(0x56656a);
    p.waterDeep = Hex(0x29424a);
    p.waterShallow = Hex(0x587a7a);
    p.foam = Hex(0xd4dad8);
    p.foamAmount = 0.15f;
    p.waveScale = 0.6f;
    p.choppiness = 0.4f;
    p.windSpeed = 1.0f;
    p.saturation = 0.55f;
    p.contrast = 0.95f;
    p.vignette = 0.3f;
    return p;
}

WeatherPreset Storm()
{
    WeatherPreset p = Base();
    p.name = "Storm";
    p.skyZenith = Hex(0x1b2128);
    p.skyHorizon = Hex(0x4a5560);
    p.sunGlowColor = Hex(0x707a80);
    p.sunGlow = 0.2f;
    p.sunDir = Vector3Normalize({ 0.3f, 0.8f, 0.4f });
    p.sunColor = Vector3Scale(Hex(0xb0c0d0), 0.45f);
    p.sunDisc = 0.0f;
    p.cloudCover = 0.92f;
    p.cloudLit = Hex(0x5c6670);
    p.cloudShade = Hex(0x23292f);
    p.mountains = 0.25f;
    p.fogNear = 15.0f;
    p.fogFar = 190.0f;
    p.ambientSky = Hex(0x7c8a98);
    p.ambientGround = Hex(0x33434c);
    p.waterDeep = Hex(0x12303a);
    p.waterShallow = Hex(0x3f7c76);
    p.foam = Hex(0xd8e2e6);
    p.foamAmount = 0.9f;
    p.waveScale = 1.65f;
    p.waveLength = 1.5f;
    p.choppiness = 1.0f;
    p.windDir = Dir2(0.85f, -0.6f);   // more abeam: the ship rolls
    p.windSpeed = 14.0f;
    p.rain = 1.0f;
    p.lightning = 9.0f;
    p.saturation = 0.7f;
    p.contrast = 1.12f;
    p.exposure = 1.12f;
    p.tint = { 0.95f, 1.0f, 1.05f };
    p.vignette = 0.5f;
    return p;
}

WeatherPreset NightAurora()
{
    WeatherPreset p = Base();
    p.name = "Night aurora";
    p.skyZenith = Hex(0x03060f);
    p.skyHorizon = Hex(0x14243a);
    p.sunGlowColor = Hex(0x3a5070);
    p.sunGlow = 0.35f;
    p.sunDir = Vector3Normalize({ 0.9f, 0.12f, 0.3f });   // the moon, low in front of the side camera
    p.sunColor = Vector3Scale(Hex(0x9ab4e0), 0.55f);
    p.moon = 1.0f;
    p.cloudCover = 0.18f;
    p.cloudLit = Hex(0x3a4a66);
    p.cloudShade = Hex(0x0c1220);
    p.mountains = 0.7f;
    p.stars = 1.0f;
    p.aurora = 1.0f;
    p.fogNear = 40.0f;
    p.fogFar = 330.0f;
    p.ambientSky = Hex(0x2a4a5e);
    p.ambientGround = Hex(0x0a1622);
    p.waterDeep = Hex(0x03101c);
    p.waterShallow = Hex(0x14504a);
    p.foam = Hex(0x9fc8c8);
    p.foamAmount = 0.2f;
    p.waveScale = 0.7f;
    p.choppiness = 0.5f;
    p.windSpeed = 2.0f;
    p.saturation = 1.1f;
    p.vignette = 0.45f;
    return p;
}

// Story order: calm fjord -> golden sunset -> fog -> storm -> night/aurora finale.
const WeatherPreset kPresets[] = { CalmFjord(), GoldenSunset(), Fog(), Storm(), NightAurora() };
constexpr int kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

// Sky colors blend along a "twilight" curve instead of a straight line: a straight lerp from a warm
// bright sky to a dark one passes through muddy brown. When the target is darker, green leaves
// first and red last (dusk afterglow); when brighter, red arrives first and green last (dawn).
// Either way the in-between is a rosy/violet twilight. Endpoints are exact.
Vector3 TwilightLerp(Vector3 a, Vector3 b, float t)
{
    const float la = 0.299f * a.x + 0.587f * a.y + 0.114f * a.z;
    const float lb = 0.299f * b.x + 0.587f * b.y + 0.114f * b.z;
    // Only for big brightness changes involving a warm (sunset) sky: calm -> storm stays grey.
    const Vector3 bright = la > lb ? a : b;
    const float warm = Clamp((bright.x - bright.z) * 3.0f, 0.0f, 1.0f);
    const float k = Clamp(std::fabs(la - lb) * 3.0f, 0.0f, 1.0f) * warm;
    const float slow = Lerp(1.0f, 1.45f, k), fast = Lerp(1.0f, 0.65f, k);
    const bool darker = lb < la;
    const float tr = std::pow(t, darker ? slow : fast);
    const float tg = std::pow(t, darker ? fast : slow);
    return { Lerp(a.x, b.x, tr), Lerp(a.y, b.y, tg), Lerp(a.z, b.z, t) };
}

unsigned int NextRandom(unsigned int& state)
{
    state = state * 1664525u + 1013904223u;
    return state >> 8;
}

float Random01(unsigned int& state) { return (float)(NextRandom(state) & 0xFFFF) / 65535.0f; }

} // namespace

int WeatherPresetCount() { return kPresetCount; }

static int ClampPresetIndex(int index)
{
    return index < 0 ? 0 : (index >= kPresetCount ? kPresetCount - 1 : index);
}

const WeatherPreset& GetWeatherPreset(int index)
{
    return kPresets[ClampPresetIndex(index)];
}

WeatherPreset LerpWeather(const WeatherPreset& a, const WeatherPreset& b, float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    const auto L = [t](float x, float y) { return Lerp(x, y, t); };
    const auto V = [t](Vector3 x, Vector3 y) { return Vector3Lerp(x, y, t); };
    const auto S = [t](Vector3 x, Vector3 y) { return TwilightLerp(x, y, t); };   // sky colors
    WeatherPreset r;
    r.name = (t < 0.5f) ? a.name : b.name;
    r.skyZenith = S(a.skyZenith, b.skyZenith);
    r.skyHorizon = S(a.skyHorizon, b.skyHorizon);
    r.sunGlowColor = S(a.sunGlowColor, b.sunGlowColor);
    r.sunGlow = L(a.sunGlow, b.sunGlow);
    r.sunDir = Vector3Normalize(V(a.sunDir, b.sunDir));
    r.sunColor = V(a.sunColor, b.sunColor);
    r.sunDisc = L(a.sunDisc, b.sunDisc);
    r.moon = L(a.moon, b.moon);
    r.cloudCover = L(a.cloudCover, b.cloudCover);
    r.cloudLit = S(a.cloudLit, b.cloudLit);
    r.cloudShade = S(a.cloudShade, b.cloudShade);
    r.mountains = L(a.mountains, b.mountains);
    r.stars = L(a.stars, b.stars);
    r.aurora = L(a.aurora, b.aurora);
    r.fogNear = L(a.fogNear, b.fogNear);
    r.fogFar = L(a.fogFar, b.fogFar);
    r.ambientSky = V(a.ambientSky, b.ambientSky);
    r.ambientGround = V(a.ambientGround, b.ambientGround);
    r.waterDeep = V(a.waterDeep, b.waterDeep);
    r.waterShallow = V(a.waterShallow, b.waterShallow);
    r.foam = V(a.foam, b.foam);
    r.foamAmount = L(a.foamAmount, b.foamAmount);
    r.waveScale = L(a.waveScale, b.waveScale);
    r.waveLength = L(a.waveLength, b.waveLength);
    r.choppiness = L(a.choppiness, b.choppiness);
    r.windDir = Vector2Normalize(Vector2Lerp(a.windDir, b.windDir, t));
    r.windSpeed = L(a.windSpeed, b.windSpeed);
    r.rain = L(a.rain, b.rain);
    r.lightning = L(a.lightning, b.lightning);
    r.saturation = L(a.saturation, b.saturation);
    r.contrast = L(a.contrast, b.contrast);
    r.exposure = L(a.exposure, b.exposure);
    r.tint = V(a.tint, b.tint);
    r.vignette = L(a.vignette, b.vignette);
    return r;
}

float LightningFlash(float age)
{
    if (age < 0.0f) return 0.0f;
    if (age < 0.06f) return 1.0f;                   // first stroke
    if (age < 0.13f) return 0.25f;                  // dark gap
    if (age < 0.22f) return 1.3f;                   // return stroke, brightest
    return 1.3f * std::exp(-(age - 0.22f) * 7.0f);  // afterglow
}

void WeatherSnap(Weather& w, int index)
{
    w.target = ClampPresetIndex(index);   // also keeps the showcase cycle (target + 1) valid
    w.current = LerpWeather(GetWeatherPreset(index), GetWeatherPreset(index), 1.0f); // normalizes directions
    w.start = w.current;
    w.elapsed = 0.0f;
    w.duration = 0.0f;
}

void WeatherBlendTo(Weather& w, int index, float seconds)
{
    if (seconds <= 0.0f) {
        WeatherSnap(w, index);
        return;
    }
    w.start = w.current;
    w.target = ClampPresetIndex(index);   // also keeps the showcase cycle (target + 1) valid
    w.elapsed = 0.0f;
    w.duration = seconds;
}

void WeatherTriggerLightning(Weather& w)
{
    w.strikeAge = 0.0f;
    w.strikeCount++;
}

void WeatherUpdate(Weather& w, float dt)
{
    if (w.duration > 0.0f) {
        w.elapsed += dt;
        const float t = w.elapsed / w.duration;
        w.current = LerpWeather(w.start, GetWeatherPreset(w.target), t * t * (3.0f - 2.0f * t)); // smoothstep ease
        if (t >= 1.0f) {
            w.current = LerpWeather(w.start, GetWeatherPreset(w.target), 1.0f);
            w.duration = 0.0f;
        }
    }

    // Random strikes: exponential-ish spacing around 60 / strikesPerMinute seconds.
    if (w.current.lightning > 0.05f) {
        w.nextStrike -= dt;
        if (w.nextStrike <= 0.0f) {
            WeatherTriggerLightning(w);
            const float mean = 60.0f / w.current.lightning;
            w.nextStrike = mean * (0.3f + 1.4f * Random01(w.rng));
        }
    }
    w.strikeAge += dt;
    w.flash = LightningFlash(w.strikeAge);
}
