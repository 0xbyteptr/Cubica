#include "shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <string>

static std::string getExeDir() {
    char buf[1024];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (n <= 0) return std::string();
    buf[n] = '\0';
    std::string p(buf);
    auto pos = p.find_last_of('/');
    if (pos == std::string::npos) return std::string();
    return p.substr(0, pos);
}

static std::string tryReadFileOnce(const std::string& path) {
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f) return std::string();
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

static std::string readFile(const char* path) {
    const std::string orig(path);
    std::vector<std::string> candidates;
    candidates.push_back(orig);
    candidates.push_back(std::string("../") + orig);
    std::string exeDir = getExeDir();
    if (!exeDir.empty()) {
        candidates.push_back(exeDir + "/" + orig);
        candidates.push_back(exeDir + "/../" + orig);
    }

    for (const auto& p : candidates) {
        std::string s = tryReadFileOnce(p);
        if (!s.empty()) {
            if (p != orig) std::cerr << "Loaded shader from fallback path: " << p << "\n";
            return s;
        } else {
            std::cerr << "Tried shader path and failed: " << p << "\n";
        }
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) std::cerr << "Failed to open shader file: " << path << " (cwd=" << cwd << ")\n";
    else std::cerr << "Failed to open shader file: " << path << "\n";
    return std::string();
}

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok=0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok){ char buf[1024]; glGetShaderInfoLog(s, 1024, nullptr, buf); std::cerr<<buf<<"\n"; }
    return s;
}

bool Shader::loadFromFiles(const char* vertPath, const char* fragPath) {
    std::string vs = readFile(vertPath);
    std::string fs = readFile(fragPath);
    if (vs.empty() || fs.empty()) {
        std::cerr << "One or more shader sources are empty (" << vertPath << ", " << fragPath << ")\n";
        return false;
    }
    GLuint v = compile(GL_VERTEX_SHADER, vs.c_str());
    GLuint f = compile(GL_FRAGMENT_SHADER, fs.c_str());
    id = glCreateProgram();
    glAttachShader(id, v); glAttachShader(id, f);
    glLinkProgram(id);
    GLint ok=0; glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if(!ok){ char buf[1024]; glGetProgramInfoLog(id, 1024, nullptr, buf); std::cerr<<buf<<"\n"; glDeleteShader(v); glDeleteShader(f); glDeleteProgram(id); id = 0; return false; }
    glDeleteShader(v); glDeleteShader(f);
    return true;
}

Shader::~Shader() {
    if (id) {
        glDeleteProgram(id);
        id = 0;
    }
}
