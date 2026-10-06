#include "check.h"
#include "waves.h"
#include "weather.h"

#include "raymath.h"

namespace {

// Deterministic pseudo-random points in [-r, r].
float Rand(unsigned int& s, float r)
{
    s = s * 1664525u + 1013904223u;
    return ((float)((s >> 8) & 0xFFFF) / 65535.0f * 2.0f - 1.0f) * r;
}

// Inversion accuracy: take random rest points, displace them forward, then ask WaveHeight at the
// displaced (x, z). It must find the same surface height.
void CheckInversion(const WaveParams& p, float time, float tolerance)
{
    WaveField f = WaveFieldInit(p);
    WaveFieldAdvance(f, time, 3.0f * time);
    unsigned int seed = 99u;
    float worst = 0.0f;
    for (int i = 0; i < 500; ++i) {
        const float x0 = Rand(seed, 60.0f), z0 = Rand(seed, 60.0f);
        const Vector3 surf = GerstnerPoint(f, x0, z0);
        const float err = std::fabs(WaveHeight(f, surf.x, surf.z) - surf.y);
        worst = err > worst ? err : worst;
    }
    CHECK(worst < tolerance);
    if (worst >= tolerance) std::printf("  worst inversion error %g\n", worst);
}

WaveParams ParamsFor(const WeatherPreset& w)
{
    WaveParams p = DefaultWaveParams();
    p.amplitudeScale = w.waveScale;
    p.lengthScale = w.waveLength;
    p.choppiness = w.choppiness;
    p.windDir = w.windDir;
    return p;
}

} // namespace

void TestOcean()
{
    const WaveParams p = DefaultWaveParams();
    const WaveField f = WaveFieldInit(p);

    // Bounded by the sum of amplitudes, and actually moves.
    CHECK(f.count == p.count);
    float lo = 1e9f, hi = -1e9f;
    WaveField moving = f;
    for (int i = 0; i < 2000; ++i) {
        WaveFieldAdvance(moving, 0.013f, 0.0f);
        const float h = WaveHeight(moving, (float)(i % 50) * 1.7f, (float)(i / 50) * 2.3f);
        CHECK(h <= f.maxHeight + 1e-4f && h >= -f.maxHeight - 1e-4f);
        lo = h < lo ? h : lo;
        hi = h > hi ? h : hi;
    }
    CHECK(hi - lo > 0.3f);

    // No horizontal pinch: Gerstner reduces to a sum of sines (height = sum A sin(theta)).
    WaveParams flat = p;
    flat.choppiness = 0.0f;
    const WaveField sines = WaveFieldInit(flat);
    float expect = 0.0f;
    for (int i = 0; i < sines.count; ++i) {
        const WaveComponent& c = sines.waves[i];
        expect += c.amplitude * std::sin(c.k * (c.dir.x * 3.0f + c.dir.y * -7.0f) + c.phase);
    }
    CHECK_NEAR(WaveHeight(sines, 3.0f, -7.0f), expect, 1e-4);

    // Pinch cap: crests never fold over.
    for (int w = 0; w < WeatherPresetCount(); ++w) {
        const WaveField wf = WaveFieldInit(ParamsFor(GetWeatherPreset(w)));
        float pinch = 0.0f;
        for (int i = 0; i < wf.count; ++i) pinch += wf.waves[i].k * wf.waves[i].horizontal;
        CHECK(pinch <= p.maxPinch + 1e-4f);
    }

    // CPU height inversion matches the forward function: default, every weather preset, max chop.
    CheckInversion(p, 1.7f, 2e-3f);
    for (int w = 0; w < WeatherPresetCount(); ++w) CheckInversion(ParamsFor(GetWeatherPreset(w)), 5.3f, 2e-3f);
    WaveParams extreme = p;
    extreme.amplitudeScale = 3.0f;
    extreme.choppiness = 1.0f;
    CheckInversion(extreme, 11.0f, 5e-3f);

    // Scrolling shifts the field along +Z: advancing scroll by d == sampling d further along z.
    WaveField a = f, b = f;
    WaveFieldAdvance(a, 0.0f, 5.0f);
    CHECK_NEAR(WaveHeight(a, 2.0f, 1.0f), WaveHeight(b, 2.0f, 6.0f), 1e-3);

    // Normal is unit length, points up, and flat water gives straight up.
    const Vector3 n = GerstnerNormal(f, 4.0f, -2.0f);
    CHECK_NEAR(Vector3Length(n), 1.0, 1e-4);
    CHECK(n.y > 0.5f);
    WaveParams calm = p;
    calm.amplitudeScale = 0.0f;
    const Vector3 up = GerstnerNormal(WaveFieldInit(calm), 1.0f, 1.0f);
    CHECK_NEAR(up.y, 1.0, 1e-6);

    // Normal matches finite differences of the forward surface.
    {
        const float x0 = 7.0f, z0 = -3.0f, e = 1e-2f;
        const Vector3 dx = Vector3Subtract(GerstnerPoint(f, x0 + e, z0), GerstnerPoint(f, x0 - e, z0));
        const Vector3 dz = Vector3Subtract(GerstnerPoint(f, x0, z0 + e), GerstnerPoint(f, x0, z0 - e));
        const Vector3 fd = Vector3Normalize(Vector3CrossProduct(dz, dx));
        const Vector3 an = GerstnerNormal(f, x0, z0);
        CHECK(Vector3DotProduct(fd, an) > 0.9999f);
    }

    // count = 0 -> flat sea.
    WaveParams none = p;
    none.count = 0;
    CHECK_NEAR(WaveHeight(WaveFieldInit(none), 1.0f, 2.0f), 0.0, 1e-9);
}
