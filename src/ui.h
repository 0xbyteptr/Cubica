#pragma once
#include <glad/glad.h>
#include "shader.h"

#include <string>
#include <unordered_map>

class UIRenderer {
public:
    Shader shader;
    GLuint vao=0,vbo=0;
    UIRenderer();
    ~UIRenderer();
    bool init();
    // x,y in pixels, w,h in pixels, tile index in atlas, atlas tileCount
    void drawSprite(float x,float y,float w,float h,int tile, int atlasTiles, int windowW, int windowH);
    // draw with explicit UVs (u0,v0,u1,v1) in [0..1]
    void drawSpriteUV(float x,float y,float w,float h,float u0,float v0,float u1,float v1, int windowW, int windowH);
    // load and draw a logo image for menus
    bool loadLogo(const std::string& path);
    void drawLogo(float x,float y,float w,float h, int windowW, int windowH);
    bool hasLogo() const;

    // colored rectangle (RGBA 0..1)
    void drawRect(float x,float y,float w,float h, float r,float g,float b,float a, int windowW, int windowH);

    // very small bitmap font draw (monochrome) - scale in pixels per glyph cell
    bool loadBuiltInFont();

    // new TrueType font support
    bool loadFont(const std::string& path, int pxSize = 24);
    void drawText(float x,float y,int size,const std::string& text, float r,float g,float b,float a, int windowW, int windowH);

    // draw a small 3D-looking block in UI (simple 3-quads approximation)
    void drawBlock3D(float x,float y,float w,float h,int tileTop,int tileSide,int tileFront,int atlasTiles,int windowW,int windowH);

private:
    GLuint whiteTex = 0;
    // logo texture and size
    GLuint logoTex = 0;
    int logoW = 0, logoH = 0;

    std::unordered_map<char, std::array<uint8_t,5>> font5x5;
    void ensureWhiteTexture();

    // TrueType font atlas
    class FontAtlas* font = nullptr;
};