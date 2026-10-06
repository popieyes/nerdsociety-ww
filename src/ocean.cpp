#include "ocean.h"

#include "raymath.h"

#include <cmath>
#include <cstdlib>

namespace {

constexpr int kTileQuads = 128;   // (128+1)^2 vertices per tile fits 16-bit indices

// Grid coordinate u in [-1, 1] -> world position. Linear near the centre (fine, even spacing
// around the boat), cubic toward the edge (coarse where fog hides everything).
float GridPos(const OceanGridSettings& s, float u)
{
    const float a = s.linearShare;
    return s.radius * (a * u + (1.0f - a) * u * u * u);
}

float GridSpacing(const OceanGridSettings& s, float u)
{
    const float a = s.linearShare;
    return s.radius * (a + 3.0f * (1.0f - a) * u * u) * (2.0f / (float)s.quads);
}

Mesh GenTile(const OceanGridSettings& s, int tileX, int tileZ)
{
    const int n = kTileQuads + 1;
    Mesh m = {};
    m.vertexCount = n * n;
    m.triangleCount = kTileQuads * kTileQuads * 2;
    m.vertices = (float*)MemAlloc(sizeof(float) * 3 * m.vertexCount);
    m.texcoords = (float*)MemAlloc(sizeof(float) * 2 * m.vertexCount);
    m.indices = (unsigned short*)MemAlloc(sizeof(unsigned short) * 3 * m.triangleCount);
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            const float u = -1.0f + 2.0f * (float)(tileX * kTileQuads + i) / (float)s.quads;
            const float w = -1.0f + 2.0f * (float)(tileZ * kTileQuads + j) / (float)s.quads;
            const int v = j * n + i;
            m.vertices[v * 3 + 0] = GridPos(s, u);
            m.vertices[v * 3 + 1] = 0.0f;
            m.vertices[v * 3 + 2] = GridPos(s, w);
            m.texcoords[v * 2 + 0] = fmaxf(GridSpacing(s, u), GridSpacing(s, w));   // for the shader LOD
            m.texcoords[v * 2 + 1] = 0.0f;
        }
    }
    int t = 0;
    for (int j = 0; j < kTileQuads; ++j) {
        for (int i = 0; i < kTileQuads; ++i) {
            const unsigned short a = (unsigned short)(j * n + i), b = (unsigned short)(a + 1);
            const unsigned short c = (unsigned short)(a + n), d = (unsigned short)(c + 1);
            // counter-clockwise seen from above (+Y)
            m.indices[t++] = a; m.indices[t++] = c; m.indices[t++] = b;
            m.indices[t++] = b; m.indices[t++] = c; m.indices[t++] = d;
        }
    }
    UploadMesh(&m, false);
    return m;
}

Model GenOceanModel(const OceanGridSettings& s)
{
    const int tiles = s.quads / kTileQuads;
    Model model = {};
    model.transform = MatrixIdentity();
    model.meshCount = tiles * tiles;
    model.meshes = (Mesh*)MemAlloc(sizeof(Mesh) * model.meshCount);
    for (int tz = 0; tz < tiles; ++tz)
        for (int tx = 0; tx < tiles; ++tx) model.meshes[tz * tiles + tx] = GenTile(s, tx, tz);
    model.materialCount = 1;
    model.materials = (Material*)MemAlloc(sizeof(Material));
    model.materials[0] = LoadMaterialDefault();
    model.meshMaterial = (int*)MemAlloc(sizeof(int) * model.meshCount);   // all zero: material 0
    return model;
}

} // namespace

void OceanInit(Ocean& o)
{
    const OceanGridSettings grid;
    o.model = GenOceanModel(grid);
    o.shader = SceneShaderLoad("resources/shaders/water.vs", "resources/shaders/water.fs");
    o.model.materials[0].shader = o.shader.shader;

    const Shader& sh = o.shader.shader;
    o.locWaveA = GetShaderLocation(sh, "uWaveA");
    o.locWaveB = GetShaderLocation(sh, "uWaveB");
    o.locWaveCount = GetShaderLocation(sh, "uWaveCount");
    o.locMaxHeight = GetShaderLocation(sh, "uMaxHeight");
    o.locDeep = GetShaderLocation(sh, "uDeep");
    o.locShallow = GetShaderLocation(sh, "uShallow");
    o.locFoam = GetShaderLocation(sh, "uFoamColor");
    o.locFoamAmount = GetShaderLocation(sh, "uFoamAmount");
    o.locScroll = GetShaderLocation(sh, "uScroll");
    o.locBoat = GetShaderLocation(sh, "uBoat");
    o.locReflect = GetShaderLocation(sh, "uReflect");
    o.locRipple = GetShaderLocation(sh, "uRipple");
    o.locHull = GetShaderLocation(sh, "uHull");
    o.locHullSize = GetShaderLocation(sh, "uHullSize");
    o.locCamPos = GetShaderLocation(sh, "uCamPos");

    o.params = DefaultWaveParams();
    o.field = WaveFieldInit(o.params);
    o.time = 0.0f;
    o.scroll = 0.0f;
}

void OceanApplyWeather(Ocean& o, const WeatherPreset& w)
{
    o.params.amplitudeScale = w.waveScale;
    o.params.lengthScale = w.waveLength;
    o.params.choppiness = w.choppiness;
    o.params.windDir = w.windDir;
    WaveFieldBake(o.field, o.params);
    o.waterDeep = w.waterDeep;
    o.waterShallow = w.waterShallow;
    o.foamColor = w.foam;
    o.foamAmount = w.foamAmount;
    o.ripple = Clamp(0.5f + w.windSpeed * 0.06f, 0.5f, 1.5f);
}

void OceanUpdate(Ocean& o, float dt, float boatSpeed)
{
    const float ds = boatSpeed * dt;
    o.time += dt;
    o.scroll += ds;
    WaveFieldAdvance(o.field, dt, ds);
}

float OceanHeightAt(const Ocean& o, float x, float z)
{
    return WaveHeight(o.field, x, z);
}

void OceanDraw(Ocean& o, const AtmosphereFrame& atm, const OceanHull& hull)
{
    // Upload the wave field (see the sync rule in waves.h).
    float a[kMaxWaves * 4] = {};
    float b[kMaxWaves * 4] = {};
    for (int i = 0; i < o.field.count; ++i) {
        const WaveComponent& c = o.field.waves[i];
        a[i * 4 + 0] = c.dir.x;
        a[i * 4 + 1] = c.dir.y;
        a[i * 4 + 2] = c.k;
        a[i * 4 + 3] = c.phase;
        b[i * 4 + 0] = c.amplitude;
        b[i * 4 + 1] = c.horizontal;
    }
    const Shader& sh = o.shader.shader;
    const float scroll = std::fmod(o.scroll, 10000.0f);   // only feeds noise patterns
    SetShaderValueV(sh, o.locWaveA, a, SHADER_UNIFORM_VEC4, kMaxWaves);
    SetShaderValueV(sh, o.locWaveB, b, SHADER_UNIFORM_VEC4, kMaxWaves);
    SetShaderValue(sh, o.locWaveCount, &o.field.count, SHADER_UNIFORM_INT);
    SetShaderValue(sh, o.locMaxHeight, &o.field.maxHeight, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, o.locDeep, &o.waterDeep, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, o.locShallow, &o.waterShallow, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, o.locFoam, &o.foamColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, o.locFoamAmount, &o.foamAmount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, o.locScroll, &scroll, SHADER_UNIFORM_FLOAT);
    const Vector4 boat = { hull.pos.x, hull.pos.y, hull.speed, hull.halfBeam };
    const Vector4 hullV = { hull.pos.x, hull.pos.y, hull.waterline, hull.pitch };
    const Vector2 hullSize = { hull.halfBeam, hull.halfLength };
    SetShaderValue(sh, o.locBoat, &boat, SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, o.locHull, &hullV, SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, o.locHullSize, &hullSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, o.locCamPos, &atm.viewPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, o.locReflect, &o.reflectStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, o.locRipple, &o.ripple, SHADER_UNIFORM_FLOAT);
    SceneShaderApply(o.shader, atm);

    DrawModel(o.model, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
}

void OceanUnload(Ocean& o)
{
    UnloadModel(o.model);    // does not unload the material's shader
    SceneShaderUnload(o.shader);
    o = Ocean{};
}
