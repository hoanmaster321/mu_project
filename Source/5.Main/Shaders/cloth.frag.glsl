#version 450
precision mediump float;

layout(location = 0) in vec2 vUV;

layout(location = 0) out vec4 FragColor;

// Fragment samplers (set=2)
layout(set = 2, binding = 0) uniform sampler2D uTex;

void main() {
    vec4 c = texture(uTex, vUV);
    // Discard near-transparent texels to skip depth write — otherwise
    // cape edges would write opaque depth and occlude shadows/meshes behind.
    if (c.a < 0.05) discard;
    FragColor = c;
}
