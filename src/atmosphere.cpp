#include "atmosphere.h"

#include <string>

namespace {

constexpr const char* kCommonPath = "resources/shaders/common.glsl";

std::string ReadText(const char* path)
{
    char* text = LoadFileText(path);
    if (!text) return std::string();
    std::string s(text);
    UnloadFileText(text);
    return s;
}

// Assembles a shader source written as "#version 330": optionally inserts `common` right after the
// #version line. Desktop: the source is otherwise untouched (a vertex shader with no common comes
// back byte-identical). Web (WebGL2): the #version line becomes GLSL ES 3.00 with highp precision
// (required: wave phases and hashes break at mediump).
std::string PrepareSource(const std::string& src, const std::string& common)
{
    const size_t eol = src.find('\n');
    if (src.rfind("#version", 0) != 0 || eol == std::string::npos) return common + src;
#if defined(PLATFORM_WEB)
    const std::string header = "#version 300 es\nprecision highp float;\nprecision highp int;\n";
#else
    if (common.empty()) return src;
    const std::string header = src.substr(0, eol + 1);
#endif
    return header + common + "\n#line 2\n" + src.substr(eol + 1);
}

} // namespace

SceneShader SceneShaderLoad(const char* vsPath, const char* fsPath)
{
    SceneShader s;
    const std::string vs = vsPath ? PrepareSource(ReadText(vsPath), std::string()) : std::string();
    const std::string fs = PrepareSource(ReadText(fsPath), ReadText(kCommonPath));
    s.shader = LoadShaderFromMemory(vsPath ? vs.c_str() : nullptr, fs.c_str());

    AtmosphereLocs& l = s.locs;
    const Shader& sh = s.shader;
    l.sunDir = GetShaderLocation(sh, "uSunDir");
    l.sunColor = GetShaderLocation(sh, "uSunColor");
    l.zenith = GetShaderLocation(sh, "uZenith");
    l.horizon = GetShaderLocation(sh, "uHorizon");
    l.glowColor = GetShaderLocation(sh, "uGlowColor");
    l.glow = GetShaderLocation(sh, "uGlow");
    l.fogRange = GetShaderLocation(sh, "uFogRange");
    l.ambientSky = GetShaderLocation(sh, "uAmbientSky");
    l.ambientGround = GetShaderLocation(sh, "uAmbientGround");
    l.flash = GetShaderLocation(sh, "uFlash");
    l.time = GetShaderLocation(sh, "uTime");
    l.viewPos = GetShaderLocation(sh, "uViewPos");
    l.resolution = GetShaderLocation(sh, "uResolution");
    l.grade = GetShaderLocation(sh, "uGrade");
    l.tint = GetShaderLocation(sh, "uTint");
    l.aurora = GetShaderLocation(sh, "uAurora");
    l.mountains = GetShaderLocation(sh, "uMountains");
    l.sunDisc = GetShaderLocation(sh, "uSunDisc");
    return s;
}

void SceneShaderApply(const SceneShader& s, const AtmosphereFrame& f)
{
    const WeatherPreset& w = f.weather;
    const AtmosphereLocs& l = s.locs;
    const Shader& sh = s.shader;
    const Vector2 fog = { w.fogNear, w.fogFar };
    const Vector4 grade = { w.saturation, w.contrast, w.exposure, w.vignette };
    SetShaderValue(sh, l.sunDir, &w.sunDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.sunColor, &w.sunColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.zenith, &w.skyZenith, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.horizon, &w.skyHorizon, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.glowColor, &w.sunGlowColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.glow, &w.sunGlow, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, l.fogRange, &fog, SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, l.ambientSky, &w.ambientSky, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.ambientGround, &w.ambientGround, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.flash, &f.flash, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, l.time, &f.time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, l.viewPos, &f.viewPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.resolution, &f.resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, l.grade, &grade, SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, l.tint, &w.tint, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, l.aurora, &w.aurora, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, l.mountains, &w.mountains, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, l.sunDisc, &w.sunDisc, SHADER_UNIFORM_FLOAT);
}

void SceneShaderUnload(SceneShader& s)
{
    if (s.shader.id > 0) UnloadShader(s.shader);
    s = SceneShader{};
}
