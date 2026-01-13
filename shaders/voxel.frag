#version 330 core
in vec2 TexCoord;
in float Light;
in vec3 Color;
in float TypeId;
in float WorldY;
in float OverlayTile;

out vec4 FragColor;

uniform sampler2D atlas;
uniform int atlasTiles = 1;
uniform vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
uniform float snowLine = 80.0;
uniform float snowBlendRange = 8.0;

void main() {
    vec3 texColor = texture(atlas, TexCoord).rgb;
    float diffuse = max(dot(normalize(vec3(0.0, 1.0, 0.0)), lightDir), 0.0);

    float snowFactor = 0.0;
    if (WorldY >= 0.0) {
        snowFactor = clamp((WorldY - snowLine) / snowBlendRange, 0.0, 1.0);
    }

    if (TypeId == 0.0 && snowFactor > 0.0) {
        vec3 snowColor = vec3(0.95, 0.95, 0.95);
        texColor = mix(texColor, snowColor, snowFactor);
    }

    vec3 lit = texColor * (0.3 * Light + 0.7 * diffuse);

    if (TypeId == 0.0) {
        if (WorldY >= 0.0) {
            vec3 grassTopTint = vec3(0.2, 0.9, 0.2);
            lit *= grassTopTint;
        } else {
            lit *= Color;
        }
    } else if (TypeId == 3.0) {
        lit *= Color;
    }

    if (OverlayTile >= 0.0 && atlasTiles > 0) {
        float tileW = 1.0 / float(atlasTiles);
        float baseIndex = floor(TexCoord.x / tileW + 0.001);
        float localU = TexCoord.x - baseIndex * tileW;
        float overlayU = OverlayTile * tileW + localU;
        vec2 ouv = vec2(overlayU, TexCoord.y);
        vec4 over = texture(atlas, ouv);
        float oa = over.a;
        // Blend overlay if overlay has alpha
        lit = mix(lit, over.rgb, oa);
        // Removed unconditional debug tint (magenta) which caused purple artifacts
    }

    FragColor = vec4(lit, 1.0);
}
