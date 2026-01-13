#include "ui.h"
#include <vector>
#include <iostream>
#include <array>
#include <cstdint>

static const char* uiVertSrc = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 UV;
void main(){ gl_Position = vec4(aPos,0.0,1.0); UV=aUV; }
)";
static const char* uiFragSrc = R"(
#version 330 core
in vec2 UV; out vec4 FragColor; uniform sampler2D atlas; uniform vec4 tint; void main(){ vec4 tc = texture(atlas, UV); FragColor = tc * tint; }
)";

static GLuint compileSrc(GLenum t,const char* src){ GLuint s=glCreateShader(t); glShaderSource(s,1,&src,nullptr); glCompileShader(s); GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok); if(!ok){char buf[512];glGetShaderInfoLog(s,512,nullptr,buf);std::cerr<<buf<<"\n";} return s; }

#include "font.h"
#include "../third_party/stb_image.h"
#include <cstring>


UIRenderer::UIRenderer(): font(nullptr), logoTex(0), logoW(0), logoH(0) {}
UIRenderer::~UIRenderer(){ if(font) delete font; if(whiteTex) glDeleteTextures(1,&whiteTex); if(logoTex) glDeleteTextures(1,&logoTex); if(vbo) glDeleteBuffers(1,&vbo); if(vao) glDeleteVertexArrays(1,&vao); }

void UIRenderer::ensureWhiteTexture(){
    if (whiteTex) return;
    unsigned char px[4] = {255,255,255,255};
    glGenTextures(1,&whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,px);
    glBindTexture(GL_TEXTURE_2D,0);
}

bool UIRenderer::init(){
    shader.id = glCreateProgram();
    GLuint v=compileSrc(GL_VERTEX_SHADER, uiVertSrc); GLuint f=compileSrc(GL_FRAGMENT_SHADER, uiFragSrc);
    glAttachShader(shader.id,v); glAttachShader(shader.id,f); glLinkProgram(shader.id);
    glDeleteShader(v); glDeleteShader(f);
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);
    ensureWhiteTexture();
    loadBuiltInFont();
    // set default tint to white so drawSprite doesn't need to override it
    shader.use();
    glUniform4f(glGetUniformLocation(shader.id, "tint"), 1.0f,1.0f,1.0f,1.0f);
    return true;
}

void UIRenderer::drawSprite(float x,float y,float w,float h,int tile, int atlasTiles, int windowW, int windowH){
    float u0 = (tile + 0.0f) / (float)atlasTiles;
    float u1 = (tile + 1.0f) / (float)atlasTiles;
    // convert pixels to NDC
    float nx = (x / (float)windowW) * 2.0f - 1.0f;
    float ny = 1.0f - (y / (float)windowH) * 2.0f;
    float nw = (w / (float)windowW) * 2.0f;
    float nh = (h / (float)windowH) * 2.0f;

    float verts[6*4] = {
        nx,        ny,         u0,1.0f,
        nx+nw,     ny,         u1,1.0f,
        nx+nw,     ny-nh,      u1,0.0f,
        nx,        ny,         u0,1.0f,
        nx+nw,     ny-nh,      u1,0.0f,
        nx,        ny-nh,      u0,0.0f,
    };
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_DYNAMIC_DRAW);
    shader.use();
    // draw UI on top: disable depth test and enable alpha blending for transparency
    GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    if (prevDepth) glDisable(GL_DEPTH_TEST);
    if (!prevBlend) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
    glActiveTexture(GL_TEXTURE0);
    // caller must bind atlas or white texture before calling drawSprite
    glDrawArrays(GL_TRIANGLES,0,6);
    // restore GL state
    if (!prevBlend) glDisable(GL_BLEND);
    if (prevDepth) glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}



void UIRenderer::drawRect(float x,float y,float w,float h, float r,float g,float b,float a, int windowW, int windowH){
    // ensure shader program is active when setting uniforms
    shader.use();
    // bind white texture
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, whiteTex);
    glUniform4f(glGetUniformLocation(shader.id, "tint"), r,g,b,a);
    drawSprite(x,y,w,h,0,1,windowW,windowH);
    // reset tint
    shader.use();
    glUniform4f(glGetUniformLocation(shader.id, "tint"), 1.0f,1.0f,1.0f,1.0f);
}

bool UIRenderer::loadBuiltInFont(){
    // keep the tiny fallback; we still prefer TrueType if loaded
    font5x5.clear();
    font5x5['A'] = {0x1E,0x05,0x05,0x1E,0x00};
    font5x5['P'] = {0x1F,0x09,0x09,0x06,0x00};
    font5x5['U'] = {0x11,0x11,0x11,0x0E,0x00};
    font5x5['S'] = {0x0E,0x15,0x15,0x09,0x00};
    font5x5['R'] = {0x1F,0x09,0x19,0x16,0x00};
    font5x5['E'] = {0x1F,0x15,0x15,0x11,0x00};
    font5x5['M'] = {0x1F,0x02,0x04,0x02,0x1F};
    font5x5['N'] = {0x1F,0x02,0x04,0x1F,0x00};
    font5x5['I'] = {0x11,0x1F,0x11,0x00,0x00};
    font5x5['O'] = {0x0E,0x11,0x11,0x0E,0x00};
    font5x5['T'] = {0x01,0x1F,0x01,0x00,0x00};
    font5x5['L'] = {0x1F,0x10,0x10,0x00,0x00};
    font5x5[' ']= {0x00,0x00,0x00,0x00,0x00};
    font5x5['Q'] = {0x0E,0x11,0x19,0x1E,0x00};
    font5x5['C'] = {0x0E,0x11,0x11,0x00,0x00};
    font5x5['X'] = {0x19,0x06,0x06,0x19,0x00};
    font5x5['V'] = {0x1C,0x02,0x02,0x1C,0x00};
    font5x5['W'] = {0x1E,0x01,0x06,0x01,0x1E};
    font5x5['D'] = {0x1F,0x11,0x11,0x0E,0x00};
    return true;
}

bool UIRenderer::loadFont(const std::string& path, int pxSize) {
    if (font) { delete font; font = nullptr; }
    font = new FontAtlas();
    if (!font->loadFromFile(path, pxSize)) { delete font; font = nullptr; return false; }
    return true;
}

void UIRenderer::drawText(float x,float y,int size,const std::string& text, float r,float g,float b,float a, int windowW, int windowH){
    // prefer TrueType font if loaded
    if (font && font->valid()){
        // ensure shader program is active when setting tint
        shader.use();
        font->bind(0);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUniform4f(glGetUniformLocation(shader.id, "tint"), r,g,b,a);
        float penX = (float)x;
        float penY = (float)y;
        // Simple bitmap rendering using glyph sizes; precise UV mapping not implemented yet
        for (char ch : text){
            auto g = font->glyphFor(ch);
            float w = (float)g.w;
            float h = (float)g.h;
            // draw glyph using precise UVs from the baked font
            float u0 = g.u0, v0 = 1.0f - g.v0; // flip v
            float u1 = g.u1, v1 = 1.0f - g.v1;
            // ensure shader and font texture are bound
            shader.use(); font->bind(0);
            drawSpriteUV(penX + g.xoff, penY + g.yoff, w, h, u0, v0, u1, v1, windowW, windowH);
            penX += g.xadvance;
        }
        glDisable(GL_BLEND);
        // reset tint
        shader.use();
        glUniform4f(glGetUniformLocation(shader.id, "tint"), 1.0f,1.0f,1.0f,1.0f);
        return;
    }

    // fallback to 5x5 bitmap
    float cx = (float)x;
    for (char ch : text){
        char C = ch;
        if (C >= 'a' && C <= 'z') C = C - 'a' + 'A';
        auto it = font5x5.find(C);
        if (it == font5x5.end()) { cx += size * 6; continue; }
        auto arr = it->second;
        for (int col=0; col<5; ++col){
            uint8_t colb = arr[col];
            for (int row=0; row<5; ++row){
                if (colb & (1 << row)){
                    // draw pixel at (cx + col*size, y + row*size)
                    drawRect(cx + col*size, y + row*size, size, size, r,g,b,a, windowW, windowH);
                }
            }
        }
        cx += size * 6; // letter spacing
    }
}


void UIRenderer::drawBlock3D(float x,float y,float w,float h,int tileTop,int tileSide,int tileFront,int atlasTiles,int windowW,int windowH){
    // simple 3-quad illusion: left side, front, then top
    // left side (darker)
    shader.use();
    glUniform4f(glGetUniformLocation(shader.id, "tint"), 0.75f,0.75f,0.75f,1.0f);
    drawSprite(x, y + h*0.25f, w*0.5f, h*0.65f, tileSide, atlasTiles, windowW, windowH);
    // front face (mid-tone)
    glUniform4f(glGetUniformLocation(shader.id, "tint"), 0.9f,0.9f,0.9f,1.0f);
    drawSprite(x + w*0.45f, y + h*0.25f, w*0.5f, h*0.65f, tileFront, atlasTiles, windowW, windowH);
    // top face (bright)
    glUniform4f(glGetUniformLocation(shader.id, "tint"), 1.0f,1.0f,1.0f,1.0f);
    drawSprite(x + w*0.15f, y, w*0.7f, h*0.35f, tileTop, atlasTiles, windowW, windowH);
    // reset tint
    shader.use();
    glUniform4f(glGetUniformLocation(shader.id, "tint"), 1.0f,1.0f,1.0f,1.0f);
}

void UIRenderer::drawSpriteUV(float x,float y,float w,float h,float u0,float v0,float u1,float v1, int windowW, int windowH){
    // convert pixels to NDC
    float nx = (x / (float)windowW) * 2.0f - 1.0f;
    float ny = 1.0f - (y / (float)windowH) * 2.0f;
    float nw = (w / (float)windowW) * 2.0f;
    float nh = (h / (float)windowH) * 2.0f;

    float verts[6*4] = {
        nx,        ny,         u0,v0,
        nx+nw,     ny,         u1,v0,
        nx+nw,     ny-nh,      u1,v1,
        nx,        ny,         u0,v0,
        nx+nw,     ny-nh,      u1,v1,
        nx,        ny-nh,      u0,v1,
    };
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_DYNAMIC_DRAW);
    shader.use();
    // caller must bind the texture to unit 0
    glActiveTexture(GL_TEXTURE0);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}

bool UIRenderer::loadLogo(const std::string& path){
    int w,h,ic; unsigned char *data = stbi_load(path.c_str(), &w, &h, &ic, 4);
    if (!data) return false;
    // flip vertically to match OpenGL texture coordinate origin
    int rowBytes = w * 4;
    std::vector<unsigned char> flipped(rowBytes * h);
    for (int y = 0; y < h; ++y){
        memcpy(&flipped[y * rowBytes], &data[(h - 1 - y) * rowBytes], rowBytes);
    }
    if (logoTex) { glDeleteTextures(1, &logoTex); logoTex = 0; }
    glGenTextures(1, &logoTex);
    glBindTexture(GL_TEXTURE_2D, logoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    // store dimensions
    logoW = w; logoH = h;
    return true;
}

void UIRenderer::drawLogo(float x,float y,float w,float h, int windowW, int windowH){
    if (!logoTex || logoW == 0 || logoH == 0) return;
    // preserve aspect ratio
    float aspect = (float)logoW / (float)logoH;
    float targetW = w, targetH = h;
    if (targetW / targetH > aspect) {
        // constrained by height
        targetW = targetH * aspect;
    } else {
        // constrained by width
        targetH = targetW / aspect;
    }
    float ox = x + (w - targetW) * 0.5f;
    float oy = y + (h - targetH) * 0.5f;

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, logoTex);
    // ensure shader tint is neutral
    shader.use(); glUniform4f(glGetUniformLocation(shader.id, "tint"), 1.0f,1.0f,1.0f,1.0f);
    drawSprite(ox, oy, targetW, targetH, 0, 1, windowW, windowH);
    // unbind logo texture to avoid interfering with atlas usage
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool UIRenderer::hasLogo() const { return logoTex != 0; }