#include "waves.h"

#include "raymath.h"

#include <cmath>

namespace {

constexpr float kTwoPi = 6.28318530718f;
constexpr int kInvertIterations = 5;

} // namespace

WaveParams DefaultWaveParams()
{
    WaveParams p = {};
    //            angle   wavelength amplitude steepness phase
    p.waves[0] = {  0.00f, 31.0f,  0.42f,  0.90f, 0.0f };   // main swell
    p.waves[1] = {  0.45f, 19.0f,  0.24f,  0.90f, 1.7f };
    p.waves[2] = { -0.38f, 13.0f,  0.15f,  0.85f, 4.1f };
    p.waves[3] = {  0.90f,  9.0f,  0.090f, 0.80f, 2.6f };
    p.waves[4] = { -0.80f,  6.5f,  0.060f, 0.75f, 5.3f };
    p.waves[5] = {  0.22f,  4.7f,  0.036f, 0.70f, 0.9f };   // chop
    p.waves[6] = { -1.30f,  3.6f,  0.026f, 0.60f, 3.3f };
    p.waves[7] = {  1.50f,  2.8f,  0.018f, 0.60f, 1.2f };
    p.count = 8;
    p.amplitudeScale = 1.0f;
    p.lengthScale = 1.0f;
    p.choppiness = 0.6f;
    p.windDir = { 0.0f, 1.0f };
    p.gravity = 9.81f;
    p.speedScale = 0.9f;
    p.maxPinch = 0.8f;
    return p;
}

void WaveFieldBake(WaveField& f, const WaveParams& p)
{
    f.count = p.count < kMaxWaves ? p.count : kMaxWaves;
    const float windAngle = std::atan2(p.windDir.y, p.windDir.x);
    float pinch = 0.0f;
    f.maxHeight = 0.0f;
    for (int i = 0; i < f.count; ++i) {
        const GerstnerWave& w = p.waves[i];
        WaveComponent& c = f.waves[i];
        const float a = windAngle + w.angle;
        const float length = w.wavelength * p.lengthScale;
        c.dir = { std::cos(a), std::sin(a) };
        c.k = kTwoPi / length;
        c.omega = std::sqrt(p.gravity * c.k) * p.speedScale;
        c.amplitude = w.amplitude * p.amplitudeScale * p.lengthScale;
        c.horizontal = c.amplitude * w.steepness * p.choppiness;
        pinch += c.k * c.horizontal;
        f.maxHeight += c.amplitude;
    }
    // Keep sum(k * Q*A) below maxPinch so crests never fold over (and the inversion stays well posed).
    if (pinch > p.maxPinch) {
        const float s = p.maxPinch / pinch;
        for (int i = 0; i < f.count; ++i) f.waves[i].horizontal *= s;
    }
}

WaveField WaveFieldInit(const WaveParams& p)
{
    WaveField f = {};
    WaveFieldBake(f, p);
    for (int i = 0; i < f.count; ++i) f.waves[i].phase = p.waves[i].phase;
    return f;
}

void WaveFieldAdvance(WaveField& f, float dt, float scrollDelta)
{
    // height(x, z) must equal the unscrolled field at (x, z + scroll), moving with time:
    // theta = k * dot(dir, (x, z + scroll)) - omega * t + phase0
    for (int i = 0; i < f.count; ++i) {
        WaveComponent& c = f.waves[i];
        c.phase += c.k * c.dir.y * scrollDelta - c.omega * dt;
        c.phase = std::fmod(c.phase, kTwoPi);   // keep precision over long sessions
    }
}

// Mirror of gerstner() in water.vs (without the far-distance LOD fade).
Vector3 GerstnerPoint(const WaveField& f, float x0, float z0)
{
    Vector3 p = { x0, 0.0f, z0 };
    for (int i = 0; i < f.count; ++i) {
        const WaveComponent& c = f.waves[i];
        const float theta = c.k * (c.dir.x * x0 + c.dir.y * z0) + c.phase;
        const float s = std::sin(theta), co = std::cos(theta);
        p.x += c.dir.x * c.horizontal * co;
        p.z += c.dir.y * c.horizontal * co;
        p.y += c.amplitude * s;
    }
    return p;
}

// Mirror of the normal in water.fs: cross of the analytic tangents dP/dz0 x dP/dx0.
Vector3 GerstnerNormal(const WaveField& f, float x0, float z0)
{
    Vector3 tx = { 1.0f, 0.0f, 0.0f };   // dP/dx0
    Vector3 tz = { 0.0f, 0.0f, 1.0f };   // dP/dz0
    for (int i = 0; i < f.count; ++i) {
        const WaveComponent& c = f.waves[i];
        const float theta = c.k * (c.dir.x * x0 + c.dir.y * z0) + c.phase;
        const float s = std::sin(theta), co = std::cos(theta);
        const float qs = c.horizontal * c.k * s;
        const float ac = c.amplitude * c.k * co;
        tx.x -= c.dir.x * c.dir.x * qs;
        tx.y += c.dir.x * ac;
        tx.z -= c.dir.x * c.dir.y * qs;
        tz.x -= c.dir.x * c.dir.y * qs;
        tz.y += c.dir.y * ac;
        tz.z -= c.dir.y * c.dir.y * qs;
    }
    return Vector3Normalize(Vector3CrossProduct(tz, tx));
}

Vector2 GerstnerInvert(const WaveField& f, float x, float z)
{
    // Solve x0 + dx(x0, z0) = x (and same for z) with Newton steps on the 2x2 horizontal Jacobian.
    float x0 = x, z0 = z;
    for (int it = 0; it < kInvertIterations; ++it) {
        float fx = x0 - x, fz = z0 - z;
        float jxx = 1.0f, jxz = 0.0f, jzz = 1.0f;
        for (int i = 0; i < f.count; ++i) {
            const WaveComponent& c = f.waves[i];
            const float theta = c.k * (c.dir.x * x0 + c.dir.y * z0) + c.phase;
            const float co = std::cos(theta);
            const float qs = c.horizontal * c.k * std::sin(theta);
            fx += c.dir.x * c.horizontal * co;
            fz += c.dir.y * c.horizontal * co;
            jxx -= c.dir.x * c.dir.x * qs;
            jxz -= c.dir.x * c.dir.y * qs;
            jzz -= c.dir.y * c.dir.y * qs;
        }
        const float det = jxx * jzz - jxz * jxz;
        if (det > 0.05f) {
            x0 -= (jzz * fx - jxz * fz) / det;
            z0 -= (jxx * fz - jxz * fx) / det;
        } else {
            x0 -= fx;   // plain fixed-point step near a pinched crest
            z0 -= fz;
        }
    }
    return { x0, z0 };
}

float WaveHeight(const WaveField& f, float x, float z)
{
    const Vector2 rest = GerstnerInvert(f, x, z);
    return GerstnerPoint(f, rest.x, rest.y).y;
}
