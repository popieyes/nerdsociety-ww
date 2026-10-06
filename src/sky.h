#pragma once
#include "raylib.h"
#include "atmosphere.h"

// Full-screen sky pass (resources/shaders/sky.fs): horizon->zenith gradient shared with the fog,
// sun/moon disc + halo, wind-driven procedural clouds, distant fjord ridges, stars and aurora.
// Drawn first each frame (no depth), so the 3D scene draws over it.
struct Sky {
    SceneShader shader;
    Vector2 cloudOffset = {};   // accumulated wind drift of the cloud layer
    int locCamFwd = -1, locCamRight = -1, locCamUp = -1, locTanFov = -1;
    int locMoon = -1, locCloudCover = -1, locCloudLit = -1, locCloudShade = -1;
    int locCloudOffset = -1, locStars = -1;
};

void SkyInit(Sky& s);
void SkyUpdate(Sky& s, const WeatherPreset& w, float dt);
void SkyDraw(Sky& s, const AtmosphereFrame& atm, const Camera3D& camera);
void SkyUnload(Sky& s);
