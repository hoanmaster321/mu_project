#version 450
precision highp float;

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec3 vWorldPos;

layout(location = 0) out vec4 FragColor;

struct PointLight {
    vec4 posRadius;      // xyz = world pos, w = radius
    vec4 colorIntensity; // rgb = light color, w = intensity
};

layout(std140, set = 1, binding = 0) uniform TerrainUBO {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
    PointLight uLights[32];
    uint uNumLights;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

layout(constant_id = 0) const bool ENABLE_DYNAMIC_LIGHT = true;

// Fragment samplers (set=2)
layout(set = 2, binding = 0) uniform sampler2D uTexture;

void main()
{
    vec4 texColor = texture(uTexture, vUV);
    float finalAlpha = vColor.a * texColor.a;
    if (finalAlpha < 0.01) discard;

    vec3 dynamicLight = vec3(0.0);
    if (ENABLE_DYNAMIC_LIGHT)
    {
        uint count = min(uNumLights, 32u);
        for (uint i = 0u; i < count; ++i)
        {
            vec3 lightPos = uLights[i].posRadius.xyz;
            float radius = uLights[i].posRadius.w;
            if (radius <= 0.0) continue;

            float dist = distance(vWorldPos, lightPos);
            if (dist < radius)
            {
                float atten = clamp(1.0 - dist / radius, 0.0, 1.0);
                atten *= atten; // Smooth quadratic falloff
                dynamicLight += uLights[i].colorIntensity.rgb * (atten * uLights[i].colorIntensity.w);
            }
        }
    }

    vec3 finalRGB = texColor.rgb * (vColor.rgb + dynamicLight);
    FragColor = vec4(clamp(finalRGB, 0.0, 1.0), finalAlpha);
}
