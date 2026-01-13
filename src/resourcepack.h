#pragma once
#include <string>
#include <unordered_map>
#include "texture.h"
#include "block.h"

class ResourcePack {
public:
    TextureAtlas atlas;
    std::unordered_map<std::string,int> nameToIndex;

    // model name -> texture key -> texture reference (e.g., "top" -> "block/grass_block_top")
    std::unordered_map<std::string, std::unordered_map<std::string,std::string>> modelTextures;
    // block resource name (e.g., "grass_block") -> model name (e.g., "block/grass_block")
    std::unordered_map<std::string, std::string> blockToModel;

    enum Face { TOP=0, SIDE=1, BOTTOM=2 };

    // load assets from a resourcepack directory (expects assets/minecraft/textures/block/*.png)
    // returns true on success (atlas built), false if not found / invalid
    bool loadFromDir(const std::string& dir);

    // get tile index for a block type and face; -1 if not found
    int getTileFor(BlockType t, Face face) const;
    // get overlay tile index for a block type, -1 if not present
    int getOverlayFor(BlockType t) const;
    // helper to resolve texture reference strings to atlas indices
    int findIndexForTextureRef(const std::string& ref) const;
    
private:
    // helper to load model jsons and blockstates from the given dir
    void loadModelsAndBlockstates(const std::string& dir);
};