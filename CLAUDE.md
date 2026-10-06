# raylib-boat — "Dando la Nota" game jam

## Concept
Patapon-style rhythm game. The player commands a crew of Viking sailors on a longship by
playing drum/horn "notes" in time with the music (Viking-themed soundtrack). Correct rhythmic
commands make the crew row, steer, brace, raise sail, etc. The boat sails through different
weathers (calm, sunset, fog, storm…) and the scenery + music tell a short, fun story.
Theme: **"Dando la Nota"** (Spanish: "hitting the note" / "making a scene" — the double meaning is welcome).

Timeline: **~1 month** jam. Small team. Scope must stay achievable — a polished small game beats an
unfinished big one.

## Decisions so far (2026-10-05)
- Camera: **side view**. Compose the scene for it.
- Current focus: **graphics only** (water, sky, weather, lighting, buoyancy). Gameplay and narrative are still
  open for discussion; don't implement them yet.
- Story/theme: undecided ("band on tour" is one option among several).
- Difficulty: deferred until a core idea is chosen.
- Music: **no composer on the team**, so plan around licensed/CC tracks or code-sequenced drums.
- Web build: SHOULD-have. Keep shaders and the main loop portable to GLSL ES / Emscripten.

## Tech
- C++17, raylib 6.0 (fetched by CMake), raymath, rlgl. No other dependencies unless justified.
- Desktop (Windows) is the primary target. Web (Emscripten) is a should-have: avoid desktop-only APIs,
  keep the main loop compatible with `emscripten_set_main_loop`.
- Shaders live in `resources/shaders/` (GLSL 330).

## Build & run (Windows, Visual Studio generator already configured in `build/`)
```
CMAKE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
"$CMAKE" -B build                       # only when CMakeLists changes / new files added
"$CMAKE" --build build --config Debug
build/Debug/RAYLIB-BOAT.exe
```
`resources/` is copied next to the exe after each build; the game `ChangeDirectory`s to the exe dir.

## Testing
See the "Testing" section in `docs/ARCHITECTURE.md` (headless-ish smoke run with screenshot,
and the `tests` target for pure-logic unit tests). Every change must build cleanly and pass the tests.

## Team (subagents in `.claude/agents/`)
- `architect` — code structure, build system, module boundaries, cleanup.
- `game-designer` — ideas, mechanics, scope; keeps `docs/DESIGN.md`.
- `scene-artist` — water, sky, weather, lighting, VFX, buoyancy physics.
- `coordinator` — reviews, builds, tests, and guards against over-engineering.

## Ground rules
- Keep it simple: plain structs + free functions/small classes, no ECS, no inheritance trees,
  no frameworks. Add abstraction only when a second real user exists.
- One module = one `.h/.cpp` pair under `src/`. Gameplay code must not call raw GL.
- Tunables (colors, wave params, timings) live in plain structs so they're easy to tweak.
- Don't commit unless the user asks.
