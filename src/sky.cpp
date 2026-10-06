#include "sky.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>

void SkyInit(Sky& s)
{
    s.shader = SceneShaderLoad(nullptr, "resources/shaders/sky.fs");
    const Shader& sh = s.shader.shader;
    s.locCamFwd = GetShaderLocation(sh, "uCamFwd");
    s.locCamRight = GetShaderLocation(sh, "uCamRight");
    s.locCamUp = GetShaderLocation(sh, "uCamUp");
    s.locTanFov = GetShaderLocation(sh, "uTanFov");
    s.locMoon = GetShaderLocation(sh, "uMoon");
    s.locCloudCover = GetShaderLocation(sh, "uCloudCover");
    s.locCloudLit = GetShaderLocation(sh, "uCloudLit");
    s.locCloudShade = GetShaderLocation(sh, "uCloudShade");
    s.locCloudOffset = GetShaderLocation(sh, "uCloudOffset");
    s.locStars = GetShaderLocation(sh, "uStars");
    s.cloudOffset = {};
}

void SkyUpdate(Sky& s, const WeatherPreset& w, float dt)
{
    // Clouds drift with the wind (scaled down: they are far away).
    s.cloudOffset = Vector2Add(s.cloudOffset, Vector2Scale(w.windDir, w.windSpeed * 0.004f * dt));
    s.cloudOffset.x = std::fmod(s.cloudOffset.x, 1000.0f);
    s.cloudOffset.y = std::fmod(s.cloudOffset.y, 1000.0f);
}

void SkyDraw(Sky& s, const AtmosphereFrame& atm, const Camera3D& camera)
{
    // Ray basis matching BeginMode3D's perspective (fovy, aspect = width / height).
    const Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    const Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));
    const Vector3 up = Vector3CrossProduct(right, fwd);
    const float tanHalf = std::tan(camera.fovy * DEG2RAD * 0.5f);
    const Vector2 tanFov = { tanHalf * atm.resolution.x / atm.resolution.y, tanHalf };

    const Shader& sh = s.shader.shader;
    const WeatherPreset& w = atm.weather;
    SetShaderValue(sh, s.locCamFwd, &fwd, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, s.locCamRight, &right, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, s.locCamUp, &up, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, s.locTanFov, &tanFov, SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, s.locMoon, &w.moon, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, s.locCloudCover, &w.cloudCover, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, s.locCloudLit, &w.cloudLit, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, s.locCloudShade, &w.cloudShade, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, s.locCloudOffset, &s.cloudOffset, SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, s.locStars, &w.stars, SHADER_UNIFORM_FLOAT);
    SceneShaderApply(s.shader, atm);

    rlDisableDepthTest();
    BeginShaderMode(sh);
    DrawRectangle(0, 0, (int)atm.resolution.x, (int)atm.resolution.y, WHITE);
    EndShaderMode();
}

void SkyUnload(Sky& s)
{
    SceneShaderUnload(s.shader);
    s = Sky{};
}
