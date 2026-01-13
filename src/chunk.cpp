#include "chunk.h"
#include <cmath>
#include "noise.h"
#include "mesh.h"

Chunk::Chunk(int cx, int cz) : x(cx), z(cz) {
    for (int i = 0; i < CHUNK_SIZE; i++)
        for (int j = 0; j < CHUNK_HEIGHT; j++)
            for (int k = 0; k < CHUNK_SIZE; k++)
                blocks[i][j][k] = {BlockType::AIR};
}

void Chunk::generate() {
    const float scale = 0.01f; // controls feature size
    const int baseHeight = 60;
    const int amplitude = 24;

    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            float wx = static_cast<float>(x * CHUNK_SIZE + lx);
            float wz = static_cast<float>(z * CHUNK_SIZE + lz);
            float n = Noise::fbm2d(wx * scale, wz * scale, 5, 2.0f, 0.5f); // in approx [-1,1]
            int height = baseHeight + static_cast<int>(n * amplitude);
            if (height < 0) height = 0;
            if (height >= CHUNK_HEIGHT) height = CHUNK_HEIGHT - 1;

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                if (y > height)
                    blocks[lx][y][lz] = {BlockType::AIR};
                else if (y == height)
                    blocks[lx][y][lz] = {BlockType::GRASS};
                else if (y > height - 3)
                    blocks[lx][y][lz] = {BlockType::DIRT};
                else
                    blocks[lx][y][lz] = {BlockType::STONE};
            }

            // improved tree placement: more likely and spherical canopy
            float treeNoise = Noise::perlin2d(wx * 0.05f, wz * 0.05f);
            // reduced density but more forgiving threshold
            const float treeThreshold = 0.68f;
            if (blocks[lx][height][lz].type == BlockType::GRASS && treeNoise > treeThreshold && ((lx + lz) % 6 == 0) && height + 6 < CHUNK_HEIGHT) {
                int trunkH = 4 + std::max(0, static_cast<int>(std::floor((treeNoise - treeThreshold) * 6.0f)));
                // clamp trunk height
                trunkH = std::min(trunkH, 6);
                // trunk
                for (int ty = height+1; ty <= height + trunkH; ++ty) {
                    blocks[lx][ty][lz] = {BlockType::WOOD};
                }
                // spherical-ish leaves canopy
                int top = height + trunkH;
                int radius = 2;
                for (int dx = -radius; dx <= radius; ++dx) for (int dz = -radius; dz <= radius; ++dz) for (int dy = -1; dy <= 2; ++dy) {
                    float dd = dx*dx + dz*dz + (dy*1.5f)*(dy*1.5f);
                    if (dd > (radius+0.5f)*(radius+0.5f)) continue;
                    int ax = lx + dx; int ay = top + dy; int az = lz + dz;
                    if (ax < 0 || ax >= CHUNK_SIZE || az < 0 || az >= CHUNK_SIZE || ay < 0 || ay >= CHUNK_HEIGHT) continue;
                    // don't overwrite trunk
                    if (dx == 0 && dz == 0 && dy <= 2 && dy >= 0) continue;
                    if (blocks[ax][ay][az].type == BlockType::AIR) blocks[ax][ay][az] = {BlockType::LEAVES};
                }
            }
        }
    }
    // mark mesh to be built on the main (GL) thread
    needsMesh = true;
}

void Chunk::rebuildMesh(const ResourcePack* rp) {
    if (mesh) delete mesh;
    mesh = new Mesh();
    mesh->buildFromChunk(this, x, z, rp);
    needsMesh = false;
}

Block Chunk::getBlock(int lx, int y, int lz) const {
    return blocks[lx][y][lz];
}

void Chunk::setBlock(int lx, int y, int lz, Block block) {
    blocks[lx][y][lz] = block;
}

Chunk::~Chunk() {
    if (mesh) delete mesh;
}
