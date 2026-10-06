#include "check.h"
#include "floater.h"

#include <cmath>

namespace {

FloaterShape BoxShape()
{
    FloaterShape s;
    const Vector3 pts[] = { { 0.5f, 0.0f, 0.5f }, { -0.5f, 0.0f, 0.5f }, { 0.5f, 0.0f, -0.5f }, { -0.5f, 0.0f, -0.5f } };
    s.pointCount = 4;
    for (int i = 0; i < 4; ++i) s.points[i] = pts[i];
    s.mass = 50.0f;
    s.heaveFrequency = 1.0f;
    s.heaveDamping = 0.4f;
    s.maxSubmersion = 2.0f;
    return s;
}

bool Finite(const Floater& f)
{
    return std::isfinite(f.position.x) && std::isfinite(f.position.y) && std::isfinite(f.position.z) &&
           std::isfinite(f.pitch) && std::isfinite(f.roll) && std::isfinite(f.yaw);
}

} // namespace

void TestFloater()
{
    WaveParams calmParams = DefaultWaveParams();
    calmParams.amplitudeScale = 0.0f;   // flat water at y = 0
    const WaveField calm = WaveFieldInit(calmParams);
    WaterEnv env;
    env.waves = &calm;

    // Dropped from above calm water: falls, settles at the analytic equilibrium depth, stays level.
    const FloaterShape box = BoxShape();
    const float eq = FloaterEquilibriumDepth(box);
    CHECK_NEAR(eq, 9.81 / std::pow(2.0 * 3.14159265, 2.0), 1e-3);
    Floater f;
    f.position = { 0.0f, 3.0f, 0.0f };
    f.pitch = 0.3f;
    f.roll = -0.2f;
    float maxAbsY = 0.0f;
    for (int i = 0; i < 60 * 30; ++i) {   // 30 s at 60 FPS
        FloaterUpdate(f, box, env, 1.0f / 60.0f);
        maxAbsY = std::fabs(f.position.y) > maxAbsY ? std::fabs(f.position.y) : maxAbsY;
    }
    CHECK(Finite(f));
    CHECK(maxAbsY < 5.0f);                       // bounded: no explosion
    CHECK_NEAR(f.position.y, -eq, 2e-3);         // hull points sit eq below the surface
    CHECK_NEAR(f.velocity.y, 0.0, 1e-3);
    CHECK_NEAR(f.pitch, 0.0, 1e-3);
    CHECK_NEAR(f.roll, 0.0, 1e-3);

    // Frame-rate independence: 30 FPS and 144 FPS land on the same state (fixed sub-steps).
    Floater a, b;
    a.position = b.position = { 0.0f, 1.0f, 0.0f };
    for (int i = 0; i < 30 * 2; ++i) FloaterUpdate(a, box, env, 1.0f / 30.0f);
    for (int i = 0; i < 144 * 2; ++i) FloaterUpdate(b, box, env, 1.0f / 144.0f);
    CHECK_NEAR(a.position.y, b.position.y, 0.02);

    // A huge frame hitch is clamped and stays stable.
    Floater h;
    h.position = { 0.0f, 2.0f, 0.0f };
    FloaterUpdate(h, box, env, 5.0f);
    CHECK(Finite(h));
    CHECK(h.position.y < 2.5f);

    // On a stormy sea: stays finite, near the surface, and actually moves.
    WaveParams stormParams = DefaultWaveParams();
    stormParams.amplitudeScale = 2.5f;
    stormParams.lengthScale = 1.4f;
    stormParams.choppiness = 1.0f;
    WaveField storm = WaveFieldInit(stormParams);
    WaterEnv senv;
    senv.waves = &storm;
    senv.current = { 0.3f, -5.0f };
    FloaterShape drifting = box;
    Floater s;
    s.position = { 0.0f, 0.0f, 0.0f };
    float lo = 1e9f, hi = -1e9f;
    bool nearSurface = true;
    for (int i = 0; i < 60 * 20; ++i) {
        WaveFieldAdvance(storm, 1.0f / 60.0f, 5.0f / 60.0f);
        FloaterUpdate(s, drifting, senv, 1.0f / 60.0f);
        const float water = WaveHeight(storm, s.position.x, s.position.z);
        if (std::fabs(s.position.y - water) > 3.0f) nearSurface = false;
        lo = s.position.y < lo ? s.position.y : lo;
        hi = s.position.y > hi ? s.position.y : hi;
    }
    CHECK(Finite(s));
    CHECK(nearSurface);
    CHECK(hi - lo > 0.5f);
    CHECK(s.position.z < -50.0f);   // carried back by the current (boat scroll)

    // Impulse: kicks, then settles back.
    Floater k;
    k.position = { 0.0f, -eq, 0.0f };
    FloaterApplyImpulse(k, { 0.0f, 2.0f, 0.0f }, { 0.5f, 0.0f, 0.0f });
    CHECK(k.velocity.y == 2.0f && k.pitchRate == 0.5f);
    for (int i = 0; i < 60 * 20; ++i) FloaterUpdate(k, box, env, 1.0f / 60.0f);
    CHECK_NEAR(k.position.y, -eq, 5e-3);
    CHECK_NEAR(k.pitch, 0.0, 5e-3);
}
