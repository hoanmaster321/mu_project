#version 450
precision mediump float;
precision mediump int;

layout(location = 0) in highp vec2 vs_uv;
layout(location = 1) in vec4 vs_color;
layout(location = 2) flat in float vs_untextured;
layout(location = 3) flat in float vs_alphaOnly;
layout(location = 4) flat in float vs_grayscale;

layout(location = 0) out vec4 FragColor;

// Fragment samplers (set=2)
layout(set = 2, binding = 0) uniform sampler2D uTexture;

void main() {
    // Early-out for fully transparent quads — skips texture sample.
    if (vs_color.a < 0.01) discard;

    if (vs_untextured > 0.5) {
        FragColor = vs_color;
    } else if (vs_alphaOnly > 0.5) {
        // R8_UNORM atlas (glyphs): single channel sampled as alpha against
        // white. RGB comes from vs_color so glyphs can be tinted.
        float a = texture(uTexture, vs_uv).r;
        FragColor = vec4(vs_color.rgb, vs_color.a * a);
        if (FragColor.a < 0.01) discard;
    } else {
        vec4 texel = texture(uTexture, vs_uv);
        if (vs_grayscale > 0.5) {
            float g = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
            texel.rgb = vec3(g, g, g);
        }
        FragColor = texel * vs_color;
        if (FragColor.a < 0.01) discard;
    }
}
