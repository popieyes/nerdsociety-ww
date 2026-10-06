#version 330
// Sum-of-Gerstner displacement. CPU mirror: GerstnerPoint() in src/waves.cpp (keep in sync).
#define MAX_WAVES 8

in vec3 vertexPosition;
in vec2 vertexTexCoord;             // x = local grid spacing (world units), baked by OceanInit
uniform mat4 mvp;
uniform mat4 matModel;
uniform vec4 uWaveA[MAX_WAVES];     // xy = direction, z = k (2pi/L), w = phase
uniform vec4 uWaveB[MAX_WAVES];     // x = amplitude, y = horizontal (Q*A)
uniform int uWaveCount;
uniform vec4 uHull;                 // boat: x, z, waterline height, pitch (render-only dent, see below)
uniform vec2 uHullSize;             // half beam, half length
uniform vec3 uCamPos;               // camera position (sightline clearance)
uniform float uMaxHeight;           // sum of amplitudes

out vec2 fragRest;                  // undisplaced xz: the fragment shader re-evaluates the waves
out vec3 fragWorldPos;
out float fragFlatten;              // 0..1 how much the sightline pass flattened this vertex

vec3 gerstner(vec2 p, float spacing)
{
    vec3 o = vec3(p.x, 0.0, p.y);
    for (int i = 0; i < MAX_WAVES; i++) {
        if (i >= uWaveCount) break;
        // LOD: fade waves the local grid can't represent (only far from the boat, see waves.h)
        float lod = smoothstep(3.0, 5.0, 6.2831853 / uWaveA[i].z / spacing);
        float theta = uWaveA[i].z * dot(uWaveA[i].xy, p) + uWaveA[i].w;
        o.xz += uWaveA[i].xy * (uWaveB[i].y * cos(theta) * lod);
        o.y += uWaveB[i].x * sin(theta) * lod;
    }
    return o;
}

void main()
{
    vec3 rest = (matModel * vec4(vertexPosition, 1.0)).xyz;
    vec3 world = gerstner(rest.xz, vertexTexCoord.x);
    // Keep the deck dry: inside the hull footprint the surface may not rise above the waterline.
    // Cosmetic only (not in the CPU mirror); floaters never enter this area.
    float inside = 1.0 - smoothstep(0.7, 1.0, length((world.xz - uHull.xy) / uHullSize));
    float hullY = uHull.z - sin(uHull.w) * (world.z - uHull.y) - 0.1;
    world.y = mix(world.y, min(world.y, hullY), inside);
    // Clear sightline (cosmetic, not in the CPU mirror): in the wedge of water between the camera
    // and the near side of the hull, crests are soft-compressed below the line from the lens to the
    // waterline, so a storm swell never hides the ship. Props/spray never live in this wedge.
    vec2 toBoat = uHull.xy - uCamPos.xz;
    float len = max(length(toBoat), 1e-3);
    vec2 along = toBoat / len;
    vec2 rel = world.xz - uCamPos.xz;
    float s = dot(rel, along) / len;                              // 0 at the lens, 1 at the hull centre
    float sEnd = max(1.0 - uHullSize.x * 0.9 / len, 0.05);         // near side of the hull
    float lateral = abs(rel.x * along.y - rel.y * along.x);
    float halfWidth = mix(1.5, uHullSize.y + 1.5, clamp(s, 0.0, 1.0));
    float wedge = smoothstep(-0.05, 0.08, s) * (1.0 - smoothstep(sEnd, sEnd + 0.04, s))
                * (1.0 - smoothstep(halfWidth, halfWidth + 3.0, lateral));
    float sight = mix(uCamPos.y, uHull.z - 0.25, clamp(s / sEnd, 0.0, 1.0));
    float over = max(world.y - sight, 0.0);
    world.y -= wedge * (over - 0.35 * (1.0 - exp(-over / 0.35)));  // soft knee: keeps a little shape
    fragFlatten = wedge * clamp(over / max(uMaxHeight, 0.05), 0.0, 1.0);
    fragRest = rest.xz;
    fragWorldPos = world;
    gl_Position = mvp * vec4(vertexPosition + (world - rest), 1.0);
}
