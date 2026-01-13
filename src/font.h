#pragma once
#include <string>
#include <vector>

class FontAtlas {
public:
    FontAtlas();
    ~FontAtlas();

    // load from a font file (OTF/TTF). Returns true on success.
    bool loadFromFile(const std::string& path, int pxSize = 24);

    // bind the atlas texture to unit
    void bind(int unit = 0) const;

    // return true if loaded
    bool valid() const { return texId != 0; }

    // metrics
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;

    struct Glyph { float u0,v0,u1,v1; int w,h; int xoff,yoff; int xadvance; };

    // get glyph for ASCII char; returns default glyph for missing
    Glyph glyphFor(char c) const;

private:
    unsigned int texId = 0;
    int texW=0, texH=0;
    std::vector<Glyph> glyphs; // indexed by (c - 32)
};