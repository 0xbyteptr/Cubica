#pragma once
#include <glad/glad.h>
#include <vector>
#include <string>

class TextureAtlas {
public:
    GLuint id = 0;
    int width = 0, height = 0, tiles = 0;

    TextureAtlas() = default;
    ~TextureAtlas();
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;

    // create a simple procedural atlas with given tile size and colors
    bool create(int tileSize, int tileCount);

    // create atlas by packing provided image files (must have same width/height)
    bool createFromFiles(const std::vector<std::string>& paths);

    void bind(int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }
};