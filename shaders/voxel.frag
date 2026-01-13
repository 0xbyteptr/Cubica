#version 330 core

in vec2      TexCoord;
in float     Light;       // vertex light / ao / etc. [0..1]
in vec3      Color;       // per-vertex tint / biome color
in float     TypeId;      // 0 = terrain (grass/dirt), 3 = ? (leaves, etc.)
in float     WorldY;      // world-space height
in float     OverlayTile; // -1 = no overlay, otherwise atlas column index

out vec4     FragColor;

uniform sampler2D atlas;
uniform int       atlasTiles     = 1;          // number of horizontal tiles in atlas
uniform vec3      lightDir       = normalize(vec3(0.5, 1.0, 0.3));
uniform float     snowLine       = 80.0;
uniform float     snowBlendRange = 8.0;
uniform float     ambient        = 0.28;       // ← made tunable

// Helpers
const vec3 SNOW_COLOR    = vec3(0.96, 0.97, 0.98);
const vec3 GRASS_TINT    = vec3(0.22, 0.92, 0.18); // slightly more natural green
const float EPSILON      = 0.001;

void main()
{
    // ── Base texture ───────────────────────────────────────
    vec3 baseColor = texture(atlas, TexCoord).rgb;

    // ── Snow blending (only for terrain) ───────────────────
    float snowFactor = 0.0;
    if (TypeId == 0.0 && WorldY >= snowLine - snowBlendRange)
    {
        snowFactor = smoothstep(snowLine - snowBlendRange,
        snowLine + snowBlendRange * 0.5,
        WorldY);
        baseColor = mix(baseColor, SNOW_COLOR, snowFactor);
    }

    // ── Lighting ───────────────────────────────────────────
    vec3  normal     = vec3(0.0, 1.0, 0.0);           // we fake top-down lighting
    float NdotL      = max(dot(normal, lightDir), 0.0);
    float illumination = ambient + (1.0 - ambient) * NdotL;

    vec3 litColor = baseColor * (Light * 0.4 + illumination * 0.6);
    //                 ^^^ tweak these weights to taste

    // ── Biome / type specific tints ────────────────────────
    if (TypeId == 0.0)        // terrain
    {
        if (WorldY >= 0.0)
        litColor *= GRASS_TINT;
        else
        litColor *= Color;    // dirt/sand/rock color from vertex
    }
    else if (TypeId == 3.0)   // e.g. foliage / leaves
    {
        litColor *= Color;
    }

    // ── Optional overlay (foliage, moss, snow layer, etc.) ─
    if (OverlayTile >= 0.0 && atlasTiles > 0)
    {
        float tileWidth  = 1.0 / float(atlasTiles);
        float tileIndex  = floor(TexCoord.x / tileWidth + EPSILON);
        float localU     = TexCoord.x - tileIndex * tileWidth;

        vec2 overlayUV   = vec2(OverlayTile * tileWidth + localU, TexCoord.y);
        vec4 overlay     = texture(atlas, overlayUV);

        // Classic alpha blending – works well for leaves, details, damage decals…
        litColor = mix(litColor, overlay.rgb, overlay.a);
    }

    FragColor = vec4(litColor, 1.0);
}