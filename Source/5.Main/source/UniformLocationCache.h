#pragma once
#include <unordered_map>
#include <string>
#include "VulkanGLStub.h"

// Cache location cua uniform theo shaderID + ten
class UniformLocationCache {
public:
    // Lay location, neu chua co thi goi glGetUniformLocation va cache lai
    GLint GetLocation(GLuint shaderID, const std::string& name) {
        auto key = MakeKey(shaderID, name);
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it->second;
        }
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        cache[key] = loc;
        return loc;
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
