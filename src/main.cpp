// Entry point: window/audio setup, command line, main loop (desktop + Web), smoke-run mode.
#include "game.h"

#include "raylib.h"
#include "rlgl.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr float kSmokeDt = 1.0f / 60.0f;   // fixed step in smoke mode for reproducible screenshots
constexpr float kQaBlendSeconds = 4.0f;    // --blend-to: frame 120 of a smoke run = halfway

struct Options {
    int frames = 0;              // > 0: smoke mode, exit after this many frames
    std::string screenshot;      // absolute path (resolved before ChangeDirectory)
    int weather = 0;
    int blendTo = -1;            // --blend-to I: start blending toward preset I at frame 0 (QA of blends)
    bool noHud = false;          // --no-hud: clean screenshots
    bool chase = false;          // --camera chase
};

Game g_game;
Options g_opts;
int g_frame = 0;
int g_exitCode = 0;

bool IsAbsolutePath(const std::string& p)
{
    if (p.empty()) return false;
    if (p[0] == '/' || p[0] == '\\') return true;
    return p.size() > 1 && p[1] == ':';   // Windows drive letter
}

bool ParseArgs(int argc, char** argv, Options& o)
{
    for (int i = 1; i < argc; ++i) {
        const bool hasValue = i + 1 < argc;
        if (!std::strcmp(argv[i], "--frames") && hasValue) o.frames = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--screenshot") && hasValue) o.screenshot = argv[++i];
        else if (!std::strcmp(argv[i], "--weather") && hasValue) o.weather = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--blend-to") && hasValue) o.blendTo = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--no-hud")) o.noHud = true;
        else if (!std::strcmp(argv[i], "--camera") && hasValue) o.chase = !std::strcmp(argv[++i], "chase");
        else {
            std::fprintf(stderr, "usage: %s [--frames N] [--screenshot out.png] [--weather I] [--blend-to I] [--no-hud] [--camera chase]\n", argv[0]);
            return false;
        }
    }
    if (!o.screenshot.empty() && o.frames <= 0) o.frames = 1;
    return true;
}

// Reads the back buffer before EndDrawing swaps it.
bool CaptureScreen(const std::string& path)
{
    rlDrawRenderBatchActive();
    Image img = LoadImageFromScreen();
    const bool ok = ExportImage(img, path.c_str());
    UnloadImage(img);
    return ok;
}

// Returns false when the program should stop (smoke run finished).
bool Frame()
{
    const bool smoke = g_opts.frames > 0;
    GameUpdate(g_game, smoke ? kSmokeDt : GetFrameTime());

    BeginDrawing();
    GameDraw(g_game);
    const bool last = smoke && ++g_frame >= g_opts.frames;
    if (last && !g_opts.screenshot.empty() && !CaptureScreen(g_opts.screenshot)) g_exitCode = 1;
    EndDrawing();
    return !last;
}

#if defined(PLATFORM_WEB)
void WebFrame() { Frame(); }
#endif

} // namespace

int main(int argc, char** argv)
{
    if (!ParseArgs(argc, argv, g_opts)) return 2;
    if (!g_opts.screenshot.empty() && !IsAbsolutePath(g_opts.screenshot)) {
        g_opts.screenshot = std::string(GetWorkingDirectory()) + "/" + g_opts.screenshot;
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "Dando la Nota");
    InitAudioDevice();
    ChangeDirectory(GetApplicationDirectory());   // resources/ lives next to the executable

    GameInit(g_game, g_opts.weather);
    g_game.showHud = !g_opts.noHud;
    if (g_opts.chase) CameraRigSetMode(g_game.rig, RigMode::Chase, true);
    if (g_opts.blendTo >= 0) WeatherBlendTo(g_game.weather, g_opts.blendTo, kQaBlendSeconds);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(WebFrame, 0, 1);
#else
    // Smoke runs are uncapped and report throughput (a rough performance check).
    const bool smoke = g_opts.frames > 0;
    SetTargetFPS(smoke ? 0 : 60);
    const double start = GetTime();
    while (!WindowShouldClose() && Frame()) {}
    if (smoke) std::printf("smoke: %d frames, avg %.2f ms/frame (%.0f FPS uncapped)\n", g_frame,
                           1000.0 * (GetTime() - start) / (g_frame > 0 ? g_frame : 1), g_frame / (GetTime() - start));
#endif

    GameUnload(g_game);
    CloseAudioDevice();
    CloseWindow();
    return g_exitCode;
}
