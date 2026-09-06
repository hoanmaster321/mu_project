#pragma once
#include <unordered_map>
#include <string>
#include "RenderState.h"

// Cache location cua uniform theo shaderID + ten
class UniformLocationCache {
public:
    // Lay location, neu chua co thi goi glGetUniformLocation va cache lai
    GLint GetLocation(GLuint shaderID, const std::string& name) {
        return -1;
    }

    // Xoa cache khi can (vi du khi reload shader)
    void Clear() {
        cache.clear();
    }

private:
    std::unordered_map<std::string, GLint> cache;

    std::string MakeKey(GLuint shaderID, const std::string& name) {
        return std::to_string(shaderID) + "_" + name;
    }
};
