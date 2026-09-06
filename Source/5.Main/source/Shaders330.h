#pragma once
struct ShaderSource {
    const char* name;
    const char* vs;
    const char* fs;
};

const char* vs_Model = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;

void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;

    // Transform vị trí theo bone
    vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset),     vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;

    gl_Position = uProj * uView * BoneTransform;

    // Transform normal
    vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz,     aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
    vec3 n = normalize(NorTransform);
    v_cbmuNormal = n;

    // Lighting
    if (u_enableLight != 0) {
        float intensity = max(((dot(n, cbmuLightPosition.xyz) * 0.8 + 0.4) * cbmuLightPosition.w
                               + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 lightFactor = vec4(intensity, intensity, intensity, cbmuLightPosition.w);
        v_color0 = clamp(cbmuBodyLight * lightFactor, 0.0, 1.0);
    } else {
        v_color0 = cbmuBodyLight;
    }

    // UV
    v_texcoord0 = aTex + u_meshUV.xy;
}

)";

const char* fs_Model = R"(
#version 330
#extension GL_ARB_conservative_depth : enable
layout (depth_unchanged) out float gl_FragDepth;

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";

const char* fs_BlendMesh = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";

const char* vs_BlendMesh = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;


void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;
	
	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;
	}
	
	v_texcoord0 = (aTex + u_meshUV.xy);
}
)";

const char* vs_Chrome1= R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;


void main(){
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;

	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;	
	}
	vec2 tmpvar_6;
	tmpvar_6.x = (tmpvar_3.z * u_setting2.x) + u_setting1.z;
	tmpvar_6.y = (tmpvar_3.y * u_setting2.y) + (u_setting1.z * u_setting2.z);
	v_texcoord0 = tmpvar_6;
}
)";

const char* fs_Chrome1 = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";
const char* vs_Chrome2 = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;

void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;
	
	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;
	}
	vec2 tmpvar_6;
	tmpvar_6.x = (tmpvar_3.z + tmpvar_3.x) * u_setting2.x + u_setting1.w * u_setting2.y;
	tmpvar_6.y = (tmpvar_3.y + tmpvar_3.x) * u_setting2.z + u_setting1.w * u_setting2.w;
	v_texcoord0 = tmpvar_6;
}
)";

const char* fs_Chrome2 = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";

const char* vs_Chrome3 = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;

void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;
	
	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;
	}
	vec2 tmpvar_6;
	tmpvar_6.x = dot(tmpvar_3, u_setting1.xyz);
	tmpvar_6.y = 1.0 - dot(tmpvar_3, u_setting1.xyz);
    v_texcoord0 = tmpvar_6;
}
)";

const char* fs_Chrome3 = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";
const char* vs_Chrome4 = R"(
#version 330

// --- Vertex inputs ---
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// --- Uniforms ---
// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

// --- Varyings to fragment shader ---
out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;

void main() {
    // -- Skinning transform từ texture buffer hoặc fallback u_Bones --
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
    vec4 bonePos;
    bonePos.x = dot(CBMu_GetBoneRow(offset + 0), vec4(aPos, 1.0));
    bonePos.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    bonePos.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    bonePos.w = 1.0;

    // -- Compute final clip-space position --
    gl_Position = uProj * uView * bonePos;

    // -- Normal transform --
    vec3 n;
    n.x = dot(CBMu_GetBoneRow(offset + 0).xyz, aNorm);
    n.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    n.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
    n = normalize(n);
    v_cbmuNormal = n;

    // -- Lighting --
    if (u_enableLight != 0) {
        float intensity = max(((dot(n, cbmuLightPosition.xyz) * 0.8 + 0.4) * cbmuLightPosition.w + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 lightFactor = vec4(intensity, intensity, intensity, cbmuLightPosition.w);
        v_color0 = clamp(cbmuBodyLight * lightFactor, 0.0, 1.0);
    } else {
        v_color0 = cbmuBodyLight;
    }

    // -- Compute chrome4 UV coordinates exactly as on CPU --
    // base.x = dot(n, setting1.xyz)
    // base.y = 1 - dot(n, setting1.xyz)
    vec2 base;
    base.x = dot(n, u_setting1.xyz);
    base.y = 1.0 - dot(n, u_setting1.xyz);

    // dy = (n.z * setting2.x) + (setting1.w * setting2.y)
    float dy = (n.z * u_setting2.x) + (u_setting1.w * u_setting2.y);

    // chrome.x = base.x + (n.y * setting2.z) + (setting1.y * setting2.w)
    // chrome.y = base.y - dy
    vec2 chrome;
    chrome.x = base.x + (n.y * u_setting2.z) + (u_setting1.y * u_setting2.w);
    chrome.y = base.y - dy;

    // final texcoord = chrome + meshUV.xy
    v_texcoord0 = chrome + u_meshUV.xy;
}

)";

const char* fs_Chrome4 = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";

// Ví dụ một số shader
const char* vs_Chrome5 = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;

void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;
	
	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;
	}
	
	vec2 tmpvar_6;
	tmpvar_6.x = dot(tmpvar_3, u_setting1.xyz);
	tmpvar_6.y = 1.0 - dot(tmpvar_3, u_setting1.xyz);
	vec2 tmpvar_7;
	tmpvar_7.y = tmpvar_6.y - (tmpvar_3.z * u_setting2.x) + (u_setting1.w * u_setting2.y);
	tmpvar_7.x = tmpvar_6.x + (tmpvar_3.y * u_setting2.z) + (u_setting1.y * u_setting2.w);
	v_texcoord0 = tmpvar_7;
}
)";

const char* fs_Chrome5 = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";

// Ví dụ một số shader
const char* vs_Chrome6 = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;

void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;
	
	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;
	}
	float tmpvar_6;
	tmpvar_6 = (u_setting1.z * u_setting1.y);
	float tmpvar_7;
	tmpvar_7 = (tmpvar_3.z + tmpvar_3.x);
	vec2 tmpvar_8;
	tmpvar_8.x = tmpvar_7 * u_setting1.x + tmpvar_6;
	tmpvar_8.y = tmpvar_7 * u_setting1.x + tmpvar_6;
	v_texcoord0 = tmpvar_8;
}
)";

const char* fs_Chrome6 = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";
const char* vs_Chrome7 = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;


void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;
	
	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;
	}
	float tmpvar_6;
	tmpvar_6 = (tmpvar_3.z + tmpvar_3.x);
	float tmpvar_7;
	tmpvar_7 = (u_setting1.z * u_setting1.w);
	vec2 tmpvar_8;
	tmpvar_8.x = (tmpvar_6 * u_setting1.x) + tmpvar_7;
	tmpvar_8.y = (tmpvar_6 * u_setting1.y) + tmpvar_7;
	v_texcoord0 = tmpvar_8;
}
)";

const char* fs_Chrome7 = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";

const char* vs_Oil = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;


void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;
	
	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;
	}
	vec2 tmpvar_6;
	tmpvar_6.x = tmpvar_3.x + u_meshUV.x;
	tmpvar_6.y = tmpvar_3.y + u_meshUV.y;
	v_texcoord0 = tmpvar_6;
}
)";

const char* fs_Oil = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";
const char* vs_Metal = R"(
#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in uint aBone;
layout(location = 4) in uint aBoneBase;
layout(location = 5) in vec4 aInstanceBodyLight;
layout(location = 6) in vec4 aInstanceLightPosition;

// Thay toàn bộ uniform rời rạc bằng block RenderBlock
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

uniform vec4 u_Bones[256]; // fallback uniform cũ khi texture buffer không khả dụng
uniform samplerBuffer u_BoneRows; // CBMu: chứa nhiều bone row để batch nhiều NPC giống nhau trong một draw.
uniform int uUseBoneTexture;
uniform int uUseInstanceParams; // CBMu: chọn màu và hướng sáng theo từng instance khi BMD được batch.

vec4 CBMu_GetBoneRow(int index)
{
    if (uUseBoneTexture != 0)
    {
        return texelFetch(u_BoneRows, index);
    }
    return u_Bones[index];
}

out vec4 v_color0;
out vec2 v_texcoord0;
out vec3 v_cbmuNormal;


void main() {
    int offset = int(aBone) + int(aBoneBase);
    vec4 cbmuBodyLight = (uUseInstanceParams != 0) ? aInstanceBodyLight : u_bodyLight;
    vec4 cbmuLightPosition = (uUseInstanceParams != 0) ? aInstanceLightPosition : u_lightPosition;
	vec4 BoneTransform;
    BoneTransform.x = dot(CBMu_GetBoneRow(offset), vec4(aPos, 1.0));
    BoneTransform.y = dot(CBMu_GetBoneRow(offset + 1), vec4(aPos, 1.0));
    BoneTransform.z = dot(CBMu_GetBoneRow(offset + 2), vec4(aPos, 1.0));
    BoneTransform.w = 1.0;
	
    vec4 tmpvar_2;
    tmpvar_2 = BoneTransform;
	
	gl_Position = uProj * uView * tmpvar_2;
	
	vec3 NorTransform;
    NorTransform.x = dot(CBMu_GetBoneRow(offset).xyz, aNorm);
    NorTransform.y = dot(CBMu_GetBoneRow(offset + 1).xyz, aNorm);
    NorTransform.z = dot(CBMu_GetBoneRow(offset + 2).xyz, aNorm);
	
	vec3 tmpvar_3;
	tmpvar_3 = normalize(NorTransform);
	v_cbmuNormal = tmpvar_3;
	
	if(u_enableLight != 0)
	{
	   	float tmpvar_4;
		tmpvar_4 = max(((((dot(tmpvar_3, cbmuLightPosition.xyz) * 0.8) + 0.4)* cbmuLightPosition.w) + (1.0 - cbmuLightPosition.w)), 0.2);
        vec4 tmpvar_5;
        tmpvar_5.w = cbmuLightPosition.w;
        tmpvar_5.x = tmpvar_4;
        tmpvar_5.y = tmpvar_4;
        tmpvar_5.z = tmpvar_4;
		v_color0 = clamp((cbmuBodyLight * tmpvar_5), 0.0, 1.0);
	}
	else
	{
	  	v_color0 = cbmuBodyLight;
	}
	vec2 tmpvar_6;
	tmpvar_6.x = (tmpvar_3.z * u_setting2.x) + u_setting2.y;
	tmpvar_6.y = (tmpvar_3.y * u_setting2.z) + u_setting2.w;
	v_texcoord0 = tmpvar_6;
}
)";

const char* fs_Metal = R"(
#version 330

in vec4 v_color0;
in vec2 v_texcoord0;
uniform sampler2D uTexture;

out vec4 FragColor;

void main(){
    vec4 tmpvar_1;
    tmpvar_1 = (texture(uTexture, v_texcoord0) * v_color0);
	if ((tmpvar_1.w < 0.2)) {
      discard;
    };
    FragColor = tmpvar_1;
}
)";
const char* fs_Common = R"(
#version 330
#extension GL_ARB_conservative_depth : enable
layout (depth_unchanged) out float gl_FragDepth;

in vec4 v_color0;
in vec2 v_texcoord0;
in vec3 v_cbmuNormal;

uniform sampler2D uTexture;
uniform sampler2D uCompositeTexture0;
uniform sampler2D uCompositeTexture1;
uniform sampler2D uCompositeTexture2;
uniform int uCompositeOverlayCount;
uniform float uCompositeTime;
uniform vec4 uCompositeColorMode0;
uniform vec4 uCompositeColorMode1;
uniform vec4 uCompositeColorMode2;

// CBMu_BEGIN: Gộp tối đa ba lớp Chrome/Metal/Shiny vào draw body wearable.
layout(std140) uniform RenderBlock {
    mat4 uProj;
    mat4 uView;
    vec4 u_bodyLight;
    vec4 u_lightPosition;
    vec4 u_meshUV;
    vec4 u_setting1;
    vec4 u_setting2;
    int  u_enableLight;
    int  uRenderFlags;
    float uAlpha;
};

out vec4 FragColor;

const int RENDER_COLOR    = 0x00000001;
const int RENDER_TEXTURE  = 0x00000002;
const int RENDER_CHROME   = 0x00000004;
const int RENDER_METAL    = 0x00000008;
const int RENDER_LIGHTMAP = 0x00000010;
const int RENDER_CHROME2  = 0x00000200;
const int RENDER_CHROME3  = 0x00000800;
const int RENDER_CHROME4  = 0x00001000;
const int RENDER_CHROME5  = 0x00004000;
const int RENDER_CHROME6  = 0x00010000;
const int RENDER_CHROME7  = 0x00020000;
const int RENDER_CHROME8  = 0x00080000;
const int RENDER_OIL      = 0x00008000;

vec2 CBMu_ComputeCompositeUV(vec3 normalDir, float materialMode)
{
    float chromeTime = mod(uCompositeTime, 10000.0) * 0.0001;
    float chrome1WaveRate =
        abs(u_meshUV.w) > 0.000001 ? u_meshUV.w : 0.0001;
    float chrome1Time =
        mod(uCompositeTime, 10000.0) * chrome1WaveRate;
    float chrome2Time = mod(uCompositeTime, 5000.0) * 0.00024 - 0.4;

    if (abs(materialMode - 2.0) < 0.1)
    {
        return vec2(
            (normalDir.z + normalDir.x) * 0.8 + chrome2Time * 2.0,
            (normalDir.y + normalDir.x) + chrome2Time * 3.0);
    }

    if (abs(materialMode - 4.0) < 0.1)
    {
        float lightDirX = cos(uCompositeTime * 0.001);
        float lightDirY = sin(uCompositeTime * 0.002);
        float dotValue =
            normalDir.x * lightDirX +
            normalDir.y * lightDirY +
            normalDir.z;
        return vec2(
            dotValue + (normalDir.y * 0.5 + lightDirY * 3.0) + u_meshUV.x,
            (1.0 - dotValue) - (normalDir.z * 0.5 + chromeTime * 3.0) + u_meshUV.y);
    }

    if (abs(materialMode - 9.0) < 0.1)
    {
        return vec2(normalDir.z * 0.5 + 0.2, normalDir.y * 0.5 + 0.5);
    }

    return vec2(
        normalDir.z * 0.5 + chrome1Time,
        normalDir.y * 0.5 + chrome1Time * 2.0);
}

vec4 CBMu_SampleCompositeOverlay(int index, vec3 normalDir)
{
    vec4 colorMode = vec4(1.0, 1.0, 1.0, 0.0);
    vec4 overlaySample;

    if (index == 1)
    {
        colorMode = uCompositeColorMode1;
        overlaySample = texture(
            uCompositeTexture1,
            CBMu_ComputeCompositeUV(normalDir, colorMode.w));
    }
    else if (index == 2)
    {
        colorMode = uCompositeColorMode2;
        overlaySample = texture(
            uCompositeTexture2,
            CBMu_ComputeCompositeUV(normalDir, colorMode.w));
    }
    else
    {
        colorMode = uCompositeColorMode0;
        overlaySample = texture(
            uCompositeTexture0,
            CBMu_ComputeCompositeUV(normalDir, colorMode.w));
    }

    overlaySample.rgb *= colorMode.rgb;
    return overlaySample;
}

void main() {
    vec4 texColor = texture(uTexture, v_texcoord0) * v_color0;
    // bgfx dựng UV Chrome/Metal tại vertex rồi nội suy. Không chuẩn hóa lại ở fragment
    // vì thao tác đó làm co vùng sáng trên các mặt dài như lưỡi vũ khí.
    vec3 normalDir = v_cbmuNormal;

    if (uCompositeOverlayCount > 0)
    {
        for (int i = 0; i < uCompositeOverlayCount && i < 3; ++i)
        {
            vec4 overlaySample = CBMu_SampleCompositeOverlay(i, normalDir);
            texColor.rgb = clamp(texColor.rgb + overlaySample.rgb * v_color0.a * 0.45, 0.0, 1.0);
        }
    }

    bool doAlphaTest = false;

    if ((uRenderFlags & RENDER_COLOR) != 0) {
        if (uAlpha < 0.99) doAlphaTest = true;
    }
    if ((uRenderFlags & RENDER_TEXTURE) != 0) {
        if (uAlpha < 0.99) doAlphaTest = true;
    }

    int chromeMask = RENDER_CHROME | RENDER_CHROME2 | RENDER_CHROME3 |
                     RENDER_CHROME4 | RENDER_CHROME5 | RENDER_CHROME6 |
                     RENDER_CHROME7 | RENDER_CHROME8;
    if ((uRenderFlags & chromeMask) != 0 ||
        (uRenderFlags & RENDER_METAL) != 0 ||
        (uRenderFlags & RENDER_OIL)   != 0)
    {
        doAlphaTest = true;
    }

    if (doAlphaTest && texColor.a < 0.2) {
        discard;
    }

    FragColor = texColor;
}
// CBMu_END: Gộp tối đa ba lớp Chrome/Metal/Shiny vào draw body wearable.


)";

const char* vs_Joint = R"(
#version 330
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTex;

uniform mat4 uProj;
uniform mat4 uView;

out vec4 vColor;
out vec2 vTex;

void main() {
    vColor = aColor;
    vTex = aTex;
    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
)";

const char* fs_Joint = R"(
#version 330
in vec4 vColor;
in vec2 vTex;
uniform sampler2D uTexture;
uniform int uDoAlphaTest;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture, vTex) * vColor;
    if (uDoAlphaTest != 0 && texColor.a < 0.2) discard;
    FragColor = texColor;
}
)";



static ShaderSource shaderSources[] = {
    { "Model",     vs_Model,     fs_Common },
    { "BlendMesh", vs_BlendMesh, fs_Common },
    { "Chrome1",   vs_Chrome1,   fs_Common },
    { "Chrome2",   vs_Chrome2,   fs_Common },
    { "Chrome3",   vs_Chrome3,   fs_Common },
    { "Chrome4",   vs_Chrome4,   fs_Common },
    { "Chrome5",   vs_Chrome5,   fs_Common },
    { "Chrome6",   vs_Chrome6,   fs_Common },
    { "Chrome7",   vs_Chrome7,   fs_Common },
    { "Chrome8",   vs_Chrome7,   fs_Common },
    { "Oil",       vs_Oil,       fs_Common },
    { "Metal",     vs_Metal,     fs_Common },
    { "Joint",     vs_Joint,     fs_Joint },
    // ...
};
