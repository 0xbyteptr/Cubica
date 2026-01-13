#include "inventory.h"
#include "ui.h" 
#include "resourcepack.h"
#include <iostream>

Inventory::Inventory(){
    hotbar.fill(BlockType::DIRT);
    hotbar[0]=BlockType::GRASS; hotbar[1]=BlockType::DIRT; hotbar[2]=BlockType::STONE;
    // default tools: hand for most, put a wooden pick and shovel on slot 3/4 for testing
    tools.fill(TOOL_HAND);
    tools[3] = TOOL_WOODEN_PICKAXE;
    tools[4] = TOOL_WOODEN_SHOVEL;
}

void Inventory::draw(UIRenderer& ui, int windowW, int windowH, const ResourcePack* rp){
    int slotW = 40; int slotH = 40;
    int totalW = slotW * 9 + 8*4;
    int startX = (windowW - totalW) / 2;
    int y = windowH - slotH - 20;
    for (int i=0;i<9;++i){
        int x = startX + i*(slotW+4);
        int tile = 0;
        if (rp){
            switch(hotbar[i]){
                case BlockType::GRASS: tile = rp->nameToIndex.count("grass_block") ? rp->nameToIndex.at("grass_block") : 0; break;
                case BlockType::DIRT: tile = rp->nameToIndex.count("dirt") ? rp->nameToIndex.at("dirt") : 1; break;
                case BlockType::STONE: tile = rp->nameToIndex.count("stone") ? rp->nameToIndex.at("stone") : 2; break;
                default: tile=0; break;
            }
        } else {
            // fallback: procedural atlas tiles 0..4
            switch(hotbar[i]){ case BlockType::GRASS: tile=0; break; case BlockType::DIRT: tile=1; break; case BlockType::STONE: tile=2; break; case BlockType::WOOD: tile=3; break; case BlockType::LEAVES: tile=4; break; default: tile=0; }
        }
        // draw slot background (not textured) - draw block as 3D cube instead of flat sprite
        int topTile = tile;
        int sideTile = tile;
        int frontTile = tile;
        if (rp){
            int ttop = rp->getTileFor(hotbar[i], ResourcePack::Face::TOP);
            int tside = rp->getTileFor(hotbar[i], ResourcePack::Face::SIDE);
            int tbottom = rp->getTileFor(hotbar[i], ResourcePack::Face::BOTTOM);
            if (ttop >= 0) topTile = ttop;
            if (tside >= 0) sideTile = tside;
            // front use side as well
            frontTile = sideTile;
        } else {
            // fallback: for grass, prefer showing a 'top' look
            if (hotbar[i] == BlockType::GRASS) {
                topTile = 0; sideTile = 0; frontTile = 1;
            } else if (hotbar[i] == BlockType::DIRT) {
                topTile = 1; sideTile = 1; frontTile = 1;
            } else if (hotbar[i] == BlockType::STONE) {
                topTile = 2; sideTile = 2; frontTile = 2;
            }
        }
        ui.drawBlock3D((float)x, (float)y, (float)slotW, (float)slotH, topTile, sideTile, frontTile, rp?rp->atlas.tiles:5, windowW, windowH);
        // highlight selected
        if (i==selected){
            // draw highlight border
            ui.drawRect((float)x-2.0f, (float)y-2.0f, (float)slotW+4.0f, (float)slotH+4.0f, 1.0f,1.0f,1.0f,0.12f, windowW, windowH);
        }
    }
}
