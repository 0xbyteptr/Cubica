#include "player_renderer.h"
#include "resourcepack.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include "../third_party/stb_image.h"

static const char* quadVertSrc = R"(#version 330 core
layout (location=0) in vec2 aPos;
layout (location=1) in vec2 aUV;
out vec2 uv;
void main(){ uv = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }
)";
static const char* quadFragSrc = R"(#version 330 core
in vec2 uv; out vec4 FragColor; uniform sampler2D tex; void main(){ FragColor = texture(tex, uv); }
)";

static unsigned int compileSimpleShader(const char* vs, const char* fs){
    unsigned int v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v,1,&vs,nullptr); glCompileShader(v);
    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f,1,&fs,nullptr); glCompileShader(f);
    unsigned int p = glCreateProgram(); glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f); return p;
}

PlayerRenderer::PlayerRenderer(){ initGL(); }
PlayerRenderer::~PlayerRenderer(){ if (textureId) glDeleteTextures(1,&textureId); if (vbo) glDeleteBuffers(1,&vbo); if (vao) glDeleteVertexArrays(1,&vao); if (shader) glDeleteProgram(shader); }

bool PlayerRenderer::initGL(){
    // quad (-1..1) we'll scale in draw
    float verts[] = {
        -0.2f, -0.2f, 0.0f, 1.0f,
         0.2f, -0.2f, 1.0f, 1.0f,
         0.2f,  0.2f, 1.0f, 0.0f,
        -0.2f,  0.2f, 0.0f, 0.0f
    };
    unsigned int inds[] = {0,1,2, 0,2,3};
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo); glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);
    shader = compileSimpleShader(quadVertSrc, quadFragSrc);
    return shader!=0;
}

bool PlayerRenderer::loadTextureFromPath(const std::string& p){
    int w,h,ic; unsigned char *data = stbi_load(p.c_str(), &w, &h, &ic, 4);
    if (!data) return false;
    if (textureId) glDeleteTextures(1,&textureId);
    glGenTextures(1,&textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    glBindTexture(GL_TEXTURE_2D,0);
    stbi_image_free(data);
    return true;
}

bool PlayerRenderer::loadSkin(const ResourcePack* rp){
    // try resourcepack first
    if (rp) {
        // check common names
        std::vector<std::string> candidates = {"player_skin","skin","steve","alex"};
        for (auto &k : candidates) {
            auto it = rp->nameToIndex.find(k);
            if (it != rp->nameToIndex.end()) {
                // we don't have file path here; ResourcePack doesn't expose paths; skip for now
                break;
            }
        }
    }
    // fallback: check ~/.Cubica/skin.png or assets/skins/player_skin.png
    const char* homedir = getenv("HOME");
    if (homedir) {
        std::string p = std::string(homedir) + "/.Cubica/skin.png";
        if (std::ifstream(p)) {
            if (loadTextureFromPath(p)) return true;
        }
    }
    // assets fallback
    std::string asset = "assets/skins/player_skin.png";
    if (std::ifstream(asset)) {
        if (loadTextureFromPath(asset)) return true;
    }

    // generate small placeholder texture
    unsigned char pixels[4*4] = {200,100,100,255, 100,200,100,255, 100,100,200,255, 200,200,100,255};
    if (textureId) glDeleteTextures(1,&textureId);
    glGenTextures(1,&textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D,0);
    return true;
}

void PlayerRenderer::drawHUD(int winW, int winH){
    if (!visible) return;
    if (!shader || !vao) return;
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(glGetUniformLocation(shader, "tex"), 0);
    glBindVertexArray(vao);
    // draw at top-left using viewport transform: we constructed quad in NDC, so just draw
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);
}