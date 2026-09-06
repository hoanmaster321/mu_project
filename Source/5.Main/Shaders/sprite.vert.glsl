#version 450

// Per-vertex (quad corners)
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

// Storage buffer (set=0) — instance data uploaded as SSBO
struct SpriteInstance {
    vec4 position;    // xyz = world/screen pos, w = unused
    vec2 size;        // half-extents
    float rotation;   // degrees
    float _pad0;      // align to vec4
    vec4 light;       // rgb + unused alpha
    vec4 uv;          // xy = uv origin, zw = uv size
    float alpha;
    float depth;
    vec2 _pad1;       // pad struct to 80 bytes (multiple of 16)
};

layout(std430, set = 0, binding = 0) readonly buffer Instances {
    SpriteInstance inst[];
};

// Push uniforms (set=1)
layout(std140, set = 1, binding = 0) uniform VertUniforms {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
    vec2 uScreenSize;
    float uScale;
    uint uInstanceBase;
};

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec3 vLight;
layout(location = 2) out float vAlpha;
layout(location = 3) out float vCustomTint;

void main()
{
    SpriteInstance I = inst[uInstanceBase + gl_InstanceIndex];

    vec2 halfSize = I.size * uScale;
    vec2 local = aPos * halfSize;

    if (I.rotation != 0.0) {
        float r = radians(I.rotation);
        float c = cos(r), s = sin(r);
        local = vec2(local.x * c - local.y * s,
                     local.x * s + local.y * c);
    }

    vec4 worldPos;

    if (I.position.z == -1.0) {
        // === UI MODE (2D screen-space) ===
        vec2 screenPos = I.position.xy + local;

        // Convert to clip-space [-1, +1] (Vulkan Y-down: -1 is top, +1 is bottom)
        vec2 clip = (screenPos / uScreenSize) * 2.0 - 1.0;

        worldPos = vec4(clip, 0.0, 1.0);
    }
    else {
        // === WORLD MODE (3D billboard) ===
        vec3 right = vec3(uViewMatrix[0][0], uViewMatrix[1][0], uViewMatrix[2][0]);
        vec3 up    = vec3(uViewMatrix[0][1], uViewMatrix[1][1], uViewMatrix[2][1]);

        vec3 offset = local.x * right + local.y * up;
        vec4 posWS = vec4(I.position.xyz + offset, 1.0);
        worldPos = uProjMatrix * uViewMatrix * posWS;
    }

    gl_Position = worldPos;

    // Outputs
    vTexCoord = I.uv.xy + aUV * I.uv.zw;
    vLight    = clamp(I.light.rgb, 0.0, 1.0);
    vAlpha    = I.alpha;
    vCustomTint = I.light.a;
}
