#pragma once
#include "raylib.h"
#include "waves.h"

// Rigid body floating on the Gerstner sea: samples the CPU wave height at a few hull points and
// integrates heave, pitch, roll (+ optional horizontal drift) with fixed sub-steps, so it behaves
// the same at any frame rate. Pure logic (tested in tests/test_floater.cpp). Used by the boat and
// the floating props.
//
// Body frame: +Z forward (bow), +Y up, +X to port (left). Orientation = yaw (about Y),
// then pitch (about X, bow down is positive), then roll (about Z).

constexpr int kMaxFloatPoints = 8;

struct FloaterShape {
    Vector3 points[kMaxFloatPoints];   // body-space sample points on the hull bottom
    int pointCount = 0;
    Vector3 halfExtents = { 0.5f, 0.5f, 0.5f };   // box approximation for the inertia
    float mass = 1.0f;
    float heaveFrequency = 0.8f;   // Hz: natural bobbing frequency when fully afloat
    float heaveDamping = 0.6f;     // damping ratio of the bob (1 = critical)
    float angularDamping = 1.5f;   // extra rotational damping (1/s)
    float maxSubmersion = 1.0f;    // per-point depth where buoyancy stops growing (fully under)
    // Horizontal (props only): drift toward the surface current and slide down wave slopes.
    bool lockHorizontal = false;
    float waterDrag = 1.2f;        // 1/s, relaxes horizontal velocity toward the current
    float slopePush = 0.6f;        // fraction of gravity pushing down wave slopes
    float yawDamping = 0.8f;       // 1/s
};

struct Floater {
    Vector3 position = {};
    Vector3 velocity = {};
    float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
    float yawRate = 0.0f, pitchRate = 0.0f, rollRate = 0.0f;
    float accumulator = 0.0f;   // leftover time for the fixed sub-steps
};

// What the water is doing around the floaters this frame.
struct WaterEnv {
    const WaveField* waves = nullptr;
    Vector2 current = {};   // surface drift (x, z) in world units/s (wind drift - boat scroll)
};

constexpr float kFloaterStep = 1.0f / 120.0f;
constexpr float kFloaterMaxFrame = 0.1f;   // dt clamp (hitches, breakpoints)
constexpr float kGravity = 9.81f;

// Depth below the calm surface that the hull points settle at (mass * g = total buoyancy).
float FloaterEquilibriumDepth(const FloaterShape& s);
void FloaterStep(Floater& f, const FloaterShape& s, const WaterEnv& env, float h);
void FloaterUpdate(Floater& f, const FloaterShape& s, const WaterEnv& env, float dt);   // fixed sub-steps
// Instant velocity change. linear in world units/s, angular = (pitch, yaw, roll) rad/s.
void FloaterApplyImpulse(Floater& f, Vector3 linear, Vector3 angular);
Vector3 FloaterLocalToWorld(const Floater& f, Vector3 local);
Matrix FloaterTransform(const Floater& f);   // rotation + translation, for drawing
