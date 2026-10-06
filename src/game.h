#pragma once
#include "atmosphere.h"
#include "boat.h"
#include "camera_rig.h"
#include "conductor.h"
#include "ocean.h"
#include "props.h"
#include "sky.h"
#include "vfx.h"
#include "weather.h"

// Everything the running game owns. Future gameplay state (crew, commands, story progress)
// gets its own module and a member here; see docs/ARCHITECTURE.md.
struct Game {
    Conductor conductor;
    Weather weather;
    Ocean ocean;
    Boat boat;
    Props props;
    Sky sky;
    Vfx vfx;
    SceneShader lit;          // shared lit shader for the boat and props
    CameraRig rig;
    float time = 0.0f;

    // Debug
    bool showHud = true;
    bool showcase = false;    // auto-cycle weather presets
    float showcaseTimer = 0.0f;
    bool hasHit = false;
    Judgement lastJudgement = Judgement::Miss;
    double lastHitOffset = 0.0;
};

void GameInit(Game& g, int weatherIndex);
void GameUpdate(Game& g, float dt);
void GameDraw(Game& g);   // call between BeginDrawing/EndDrawing
void GameUnload(Game& g);
