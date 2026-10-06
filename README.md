# Dando la Nota (raylib-boat)

Rhythm game jam project: command a Viking longship's crew by playing in time with the music.
C++17 + [raylib](https://www.raylib.com) 6.0 (fetched by CMake). See `docs/ARCHITECTURE.md` for the
code map and `docs/DESIGN.md` for the game design.

## Build & run (desktop)
```bash
cmake -B build
cmake --build build --config Debug
build/Debug/RAYLIB-BOAT.exe        # single-config generators: build/RAYLIB-BOAT
```

## Test
```bash
cd build && ctest -C Debug --output-on-failure
build/Debug/RAYLIB-BOAT.exe --frames 120 --screenshot out.png --weather 0   # smoke run
```

## Web (optional)
Requires the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):
```bash
emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
emrun build-web/RAYLIB-BOAT.html
```
Targets WebGL2; not verified yet. See "Web build" in `docs/ARCHITECTURE.md`.
