#include "floater.h"

#include "raymath.h"

#include <cmath>

namespace {

constexpr float kTwoPi = 6.28318530718f;
constexpr float kMaxTilt = 1.2f;   // rad, safety clamp for pitch/roll

Vector3 AxisX(float yaw) { return { std::cos(yaw), 0.0f, -std::sin(yaw) }; }
Vector3 AxisZ(float yaw) { return { std::sin(yaw), 0.0f, std::cos(yaw) }; }

bool Finite(const Floater& f)
{
    return std::isfinite(f.position.x) && std::isfinite(f.position.y) && std::isfinite(f.position.z) &&
           std::isfinite(f.velocity.y) && std::isfinite(f.pitch) && std::isfinite(f.roll) && std::isfinite(f.yaw);
}

} // namespace

float FloaterEquilibriumDepth(const FloaterShape& s)
{
    const float w = kTwoPi * s.heaveFrequency;
    return kGravity / (w * w);
}

Vector3 FloaterLocalToWorld(const Floater& f, Vector3 local)
{
    // world = Ry(yaw) * Rx(pitch) * Rz(roll) * local + position
    const float cr = std::cos(f.roll), sr = std::sin(f.roll);
    Vector3 p = { local.x * cr - local.y * sr, local.x * sr + local.y * cr, local.z };
    const float cp = std::cos(f.pitch), sp = std::sin(f.pitch);
    p = { p.x, p.y * cp - p.z * sp, p.y * sp + p.z * cp };
    const float cy = std::cos(f.yaw), sy = std::sin(f.yaw);
    p = { p.x * cy + p.z * sy, p.y, -p.x * sy + p.z * cy };
    return Vector3Add(p, f.position);
}

Matrix FloaterTransform(const Floater& f)
{
    const Matrix r = MatrixMultiply(MatrixMultiply(MatrixRotateZ(f.roll), MatrixRotateX(f.pitch)), MatrixRotateY(f.yaw));
    return MatrixMultiply(r, MatrixTranslate(f.position.x, f.position.y, f.position.z));
}

void FloaterStep(Floater& f, const FloaterShape& s, const WaterEnv& env, float h)
{
    const int n = s.pointCount > 0 ? (s.pointCount < kMaxFloatPoints ? s.pointCount : kMaxFloatPoints) : 0;
    if (n == 0 || !env.waves) return;

    const float w0 = kTwoPi * s.heaveFrequency;
    const float k = s.mass * w0 * w0 / (float)n;                       // buoyancy per unit depth, per point
    const float c = 2.0f * s.heaveDamping * s.mass * w0 / (float)n;    // vertical damping per point
    const Vector3 e = s.halfExtents;
    const Vector3 inertia = { s.mass / 3.0f * (e.y * e.y + e.z * e.z),
                              s.mass / 3.0f * (e.x * e.x + e.z * e.z),
                              s.mass / 3.0f * (e.x * e.x + e.y * e.y) };
    const Vector3 ax = AxisX(f.yaw), az = AxisZ(f.yaw);
    const Vector3 omega = Vector3Add(Vector3Add(Vector3Scale(ax, f.pitchRate), { 0.0f, f.yawRate, 0.0f }),
                                     Vector3Scale(az, f.rollRate));

    float forceY = -s.mass * kGravity;
    Vector3 torque = {};
    int wet = 0;
    for (int i = 0; i < n; ++i) {
        const Vector3 world = FloaterLocalToWorld(f, s.points[i]);
        const float depth = WaveHeight(*env.waves, world.x, world.z) - world.y;
        if (depth <= 0.0f) continue;
        ++wet;
        const Vector3 r = Vector3Subtract(world, f.position);
        const float pointVelY = f.velocity.y + Vector3CrossProduct(omega, r).y;
        const float fy = k * fminf(depth, s.maxSubmersion) - c * pointVelY;
        forceY += fy;
        torque = Vector3Add(torque, Vector3CrossProduct(r, { 0.0f, fy, 0.0f }));
    }

    // Semi-implicit Euler: velocities first, then positions with the new velocities.
    f.velocity.y += forceY / s.mass * h;
    f.pitchRate += (Vector3DotProduct(torque, ax) / inertia.x - s.angularDamping * f.pitchRate) * h;
    f.rollRate += (Vector3DotProduct(torque, az) / inertia.z - s.angularDamping * f.rollRate) * h;
    f.yawRate -= s.yawDamping * f.yawRate * h;

    if (!s.lockHorizontal) {
        Vector2 accel = {};
        if (wet > 0) {
            // Slide down the local wave slope and get carried by the surface current.
            const float d = 0.5f;
            const float hx = WaveHeight(*env.waves, f.position.x + d, f.position.z) - WaveHeight(*env.waves, f.position.x - d, f.position.z);
            const float hz = WaveHeight(*env.waves, f.position.x, f.position.z + d) - WaveHeight(*env.waves, f.position.x, f.position.z - d);
            accel.x = -s.slopePush * kGravity * hx / (2.0f * d) + s.waterDrag * (env.current.x - f.velocity.x);
            accel.y = -s.slopePush * kGravity * hz / (2.0f * d) + s.waterDrag * (env.current.y - f.velocity.z);
        }
        f.velocity.x += accel.x * h;
        f.velocity.z += accel.y * h;
        f.position.x += f.velocity.x * h;
        f.position.z += f.velocity.z * h;
    }
    f.position.y += f.velocity.y * h;
    f.pitch = Clamp(f.pitch + f.pitchRate * h, -kMaxTilt, kMaxTilt);
    f.roll = Clamp(f.roll + f.rollRate * h, -kMaxTilt, kMaxTilt);
    f.yaw += f.yawRate * h;

    if (!Finite(f)) {   // never let a bad frame poison the simulation
        const Vector3 p = f.position;
        f = Floater{};
        f.position = { std::isfinite(p.x) ? p.x : 0.0f, 0.0f, std::isfinite(p.z) ? p.z : 0.0f };
    }
}

void FloaterUpdate(Floater& f, const FloaterShape& s, const WaterEnv& env, float dt)
{
    f.accumulator += Clamp(dt, 0.0f, kFloaterMaxFrame);
    while (f.accumulator >= kFloaterStep) {
        FloaterStep(f, s, env, kFloaterStep);
        f.accumulator -= kFloaterStep;
    }
}

void FloaterApplyImpulse(Floater& f, Vector3 linear, Vector3 angular)
{
    f.velocity = Vector3Add(f.velocity, linear);
    f.pitchRate += angular.x;
    f.yawRate += angular.y;
    f.rollRate += angular.z;
}
