#version 450
precision mediump float;
precision mediump int;

layout(location = 0) in highp vec2 vUV;
layout(location = 1) in vec4 vColor;
layout(location = 2) in float vCustomTint;

layout(location = 0) out vec4 FragColor;

// Fragment samplers (set=2)
layout(set = 2, binding = 0) uniform sampler2D uTex;

// Fragment push uniforms (set=3)
layout(std140, set = 3, binding = 0) uniform FragUniforms {
    int uUseTexture;
    float uAlphaTestThreshold;
    float _pad_a; float _pad_b;
};

void main() {
    vec4 result;
    if (uUseTexture != 0) {
        vec4 texColor = texture(uTex, vUV);
        if (vCustomTint > 0.5) {
            // Custom color tint: multiply texture color with vertex color directly
            result = vec4(texColor.rgb * vColor.rgb, texColor.a * vColor.a);
        } else {
            result = texColor * vColor;
        }
    } else {
        result = vColor;
    }
    if (result.a < uAlphaTestThreshold) {
        discard;
    }
    FragColor = result;
}
