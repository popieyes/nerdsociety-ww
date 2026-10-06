#pragma once
#include "raylib.h"
#include "atmosphere.h"
#include "floater.h"
#include "ocean.h"

// Floating barrels and crates around the boat. They bob/drift with the waves (Floater), stream
// past as the boat sails (the ocean scrolls), and respawn ahead once they fall far behind.
constexpr int kMaxProps = 12;

enum class PropKind { Barrel, Crate };

struct Prop {
    PropKind kind = PropKind::Barrel;
    Floater body;
    Color tint = WHITE;
};

struct PropSpawnArea {
    float aheadMin = 22.0f;     // respawn band ahead of the boat (z), just past the right screen edge
    float aheadMax = 50.0f;
    float behindLimit = -22.0f; // respawn once z drops below this (off the left edge)
    float xMin = 4.5f;          // beyond the boat only: the side camera (x = -15) must always see the ship
    float xMax = 32.0f;
    float boatClearance = 4.5f; // keep |x| above this when spawning (boat lane)
};

struct Props {
    Prop items[kMaxProps];
    int count = 10;
    PropSpawnArea area;
    Model barrel = {}, crate = {};
    Matrix barrelBase = {}, crateBase = {};   // mesh -> body frame
    Texture2D barrelTex = {}, crateTex = {};
    FloaterShape barrelShape, crateShape;
    unsigned int rng = 2024u;
};

void PropsInit(Props& p, const SceneShader& lit, const Ocean& ocean);
// windDrift: surface drift from the wind (x, z) world units/s; boatSpeed scrolls them past.
void PropsUpdate(Props& p, const Ocean& ocean, Vector2 windDrift, float boatSpeed, float boatHalfBeam, float dt);
void PropsDraw(Props& p);
void PropsUnload(Props& p);
