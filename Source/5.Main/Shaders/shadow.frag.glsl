#version 450
precision mediump float;

layout(location = 0) in float uShadowAlpha;

layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(0.0, 0.0, 0.0, uShadowAlpha);
}
