#include "check.h"
#include "weather.h"

#include "raymath.h"

#include <cstddef>
#include <cstdlib>

namespace {

// Every numeric field of a preset, as floats (everything after `name` up to and including `vignette`).
// Catches a field added to WeatherPreset but forgotten in LerpWeather.
constexpr size_t kFirstField = offsetof(WeatherPreset, skyZenith);
constexpr size_t kFieldCount = (offsetof(WeatherPreset, vignette) + sizeof(float) - kFirstField) / sizeof(float);

const float* Fields(const WeatherPreset& p)
{
    return reinterpret_cast<const float*>(reinterpret_cast<const char*>(&p) + kFirstField);
}

float MaxFieldDiff(const WeatherPreset& a, const WeatherPreset& b)
{
    float worst = 0.0f;
    for (size_t i = 0; i < kFieldCount; ++i) {
        const float d = std::fabs(Fields(a)[i] - Fields(b)[i]);
        worst = d > worst ? d : worst;
    }
    return worst;
}

int StrikesIn(int preset, float seconds, float dt)
{
    Weather w;
    WeatherSnap(w, preset);
    for (float t = 0.0f; t < seconds; t += dt) WeatherUpdate(w, dt);
    return w.strikeCount;
}

} // namespace

void TestWeather()
{
    CHECK(WeatherPresetCount() >= 2);
    const WeatherPreset& a = GetWeatherPreset(0);
    const WeatherPreset& b = GetWeatherPreset(1);
    CHECK(&GetWeatherPreset(-5) == &a);                      // clamped
    CHECK(&GetWeatherPreset(999) == &GetWeatherPreset(WeatherPresetCount() - 1));

    const WeatherPreset mid = LerpWeather(a, b, 0.5f);
    CHECK_NEAR(mid.fogNear, (a.fogNear + b.fogNear) * 0.5f, 1e-5);
    CHECK_NEAR(mid.waveScale, (a.waveScale + b.waveScale) * 0.5f, 1e-5);
    CHECK_NEAR(LerpWeather(a, b, 2.0f).fogFar, b.fogFar, 1e-5); // t clamped

    // Blend endpoints reproduce the presets exactly, for every pair and every field
    // (directions are re-normalized, hence the small tolerance).
    for (int i = 0; i < WeatherPresetCount(); ++i) {
        for (int j = 0; j < WeatherPresetCount(); ++j) {
            const WeatherPreset& p = GetWeatherPreset(i);
            const WeatherPreset& q = GetWeatherPreset(j);
            CHECK(MaxFieldDiff(LerpWeather(p, q, 0.0f), p) < 1e-5f);
            CHECK(MaxFieldDiff(LerpWeather(p, q, 1.0f), q) < 1e-5f);
            const WeatherPreset m = LerpWeather(p, q, 0.5f);
            CHECK_NEAR(Vector3Length(m.sunDir), 1.0, 1e-4);     // directions stay unit length mid-blend
            CHECK_NEAR(Vector2Length(m.windDir), 1.0, 1e-4);
        }
    }

    // Blend reaches the target after its duration and stops.
    Weather w;
    WeatherSnap(w, 0);
    CHECK_NEAR(w.current.waveScale, a.waveScale, 1e-6);
    WeatherBlendTo(w, 1, 2.0f);
    WeatherUpdate(w, 1.0f);
    CHECK_NEAR(w.current.waveScale, mid.waveScale, 1e-5);
    WeatherUpdate(w, 1.5f);
    CHECK(MaxFieldDiff(w.current, b) < 1e-5f);
    CHECK(w.duration == 0.0f);

    // Re-targeting mid-blend continues from where the sky is now (no visible jump).
    WeatherSnap(w, 0);
    WeatherBlendTo(w, 3, 4.0f);
    WeatherUpdate(w, 1.3f);
    const WeatherPreset before = w.current;
    WeatherBlendTo(w, 4, 4.0f);
    WeatherUpdate(w, 1e-4f);
    CHECK(MaxFieldDiff(w.current, before) < 1e-3f);

    // Out-of-range indices are clamped (CLI --weather 9, showcase cycling).
    WeatherSnap(w, 99);
    CHECK(w.target == WeatherPresetCount() - 1);
    WeatherBlendTo(w, -3, 1.0f);
    CHECK(w.target == 0);

    // Lightning flash: bright double flicker, then fades out completely.
    CHECK(LightningFlash(-1.0f) == 0.0f);
    CHECK(LightningFlash(0.0f) >= 1.0f);
    CHECK(LightningFlash(0.1f) < LightningFlash(0.0f));      // dark gap between strokes
    CHECK(LightningFlash(0.2f) > 1.0f);                       // return stroke
    CHECK(LightningFlash(1.5f) < 0.01f);

    // Random strikes: only in presets that ask for them, at roughly the authored rate,
    // independent of the frame rate.
    int stormIndex = -1;
    for (int i = 0; i < WeatherPresetCount(); ++i) {
        if (GetWeatherPreset(i).lightning > 0.05f) stormIndex = i;
        else CHECK(StrikesIn(i, 300.0f, 1.0f / 60.0f) == 0);
    }
    CHECK(stormIndex >= 0);
    if (stormIndex >= 0) {
        const float perMinute = GetWeatherPreset(stormIndex).lightning;
        const int at60 = StrikesIn(stormIndex, 600.0f, 1.0f / 60.0f);
        const int at144 = StrikesIn(stormIndex, 600.0f, 1.0f / 144.0f);
        CHECK(at60 > perMinute * 10.0f * 0.6f && at60 < perMinute * 10.0f * 1.5f);
        CHECK(std::abs(at60 - at144) <= 2);
    }
}
