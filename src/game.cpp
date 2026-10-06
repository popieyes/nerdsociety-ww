#include "game.h"

#include "raymath.h"

namespace {

constexpr float kBpm = 100.0f;
constexpr const char* kMusicPath = "resources/music/theme.ogg";   // optional; not in the repo yet
constexpr float kWeatherBlendSeconds = 4.0f;
constexpr float kShowcaseHold = 14.0f;        // seconds per preset when auto-cycling (P)
constexpr float kShowcaseBlend = 5.0f;
constexpr float kPropWindDrift = 0.06f;       // fraction of the wind speed props drift with

struct FrameInput {
    bool drumHit = false;
    int weatherRequest = -1;
    bool toggleShowcase = false;
    bool toggleCamera = false;
    bool lightning = false;
    bool toggleHud = false;
};

FrameInput ReadInput()
{
    FrameInput in;
    in.drumHit = IsKeyPressed(KEY_SPACE);
    for (int i = 0; i < WeatherPresetCount() && i < 9; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) in.weatherRequest = i;
    }
    in.toggleShowcase = IsKeyPressed(KEY_P);
    in.toggleCamera = IsKeyPressed(KEY_C);
    in.lightning = IsKeyPressed(KEY_L);
    in.toggleHud = IsKeyPressed(KEY_H);
    return in;
}

void DrawDebugHud(const Game& g)
{
    const BeatInfo beat = ConductorBeat(g.conductor);
    const Color text = RAYWHITE;
    DrawFPS(10, 10);
    DrawText(TextFormat("Weather: %s  [1-%d]  showcase [P] %s", g.weather.current.name, WeatherPresetCount(),
                        g.showcase ? "ON" : "off"), 10, 34, 20, text);
    DrawText(TextFormat("Camera [C]: %s   lightning [L]   hud [H]", g.rig.mode == RigMode::Side ? "side" : "chase"),
             10, 58, 20, text);
    DrawText(TextFormat("BPM %.0f  beat %ld  %s", g.conductor.bpm, beat.index,
                        g.conductor.hasMusic ? "(music)" : "(internal clock)"), 10, 82, 20, text);
    if (g.hasHit) {
        DrawText(TextFormat("[SPACE] %s %+.0f ms", JudgementName(g.lastJudgement), g.lastHitOffset * 1000.0),
                 10, 106, 20, text);
    }
    // Metronome dot: flashes on each beat.
    const float pulse = 1.0f - beat.phase;
    DrawCircle(GetScreenWidth() - 30, 30, 8.0f + 8.0f * pulse * pulse, Fade(MAROON, 0.4f + 0.6f * pulse));
}

} // namespace

void GameInit(Game& g, int weatherIndex)
{
    ConductorInit(g.conductor, kBpm, 0.0f, kMusicPath);
    WeatherSnap(g.weather, weatherIndex);
    g.lit = SceneShaderLoad("resources/shaders/lit.vs", "resources/shaders/lit.fs");
    OceanInit(g.ocean);
    OceanApplyWeather(g.ocean, g.weather.current);
    SkyInit(g.sky);
    BoatInit(g.boat, g.lit, g.ocean);
    PropsInit(g.props, g.lit, g.ocean);
    VfxInit(g.vfx);
    CameraRigInit(g.rig, BoatPosition(g.boat));
}

void GameUpdate(Game& g, float dt)
{
    // 1. Input
    const FrameInput in = ReadInput();

    // 2. Rhythm
    ConductorUpdate(g.conductor, dt);

    // 3. Gameplay (crew/commands/story go here). For now: debug drum hit + weather/camera keys.
    if (in.drumHit) {
        g.hasHit = true;
        g.lastJudgement = ConductorJudge(g.conductor, g.conductor.songTime);
        g.lastHitOffset = NearestBeatOffset(g.conductor.songTime, g.conductor.bpm, g.conductor.offset);
    }
    if (in.weatherRequest >= 0) {
        g.showcase = false;
        WeatherBlendTo(g.weather, in.weatherRequest, kWeatherBlendSeconds);
    }
    if (in.toggleShowcase) {
        g.showcase = !g.showcase;
        g.showcaseTimer = 0.0f;
    }
    if (g.showcase) {
        g.showcaseTimer += dt;
        if (g.showcaseTimer >= kShowcaseHold) {
            g.showcaseTimer = 0.0f;
            WeatherBlendTo(g.weather, (g.weather.target + 1) % WeatherPresetCount(), kShowcaseBlend);
        }
    }
    if (in.toggleCamera) CameraRigToggle(g.rig);
    if (in.lightning) WeatherTriggerLightning(g.weather);
    if (in.toggleHud) g.showHud = !g.showHud;

    // 4. World + physics
    g.time += dt;
    WeatherUpdate(g.weather, dt);
    const WeatherPreset& w = g.weather.current;
    OceanApplyWeather(g.ocean, w);
    OceanUpdate(g.ocean, dt, g.boat.speed);
    BoatUpdate(g.boat, g.ocean, dt);
    PropsUpdate(g.props, g.ocean, Vector2Scale(w.windDir, w.windSpeed * kPropWindDrift), g.boat.speed, g.boat.halfBeam, dt);
    SkyUpdate(g.sky, w, dt);

    // 5. Camera
    const float sway = Clamp((w.waveScale - 0.8f) / 0.85f, 0.0f, 1.0f);
    const Vector3 cam = g.rig.camera.position;
    CameraRigUpdate(g.rig, BoatPosition(g.boat), sway, OceanHeightAt(g.ocean, cam.x, cam.z), dt);
    VfxUpdate(g.vfx, g.weather, g.boat, g.ocean, g.rig.camera, dt);
}

void GameDraw(Game& g)
{
    AtmosphereFrame atm;
    atm.weather = g.weather.current;
    atm.flash = g.weather.flash;
    atm.time = g.time;
    atm.viewPos = g.rig.camera.position;
    atm.resolution = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    ClearBackground(BLACK);
    SkyDraw(g.sky, atm, g.rig.camera);
    BeginMode3D(g.rig.camera);
        SceneShaderApply(g.lit, atm);
        BoatDraw(g.boat);
        PropsDraw(g.props);

        OceanDraw(g.ocean, atm, BoatHull(g.boat));
        VfxDraw(g.vfx, atm.weather, atm.flash, g.rig.camera);
    EndMode3D();
    if (g.showHud) DrawDebugHud(g);
}

void GameUnload(Game& g)
{
    BoatUnload(g.boat);
    PropsUnload(g.props);
    OceanUnload(g.ocean);
    SkyUnload(g.sky);
    VfxUnload(g.vfx);
    SceneShaderUnload(g.lit);
    ConductorUnload(g.conductor);
}
