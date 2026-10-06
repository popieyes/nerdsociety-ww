#version 330
// Stylized lighting for the boat and props: wrapped two-tone sun diffuse + hemispheric ambient
// from the weather + soft rim from the horizon, then the same fog/grading as the water and sky.
in vec3 fragWorldPos;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main()
{
    vec4 albedo = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec3 n = normalize(fragNormal);
    if (!gl_FrontFacing) n = -n;   // thin sails/oars are single-sheet

    vec3 v = normalize(uViewPos - fragWorldPos);
    float ndl = dot(n, uSunDir);
    float sun = smoothstep(-0.1, 0.35, ndl) * 0.8 + 0.2 * max(ndl, 0.0);   // soft toon terminator
    vec3 light = ambientLight(n) * 0.9 + uSunColor * sun * 0.85;

    // Rim: the horizon/sky color wraps around silhouettes (ties the boat into the backdrop).
    float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);
    vec3 col = albedo.rgb * light + fogColor(-v) * rim * 0.25;

    col = applyFog(col, fragWorldPos);
    finalColor = vec4(grade(col), albedo.a);
}
