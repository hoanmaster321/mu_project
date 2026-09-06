#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

// Push uniforms (set=1) — must match TerrainUBO in terrain.frag.glsl
struct PointLight {
    vec4 posRadius;
    vec4 colorIntensity;
};
layout(std140, set = 1, binding = 0) uniform TerrainUBO {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
    PointLight uLights[32];
    uint uNumLights;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColor;
layout(location = 2) out vec3 vWorldPos;

void main()
{
    vUV = aTexCoord;
    vColor = aColor;   // UBYTE4_NORM arrives pre-normalized to [0,1]
    vWorldPos = aPos;

    gl_Position = uProjMatrix * uViewMatrix * vec4(aPos, 1.0);
}
