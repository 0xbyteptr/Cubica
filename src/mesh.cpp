#include "mesh.h"
#include <vector>
#include <cstring>
#include "resourcepack.h"

// Vertex layout: pos(xyz), tex(u,v), light, color(r,g,b), typeId, worldY
// floats per vertex: 3 + 2 + 1 + 3 + 1 + 1 = 11

static void pushVertex(std::vector<float>& v, float x,float y,float z, float u, float tv, float light, float r, float g, float b, float typeId = -1.0f, float worldY = -1.0f, float overlayTile = -1.0f) {
    v.push_back(x); v.push_back(y); v.push_back(z);
    v.push_back(u); v.push_back(tv);
    v.push_back(light);
    v.push_back(r); v.push_back(g); v.push_back(b);
    v.push_back(typeId);
    v.push_back(worldY);
    v.push_back(overlayTile);
}

Mesh::Mesh() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
}
Mesh::~Mesh() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void Mesh::upload(const std::vector<float>& data) {
    // now each vertex has 12 floats (pos(3),tex(2),light(1),color(3),typeId(1),worldY(1),overlay(1))
    const int strideFloats = 12;
    vertexCount = data.size() / strideFloats;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    // layout
    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideFloats * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(1); // tex
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, strideFloats * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); // light
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, strideFloats * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(3); // color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, strideFloats * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(4); // typeId
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, strideFloats * sizeof(float), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(5); // worldY
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, strideFloats * sizeof(float), (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(6); // overlayTile
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, strideFloats * sizeof(float), (void*)(11 * sizeof(float)));

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (vertexCount == 0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
    glBindVertexArray(0);
}

void Mesh::buildFromChunk(const Chunk* c, int cx, int cz, const ResourcePack* rp) {
    std::vector<float> verts;
    float ox = cx * CHUNK_SIZE;
    float oz = cz * CHUNK_SIZE;

    auto isAir = [&](int lx, int y, int lz) -> bool {
        if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT)
            return true;
        return !c->getBlock(lx, y, lz).isSolid();
    };

    // Base color for most blocks (used on sides/bottom, and top when not special)
    auto baseColorOf = [](BlockType t) -> std::array<float, 3> {
        switch (t) {
            case BlockType::DIRT:   return {0.60f, 0.39f, 0.22f};
            case BlockType::STONE:  return {0.58f, 0.58f, 0.58f};
            case BlockType::WOOD:   return {0.64f, 0.32f, 0.16f};
            case BlockType::LEAVES: return {0.40f, 0.70f, 0.30f};
            default:                return {1.0f, 1.0f, 1.0f};
        }
    };

    // Special top color (only for grass when no resource pack)
    auto topColorOf = [&](BlockType t) -> std::array<float, 3> {
        if (t == BlockType::GRASS) {
            if (!rp) return {0.55f, 0.85f, 0.35f};  // nice natural green tint
            else     return {1.0f, 1.0f, 1.0f};     // let RP texture decide
        }
        return baseColorOf(t);
    };

    auto typeIdOf = [](BlockType t) -> float {
        switch (t) {
            case BlockType::GRASS:  return 0.0f;
            case BlockType::DIRT:   return 1.0f;
            case BlockType::STONE:  return 2.0f;
            case BlockType::LEAVES: return 3.0f;
            default:                return -1.0f;
        }
    };

    int atlasTileCount = rp ? std::max(1, rp->atlas.tiles) : 5;  // increased fallback
    float tileW = 1.0f / static_cast<float>(atlasTileCount);

    auto getTileForFace = [&](BlockType bt, int face) -> int {
        if (!rp) {
            // Fallback without RP: top different for grass, sides dirt-like
            if (bt == BlockType::GRASS) return (face == 5) ? 0 : 1;
            if (bt == BlockType::DIRT)   return 1;
            if (bt == BlockType::STONE)  return 2;
            if (bt == BlockType::WOOD)   return 3;
            if (bt == BlockType::LEAVES) return 4;
            return 0;
        }

        ResourcePack::Face f = ResourcePack::SIDE;
        if (face == 4) f = ResourcePack::BOTTOM;
        if (face == 5) f = ResourcePack::TOP;

        int idx = rp->getTileFor(bt, f);
        return (idx >= 0) ? idx : 0;
    };

    for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
        for (int lz = 0; lz < CHUNK_SIZE; ++lz) {
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                Block b = c->getBlock(lx, y, lz);
                if (!b.isSolid()) continue;

                BlockType bt = b.type;
                float tid = typeIdOf(bt);

                // Grass overlay only on sides
                float overlayIdx = -1.0f;
                if (bt == BlockType::GRASS && rp) {
                    int oi = rp->getOverlayFor(bt);
                    if (oi >= 0) {
                        overlayIdx = static_cast<float>(oi);
                    }
                }

                float x0 = ox + lx;     float x1 = x0 + 1.0f;
                float z0 = oz + lz;     float z1 = z0 + 1.0f;
                float y0 = static_cast<float>(y);
                float y1 = y0 + 1.0f;

                // Bright placeholder – replace with proper AO later
                float lightVal = 0.95f;

                auto pushQuad = [&](float px0, float py0, float pz0,
                                    float px1, float py1, float pz1,
                                    float u0, float u1, float v0, float v1,
                                    const std::array<float,3>& col,
                                    float worldYForShader, float ovr) {
                    pushVertex(verts, px0, py0, pz0, u0, v0, lightVal, col[0],col[1],col[2], tid, worldYForShader, ovr);
                    pushVertex(verts, px1, py0, pz0, u1, v0, lightVal, col[0],col[1],col[2], tid, worldYForShader, ovr);
                    pushVertex(verts, px1, py1, pz1, u1, v1, lightVal, col[0],col[1],col[2], tid, worldYForShader, ovr);
                    pushVertex(verts, px0, py0, pz0, u0, v0, lightVal, col[0],col[1],col[2], tid, worldYForShader, ovr);
                    pushVertex(verts, px1, py1, pz1, u1, v1, lightVal, col[0],col[1],col[2], tid, worldYForShader, ovr);
                    pushVertex(verts, px0, py1, pz1, u0, v1, lightVal, col[0],col[1],col[2], tid, worldYForShader, ovr);
                };

                // ──────────────────────────────────────────────
                // Side faces: use neutral color for grass (so overlay can shine)
                std::array<float,3> sideCol = (bt == BlockType::GRASS) ? std::array<float,3>{1.0f,1.0f,1.0f} : baseColorOf(bt);

                // -X
                if (isAir(lx-1, y, lz)) {
                    int tile = getTileForFace(bt, 0);
                    float ti = tile * tileW;
                    pushQuad(x0,y0,z0, x0,y1,z1, ti, ti + tileW, 0.0f, 1.0f, sideCol, y0, overlayIdx);
                }
                // +X
                if (isAir(lx+1, y, lz)) {
                    int tile = getTileForFace(bt, 1);
                    float ti = tile * tileW;
                    pushQuad(x1,y0,z1, x1,y1,z0, ti, ti + tileW, 0.0f, 1.0f, sideCol, y0, overlayIdx);
                }
                // -Z
                if (isAir(lx, y, lz-1)) {
                    int tile = getTileForFace(bt, 2);
                    float ti = tile * tileW;
                    pushQuad(x1,y0,z0, x0,y1,z0, ti, ti + tileW, 0.0f, 1.0f, sideCol, y0, overlayIdx);
                }
                // +Z
                if (isAir(lx, y, lz+1)) {
                    int tile = getTileForFace(bt, 3);
                    float ti = tile * tileW;
                    pushQuad(x0,y0,z1, x1,y1,z1, ti, ti + tileW, 0.0f, 1.0f, sideCol, y0, overlayIdx);
                }

                // Bottom
                if (isAir(lx, y-1, lz)) {
                    int tile = getTileForFace(bt, 4);
                    float ti = tile * tileW;
                    pushQuad(x0,y0,z0, x1,y0,z1, ti, ti + tileW, 0.0f, 1.0f, baseColorOf(bt), y0, -1.0f);
                }

                // Top – special color for grass when no RP
                if (isAir(lx, y+1, lz)) {
                    int tile = getTileForFace(bt, 5);
                    float ti = tile * tileW;
                    auto topCol = topColorOf(bt);
                    pushQuad(x0,y1,z1, x1,y1,z0, ti, ti + tileW, 0.0f, 1.0f, topCol, y1, -1.0f);
                }
            }
        }
    }

    upload(verts);
}