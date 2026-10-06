// ---------------------------------------------------------------------------------------------
// common.glsl - prepended (after #version) to every scene fragment shader by SceneShaderLoad()
// in src/atmosphere.cpp. Uniforms are set from the current WeatherPreset each frame.
// The sky gradient here is the single source of truth for the horizon/fog color: the sky draws
// skyGradient(), water and lit objects fade into fogColor(), which is skyGradient() at the horizon.
// ---------------------------------------------------------------------------------------------
uniform vec3 uSunDir;          // normalized, toward the sun (or moon)
uniform vec3 uSunColor;        // direct light
uniform float uSunDisc;        // 0..1 visibility of the sun/moon disc (also scales water glints)
uniform vec3 uZenith;
uniform vec3 uHorizon;
uniform vec3 uGlowColor;       // halo/horizon tint toward the sun
uniform float uGlow;
uniform vec2 uFogRange;        // x = fog start, y = full fog (world units)
uniform vec3 uAmbientSky;
uniform vec3 uAmbientGround;
uniform float uFlash;          // lightning, 0..~1.3
uniform float uTime;
uniform vec3 uViewPos;
uniform vec2 uResolution;
uniform vec4 uGrade;           // x saturation, y contrast, z exposure, w vignette
uniform vec3 uTint;
uniform float uAurora;
uniform float uMountains;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash13(vec3 p3)
{
    p3 = fract(p3 * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

float vnoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash12(i), hash12(i + vec2(1.0, 0.0)), u.x),
               mix(hash12(i + vec2(0.0, 1.0)), hash12(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p)
{
    float s = 0.0, a = 0.5;
    for (int i = 0; i < 5; i++) {
        s += a * vnoise(p);
        p = mat2(1.6, 1.2, -1.2, 1.6) * p + 17.0;
        a *= 0.5;
    }
    return s / 0.96875;
}

// Northern lights: curtains hanging above the horizon, mostly toward +X/+Z (in front of both cameras).
vec3 aurora(vec3 dir)
{
    if (uAurora <= 0.001 || dir.y <= 0.0) return vec3(0.0);
    float az = atan(dir.x, dir.z);
    float e = asin(clamp(dir.y, 0.0, 1.0));
    vec3 col = vec3(0.0);
    for (int i = 0; i < 2; i++) {
        float fi = float(i);
        // Wavy ribbon: the lower edge folds back and forth across the sky.
        float base = 0.075 + 0.07 * fi + 0.045 * sin(az * 5.0 + uTime * 0.07 + fi * 2.1)
                   + 0.03 * sin(az * 11.0 - uTime * 0.13 + fi);
        float above = e - base;
        float curtain = smoothstep(-0.012, 0.01, above) * exp(-max(above, 0.0) * (11.0 + 4.0 * fi));
        float rays = vnoise(vec2(az * (45.0 + 25.0 * fi) + uTime * 0.25, fi * 7.0));
        rays = 0.3 + 0.7 * rays * rays;
        float patches = smoothstep(0.25, 0.75, vnoise(vec2(az * 3.0 + uTime * 0.04 + fi * 5.0, fi)));
        vec3 c = mix(vec3(0.15, 1.0, 0.55), vec3(0.6, 0.3, 1.0), clamp(above * 9.0, 0.0, 1.0)) * (1.0 - 0.45 * clamp(above * 9.0, 0.0, 1.0));
        col += c * curtain * rays * patches * (0.7 - 0.3 * fi);
    }
    float mask = smoothstep(-0.3, 0.7, dot(normalize(dir.xz + 1e-5), normalize(vec2(1.0, 0.7))));
    return col * mask * uAurora * smoothstep(0.0, 0.05, dir.y);
}

// Horizon color in the azimuth of dir: base horizon + glow toward the sun.
vec3 horizonColor(vec3 dir)
{
    vec2 d = normalize(dir.xz + 1e-5);
    vec2 s = normalize(uSunDir.xz + 1e-5);
    float toSun = max(dot(d, s), 0.0);
    return mix(uHorizon, uGlowColor, uGlow * toSun * toSun * toSun);
}

// Sky without the sun disc, clouds and stars (also used for water reflections).
vec3 skyGradient(vec3 dir)
{
    float h = clamp(dir.y, 0.0, 1.0);
    vec3 col = mix(horizonColor(dir), uZenith, smoothstep(0.0, 1.0, pow(h, 0.55)));
    float s = max(dot(dir, uSunDir), 0.0);
    col += uGlowColor * (pow(s, 10.0) * 0.35 + pow(s, 120.0) * 0.6 * uSunDisc) * uGlow;   // halo
    col += aurora(dir);
    return col;
}

// Fog color seen along a view direction = the sky right at the horizon there.
vec3 fogColor(vec3 viewDir)
{
    return skyGradient(normalize(vec3(viewDir.x, 0.0, viewDir.z) + vec3(1e-5, 0.0, 0.0)));
}

vec3 applyFog(vec3 col, vec3 worldPos)
{
    vec3 v = worldPos - uViewPos;
    float d = length(v);
    float f = clamp((d - uFogRange.x) / max(uFogRange.y - uFogRange.x, 1e-3), 0.0, 1.0);
    f = 1.0 - (1.0 - f) * (1.0 - f);   // eases in quickly, settles into the horizon
    return mix(col, fogColor(v / max(d, 1e-4)), f);
}

// Hemispheric ambient: sky from above, sea bounce from below.
vec3 ambientLight(vec3 n)
{
    return mix(uAmbientGround, uAmbientSky, n.y * 0.5 + 0.5);
}

// Final color grading, applied per shader (keeps MSAA; no post-process pass needed).
vec3 grade(vec3 c)
{
    c = mix(c, c * 1.5 + vec3(0.08, 0.09, 0.13), clamp(uFlash, 0.0, 1.5) * 0.3);   // lightning
    c *= uGrade.z;
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, uGrade.x);
    c = (c - 0.5) * uGrade.y + 0.5;
    c *= uTint;
    vec2 q = gl_FragCoord.xy / uResolution - 0.5;
    q.x *= uResolution.x / uResolution.y;
    c *= 1.0 - uGrade.w * smoothstep(0.45, 1.15, length(q));
    c += (hash12(gl_FragCoord.xy) - 0.5) / 255.0;   // dither: no banding in the sky gradient
    return clamp(c, 0.0, 1.0);
}
