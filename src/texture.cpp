#include "texture.h"
#include <vector>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

bool TextureAtlas::create(int tileSize, int tileCount) {
    tiles = tileCount;
    width = tileSize * tileCount;
    height = tileSize;

    std::vector<unsigned char> pixels(width * height * 3, 0);

    // simple colors per tile: grass, dirt, stone, wood, leaves
    const unsigned char colors[][3] = {
        { 34, 139, 34 },   // grass (green)
        { 139, 69, 19 },   // dirt (brown)
        { 128, 128, 128 }, // stone (gray)
        { 160, 82, 45 },   // wood (sienna)
        { 34, 139, 34 }    // leaves (green)
    };

    for (int t = 0; t < tileCount; ++t) {
        for (int y = 0; y < tileSize; ++y) {
            for (int x = 0; x < tileSize; ++x) {
                int px = t * tileSize + x;
                int idx = (y * width + px) * 3;
                int ci = t % 5;
                pixels[idx+0] = colors[ci][0];
                pixels[idx+1] = colors[ci][1];
                pixels[idx+2] = colors[ci][2];
            }
        }
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return id != 0;
}

bool TextureAtlas::createFromFiles(const std::vector<std::string>& paths) {
    if (paths.empty()) return false;
    int w=0,h=0,channels=0;
    std::vector<std::vector<unsigned char>> imgs;
    imgs.reserve(paths.size());
    for (auto &p : paths) {
        int iw, ih, ic;
        unsigned char *data = stbi_load(p.c_str(), &iw, &ih, &ic, 4);
        if (!data) { std::cerr << "Failed to load texture: "<<p<<"\n"; return false; }
        if (w==0) { w = iw; h = ih; channels = 4; }
        if (iw != w || ih != h) { std::cerr << "Texture sizes mismatch in atlas\n"; stbi_image_free(data); return false; }
        imgs.emplace_back(data, data + (w*h*4));
        stbi_image_free(data);
    }
    tiles = static_cast<int>(imgs.size());
    width = w * tiles;
    height = h;
    std::vector<unsigned char> pixels(width * height * 4, 0);
    for (int t=0; t<tiles; ++t) {
        for (int y=0;y<h;++y) for (int x=0;x<w;++x) {
            int sx = t*w + x;
            int dst = (y*width + sx) * 4;
            int src = (y*w + x) * 4;
            pixels[dst+0] = imgs[t][src+0];
            pixels[dst+1] = imgs[t][src+1];
            pixels[dst+2] = imgs[t][src+2];
            pixels[dst+3] = imgs[t][src+3];
        }
    }

    // if tiles include leaves/logs, ensure we recognize them (no-op here, just kept for future)

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return id != 0;
}
