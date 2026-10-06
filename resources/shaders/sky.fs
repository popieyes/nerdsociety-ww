#version 330
// Full-screen sky. The gradient, halo and aurora come from common.glsl (shared with the fog);
// this adds stars, the sun/moon disc, clouds, distant fjord ridges and the lightning glow.
uniform vec3 uCamFwd;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform vec2 uTanFov;          // tan(fovy/2) * aspect, tan(fovy/2)
uniform float uMoon;           // 0 sun, 1 moon
uniform float uCloudCover;
uniform vec3 uCloudLit;
uniform vec3 uCloudShade;
uniform vec2 uCloudOffset;
uniform float uStars;

out vec4 finalColor;

float stars(vec3 dir)
{
    vec3 p = dir * 230.0;
    vec3 c = floor(p);
    float h = hash13(c);
    vec3 jitter = vec3(hash13(c + 1.3), hash13(c + 2.7), hash13(c + 5.1)) - 0.5;
    float d = length(fract(p) - 0.5 - jitter * 0.5);
    float twinkle = 0.7 + 0.3 * sin(uTime * (1.5 + h * 4.0) + h * 40.0);
    float big = step(0.993, h);
    return step(0.962, h) * smoothstep(0.16 + 0.1 * big, 0.0, d) * (0.35 + 0.65 * hash13(c + 9.1)) * twinkle;
}

vec3 disc(vec3 dir, vec3 col)
{
    float d = dot(dir, uSunDir);
    float radius = mix(0.026, 0.03, uMoon);
    // Sun bloom just outside the disc (the moon gets a faint cool one).
    float a = acos(clamp(d, -1.0, 1.0));
    col += mix(normalize(uSunColor + 1e-4) * 0.9, vec3(0.5, 0.6, 0.8) * 0.25, uMoon) * exp(-max(a - radius, 0.0) * 60.0) * 0.6 * uSunDisc;
    float haze = 1.0 - uSunDisc;   // a veiled sun (fog) gets a soft, wider edge
    float edge = smoothstep(cos(radius * (1.08 + 1.2 * haze)), cos(radius * (1.0 - 0.6 * haze)), d);
    if (edge <= 0.0) return col;
    // Sun: hot white-yellow core. Moon: pale, mottled maria, limb darkening.
    vec3 sunCol = vec3(1.25, 1.15, 0.95) + normalize(uSunColor + 1e-4) * 0.3;
    vec3 t1 = normalize(cross(uSunDir, vec3(0.0, 1.0, 0.0)));
    vec3 t2 = cross(t1, uSunDir);
    vec2 uv = vec2(dot(dir, t1), dot(dir, t2)) / radius;
    float maria = smoothstep(0.45, 0.7, vnoise(uv * 2.2 + 3.0) * 0.7 + vnoise(uv * 5.0) * 0.3);
    vec3 moonCol = mix(vec3(0.93, 0.94, 0.98), vec3(0.62, 0.66, 0.76), maria * 0.6) * (1.0 - 0.25 * dot(uv, uv));
    vec3 c = mix(sunCol, moonCol, uMoon);
    return mix(col, c, edge * uSunDisc);
}

// Returns cloud density; `shade` gets the cloud color.
float clouds(vec3 dir, out vec3 shade)
{
    shade = vec3(0.0);
    if (uCloudCover <= 0.001 || dir.y <= 0.0) return 0.0;
    vec2 uv = dir.xz / (dir.y + 0.09) * 0.35 + uCloudOffset;
    vec2 toSun = normalize(uSunDir.xz + 1e-5);
    float n = fbm(uv * 1.6);
    float n2 = fbm((uv + toSun * 0.05) * 1.6);
    float th = 1.0 - uCloudCover * 0.88 - 0.08;
    float dens = smoothstep(th, th + 0.14, n);
    float light = clamp(0.55 + (n - n2) * 6.0, 0.0, 1.0);
    light = mix(light, smoothstep(0.42, 0.58, light), 0.7);   // toon-ish two tone
    vec3 c = mix(uCloudShade, uCloudLit, light) * (0.88 + 0.35 * (n - 0.5));
    float s = max(dot(dir, uSunDir), 0.0);
    c += uGlowColor * pow(s, 8.0) * uGlow * (1.0 - smoothstep(th, th + 0.3, n)) * 0.9;   // silver lining
    c = mix(c, horizonColor(dir), 1.0 - smoothstep(0.0, 0.35, dir.y));                    // aerial haze
    shade = c;
    return dens * smoothstep(0.0, 0.1, dir.y);
}

// Fjord walls on both sides of the heading (+Z): two hazy ridge layers sitting on the horizon.
vec3 mountains(vec3 dir, vec3 col)
{
    if (uMountains <= 0.001 || dir.y <= 0.0 || dir.y > 0.2) return col;
    vec2 d = normalize(dir.xz + 1e-5);
    float side = smoothstep(0.12, 0.5, abs(d.x));
    float far = fbm(d * 2.2 + 3.0);
    float hFar = (0.012 + 0.09 * far * far * far) * side * uMountains;
    float nearN = fbm(d * 3.4 + 11.0);
    float hNear = (0.004 + 0.06 * nearN * nearN * nearN * nearN) * side * uMountains;
    vec3 hz = horizonColor(dir);
    vec3 farCol = mix(hz, uZenith * 0.9 + uAmbientGround * 0.3, 0.32);
    farCol = mix(farCol, hz * 1.12 + 0.04, smoothstep(hFar * 0.72, hFar * 0.8, dir.y) * 0.6 * step(0.04, hFar)); // snow
    vec3 nearCol = mix(hz, uAmbientGround * 0.9, 0.55);
    float mFar = smoothstep(hFar + 0.0015, hFar, dir.y);
    float mNear = smoothstep(hNear + 0.0015, hNear, dir.y);
    float mist = smoothstep(0.0, 0.035, dir.y);   // base melts into the horizon fog
    col = mix(col, mix(hz, farCol, mist), mFar);
    col = mix(col, mix(hz, nearCol, mist), mNear);
    return col;
}

void main()
{
    vec2 ndc = gl_FragCoord.xy / uResolution * 2.0 - 1.0;
    vec3 dir = normalize(uCamFwd + ndc.x * uTanFov.x * uCamRight + ndc.y * uTanFov.y * uCamUp);

    vec3 col;
    if (dir.y < 0.0) {
        col = fogColor(dir);   // under the horizon: exactly what the ocean fades into
    } else {
        col = skyGradient(dir);
        col += vec3(1.0, 1.0, 1.1) * stars(dir) * 1.4 * uStars * smoothstep(0.02, 0.25, dir.y);
        col = disc(dir, col);
        vec3 cc;
        float dens = clouds(dir, cc);
        col = mix(col, cc, dens);
        col += vec3(0.5, 0.55, 0.75) * uFlash * (0.08 + 0.3 * dens);
        col = mountains(dir, col);
    }
    finalColor = vec4(grade(col), 1.0);
}
