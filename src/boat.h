#pragma once
#include "raylib.h"
#include "atmosphere.h"
#include "floater.h"
#include "ocean.h"

// The longship. Its body is a Floater (heave/pitch/roll from 8 hull samples); x/z stay where
// gameplay puts them (the ocean scrolls instead, see Ocean::scroll). Heading is +Z.
struct Boat {
    Model model = {};
    Texture2D albedo = {};
    Floater body;              // position = waterline centre of the hull
    FloaterShape shape;
    float scale = 3.0f;
    float speed = 5.0f;        // world units per second, drives the ocean scroll
    float waterline = 0.95f;   // model origin height above the water at rest
    float halfBeam = 1.3f;     // half width at the waterline (wake/foam ring)
    float bowSplash = 0.0f;    // 0..1 how hard the bow is slamming this frame (spray VFX)
};

void BoatInit(Boat& b, const SceneShader& lit, const Ocean& ocean);
void BoatUpdate(Boat& b, const Ocean& ocean, float dt);
void BoatDraw(const Boat& b);
void BoatUnload(Boat& b);

// Gameplay hooks
// Instant kick: linear (world units/s), angular = (pitch, yaw, roll) rad/s. E.g. rowing surge,
// a wave hit, bracing. The floater springs settle it back.
void BoatApplyImpulse(Boat& b, Vector3 linear, Vector3 angular);
Vector3 BoatPosition(const Boat& b);                      // waterline centre (camera anchor)
OceanHull BoatHull(const Boat& b);                       // for the water shader (wake, dry deck)
Vector3 BoatPointToWorld(const Boat& b, Vector3 local);   // local in world units, bow = +Z
