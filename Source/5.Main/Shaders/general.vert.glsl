#version 450

// Vertex inputs
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
vec3 BoneTransformPos(uint base, vec3 p) {
    vec4 h = vec4(p, 1.0);
    return vec3(dot(BoneData[base + 0u], h),
                dot(BoneData[base + 1u], h),
                dot(BoneData[base + 2u], h));
}

// Direction transform ignores translation (the .w column).
vec3 BoneTransformDir(uint base, vec3 d) {
    return vec3(dot(BoneData[base + 0u].xyz, d),
                dot(BoneData[base + 1u].xyz, d),
                dot(BoneData[base + 2u].xyz, d));
}

// Push uniforms (set=1)
layout(std140, set = 1, binding = 0) uniform VertUniforms {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
    float uWorldTime;
    float _pad0a; float _pad0b; float _pad0c;  // individual floats: 4 bytes each (not array — std140 arrays pad to vec4!)
    vec3 uShadowAngle;
    float _pad1;
    vec4 uLightPosition;
    uint uInstanceBase;
    uint uVertexBase;
    float _pad3b; float _pad3c;
};

// Outputs
layout(location = 0) out vec2 FinalUV;
layout(location = 1) out float Alpha;
layout(location = 2) out float LightFactor;
layout(location = 3) out vec3 BodyLightColor;
layout(location = 4) out float vIntensity;
layout(location = 5) out float enableTexture;
layout(location = 6) flat out int textureIndex;
layout(location = 7) flat out float vCustomColorTint; // 1.0 when custom color tint active

// Helper functions
mat3 AngleMatrix(vec3 angle) {
    float yaw   = radians(angle.y);
    float pitch = radians(angle.x);
    float roll  = radians(angle.z);

    float sx = sin(pitch), cx = cos(pitch);
    float sy = sin(yaw),   cy = cos(yaw);
    float sz = sin(roll),  cz = cos(roll);

    mat3 m;
    m[0][0] = cy * cz;
    m[0][1] = sx * sy * cz + cx * -sz;
    m[0][2] = cx * sy * cz + sx * sz;

    m[1][0] = cy * sz;
    m[1][1] = sx * sy * sz + cx * cz;
    m[1][2] = cx * sy * sz - sx * cz;

    m[2][0] = -sy;
    m[2][1] = sx * cy;
    m[2][2] = cx * cy;

    return m;
}

vec3 ComputeLightDir(vec3 shadowAngle, bool highlight, bool battleCastle) {
    vec3 rawPos = highlight
                ? vec3(1.3, 0.0, 2.0)
                : (battleCastle ? vec3(0.5, -1.0, 1.0) : vec3(0.0, -1.5, 0.0));
    return normalize(transpose(AngleMatrix(shadowAngle)) * rawPos);
}

void main() {
    InstanceData inst = instances[uInstanceBase + gl_InstanceIndex];

    textureIndex = int(inst.sourceIndex);
    bool bEnableTexture    = (inst.flags & 0x02u) != 0u;
    enableTexture          = bEnableTexture ? 1.0 : 0.0;

    bool uTranslate        = (inst.flags & 0x80u) != 0u;
    bool uEnableWave       = (inst.flags & 0x01u) != 0u;
    bool uEnableChrome     = (inst.flags & 0x10u) != 0u;
    bool uEnableLighting   = (inst.flags & 0x04u) != 0u;
    bool uEnableColor      = (inst.flags & 0x08u) != 0u;
    bool uHighLight        = (inst.flags & 0x20u) != 0u;
    bool uInBattleCastle   = (inst.flags & 0x40u) != 0u;
    bool uCustomColorTint  = (inst.flags & 0x100u) != 0u;
    uint uRenderMode       = uint(inst.renderMode);
    uint uFinalRenderMode  = uint(inst.finalRenderMode);
    uint uChromeMode       = uint(inst.chromeMode);

    uint boneOffset        = uint(max(0, inst.boneOffset));
    uint vertBoneIndex     = aBoneIndicesPacked & 0xFFFFu;
    uint normBoneIndex     = (aBoneIndicesPacked >> 16u) & 0xFFFFu;
    if (normBoneIndex == 0u)
        normBoneIndex = vertBoneIndex;

    // Scale
    vec3 scaledPos = aPos;
    if (inst.newScale > 0.0)
        scaledPos *= inst.newScale;
    else if (inst.boneScale > 0.0 && inst.boneScale != 1.0)
        scaledPos *= inst.boneScale;

    vec3 posWS    = BoneTransformPos(boneOffset + vertBoneIndex, scaledPos);
    vec3 normalWS = normalize(BoneTransformDir(boneOffset + normBoneIndex, aNormal));

    vec3 origin = inst.modelMatrix.xyz;
    if (uTranslate)
        posWS = posWS * inst.bodyScale + origin;

    // renderMode 2: wave vertex deformation (cloth, water)
    if (uRenderMode == 2u) {
        // gl_VertexIndex includes the draw's vertexOffset; subtract it so the seed
        // stays mesh-relative and the sine argument stays small.
        float vi = float(gl_VertexIndex - int(uVertexBase));
        float timeSin = sin((uWorldTime + vi * 931.0) * 0.007) * 28.0;
        posWS += normalWS * timeSin;
    }

    // renderMode 1: shadow projection (flatten onto ground)
    if (uRenderMode == 1u) {
        float shadowSx = uInBattleCastle ? 2500.0 : 2000.0;
        vec3 local = posWS - origin;
        float denom = local.z - 4000.0;
        if (abs(denom) > 0.001)
            local.x += local.z * (local.x + shadowSx) / denom;
        local.z = 5.0;
        posWS = local + origin;
    }

    gl_Position = uProjMatrix * uViewMatrix * vec4(posWS, 1.0);

    // Lighting
    if (uEnableLighting) {
        vec3 lightDir = (length(uLightPosition.xyz) > 0.001) ? normalize(uLightPosition.xyz) : vec3(0.0, -1.0, 0.0);
        float lightWeight = (uLightPosition.w > 0.0) ? uLightPosition.w : 1.0;
        float intensity = max(((dot(normalWS, lightDir) * 0.25 + 0.85) * lightWeight + (1.0 - lightWeight)), 0.75);
        if (uHighLight) intensity = intensity * 1.15 + 0.1;
        else if (uInBattleCastle) intensity = intensity * 0.85;
        LightFactor = clamp(intensity, 0.75, 1.0);
    } else {
        LightFactor = 1.0;
    }

    vec3 baseLightCol = clamp(uEnableColor ? inst.bodyLightColor : inst.lightColor.rgb, 0.0, 1.0);
    BodyLightColor = (uEnableLighting && !uEnableChrome) ? clamp(baseLightCol * LightFactor, 0.0, 1.0) : baseLightCol;
    Alpha = uEnableColor ? inst.alpha : inst.lightColor.a;

    vCustomColorTint = uCustomColorTint ? 1.0 : 0.0;

    // Chrome UV calculation
    vec2 texUV = aTexCoord;
    vec2 chromeUV = vec2(0.0);

    if (uEnableChrome) {
        float wave  = mod(uWorldTime, 10000.0) * 0.0001;
        float wave2 = mod(uWorldTime, 4000.0)  * 0.00024 - 0.4;

        vec3 L = normalize(vec3(
            cos(uWorldTime * 0.001),
            sin(uWorldTime * 0.002),
            1.0
        ));

        switch (uChromeMode) {
            case 0u:
                chromeUV = vec2(
                    normalWS.z * 0.5 + wave,
                    normalWS.y * 0.5 + wave * 2.0
                );
                break;

            case 1u:
                chromeUV = vec2(
                    normalWS.z * 0.5 + 0.2,
                    normalWS.y * 0.5 + 0.5
                );
                break;

            case 2u:
                chromeUV = vec2(
                    (normalWS.z + normalWS.x) * 0.8 + wave2 * 2.0,
                    (normalWS.y + normalWS.x) * 1.0 + wave2 * 3.0
                );
                break;

            case 3u: {
                float d = dot(normalWS, uLightPosition.xyz);
                chromeUV = vec2(d, 1.0 - d);
                break;
            }

            case 4u: {
                float d = dot(normalWS, L);
                chromeUV.x = d + normalWS.y * 0.5 + L.y * 3.0;
                chromeUV.y = 1.0 - d - normalWS.z * 0.5 - wave * 3.0;
                break;
            }

            case 5u: {
                float d = dot(normalWS, L);
                chromeUV.x = d + normalWS.y * 3.0 + L.y * 5.0;
                chromeUV.y = 1.0 - d - normalWS.z * 2.5 - wave;
                break;
            }

            case 6u: {
                float v = (normalWS.z + normalWS.x) * 0.8 + wave2 * 2.0;
                chromeUV = vec2(v, v);
                break;
            }

            case 7u: {
                float v = (normalWS.z + normalWS.x) * 0.8 + uWorldTime * 0.00006;
                chromeUV = vec2(v, v);
                break;
            }

            case 8u:
                chromeUV = vec2(normalWS.x, normalWS.y);
                break;

            default:
                chromeUV = vec2(
                    normalWS.z * 0.5 + 0.2,
                    normalWS.y * 0.5 + 0.5
                );
                break;
        }
    }

    // Final UV — select between texture, chrome, or blend
    switch (uFinalRenderMode) {
        case 2u:
            FinalUV = chromeUV;
            break;

        case 3u:
            FinalUV = chromeUV + inst.blendUV;
            break;

        case 4u:
            FinalUV = chromeUV * texUV + inst.blendUV;
            break;

        default:
            FinalUV = texUV;
            if (uEnableWave)
                FinalUV += inst.blendUV;
            break;
    }

    vIntensity = inst.customIntensity;
}
