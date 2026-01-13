#pragma once
#include "block.h"
#include <array>

constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_HEIGHT = 128;

class Chunk {
public:
    int x, z;
    std::array<std::array<std::array<Block, CHUNK_SIZE>, CHUNK_HEIGHT>, CHUNK_SIZE> blocks;

    // GPU mesh
    class Mesh* mesh = nullptr;

    // set when block data exists but mesh needs rebuilding on main thread
    bool needsMesh = false;

    Chunk(int cx, int cz);

    void generate(); // fill blocks (can be called from background thread)
    void rebuildMesh(const class ResourcePack* rp = nullptr); // must be called from GL thread
    Block getBlock(int lx, int y, int lz) const;
    void setBlock(int lx, int y, int lz, Block block);
    ~Chunk();
};
