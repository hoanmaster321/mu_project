#version 450
precision highp float;

// Inputs from vertex shader
layout(location = 0) in vec2 FinalUV;
layout(location = 1) in float Alpha;
layout(location = 2) in float LightFactor;
layout(location = 3) in vec3 BodyLightColor;
layout(location = 4) in float vIntensity;
layout(location = 5) in float enableTexture;
layout(location = 6) flat in int textureIndex;
layout(location = 7) flat in float vCustomColorTint;

// Output
layout(location = 0) out vec4 FragColor;

// Fragment sampler (set=2) — one texture per draw group
layout(set = 2, binding = 0) uniform sampler2D textures;

// Fragment push uniforms (set=3)
layout(std140, set = 3, binding = 0) uniform FragUniforms {
    float uBatchTexture;
    float uBrightness;
    float uAlphaTestThreshold;
    float _pad;
};

void main()
{
    vec4 texColor = texture(textures, FinalUV);

    vec3 baseColor = (enableTexture > 0.5) ? texColor.rgb : vec3(1.0);

    vec3 finalColor;
    if (vCustomColorTint > 0.5)
    {
        // Custom color tint: convert texture to max-RGB luma, then multiply with BodyLightColor
        float lum = max(max(baseColor.r, baseColor.g), baseColor.b);
        finalColor = vec3(lum) * BodyLightColor;
    }
    else
    {
        finalColor = baseColor * BodyLightColor;
    }
    finalColor *= uBrightness;

    float finalAlpha = (enableTexture > 0.5) ? texColor.a * Alpha : Alpha;

    if (finalAlpha < uAlphaTestThreshold)
        discard;

    FragColor = vec4(clamp(finalColor, 0.0, 1.0), finalAlpha);
}
