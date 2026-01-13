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
    // world origin for this chunk
    float ox = cx * CHUNK_SIZE;
    float oz = cz * CHUNK_SIZE;

    auto isAir = [&](int lx,int y,int lz){
        if (lx<0||lx>=CHUNK_SIZE||lz<0||lz>=CHUNK_SIZE||y<0||y>=CHUNK_HEIGHT) return true;
        return !c->getBlock(lx,y,lz).isSolid();
    };

    auto colorOf = [](BlockType t){
        switch(t){
            // base color is neutral for grass so only the top face is tinted green (topCol)
            case BlockType::GRASS: return std::array<float,3>{1.0f,1.0f,1.0f};
            case BlockType::DIRT:  return std::array<float,3>{0.545f,0.271f,0.075f};
            case BlockType::STONE: return std::array<float,3>{0.5f,0.5f,0.5f};
            case BlockType::WOOD:  return std::array<float,3>{0.64f,0.32f,0.16f};
            case BlockType::LEAVES:return std::array<float,3>{0.3f,0.8f,0.3f};
            default: return std::array<float,3>{1.0f,0.0f,1.0f};
        }
    };

    auto tileIndexOfDefault = [](BlockType t){
        switch(t){
            case BlockType::GRASS: return 0;
            case BlockType::DIRT: return 1;
            case BlockType::STONE: return 2;
            case BlockType::WOOD: return 3;
            case BlockType::LEAVES: return 4;
            default: return 0;
        }
    };

    auto typeIdOf = [](BlockType t)->float{
        switch(t){
            case BlockType::GRASS: return 0.0f;
            case BlockType::DIRT: return 1.0f;
            case BlockType::STONE: return 2.0f;
            case BlockType::LEAVES: return 3.0f;
            default: return -1.0f;
        }
    };

    int tileCount = 3;
    float tileW = 1.0f / static_cast<float>(tileCount);

    if (rp) {
        tileCount = std::max(1, rp->atlas.tiles);
        tileW = 1.0f / static_cast<float>(tileCount);
    }

    auto getTileForFace = [&](BlockType bt, int face)->int{
      if (!rp) return tileIndexOfDefault(bt);

      ResourcePack::Face f = ResourcePack::SIDE;
      switch(face){
          case 0: f = ResourcePack::SIDE; break; // -X
          case 1: f = ResourcePack::SIDE; break; // +X
          case 2: f = ResourcePack::SIDE; break; // -Z
          case 3: f = ResourcePack::SIDE; break; // +Z
          case 4: f = ResourcePack::BOTTOM; break; // -Y
          case 5: f = ResourcePack::TOP; break; // +Y
      }

      int idx = rp->getTileFor(bt, f);
      if (idx < 0) return tileIndexOfDefault(bt);
      return idx;
    };


    for (int lx=0; lx<CHUNK_SIZE; ++lx) for (int lz=0; lz<CHUNK_SIZE; ++lz) for (int y=0; y<CHUNK_HEIGHT; ++y) {
        Block b = c->getBlock(lx,y,lz);
        if (!b.isSolid()) continue;
        auto col = colorOf(b.type);
        float tid = typeIdOf(b.type);
        // determine top-face color (greenish) when no resource pack present
        auto topCol = col;
        if (b.type == BlockType::GRASS) {
            if (!rp) {
                topCol = std::array<float,3>{0.2f, 0.9f, 0.2f};
            } else {
                topCol = col; // rely on texture from resource pack
            }
        }
        // compute overlay index for grass sides (if any)
        float overlayIdx = -1.0f;
        if (rp && b.type == BlockType::GRASS) {
            int oi = rp->getOverlayFor(b.type);
            if (oi >= 0) { overlayIdx = (float)oi; printf("Mesh: grass at %d,%d,%d overlay tile %d\n", lx,lz,y,oi); }
        }

        // each face if exposed add two triangles
        float x0 = ox + lx; float x1 = x0+1.0f;
        float z0 = oz + lz; float z1 = z0+1.0f;
        float y0 = y; float y1 = y+1.0f;
        float l = 1.0f; // light placeholder
        // -X
        if (isAir(lx-1,y,lz)) {
            int tile = getTileForFace(b.type, 0);
            float ti = tile * tileW;
            float u0 = ti, u1 = ti + tileW;
            pushVertex(verts, x0,y0,z0, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y0,z1, u1,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y1,z1, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y0,z0, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y1,z1, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y1,z0, u0,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
        }
        // +X
        if (isAir(lx+1,y,lz)) {
            int tile = getTileForFace(b.type, 1);
            float ti = tile * tileW;
            float u0 = ti, u1 = ti + tileW;
            pushVertex(verts, x1,y0,z1, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y0,z0, u1,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y1,z0, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y0,z1, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y1,z0, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y1,z1, u0,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
        }
        // -Z
        if (isAir(lx,y,lz-1)) {
            int tile = getTileForFace(b.type, 2);
            float ti = tile * tileW;
            float u0 = ti, u1 = ti + tileW;

            pushVertex(verts, x1,y0,z0, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y0,z0, u1,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y1,z0, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y0,z0, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y1,z0, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y1,z0, u0,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
        }
        // +Z
        if (isAir(lx,y,lz+1)) {
            int tile = getTileForFace(b.type, 3);
            float ti = tile * tileW;
            float u0 = ti, u1 = ti + tileW;
            pushVertex(verts, x0,y0,z1, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y0,z1, u1,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y1,z1, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y0,z1, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y1,z1, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y1,z1, u0,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
        }
        // -Y (bottom)
        if (isAir(lx,y-1,lz)) {
            int tile = getTileForFace(b.type, 0);
            float ti = tile * tileW;
            float u0 = ti, u1 = ti + tileW;
            pushVertex(verts, x0,y0,z0, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y0,z0, u1,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y0,z1, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y0,z0, u0,0, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x1,y0,z1, u1,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
            pushVertex(verts, x0,y0,z1, u0,1, l, col[0],col[1],col[2], tid, -1.0f, overlayIdx);
        }
        // +Y (top)
        if (isAir(lx,y+1,lz)) {
            int tile = getTileForFace(b.type, 5);
            float ti = tile * tileW;
            float u0 = ti, u1 = ti + tileW;
            // use topCol for grass tops when appropriate
            pushVertex(verts, x0,y1,z1, u0,0, l, topCol[0],topCol[1],topCol[2], tid, y1, overlayIdx);
            pushVertex(verts, x1,y1,z1, u1,0, l, topCol[0],topCol[1],topCol[2], tid, y1, overlayIdx);
            pushVertex(verts, x1,y1,z0, u1,1, l, topCol[0],topCol[1],topCol[2], tid, y1, overlayIdx);
            pushVertex(verts, x0,y1,z1, u0,0, l, topCol[0],topCol[1],topCol[2], tid, y1, overlayIdx);
            pushVertex(verts, x1,y1,z0, u1,1, l, topCol[0],topCol[1],topCol[2], tid, y1, overlayIdx);
            pushVertex(verts, x0,y1,z0, u0,1, l, topCol[0],topCol[1],topCol[2], tid, y1, overlayIdx);
        }
    }

    upload(verts);
}
