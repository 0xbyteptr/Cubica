#include "world.h"
#include <memory>
#include <cmath>
#include <algorithm>
#include <thread>
#include <mutex>
#include <iostream>
#include <vector>

World::World() {}

World::~World() {
    for (auto& [key, chunk] : chunks)
        delete chunk;
}

Chunk* World::getChunk(int cx, int cz) {
    std::lock_guard<std::mutex> lk(chunksMutex);
    auto it = chunks.find({cx, cz});
    if (it != chunks.end()) return it->second;
    return nullptr;
}

void World::generateChunk(int cx, int cz) {
    // quick double-check: avoid holding lock for generation
    {
        std::lock_guard<std::mutex> lk(chunksMutex);
        auto it = chunks.find({cx, cz});
        if (it != chunks.end()) return; // already exists
        // reserve spot
        chunks[{cx, cz}] = nullptr;
    }

    Chunk* c = new Chunk(cx, cz);
    c->generate(); // safe to do off-main thread

    {
        std::lock_guard<std::mutex> lk(chunksMutex);
        chunks[{cx, cz}] = c;
    }
    // mesh rebuild should occur on main thread
}

int World::getHeightAt(float wx, float wz) {
    // convert world coords to chunk and local coords
    int cx = static_cast<int>(std::floor(wx / CHUNK_SIZE));
    int cz = static_cast<int>(std::floor(wz / CHUNK_SIZE));
    int lx = static_cast<int>(std::floor(wx)) - cx * CHUNK_SIZE;
    int lz = static_cast<int>(std::floor(wz)) - cz * CHUNK_SIZE;

    // generate chunk if missing
    generateChunk(cx, cz);
    Chunk* c = getChunk(cx, cz);
    if (!c) return 0;

    // clamp local indices
    lx = std::clamp(lx, 0, CHUNK_SIZE - 1);
    lz = std::clamp(lz, 0, CHUNK_SIZE - 1);

    for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
        auto b = c->getBlock(lx, y, lz);
        if (b.type != BlockType::AIR) return y;
    }
    return -1;
}

Block World::getBlockAt(int wx, int wy, int wz) const {
    int cx = static_cast<int>(std::floor((float)wx / CHUNK_SIZE));
    int cz = static_cast<int>(std::floor((float)wz / CHUNK_SIZE));
    int lx = wx - cx * CHUNK_SIZE;
    int lz = wz - cz * CHUNK_SIZE;
    // need to const_cast to call generateChunk/getChunk? we won't generate here — assume chunk exists
    auto it = chunks.find({cx, cz});
    if (it == chunks.end() || !it->second) return Block{BlockType::AIR};
    Chunk* c = it->second;
    if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE || wy < 0 || wy >= CHUNK_HEIGHT) return Block{BlockType::AIR};
    return c->getBlock(lx, wy, lz);
}

void World::setBlockAt(int wx, int wy, int wz, Block block) {
    int cx = static_cast<int>(std::floor((float)wx / CHUNK_SIZE));
    int cz = static_cast<int>(std::floor((float)wz / CHUNK_SIZE));
    int lx = wx - cx * CHUNK_SIZE;
    int lz = wz - cz * CHUNK_SIZE;
    generateChunk(cx, cz);
    Chunk* c = getChunk(cx, cz);
    if (!c) return;
    if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE || wy < 0 || wy >= CHUNK_HEIGHT) return;
    c->setBlock(lx, wy, lz, block);
    c->needsMesh = true; // schedule mesh rebuild
}


void World::pregenerateAsync(int radius) {
    // spawn a detached thread to generate blocks in a square radius
    std::thread([this, radius]() {
        std::cout << "Pregeneration: generating radius=" << radius << "\n";
        for (int cx = -radius; cx <= radius; ++cx) {
            for (int cz = -radius; cz <= radius; ++cz) {
                this->generateChunk(cx, cz);
            }
        }
        std::cout << "Pregeneration: done\n";
    }).detach();
}

void World::processMeshQueue(int maxRebuild) {
    int rebuilt = 0;
    // find chunks needing mesh rebuild
    std::vector<Chunk*> toRebuild;
    {
        std::lock_guard<std::mutex> lk(chunksMutex);
        for (auto &p : chunks) {
            Chunk* c = p.second;
            if (c && c->needsMesh) toRebuild.push_back(c);
            if ((int)toRebuild.size() >= maxRebuild) break;
        }
    }

    for (Chunk* c : toRebuild) {
        c->rebuildMesh(resourcePack);
        ++rebuilt;
        if (rebuilt >= maxRebuild) break;
    }
}

size_t World::getChunkCount() {
    std::lock_guard<std::mutex> lk(chunksMutex);
    return chunks.size();
}

int World::getPendingMeshCount() {
    int count = 0;
    std::lock_guard<std::mutex> lk(chunksMutex);
    for (auto &p : chunks) {
        Chunk* c = p.second;
        if (c && c->needsMesh) ++count;
    }
    return count;
}
