#pragma once
#include "chunk.h"
#include <unordered_map>
#include <utility>
#include <cstdint>
#include <mutex>

class World {
public:
    struct PairHash {
        std::size_t operator()(const std::pair<int,int>& p) const noexcept {
            uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(p.first)) << 32) |
                           static_cast<uint32_t>(p.second);
            return std::hash<uint64_t>{}(key);
        }
    };
    std::unordered_map<std::pair<int,int>, Chunk*, PairHash> chunks;

    // optional resource pack pointer
    class ResourcePack* resourcePack = nullptr;

    World();
    ~World();

    Chunk* getChunk(int cx, int cz);
    void generateChunk(int cx, int cz);
    void setBlockAt(int wx, int wy, int wz, Block block);
    Block getBlockAt(int wx, int wy, int wz) const;

    // returns the surface y coordinate (top solid block) at world x,z coordinates
    int getHeightAt(float wx, float wz);

    void setResourcePack(class ResourcePack* rp) { resourcePack = rp; }

    // pregenerate chunk block data in a background thread (no GL calls)
    void pregenerateAsync(int radius);

    // called on main thread to process queued mesh rebuilds (build up to maxRebuild meshes)
    void processMeshQueue(int maxRebuild = 1);

    // utilities for debugging
    size_t getChunkCount();
    int getPendingMeshCount();

private:
    std::mutex chunksMutex;
};