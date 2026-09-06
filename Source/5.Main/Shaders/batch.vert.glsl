#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in uint aModelIdx;

// Per-frame model matrix table; one matrix per ResolveCurrentModelIdx() result.
layout(std430, set = 0, binding = 0) readonly buffer ModelMatrices {
    mat4 uModels[];
};

// Push uniforms (set=1)
layout(std140, set = 1, binding = 0) uniform VertUniforms {
    mat4 uProjMatrix;
    mat4 uViewMatrix;
    uint uModelBase;  // ring offset: aModelIdx is relative to this flush
};

layout(location = 0) out highp vec2 vUV;
layout(location = 1) out vec4 vColor;
layout(location = 2) out float vCustomTint;

void main() {
    gl_Position = uProjMatrix * uViewMatrix * uModels[uModelBase + aModelIdx] * vec4(aPos, 1.0);
    vUV = aUV;
    vColor = aColor;
    vCustomTint = aNormal.z > 1.5 ? 1.0 : 0.0; // normal.z == 2.0 signals custom tint
}
