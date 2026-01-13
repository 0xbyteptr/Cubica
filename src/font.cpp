#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb_truetype.h"
#include "font.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <glad/glad.h>

FontAtlas::FontAtlas(){ glyphs.resize(96); }
FontAtlas::~FontAtlas(){ if (texId) glDeleteTextures(1, &texId); }

static std::vector<unsigned char> readFile(const std::string &p){ std::ifstream f(p, std::ios::binary); if(!f) return {}; f.seekg(0,std::ios::end); size_t n=f.tellg(); f.seekg(0); std::vector<unsigned char> b(n); f.read((char*)b.data(), n); return b; }

bool FontAtlas::loadFromFile(const std::string& path, int pxSize){
    auto data = readFile(path);
    if (data.empty()) return false;

    // bake ASCII 32..126
    const int FIRST = 32; const int COUNT = 95;
    const int BITMAP_W = 512, BITMAP_H = 512;
    std::vector<unsigned char> bmp(BITMAP_W * BITMAP_H);
    stbtt_bakedchar chardata[COUNT];
    if (stbtt_BakeFontBitmap(data.data(), 0, (float)pxSize, bmp.data(), BITMAP_W, BITMAP_H, FIRST, COUNT, chardata) <= 0) {
        std::cerr << "Font bake failed" << std::endl;
        return false;
    }

    // upload to GL texture (expand single-channel bitmap to RGBA so shader sampling works correctly)
    std::vector<unsigned char> rgba(BITMAP_W * BITMAP_H * 4);
    for (int i = 0; i < BITMAP_W * BITMAP_H; ++i){ unsigned char v = bmp[i]; rgba[i*4 + 0] = v; rgba[i*4 + 1] = v; rgba[i*4 + 2] = v; rgba[i*4 + 3] = v; }
    if (texId) { glDeleteTextures(1, &texId); texId = 0; }
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, BITMAP_W, BITMAP_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    texW = BITMAP_W; texH = BITMAP_H;
    // fill glyphs
    for (int i=0;i<COUNT;++i){
        stbtt_bakedchar &b = chardata[i];
        Glyph g;
        g.u0 = b.x0 / (float)texW; g.v0 = b.y0 / (float)texH;
        g.u1 = b.x1 / (float)texW; g.v1 = b.y1 / (float)texH;
        g.w = b.x1 - b.x0; g.h = b.y1 - b.y0;
        g.xoff = b.xoff; g.yoff = b.yoff;
        g.xadvance = b.xadvance;
        glyphs[i] = g;
    }
    // metrics
    stbtt_fontinfo fi;
    stbtt_InitFont(&fi, data.data(), 0);
    stbtt_GetFontVMetrics(&fi, &ascent, &descent, &lineGap);

    return true;
}

void FontAtlas::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texId);
}

FontAtlas::Glyph FontAtlas::glyphFor(char c) const {
    int idx = static_cast<unsigned char>(c) - 32;
    if (idx < 0 || idx >= (int)glyphs.size()) return glyphs[0];
    return glyphs[idx];
}
