#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>

class Shader {
public:
    GLuint id = 0;
    Shader() = default;
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    bool loadFromFiles(const char* vertPath, const char* fragPath);
    void use() const { if (id) glUseProgram(id); }
    void setMat4(const char* name, const float* data) const { if (id) glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, data); }
    void setInt(const char* name, int value) const { if (id) glUniform1i(glGetUniformLocation(id, name), value); }
    void setFloat(const char* name, float value) const { if (id) glUniform1f(glGetUniformLocation(id, name), value); }
};
