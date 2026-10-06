#version 330
// Stylized water: analytic Gerstner normals per pixel, fresnel sky reflection, toon sun glints,
// crest subsurface tint, pinch/crest foam, boat wake, fog into the sky horizon (common.glsl).
#define MAX_WAVES 8

in vec2 fragRest;
in vec3 fragWorldPos;
in float fragFlatten;

uniform vec4 uWaveA[MAX_WAVES];     // xy = direction, z = k, w = phase   (see water.vs)
uniform vec4 uWaveB[MAX_WAVES];     // x = amplitude, y = horizontal
uniform int uWaveCount;
uniform float uMaxHeight;           // sum of amplitudes
uniform vec3 uDeep;
uniform vec3 uShallow;
uniform vec3 uFoamColor;
uniform float uFoamAmount;
uniform float uScroll;              // distance travelled: foam/wake patterns stream past the boat
uniform vec4 uBoat;                 // x, z, speed, half beam
uniform float uReflect;             // 0..1 strength of the sky reflection (stylized cap)
uniform float uRipple;              // shading-only ripple strength

out vec4 finalColor;

void main()
{
    vec2 p = fragRest;
    float footprint = length(fwidth(p));   // world units per pixel

    // Analytic tangents (mirror of GerstnerNormal() in src/waves.cpp), faded per pixel so tiny
    // waves don't sparkle/alias in the distance.
    vec3 tx = vec3(1.0, 0.0, 0.0);
    vec3 tz = vec3(0.0, 0.0, 1.0);
    for (int i = 0; i < MAX_WAVES; i++) {
        if (i >= uWaveCount) break;
        vec2 d = uWaveA[i].xy;
        float k = uWaveA[i].z;
        float fade = 1.0 - smoothstep(0.06, 0.25, footprint * k / 6.2831853);
        float theta = k * dot(d, p) + uWaveA[i].w;
        float qs = uWaveB[i].y * k * sin(theta) * fade;
        float ac = uWaveB[i].x * k * cos(theta) * fade;
        tx += vec3(-d.x * d.x * qs, d.x * ac, -d.x * d.y * qs);
        tz += vec3(-d.x * d.y * qs, d.y * ac, -d.y * d.y * qs);
    }
    float pinch = 1.0 - (tx.x * tz.z - tx.z * tz.x);   // 0 = flat, -> 1 where crests bunch up

    // Shading-only ripples (not in the CPU mirror: too small to move anything). Wind-aligned,
    // they break up the mirror-smooth look and make the glints sparkle.
    vec2 wind = uWaveCount > 0 ? uWaveA[0].xy : vec2(0.0, 1.0);
    for (int i = 0; i < 4; i++) {
        float fi = float(i);
        float ang = (fi - 1.5) * 0.7;
        vec2 d = vec2(wind.x * cos(ang) - wind.y * sin(ang), wind.x * sin(ang) + wind.y * cos(ang));
        float k = 6.2831853 / (1.9 - fi * 0.38);
        float fade = 1.0 - smoothstep(0.06, 0.25, footprint * k / 6.2831853);
        float slope = uRipple * (0.07 - fi * 0.01) * cos(k * dot(d, vec2(p.x, p.y + uScroll)) - uTime * (2.2 + fi * 0.7) + fi * 1.7) * fade;
        tx.y += d.x * slope;
        tz.y += d.y * slope;
    }
    vec3 n = normalize(cross(tz, tx));
    n = normalize(mix(n, vec3(0.0, 1.0, 0.0), clamp(fragFlatten * 1.5, 0.0, 0.85)));   // compressed crests read flatter

    vec3 toEye = uViewPos - fragWorldPos;
    float dist = length(toEye);
    vec3 v = toEye / dist;
    float h01 = clamp(fragWorldPos.y / max(uMaxHeight, 0.05) * 0.5 + 0.5, 0.0, 1.0);   // trough 0 .. crest 1

    // Body color: deep in troughs, lighter toward crests; lit by sun + hemispheric ambient.
    float ndl = dot(n, uSunDir);
    vec3 body = mix(uDeep, uShallow, smoothstep(0.35, 0.9, h01) * 0.55);
    // Toon-ish: two soft bands for faces toward/away from the sun.
    float diff = mix(0.6, 1.0, smoothstep(-0.05, 0.0, ndl - uSunDir.y * 0.93));
    vec3 col = body * (ambientLight(n) * 0.75 + uSunColor * diff * 0.5);

    // Subsurface: light through the crests, strongest looking toward the sun.
    float back = pow(max(dot(-v, normalize(vec3(-uSunDir.x, 0.2, -uSunDir.z))), 0.0), 3.0);
    float crest = smoothstep(0.5, 1.0, h01);
    col += uShallow * crest * (0.18 + 0.9 * back) * (uSunColor * 0.6 + uAmbientSky * 0.25);

    // Fresnel reflection of the sky.
    vec3 r = reflect(-v, n);
    r.y = abs(r.y);
    float fres = 0.03 + 0.97 * pow(1.0 - max(dot(n, v), 0.0), 5.0);
    fres = smoothstep(0.0, 1.0, fres) * uReflect;   // stylized: keep the body color readable
    col = mix(col, skyGradient(r), fres);

    // Sun glints: a hard toon highlight + a soft sheen.
    float nh = max(dot(n, normalize(uSunDir + v)), 0.0);
    float glint = smoothstep(0.55, 0.6, pow(nh, 350.0)) + pow(nh, 60.0) * 0.25;
    col += uSunColor * glint * smoothstep(-0.02, 0.06, uSunDir.y) * (0.1 + 0.9 * uSunDisc);

    // Foam pattern lives in the scrolling field frame, so it streams past the boat.
    vec2 fp = vec2(p.x, p.y + uScroll);
    float nz = vnoise(fp * 0.9 + uTime * 0.05) * 0.6 + vnoise(fp * 2.7 - uTime * 0.11) * 0.4;

    float foamMask = smoothstep(0.15, 0.55, pinch) * (0.5 + 0.6 * uFoamAmount) + crest * crest * uFoamAmount * 0.55;

    // Boat wake (boat heads +Z): Kelvin arms + turbulent centre trail + ring around the hull.
    vec2 q = fragWorldPos.xz - uBoat.xy;
    float speedK = clamp(uBoat.z / 5.0, 0.0, 1.5);
    float behind = -q.y - 2.2;
    float wake = 0.0;
    if (behind > -0.5) {
        float b = max(behind, 0.0);
        float arm = 1.0 - smoothstep(0.0, 0.5 + b * 0.05, abs(abs(q.x) - (uBoat.w * 0.8 + b * 0.36)));
        float trail = 1.0 - smoothstep(0.3, 1.1 + b * 0.06, abs(q.x));
        // Trail kept below the solid-foam threshold (0.62): seen edge-on from the low side camera a
        // solid trail collapses into a flat, hard-edged white slab behind the stern. Lacy only.
        wake = max(arm * 0.55, trail * 0.5) * exp(-b * 0.05) * speedK * smoothstep(-0.5, 0.5, behind);
    }
    // Hull wash: a soft, noise-broken band (mostly the lacy foam tier, solid only at the bow).
    float ring = 1.0 - smoothstep(0.0, 0.3, abs(length(q / vec2(uBoat.w * 0.95, 3.0)) - 1.0));
    wake = max(wake, ring * (0.3 + 0.32 * smoothstep(0.5, 2.8, q.y)) * (0.75 + 0.5 * nz));
    foamMask = max(foamMask, wake);
    foamMask *= 1.0 - clamp(fragFlatten * 2.5, 0.0, 1.0);   // compressed foreground crests: no foam slab
    foamMask *= mix(0.5, 1.0, smoothstep(5.0, 16.0, dist));   // right under the lens foam reads as a flat slab

    float foam = smoothstep(0.62, 0.68, foamMask + (nz - 0.5) * 0.7);
    foam += 0.35 * smoothstep(0.4, 0.45, foamMask + (nz - 0.5) * 0.9) * (1.0 - foam);   // lacy edge
    foam *= 1.0 - smoothstep(uFogRange.y * 0.3, uFogRange.y * 0.6, dist);
    vec3 foamLit = uFoamColor * (ambientLight(vec3(0.0, 1.0, 0.0)) * 0.55 + uSunColor * (0.35 + 0.35 * max(ndl, 0.0)) + 0.12);
    col = mix(col, foamLit, clamp(foam, 0.0, 1.0));

    col = applyFog(col, fragWorldPos);
    finalColor = vec4(grade(col), 1.0);
}
