#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

// Push uniforms (set=1)
layout(std140, set = 1, binding = 0) uniform VertUniforms {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
    mat4 uModelMatrix;
};

layout(location = 0) out vec2 vUV;

void main() {
    vUV = aUV;
    gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * vec4(aPos, 1.0);
}
