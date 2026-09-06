#version 450

layout(location = 0) in vec2 aPos01; // (0,0),(0,1),(1,1),(1,0)

// Storage buffer (set=0)
struct Instance {
    vec4 xywh;   // x, y, width, height  (pixel-space, y-down)
    vec4 uvwh;   // u, v, uWidth, vHeight (normalized 0..1)
    vec4 color;  // r, g, b, a
    vec4 extra;  // x=rotation (radians), y=untextured, z=alphaOnly (R8 sampled as alpha)
};

layout(set = 0, binding = 0) readonly buffer Instances {
    Instance inst[];
};

// Push uniforms (set=1)
layout(std140, set = 1, binding = 0) uniform VertUniforms {
    vec2 uScreenSize;
    uint uInstanceBase;  // per-batch base index into inst[]
    float _pad;
};

layout(location = 0) out highp vec2 vs_uv;
layout(location = 1) out vec4 vs_color;
layout(location = 2) flat out float vs_untextured;
layout(location = 3) flat out float vs_alphaOnly;
layout(location = 4) flat out float vs_grayscale;

void main() {
    Instance I = inst[uInstanceBase + gl_InstanceIndex];

    // Rotate quad corners around center in pixel space
    float rot = I.extra.x;
    vec2 center = I.xywh.xy + I.xywh.zw * 0.5;
    vec2 offset = (aPos01 - vec2(0.5)) * I.xywh.zw;
    if (abs(rot) > 0.0001) {
        float c = cos(rot), s = sin(rot);
        offset = vec2(offset.x * c - offset.y * s, offset.x * s + offset.y * c);
    }
    vec2 pos = center + offset;

    // pixel -> NDC
    vec2 inv = 1.0 / uScreenSize;
    vec2 ndc;
    ndc.x = pos.x * 2.0 * inv.x - 1.0;
    ndc.y = pos.y * 2.0 * inv.y - 1.0;

    gl_Position = vec4(ndc, 0.0, 1.0);

    vs_uv = I.uvwh.xy + aPos01 * I.uvwh.zw;
    vs_color = I.color;
    vs_untextured = I.extra.y;
    vs_alphaOnly = I.extra.z;
    vs_grayscale = I.extra.w;
}
