#version 450
precision mediump float;

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec3 vLight;
layout(location = 2) in float vAlpha;
layout(location = 3) in float vCustomTint;

layout(location = 0) out vec4 FragColor;

// Fragment samplers (set=2)
layout(set = 2, binding = 0) uniform sampler2D uTexture;

void main()
{
    vec4 texColor = texture(uTexture, vTexCoord);
    float finalAlpha = texColor.a * vAlpha;
    if (finalAlpha < 0.01)
        discard;
    vec3 finalColor;
    if (vCustomTint > 0.5)
    {
        // Custom color tint: convert texture to max-RGB luma, then multiply with vLight
        float lum = max(max(texColor.r, texColor.g), texColor.b);
        finalColor = vec3(lum) * vLight;
    }
    else
    {
        finalColor = texColor.rgb * vLight;
    }
    FragColor = vec4(finalColor, finalAlpha);
}
