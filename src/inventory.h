#pragma once
#include <array>
#include <cstdint>
#include "block.h"

class Inventory {
public:
    enum ToolType {
        TOOL_HAND = 0,
        TOOL_WOODEN_PICKAXE,
        TOOL_STONE_PICKAXE,
        TOOL_IRON_PICKAXE,
        TOOL_WOODEN_SHOVEL,
        TOOL_STONE_SHOVEL,
        TOOL_IRON_SHOVEL
    };

    std::array<BlockType,9> hotbar{};
    std::array<ToolType,9> tools{}; // tool associated with each hotbar slot
    int selected = 0;

    Inventory();
    void draw(class UIRenderer& ui, int windowW, int windowH, const class ResourcePack* rp=nullptr);

    ToolType getSelectedTool() const { return tools[selected]; }
};