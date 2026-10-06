#include "boat.h"

#include "raymath.h"

namespace {

constexpr float kBowZ = 2.7f;   // bow tip distance from the centre (world units, scaled model)

FloaterShape LongshipShape()
{
    FloaterShape s;
    // Hull samples at the waterline plane: bow, stern, 2 beams, 4 shoulders.
    const Vector3 pts[] = { { 0.0f, 0.0f, 2.6f }, { 0.0f, 0.0f, -2.6f }, { 1.1f, 0.0f, 0.0f }, { -1.1f, 0.0f, 0.0f },
                            { 0.85f, 0.0f, 1.4f }, { -0.85f, 0.0f, 1.4f }, { 0.85f, 0.0f, -1.4f }, { -0.85f, 0.0f, -1.4f } };
    s.pointCount = 8;
    for (int i = 0; i < s.pointCount; ++i) s.points[i] = pts[i];
    s.halfExtents = { 1.3f, 1.4f, 3.0f };   // taller than the hull: includes mast/cargo, feels heavier
    s.mass = 400.0f;
    s.heaveFrequency = 1.15f;   // well above the wave encounter frequency, so the hull rides the swell
    s.heaveDamping = 0.7f;
    s.angularDamping = 2.5f;
    s.maxSubmersion = 1.6f;
    s.lockHorizontal = true;
    return s;
}

} // namespace

void BoatInit(Boat& b, const SceneShader& lit, const Ocean& ocean)
{
    b = Boat{};
    b.model = LoadModel("resources/models/low_poly_viking_ship.obj");
    b.albedo = LoadTexture("resources/textures/_Barkito_Barcowire_088144225_albedo.png");
    GenTextureMipmaps(&b.albedo);
    SetTextureFilter(b.albedo, TEXTURE_FILTER_TRILINEAR);
    for (int i = 0; i < b.model.materialCount; ++i) {
        b.model.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = b.albedo;
        b.model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        b.model.materials[i].shader = lit.shader;
    }
    b.shape = LongshipShape();
    b.body = Floater{};
    b.body.position.y = OceanHeightAt(ocean, 0.0f, 0.0f) - FloaterEquilibriumDepth(b.shape);   // start at rest on the swell
}

void BoatUpdate(Boat& b, const Ocean& ocean, float dt)
{
    const Vector3 bowBefore = BoatPointToWorld(b, { 0.0f, 0.0f, kBowZ });
    const float waterBefore = OceanHeightAt(ocean, bowBefore.x, bowBefore.z);

    WaterEnv env;
    env.waves = &ocean.field;
    FloaterUpdate(b.body, b.shape, env, dt);

    // Bow slam: the bow moving down into the water (relative to the surface) throws spray.
    if (dt > 0.0f) {
        const Vector3 bow = BoatPointToWorld(b, { 0.0f, 0.0f, kBowZ });
        const float water = OceanHeightAt(ocean, bow.x, bow.z);
        const float relVel = ((bow.y - bowBefore.y) - (water - waterBefore)) / dt;   // < 0: digging in
        const float depth = water - (bow.y - 0.3f);
        const float slam = Clamp(-relVel * 0.5f, 0.0f, 1.0f) * Clamp(depth * 2.0f + 0.5f, 0.0f, 1.0f);
        b.bowSplash = fmaxf(slam, b.bowSplash - dt * 3.0f);
    }

    // Model: scale, lift the origin above the waterline, then the floating body's transform.
    const float lift = b.waterline + FloaterEquilibriumDepth(b.shape);
    b.model.transform = MatrixMultiply(MatrixMultiply(MatrixScale(b.scale, b.scale, b.scale), MatrixTranslate(0.0f, lift, 0.0f)),
                                       FloaterTransform(b.body));
}

void BoatDraw(const Boat& b)
{
    DrawModel(b.model, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
}

void BoatUnload(Boat& b)
{
    UnloadModel(b.model);
    UnloadTexture(b.albedo);
    b = Boat{};
}

void BoatApplyImpulse(Boat& b, Vector3 linear, Vector3 angular)
{
    FloaterApplyImpulse(b.body, linear, angular);
}

Vector3 BoatPosition(const Boat& b)
{
    return b.body.position;
}

Vector3 BoatPointToWorld(const Boat& b, Vector3 local)
{
    return FloaterLocalToWorld(b.body, local);
}

OceanHull BoatHull(const Boat& b)
{
    OceanHull h;
    h.pos = { b.body.position.x, b.body.position.z };
    h.speed = b.speed;
    h.halfBeam = b.halfBeam;
    h.halfLength = 2.9f;
    h.waterline = b.body.position.y + FloaterEquilibriumDepth(b.shape);
    h.pitch = b.body.pitch;
    return h;
}
