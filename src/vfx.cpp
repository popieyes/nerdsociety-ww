#include "vfx.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>

namespace {

unsigned int NextRandom(unsigned int& s)
{
    s = s * 1664525u + 1013904223u;
    return s >> 8;
}

float Rand(unsigned int& s, float lo, float hi)
{
    return lo + (hi - lo) * (float)(NextRandom(s) & 0xFFFF) / 65535.0f;
}

float Wrap(float x, float size)
{
    x = std::fmod(x, size);
    return x < 0.0f ? x + size : x;
}

Color ToColor(Vector3 c, float a)
{
    return { (unsigned char)Clamp(c.x * 255.0f, 0.0f, 255.0f), (unsigned char)Clamp(c.y * 255.0f, 0.0f, 255.0f),
             (unsigned char)Clamp(c.z * 255.0f, 0.0f, 255.0f), (unsigned char)Clamp(a * 255.0f, 0.0f, 255.0f) };
}

// Camera-facing quad along segment a->b (call between rlBegin(RL_QUADS)/rlEnd).
void SegmentQuad(Vector3 a, Vector3 b, float width, Vector3 camPos, Color ca, Color cb)
{
    const Vector3 along = Vector3Subtract(b, a);
    Vector3 side = Vector3CrossProduct(along, Vector3Subtract(camPos, a));
    const float len = Vector3Length(side);
    if (len < 1e-6f) return;
    side = Vector3Scale(side, width * 0.5f / len);
    rlColor4ub(ca.r, ca.g, ca.b, ca.a);
    rlVertex3f(a.x - side.x, a.y - side.y, a.z - side.z);
    rlColor4ub(cb.r, cb.g, cb.b, cb.a);
    rlVertex3f(b.x - side.x, b.y - side.y, b.z - side.z);
    rlVertex3f(b.x + side.x, b.y + side.y, b.z + side.z);
    rlColor4ub(ca.r, ca.g, ca.b, ca.a);
    rlVertex3f(a.x + side.x, a.y + side.y, a.z + side.z);
}

// Textured billboard (call between rlSetTexture + rlBegin(RL_QUADS)/rlEnd).
void Billboard(Vector3 p, float size, Vector3 right, Vector3 up, Color c)
{
    const Vector3 r = Vector3Scale(right, size * 0.5f), u = Vector3Scale(up, size * 0.5f);
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(p.x - r.x - u.x, p.y - r.y - u.y, p.z - r.z - u.z);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(p.x + r.x - u.x, p.y + r.y - u.y, p.z + r.z - u.z);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(p.x + r.x + u.x, p.y + r.y + u.y, p.z + r.z + u.z);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(p.x - r.x + u.x, p.y - r.y + u.y, p.z - r.z + u.z);
}

void GenerateBolt(Vfx& v, const Camera3D& cam)
{
    unsigned int& r = v.rng;
    const Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    const float baseAngle = std::atan2(fwd.x, fwd.z) + Rand(r, -0.3f, 0.3f);   // inside the horizontal FOV
    const float dist = Rand(r, 120.0f, 200.0f);
    Vector3 p = { cam.position.x + std::sin(baseAngle) * dist, 75.0f, cam.position.z + std::cos(baseAngle) * dist };
    const int mainCount = 18;
    v.boltCount = 0;
    for (int i = 0; i < mainCount; ++i) {
        v.bolt[v.boltCount++] = p;
        p.y -= 75.0f / (float)(mainCount - 1);
        p.x += Rand(r, -4.5f, 4.5f);
        p.z += Rand(r, -4.5f, 4.5f);
    }
    // One side branch forking off the upper half.
    v.boltBranchFrom = 4 + (int)(NextRandom(r) % 5);
    v.boltBranchStart = v.boltCount;
    Vector3 q = v.bolt[v.boltBranchFrom];
    const float bx = Rand(r, -1.0f, 1.0f) > 0.0f ? 1.0f : -1.0f;
    for (int i = 0; i < 7 && v.boltCount < kMaxBoltPoints; ++i) {
        q.y -= Rand(r, 2.5f, 4.5f);
        q.x += bx * Rand(r, 1.5f, 4.5f);
        q.z += Rand(r, -2.0f, 2.0f);
        v.bolt[v.boltCount++] = q;
    }
}

} // namespace

void VfxInit(Vfx& v)
{
    v = Vfx{};
    Image img = GenImageGradientRadial(32, 32, 0.0f, WHITE, BLANK);
    v.dot = LoadTextureFromImage(img);
    UnloadImage(img);
    for (int i = 0; i < kMaxRain; ++i) {
        v.rain[i] = { Rand(v.rng, 0.0f, v.settings.rainBox.x), Rand(v.rng, 0.0f, v.settings.rainBox.y),
                      Rand(v.rng, 0.0f, v.settings.rainBox.z) };
    }
}

void VfxUpdate(Vfx& v, const Weather& weather, const Boat& boat, const Ocean& ocean, const Camera3D& cam, float dt)
{
    const WeatherPreset& w = weather.current;
    const VfxSettings& s = v.settings;

    // Rain: wrap inside a box placed a bit in front of the camera. Falls, slants with the wind and
    // streams back past the boat.
    const Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    v.rainCenter = Vector3Add(cam.position, Vector3Scale(fwd, s.rainBox.x * 0.45f));
    v.rainVel = { w.windDir.x * w.windSpeed * 0.6f, -s.rainFall, w.windDir.y * w.windSpeed * 0.6f - boat.speed };
    if (w.rain > 0.01f) {
        for (int i = 0; i < kMaxRain; ++i) {
            Vector3& p = v.rain[i];
            p = Vector3Add(p, Vector3Scale(v.rainVel, dt * (0.85f + 0.3f * (float)(i % 7) / 6.0f)));
            p = { Wrap(p.x, s.rainBox.x), Wrap(p.y, s.rainBox.y), Wrap(p.z, s.rainBox.z) };
        }
    }

    // Lightning bolt shape for each new strike.
    if (weather.strikeCount != v.boltStrike) {
        v.boltStrike = weather.strikeCount;
        GenerateBolt(v, cam);
    }

    // Bow spray: bursts when the bow slams, a light trickle otherwise.
    const float rate = s.sprayRate * boat.bowSplash + s.sprayIdleRate * Clamp(boat.speed / 5.0f, 0.0f, 1.5f) * (0.3f + 0.7f * Clamp(w.waveScale - 0.4f, 0.0f, 1.0f));
    v.sprayAccum += rate * dt;
    const Vector3 bow = BoatPointToWorld(boat, { 0.0f, 0.1f, 2.75f });
    for (int i = 0; i < kMaxSpray && v.sprayAccum >= 1.0f; ++i) {
        SprayParticle& p = v.spray[i];
        if (p.life > 0.0f) continue;
        v.sprayAccum -= 1.0f;
        const float sideSign = (NextRandom(v.rng) & 1) ? 1.0f : -1.0f;
        const float burst = 0.5f + boat.bowSplash;
        p.pos = Vector3Add(bow, { sideSign * Rand(v.rng, 0.2f, 0.6f), Rand(v.rng, -0.2f, 0.2f), Rand(v.rng, -0.6f, 0.2f) });
        p.vel = { sideSign * Rand(v.rng, 1.0f, 3.2f) * burst, Rand(v.rng, 1.5f, 4.5f) * burst, Rand(v.rng, -2.5f, 0.5f) };
        p.maxLife = p.life = Rand(v.rng, 0.5f, 1.1f);
        p.size = Rand(v.rng, 0.07f, 0.2f);
    }
    if (v.sprayAccum > 4.0f) v.sprayAccum = 4.0f;   // pool exhausted: don't build a backlog
    for (int i = 0; i < kMaxSpray; ++i) {
        SprayParticle& p = v.spray[i];
        if (p.life <= 0.0f) continue;
        p.life -= dt;
        p.vel.y -= kGravity * dt;
        p.vel.z += (-boat.speed - p.vel.z) * 1.5f * dt;   // air drag in the boat's frame
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        if (p.vel.y < 0.0f && p.pos.y < OceanHeightAt(ocean, p.pos.x, p.pos.z)) p.life = 0.0f;
    }
}

void VfxDraw(const Vfx& v, const WeatherPreset& w, float flash, const Camera3D& cam)
{
    const VfxSettings& s = v.settings;
    const Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    const Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, cam.up));
    const Vector3 up = Vector3CrossProduct(right, fwd);
    // Effects aren't lit by a shader: approximate the scene light once.
    const Vector3 light = Vector3Add(Vector3Scale(w.ambientSky, 0.8f), Vector3Scale(w.sunColor, 0.45f));

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    // Lightning bolt: wide faint glow + bright core.
    if (flash > 0.05f && v.boltCount > 1) {
        const float a = Clamp(flash, 0.0f, 1.0f);
        rlBegin(RL_QUADS);
        for (int pass = 0; pass < 2; ++pass) {
            const float width = pass == 0 ? 2.6f : 0.45f;
            const Color c = pass == 0 ? ToColor({ 0.6f, 0.65f, 1.0f }, 0.22f * a) : ToColor({ 0.95f, 0.95f, 1.0f }, a);
            for (int i = 0; i + 1 < v.boltBranchStart; ++i) SegmentQuad(v.bolt[i], v.bolt[i + 1], width, cam.position, c, c);
            Vector3 prev = v.bolt[v.boltBranchFrom];
            for (int i = v.boltBranchStart; i < v.boltCount; ++i) {
                SegmentQuad(prev, v.bolt[i], width * 0.6f, cam.position, c, c);
                prev = v.bolt[i];
            }
        }
        rlEnd();
    }

    // Bow spray.
    rlSetTexture(v.dot.id);
    rlBegin(RL_QUADS);
    const Vector3 sprayCol = Vector3Multiply(w.foam, Vector3Add(light, { 0.15f, 0.15f, 0.15f }));
    for (int i = 0; i < kMaxSpray; ++i) {
        const SprayParticle& p = v.spray[i];
        if (p.life <= 0.0f) continue;
        const float t = p.life / p.maxLife;
        Billboard(p.pos, p.size * (1.4f - 0.6f * t), right, up, ToColor(sprayCol, 0.85f * fminf(t * 2.0f, 1.0f)));
    }
    rlEnd();
    rlSetTexture(rlGetTextureIdDefault());   // (0 would keep the sprite bound for the rain)

    // Rain streaks.
    if (w.rain > 0.01f) {
        const int count = (int)(w.rain * (float)kMaxRain);
        const Vector3 origin = Vector3Subtract(v.rainCenter, Vector3Scale(s.rainBox, 0.5f));
        const Vector3 streak = Vector3Scale(v.rainVel, s.rainStreak);
        const Vector3 rainCol = Vector3Add(Vector3Scale(light, 0.9f), Vector3Scale({ 0.8f, 0.85f, 1.0f }, flash * 0.6f + 0.15f));
        rlBegin(RL_QUADS);
        for (int i = 0; i < count; ++i) {
            const Vector3 p = Vector3Add(origin, v.rain[i]);
            const float d = Vector3Distance(p, cam.position);
            const float alpha = 0.6f * w.rain * Clamp(1.0f - d / (s.rainBox.x * 0.95f), 0.0f, 1.0f) * Clamp((d - 3.0f) / 6.0f, 0.0f, 1.0f);   // none right in the lens
            if (alpha < 0.01f) continue;
            SegmentQuad(p, Vector3Subtract(p, streak), s.rainWidth * (1.0f + d * 0.02f), cam.position,
                        ToColor(rainCol, alpha), ToColor(rainCol, 0.0f));   // bright head, fading tail
        }
        rlEnd();
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void VfxUnload(Vfx& v)
{
    UnloadTexture(v.dot);
    v.dot = {};
}
