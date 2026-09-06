#version 450

// Vertex inputs (must match VertexGPU: 36-byte stride)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in uint aBoneIndicesPacked;  // low 16 bits = vertexBoneIndex, high 16 = normalBoneIndex

// Storage buffers (set=0)
// Bit layout for InstanceData.flags (must match MeshInstanceFlags in CShaderGL.h):
//   bit 0=wave  1=texture  2=lighting  3=color  4=chrome  5=highlight  6=battleCastle  7=translate
struct InstanceData {
    vec4 modelMatrix;
    vec4 lightColor;
    int renderMode;
    int finalRenderMode;
    int sourceIndex;
    int chromeMode;
    vec2 blendUV;
    float newScale;
    float alpha;
    float bodyScale;
    float boneScale;
    int boneOffset;
    float customIntensity;
    vec3 bodyLightColor;
    uint flags;
};

layout(std430, set = 0, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(std430, set = 0, binding = 1) readonly buffer BoneMatrices {
    vec4 BoneData[];  // 3 vec4 rows per bone: row-major 3x4 affine transform
};

// Row-major affine transform: component i = dot(row_i, vec4(p, 1)).
vec3 BoneTransformPos(uint index, vec3 p) {
    uint base = index * 3u;
    vec4 h = vec4(p, 1.0);
    return vec3(dot(BoneData[base + 0u], h),
                dot(BoneData[base + 1u], h),
                dot(BoneData[base + 2u], h));
}

// Push uniforms (set=1)
layout(std140, set = 1, binding = 0) uniform VertUniforms {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
    uint uInstanceBase;
    float _pad0a; float _pad0b; float _pad0c;
};

// Outputs
layout(location = 0) out float uShadowAlpha;

void main()
{
    InstanceData inst = instances[uInstanceBase + gl_InstanceIndex];

    uint vertBoneIndex = aBoneIndicesPacked & 0xFFFFu;
    vertBoneIndex = (vertBoneIndex < 200u) ? vertBoneIndex : 0u;

    vec3 uBodyOrigin = inst.modelMatrix.xyz;

    vec3 scaledPos = aPos;
    if (inst.newScale > 0.0)
        scaledPos *= inst.newScale;
    else if (inst.boneScale != 1.0)
        scaledPos *= inst.boneScale;

    vec3 posWS = BoneTransformPos(uint(inst.boneOffset) + vertBoneIndex, scaledPos);

    if ((inst.flags & 0x80u) != 0u)
        posWS = posWS * inst.bodyScale + uBodyOrigin;

    float uShadowSx = ((inst.flags & 0x40u) != 0u) ? 2500.0 : 2000.0;
    float uShadowSy = 4000.0;

    vec3 local = posWS - uBodyOrigin;
    float denom = local.z - uShadowSy;
    if (abs(denom) > 0.001)
        local.x += local.z * (local.x + uShadowSx) / denom;
    local.z = 5.0;
    vec3 shadowWorldPos = local + uBodyOrigin;

    gl_Position = uProjMatrix * uViewMatrix * vec4(shadowWorldPos, 1.0);
    uShadowAlpha = inst.alpha;
}
