#pragma once
#include "raylib.h"

// Follows the boat. Two framings (which one ships is an open design question):
//  - Side: side-on, Patapon-like (default)
//  - Chase: 3/4 view from behind the stern
// Switching blends smoothly. `sway` (0..1, from the weather) adds a gentle rocking in storms.
enum class RigMode { Side, Chase };

struct CameraView {
    Vector3 positionOffset;   // relative to the boat's x/z (y absolute)
    Vector3 targetOffset;
    float fovy;
};

struct CameraRig {
    Camera3D camera = {};
    RigMode mode = RigMode::Side;
    CameraView side = { { -15.0f, 3.0f, 3.2f }, { 0.0f, 1.75f, 3.2f }, 30.0f };
    CameraView chase = { { 7.0f, 6.0f, -15.0f }, { -1.0f, 1.5f, 9.0f }, 42.0f };
    float blend = 0.0f;          // 0 = side, 1 = chase (eases toward the mode)
    float blendSpeed = 1.2f;     // 1/s
    float heaveFollow = 0.35f;   // how much of the boat's heave the camera follows
    float smoothY = 0.0f;        // filtered boat height
    float swayTime = 0.0f;
    float swayAmount = 0.0f;     // filtered sway (0..1)
    float stormLift = 2.0f;      // extra camera height at full sway (looks down over the swell)
    float waterClearance = 1.3f; // min height of the lens above the water below it
    float floorY = -100.0f;      // smoothed minimum camera height
};

void CameraRigInit(CameraRig& rig, Vector3 anchor);
void CameraRigSetMode(CameraRig& rig, RigMode mode, bool instant);
void CameraRigToggle(CameraRig& rig);
// waterAtCamera: water height under the camera (keeps the lens out of storm crests).
void CameraRigUpdate(CameraRig& rig, Vector3 anchor, float sway, float waterAtCamera, float dt);
