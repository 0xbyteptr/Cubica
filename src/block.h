#pragma once
#include <cstdint>

enum class BlockType : uint8_t {
    AIR = 0,
    GRASS,
    DIRT,
    STONE,
    WOOD,
    LEAVES,
};

struct Block {
    BlockType type = BlockType::AIR;

    bool isSolid() const {
        return type != BlockType::AIR;
    }
};