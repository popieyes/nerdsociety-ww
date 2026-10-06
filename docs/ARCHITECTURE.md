# Architecture

Small C++17 + raylib 6.0 codebase. Plain structs + free functions, each module has explicit
`Init / Update / Draw / Unload`. One module = one `.h/.cpp` pair in `src/`.

## Layout
```
src/
  main.cpp          window + audio init (MSAA 4x), CLI (smoke mode), desktop/Web main loop
  game.h/.cpp       Game struct (owns all modules) + fixed frame order + debug keys/HUD
  conductor.h/.cpp  rhythm clock: BPM, song time -> beat index/phase, hit judgement   [pure logic]
  weather.h/.cpp    WeatherPreset table (5 presets), lerp, blending, lightning timing   [pure logic]
  waves.h/.cpp      sum-of-Gerstner WaveParams -> baked WaveField; forward GerstnerPoint/Normal,
                    WaveHeight() = inverted lookup (Newton)                             [pure logic]
  floater.h/.cpp    multi-point buoyancy body (heave/pitch/roll/drift), fixed 1/120 s sub-steps [pure logic]
  atmosphere.h/.cpp SceneShader: loads a shader with common.glsl prepended and uploads the shared
                    uniforms (sun, sky gradient, fog, ambient, lightning flash, grading)
  ocean.h/.cpp      wave field state (time/scroll) + tiled non-uniform water grid + water shader
  sky.h/.cpp        full-screen sky: gradient, sun/moon disc, clouds, fjord ridges, stars, aurora
  boat.h/.cpp       longship model + Floater body; gameplay hooks (impulse, position, hull info)
  props.h/.cpp      floating barrels/crates (Floaters) that stream past and respawn ahead
  vfx.h/.cpp        rain streaks, lightning bolts, bow spray (world space, after opaque geometry)
  camera_rig.h/.cpp side-on camera (the chosen view) + debug 3/4 chase; heave follow, storm lift/sway
tests/              `tests` executable: check.h macros + test_*.cpp, no window
resources/          models/, textures/, shaders/ (GLSL 330), music/ (optional) - copied next to the exe
  shaders/common.glsl    shared uniforms + skyGradient/fogColor/applyFog/ambientLight/grade
  shaders/water.vs/.fs   Gerstner displacement + stylized water shading
  shaders/sky.fs         sky pass;  shaders/lit.vs/.fs  boat + props
docs/               ARCHITECTURE.md (this), DESIGN.md (game design), screenshots/ (one per weather)
```

## Frame order (`GameUpdate` / `GameDraw`)
1. **Input** - read keys into a `FrameInput` struct.
2. **Rhythm** - `ConductorUpdate` (song position from the music stream, or internal clock).
3. **Gameplay** - consumes input judged against the conductor. *(Crew/commands/story go here.)*
4. **World + physics** - weather blend (+ lightning) -> ocean (bake waves from weather, advance
   phases/scroll) -> boat buoyancy -> props -> sky (cloud drift).
5. **Camera** - `CameraRigUpdate` (needs the water height under the lens), then `VfxUpdate`.
6. **Render** - sky (full-screen, no depth) -> 3D (boat, props, ocean, VFX) -> HUD/UI.

## Rendering model
- **One look, many shaders.** `SceneShaderLoad()` prepends `common.glsl` to each scene fragment
  shader; `SceneShaderApply()` uploads the same weather-driven uniforms to each. The horizon is
  defined once (`skyGradient()`): the sky draws it, water and lit objects fade into
  `fogColor(viewDir)` = the sky at the horizon in that direction. That's why the ocean has no edge.
- **Color grading** (exposure/saturation/contrast/tint/vignette + lightning flash) is applied at the
  end of each scene shader (`grade()`), not in a post pass: no render texture, so MSAA keeps working.
- **Water grid**: 512x512 quads in 4x4 tiles (16-bit indices), 2.4 km wide, linear spacing near the
  boat (~0.35 u) and cubic toward the edge. The vertex shader fades waves shorter than ~4 grid cells
  (only far away). Normals, foam and shading are evaluated per pixel from the rest position, with
  per-pixel fading of tiny waves (no shimmer). Extra shading-only ripples are fragment-only.
- **Dry deck + clear sightline** (cosmetic, render-only, not in the CPU mirror): water.vs clamps the
  surface under the hull below the waterline, and soft-compresses crests in the wedge of water between
  the camera and the near side of the hull below the lens-to-waterline line, so storm swells never
  hide the ship (foam and normals are flattened there too). Props/spray never live in that wedge.
- **Sky color blends** use a twilight curve (`TwilightLerp` in weather.cpp) when a warm sky is
  involved: sunset -> night passes rose -> violet instead of muddy brown; other blends stay linear.

## Rules
- **CPU/GPU wave sync.** `WaveField` (waves.h) is uploaded to `water.vs/.fs` every frame;
  `GerstnerPoint()`/`GerstnerNormal()` (waves.cpp) mirror the shader's `gerstner()` and the
  fragment normal line by line. Change both together and keep `tests/test_ocean.cpp` passing.
  Buoyancy must only use `WaveHeight`/`OceanHeightAt`. CPU heights equal the rendered surface within
  `kOceanFullDetailRadius` of the origin (the LOD fade and the dry-deck dent are render-only).
- **Wave phases are integrated** (`WaveFieldAdvance`), not `k*x - w*t`: weather blends may change
  wavelengths/directions without the far field rushing. Weather presets should keep wind directions
  within ~40 deg of each other so blends don't visibly rotate the swell.
- **Endless ocean.** The water grid and boat stay near the origin; `Ocean::scroll` (distance travelled)
  scrolls the wave field (and foam/wake patterns) along -Z past the boat. Props drift with that current.
- **Rhythm timing comes from the audio stream** (`GetMusicTimePlayed`), never accumulated frame time,
  whenever music is loaded. Without `resources/music/theme.ogg` an internal clock is used.
- **Pure logic** (conductor, weather, waves, floater, future scoring) must not need a window or GPU at
  runtime; it may include raylib/raymath headers for types. Add such sources to `BOAT_LOGIC_SOURCES`
  in `CMakeLists.txt` so the tests link them.
- **Tunables** live in plain structs/tables: `WeatherPreset` table (weather.cpp), `WaveParams`
  (waves.cpp), `FloaterShape` (boat.cpp/props.cpp), `OceanGridSettings`, `PropSpawnArea`,
  `VfxSettings`, `CameraRig`, constants at the top of `game.cpp`.
- Gameplay code never calls raw GL/rlgl (vfx/sky use rlgl for batching; they're render modules).
- Randomness in scene modules uses their own LCG seeds (not `GetRandomValue`) so smoke runs are
  deterministic.

## Gameplay hooks (scene side)
- `WeatherBlendTo(weather, preset, seconds)` / `WeatherSnap` - story beats change the weather.
- `WeatherTriggerLightning(weather)` - scripted strike (storm boss); random strikes come from
  `WeatherPreset::lightning` (strikes/min).
- `BoatApplyImpulse(boat, linear, angular)` - rowing surge, wave hit, brace (angular = pitch, yaw, roll).
- `OceanHeightAt(ocean, x, z)` - water height anywhere; `BoatPosition`, `BoatPointToWorld`.
- Lanes: set `boat.body.position.x` (the floater keeps x/z locked; wake and camera follow x).
- `CameraRigSetMode(rig, RigMode::Side/Chase, instant)`.

## Build, test, smoke run (Windows)
```
CMAKE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
"$CMAKE/cmake.exe" -B build                      # after CMakeLists changes or new files
"$CMAKE/cmake.exe" --build build --config Debug
(cd build && "$CMAKE/ctest.exe" -C Debug --output-on-failure)   # or build/Debug/tests.exe
build/Debug/RAYLIB-BOAT.exe --frames 420 --screenshot out.png --weather 1 --no-hud
```
### Testing
- `tests` target: unit tests for pure logic (`tests/test_*.cpp`, `CHECK` / `CHECK_NEAR` from `check.h`).
  New test file -> add a `void TestX();` call in `tests/test_main.cpp`.
  `test_ocean`: Gerstner inversion accuracy (every preset + extreme chop), pinch cap, analytic vs
  finite-difference normals, scroll. `test_weather`: blend endpoints for every preset pair and field,
  re-target continuity, index clamping, lightning rate + frame-rate independence. `test_floater`: drop test settles at the analytic equilibrium
  (bounded, no NaN), frame-rate independence, hitch clamp, storm stability, impulse recovery.
- Smoke mode: `--frames N` runs N frames with a fixed 1/60 s step (deterministic, uncapped; prints
  avg ms/frame), `--screenshot path` saves the last frame (relative paths resolve against the launch
  directory), `--weather I` starts in preset I, `--blend-to J` starts a 4 s blend toward preset J
  (frame 120 = halfway), `--no-hud` hides the debug HUD, `--camera chase`
  starts in the chase view. Exit code 0 on success, 1 if the screenshot couldn't be written, 2 on bad
  arguments. Use it to check visuals: run it, then open the PNG. Good frames: 420 for any preset;
  storm frame 253 catches the first lightning strike.

### Debug keys
`1`..`5` blend to weather preset (Calm fjord, Golden sunset, Fog, Storm, Night aurora),
`P` auto-cycle presets (showcase), `C` toggle side/chase camera, `L` lightning strike,
`H` toggle HUD, `Space` = drum hit (HUD shows PERFECT/GOOD/MISS + ms offset).

## Performance
Debug build, RTX 3070, 1280x720 MSAA 4x: ~1.5-2.3 ms/frame uncapped (storm with rain ~2.3 ms).
Costliest parts: water fragment shader (8 waves + 4 ripples + 2 noise taps per pixel), sky fbm clouds
(2x5 octaves per pixel), 263k-vertex water grid. Rain is up to 1400 quads through the rlgl batch.

## Where future work goes
- **Gameplay** (commands = beat patterns, crew states, scoring): new `src/commands.*` / `src/crew.*`,
  owned by `Game`, updated in step 3. Pattern matching/scoring logic should be pure and tested.
- **Story / level script** (weather changes, events on a timeline in beats): `src/story.*`, step 3,
  calls `WeatherBlendTo` / `WeatherTriggerLightning`.
- **Hazards** (rocks, serpents) floating on the water: reuse `Floater` like `props.cpp`.
- **UI** beyond the debug HUD: `src/ui.*`, drawn last in `GameDraw`.
- Game states (title/play/end) only when there's a second screen: a `enum class Screen` in `Game`.

## Web build (should-have, UNVERIFIED: no emsdk on the dev machine yet)
- Target is **WebGL2 / GLSL ES 3.00**. Shaders stay written as `#version 330`; every shader (vertex
  and fragment) goes through `SceneShaderLoad` (atmosphere.cpp), which under `PLATFORM_WEB` replaces
  the `#version` line with `#version 300 es` + `precision highp float; precision highp int;`
  (highp is required: wave phases and hashes break at mediump). On desktop the sources are passed
  through unchanged. The sky uses raylib's default vertex shader, which rlgl emits as ES 300 under ES3.
  **Never call `LoadShader` directly**; use `SceneShaderLoad`.
- CMake: emcmake (or `-DPLATFORM=Web`) sets `BOAT_WEB`, forces raylib `GRAPHICS_API_OPENGL_ES3`
  (raylib then links `-sMIN/MAX_WEBGL_VERSION=2`; we pass them too), adds `--preload-file resources`,
  `-sASYNCIFY`, and skips the `tests` target.
- Try it (needs the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html), Ninja or make):
  ```
  emsdk activate latest && emsdk_env
  emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release -G Ninja
  cmake --build build-web
  emrun build-web/RAYLIB-BOAT.html      # must be served over http, not file://
  ```
- Not yet verified: that it compiles under em++, that all shaders compile as ES 3.00, and that it
  performs well. ES 3.00 is stricter than GLSL 330: no implicit int->float conversion (`1` vs `1.0`),
  array/const rules, every `out` must be written. Shader compile errors show up in the browser console.
  Fragment cost of water + sky clouds is the main risk on weak web GPUs: drop the ripple loop / cloud
  octaves there. Audio only starts after a user click (browser autoplay policy).

## Known gaps
- `GetMusicTimePlayed` updates in audio-buffer steps; if judgement feels jittery, smooth it
  (advance by dt, re-sync to the stream when drift exceeds a few ms).
- Storm framing: the side camera follows the heave fully and lifts/pitches down as `waveScale` grows
  (`CameraRig::stormLift`); verified storm frames 200-1000 and the calm->storm blend keep the hull in view.
