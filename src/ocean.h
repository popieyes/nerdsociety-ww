#pragma once
#include "raylib.h"
#include "atmosphere.h"
#include "waves.h"
#include "weather.h"

// Water surface: wave field state (pure math in waves.h) + rendering. The grid is a large
// non-uniform mesh centred on the boat (dense near it, coarse toward the horizon, where fog hides
// it); the boat stays near the origin and the wave field scrolls past instead (endless ocean).
struct Ocean {
    Model model = {};
    SceneShader shader;
    WaveParams params = DefaultWaveParams();
    WaveField field = {};
    float time = 0.0f;
    float scroll = 0.0f;       // distance travelled along +Z
    float foamAmount = 0.0f;
    float reflectStrength = 0.6f;   // stylized cap on the fresnel sky reflection
    float ripple = 1.0f;            // shading-only ripples (from wind speed)
    Vector3 waterDeep = {}, waterShallow = {}, foamColor = {};
    // water-specific uniform locations (shared atmosphere ones live in `shader`)
    int locWaveA = -1, locWaveB = -1, locWaveCount = -1, locMaxHeight = -1;
    int locDeep = -1, locShallow = -1, locFoam = -1, locFoamAmount = -1;
    int locScroll = -1, locBoat = -1, locReflect = -1, locRipple = -1, locHull = -1, locHullSize = -1;
    int locCamPos = -1;
};

// Grid tunables
struct OceanGridSettings {
    int quads = 512;            // per side, split into 128x128 tiles (16-bit indices)
    float radius = 1200.0f;     // half size; beyond fogFar of every preset
    float linearShare = 0.075f; // spacing near the centre = radius * linearShare * 2 / quads
};

void OceanInit(Ocean& o);
void OceanUpdate(Ocean& o, float dt, float boatSpeed);
void OceanApplyWeather(Ocean& o, const WeatherPreset& w);   // call every frame (cheap)
float OceanHeightAt(const Ocean& o, float x, float z);
// Where the boat is: drives the wake/hull foam and the dry-deck dent in the water shaders.
struct OceanHull {
    Vector2 pos = {};          // x, z
    float speed = 0.0f;
    float halfBeam = 1.0f;
    float halfLength = 3.0f;
    float waterline = 0.0f;    // world height where the hull meets calm water
    float pitch = 0.0f;
};
void OceanDraw(Ocean& o, const AtmosphereFrame& atm, const OceanHull& hull);
void OceanUnload(Ocean& o);
