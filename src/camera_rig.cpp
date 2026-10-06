#include "camera_rig.h"

#include "raymath.h"

#include <cmath>

void CameraRigInit(CameraRig& rig, Vector3 anchor)
{
    rig.camera.up = { 0.0f, 1.0f, 0.0f };
    rig.camera.projection = CAMERA_PERSPECTIVE;
    rig.smoothY = anchor.y;
    rig.blend = rig.mode == RigMode::Chase ? 1.0f : 0.0f;
    CameraRigUpdate(rig, anchor, 0.0f, -100.0f, 0.0f);
}

void CameraRigSetMode(CameraRig& rig, RigMode mode, bool instant)
{
    rig.mode = mode;
    if (instant) rig.blend = mode == RigMode::Chase ? 1.0f : 0.0f;
}

void CameraRigToggle(CameraRig& rig)
{
    CameraRigSetMode(rig, rig.mode == RigMode::Side ? RigMode::Chase : RigMode::Side, false);
}

void CameraRigUpdate(CameraRig& rig, Vector3 anchor, float sway, float waterAtCamera, float dt)
{
    const float goal = rig.mode == RigMode::Chase ? 1.0f : 0.0f;
    rig.blend += Clamp(goal - rig.blend, -rig.blendSpeed * dt, rig.blendSpeed * dt);
    const float t = rig.blend * rig.blend * (3.0f - 2.0f * rig.blend);

    const Vector3 pos = Vector3Lerp(rig.side.positionOffset, rig.chase.positionOffset, t);
    const Vector3 tgt = Vector3Lerp(rig.side.targetOffset, rig.chase.targetOffset, t);

    // Follow a damped fraction of the heave so the horizon stays calm but the boat doesn't
    // leave the frame in big seas.
    rig.smoothY += (anchor.y - rig.smoothY) * Clamp(dt * 2.5f, 0.0f, 1.0f);
    const float lift = rig.smoothY * Lerp(rig.heaveFollow, 1.0f, rig.swayAmount);   // follow fully in big seas

    // Storm sway: slow incommensurate sines (deterministic), position bob + a slight roll.
    rig.swayAmount += (sway - rig.swayAmount) * Clamp(dt * 1.5f, 0.0f, 1.0f);   // storm framing arrives with the blend
    rig.swayTime += dt;
    const float st = rig.swayTime;
    const float s = rig.swayAmount;
    const Vector3 bob = { 0.25f * s * std::sin(st * 0.53f), 0.45f * s * std::sin(st * 0.71f + 1.0f), 0.3f * s * std::sin(st * 0.37f + 2.0f) };
    const float rollAngle = 0.035f * s * std::sin(st * 0.61f + 0.5f);

    // Never dip into a crest: rise immediately when the water climbs, settle back slowly.
    const float floorGoal = waterAtCamera + rig.waterClearance;
    rig.floorY = floorGoal > rig.floorY ? floorGoal : rig.floorY + (floorGoal - rig.floorY) * Clamp(dt * 0.8f, 0.0f, 1.0f);
    const float storm = rig.stormLift * s;
    float y = pos.y + lift + bob.y + storm;
    const float raise = y < rig.floorY ? rig.floorY - y : 0.0f;
    y += raise;

    rig.camera.position = { anchor.x + pos.x + bob.x, y, anchor.z + pos.z + bob.z };
    rig.camera.target = { anchor.x + tgt.x, tgt.y + lift + bob.y * 0.5f + storm * 0.25f + raise * 0.3f, anchor.z + tgt.z };
    rig.camera.fovy = Lerp(rig.side.fovy, rig.chase.fovy, t);

    const Vector3 fwd = Vector3Normalize(Vector3Subtract(rig.camera.target, rig.camera.position));
    rig.camera.up = Vector3RotateByAxisAngle({ 0.0f, 1.0f, 0.0f }, fwd, rollAngle);
}
