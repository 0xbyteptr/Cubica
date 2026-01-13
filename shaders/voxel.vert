#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in float aLight;
layout(location = 3) in vec3 aColor;
layout(location = 4) in float aType;
layout(location = 5) in float aWorldY;
layout(location = 6) in float aOverlay;

out vec2 TexCoord;
out float Light;
out vec3 Color;
out float TypeId;
out float WorldY;
out float OverlayTile;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    Light = aLight;
    Color = aColor;
    TypeId = aType;
    WorldY = aWorldY;
    OverlayTile = aOverlay;
}
