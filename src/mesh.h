#pragma once
#include <vector>
#include <glad/glad.h>
#include "chunk.h"

class Mesh {
public:
    GLuint vao = 0, vbo = 0;
    size_t vertexCount = 0;
    Mesh();
    ~Mesh();
    void upload(const std::vector<float>& data);
    void draw() const;
    void buildFromChunk(const Chunk* c, int cx, int cz, const class ResourcePack* rp = nullptr);
};
