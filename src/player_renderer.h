#pragma once
#include <string>
#include <glad/glad.h>

class ResourcePack;

class PlayerRenderer {
public:
    PlayerRenderer();
    ~PlayerRenderer();
    bool loadSkin(const ResourcePack* rp);
    void drawHUD(int winW, int winH);
    void toggleVisible() { visible = !visible; }
    bool isVisible() const { return visible; }
private:
    unsigned int textureId = 0;
    unsigned int vao=0,vbo=0,shader=0;
    bool visible = false;
    bool initGL();
    bool loadTextureFromPath(const std::string& p);
};