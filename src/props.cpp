#include "props.h"

#include "raymath.h"

#include <cmath>

namespace {

unsigned int NextRandom(unsigned int& s)
{
    s = s * 1664525u + 1013904223u;
    return s >> 8;
}

float RandomRange(unsigned int& s, float lo, float hi)
{
    return lo + (hi - lo) * (float)(NextRandom(s) & 0xFFFF) / 65535.0f;
}

unsigned char ToByte(float v) { return (unsigned char)Clamp(v * 255.0f, 0.0f, 255.0f); }

// Wood with streaky grain; `staves` vertical dark seams, horizontal iron `hoops` (v ranges) for barrels,
// or a plank frame for crates.
Texture2D GenWoodTexture(bool barrel)
{
    const int w = 64, h = 64;
    Image img = GenImageColor(w, h, WHITE);
    Color* px = (Color*)img.data;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Barrel UVs run u along the axis, v around it (GenMeshCylinder): swap so staves follow the axis.
            const float u = barrel ? (float)y / (float)h : (float)x / (float)w;
            const float v = barrel ? (float)x / (float)w : (float)y / (float)h;
            const int sx = barrel ? y : x;
            const float grain = 0.82f + 0.18f * std::sin(v * 90.0f + std::sin(u * 25.0f) * 2.0f + (float)((barrel ? y : x) / 8) * 1.7f);
            Vector3 c = Vector3Scale({ 0.58f, 0.38f, 0.21f }, grain);
            if (barrel) {
                if (sx % 8 == 0) c = Vector3Scale(c, 0.6f);                     // stave seams
                const bool hoop = (v > 0.12f && v < 0.2f) || (v > 0.8f && v < 0.88f);
                if (hoop) c = { 0.2f, 0.19f, 0.2f };
            } else {
                const int plank = y / 16;
                if (y % 16 == 0) c = Vector3Scale(c, 0.55f);                    // plank gaps
                c = Vector3Scale(c, 0.9f + 0.1f * (float)(plank % 2));
                const bool frame = x < 6 || x >= w - 6 || y < 6 || y >= h - 6;
                const bool brace = std::abs(x - y) < 4;
                if (frame || brace) c = Vector3Scale({ 0.5f, 0.32f, 0.17f }, 0.85f);
            }
            px[y * w + x] = { ToByte(c.x), ToByte(c.y), ToByte(c.z), 255 };
        }
    }
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    return t;
}

FloaterShape BarrelShape()
{
    FloaterShape s;   // lying barrel, axis along body X, radius ~0.42, length 1.1
    const Vector3 pts[] = { { 0.45f, -0.2f, 0.0f }, { -0.45f, -0.2f, 0.0f }, { 0.0f, -0.2f, 0.3f }, { 0.0f, -0.2f, -0.3f } };
    s.pointCount = 4;
    for (int i = 0; i < 4; ++i) s.points[i] = pts[i];
    s.halfExtents = { 0.55f, 0.42f, 0.42f };
    s.mass = 30.0f;
    s.heaveFrequency = 1.1f;
    s.heaveDamping = 0.35f;
    s.angularDamping = 1.2f;
    s.maxSubmersion = 0.7f;
    return s;
}

FloaterShape CrateShape()
{
    FloaterShape s;   // cube 0.9
    const Vector3 pts[] = { { 0.38f, -0.25f, 0.38f }, { -0.38f, -0.25f, 0.38f }, { 0.38f, -0.25f, -0.38f }, { -0.38f, -0.25f, -0.38f } };
    s.pointCount = 4;
    for (int i = 0; i < 4; ++i) s.points[i] = pts[i];
    s.halfExtents = { 0.45f, 0.45f, 0.45f };
    s.mass = 40.0f;
    s.heaveFrequency = 0.95f;
    s.heaveDamping = 0.4f;
    s.angularDamping = 1.5f;
    s.maxSubmersion = 0.8f;
    return s;
}

void Spawn(Props& p, Prop& prop, const Ocean& ocean, float zMin, float zMax)
{
    const PropSpawnArea& a = p.area;
    prop.kind = (NextRandom(p.rng) % 3 == 0) ? PropKind::Crate : PropKind::Barrel;
    float x = RandomRange(p.rng, a.xMin, a.xMax);
    if (std::fabs(x) < a.boatClearance) x = (x < 0.0f ? -1.0f : 1.0f) * a.boatClearance;
    const float z = RandomRange(p.rng, zMin, zMax);
    prop.body = Floater{};
    prop.body.position = { x, OceanHeightAt(ocean, x, z), z };
    prop.body.yaw = RandomRange(p.rng, 0.0f, 6.28f);
    prop.body.yawRate = RandomRange(p.rng, -0.3f, 0.3f);
    const float shade = RandomRange(p.rng, 0.8f, 1.1f);
    prop.tint = { ToByte(shade), ToByte(shade * 0.97f), ToByte(shade * 0.92f), 255 };
}

} // namespace

void PropsInit(Props& p, const SceneShader& lit, const Ocean& ocean)
{
    p.barrel = LoadModelFromMesh(GenMeshCylinder(0.42f, 1.1f, 14));
    p.crate = LoadModelFromMesh(GenMeshCube(0.9f, 0.9f, 0.9f));
    p.barrelTex = GenWoodTexture(true);
    p.crateTex = GenWoodTexture(false);
    p.barrel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = p.barrelTex;
    p.crate.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = p.crateTex;
    p.barrel.materials[0].shader = lit.shader;
    p.crate.materials[0].shader = lit.shader;
    // Cylinder mesh stands on y=0 along +Y: centre it and lay it along X.
    p.barrelBase = MatrixMultiply(MatrixTranslate(0.0f, -0.55f, 0.0f), MatrixRotateZ(PI * 0.5f));
    p.crateBase = MatrixIdentity();
    p.barrelShape = BarrelShape();
    p.crateShape = CrateShape();

    // Start populated: spread the props over the whole band, not just ahead.
    for (int i = 0; i < p.count && i < kMaxProps; ++i) Spawn(p, p.items[i], ocean, p.area.behindLimit + 10.0f, p.area.aheadMax);
}

void PropsUpdate(Props& p, const Ocean& ocean, Vector2 windDrift, float boatSpeed, float boatHalfBeam, float dt)
{
    WaterEnv env;
    env.waves = &ocean.field;
    env.current = { windDrift.x, windDrift.y - boatSpeed };
    for (int i = 0; i < p.count && i < kMaxProps; ++i) {
        Prop& prop = p.items[i];
        FloaterUpdate(prop.body, prop.kind == PropKind::Barrel ? p.barrelShape : p.crateShape, env, dt);

        // Keep out of the hull: push sideways (cheap, the boat lane is along Z at x ~ 0).
        Vector3& pos = prop.body.position;
        if (std::fabs(pos.z) < 4.8f && std::fabs(pos.x) < boatHalfBeam + 0.9f) {
            const float side = pos.x < 0.0f ? -1.0f : 1.0f;
            pos.x = side * (boatHalfBeam + 0.9f);
            prop.body.velocity.x = side * fmaxf(std::fabs(prop.body.velocity.x), 1.0f);
        }
        const bool lost = pos.z < p.area.behindLimit || pos.x < p.area.xMin - 15.0f || pos.x > p.area.xMax + 15.0f ||
                          std::fabs(pos.y) > 50.0f;
        if (lost) Spawn(p, prop, ocean, p.area.aheadMin, p.area.aheadMax);
    }

    // Props don't interpenetrate: push overlapping pairs apart in XZ (n is tiny, O(n^2) is fine).
    const float minDist = 1.4f;
    for (int i = 0; i < p.count && i < kMaxProps; ++i) {
        for (int j = i + 1; j < p.count && j < kMaxProps; ++j) {
            Vector3& a = p.items[i].body.position;
            Vector3& b = p.items[j].body.position;
            const Vector2 d = { b.x - a.x, b.z - a.z };
            const float len = Vector2Length(d);
            if (len >= minDist) continue;
            const Vector2 n = len > 1e-4f ? Vector2Scale(d, 1.0f / len) : Vector2{ 1.0f, 0.0f };
            const float push = (minDist - len) * 0.5f;
            a.x -= n.x * push; a.z -= n.y * push;
            b.x += n.x * push; b.z += n.y * push;
        }
    }
}

void PropsDraw(Props& p)
{
    for (int i = 0; i < p.count && i < kMaxProps; ++i) {
        const Prop& prop = p.items[i];
        Model& m = prop.kind == PropKind::Barrel ? p.barrel : p.crate;
        m.transform = MatrixMultiply(prop.kind == PropKind::Barrel ? p.barrelBase : p.crateBase, FloaterTransform(prop.body));
        DrawModel(m, { 0.0f, 0.0f, 0.0f }, 1.0f, prop.tint);
    }
}

void PropsUnload(Props& p)
{
    UnloadModel(p.barrel);
    UnloadModel(p.crate);
    UnloadTexture(p.barrelTex);
    UnloadTexture(p.crateTex);
    p = Props{};
}
