#pragma once
#include "raylib.h"
#include "weather.h"

// Shared "look" of the scene: every scene shader (sky, water, lit objects) gets
// resources/shaders/common.glsl prepended to its fragment shader and receives the same
// atmosphere uniforms (sun, sky gradient, fog, ambient, lightning flash, color grading).
// That is what keeps the ocean's distance fog identical to the sky's horizon color.

// Everything the shared uniforms need for one frame. Built once per frame in GameDraw.
struct AtmosphereFrame {
    WeatherPreset weather;
    float flash = 0.0f;           // lightning brightness (Weather::flash)
    float time = 0.0f;            // seconds, for animated sky/water details
    Vector3 viewPos = {};         // camera position
    Vector2 resolution = {};      // render target size in pixels
};

struct AtmosphereLocs {
    int sunDir = -1, sunColor = -1, zenith = -1, horizon = -1, glowColor = -1, glow = -1;
    int fogRange = -1, ambientSky = -1, ambientGround = -1, flash = -1, time = -1;
    int viewPos = -1, resolution = -1, grade = -1, tint = -1, aurora = -1, mountains = -1;
    int sunDisc = -1;
};

struct SceneShader {
    Shader shader = {};
    AtmosphereLocs locs;
};

// vsPath may be null (raylib's default vertex shader).
SceneShader SceneShaderLoad(const char* vsPath, const char* fsPath);
void SceneShaderApply(const SceneShader& s, const AtmosphereFrame& frame);
void SceneShaderUnload(SceneShader& s);
