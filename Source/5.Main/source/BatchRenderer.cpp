#include <glm/gtc/matrix_transform.hpp>
#include "StdAfx.h"
#include "BatchRenderer.h"
#include "GPUContext.h"
#include "CBMu/CBMu_RenderConfig.h"
#include "ZzzTexture.h"
#include "ZzzOpenglUtil.h"
#include "ZzzBMD.h"
#include "New_RenderBMD.h"
#include "New_ModelBMD.h"
#include "Utilities/Log/muConsoleDebug.h"

// Global single instance
CBatchRenderer g_BatchRendererInstance;

CBatchRenderer& CBatchRenderer::Instance()
{
	return g_BatchRendererInstance;
}

namespace
{
	// Shader compiler helper
	GLuint CompileBatchShader(GLenum type, const char* source)
	{
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);

		GLint success = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char infoLog[1024] = { 0 };
			glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
			g_ConsoleDebug->Write(5, "[BatchRenderer] Shader compile error: %s", infoLog);
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	static std::string ReadShaderSourceFile(const char* filename)
	{
		std::string paths[] = {
			std::string("Shaders/") + filename,
			std::string("Data/Shaders/") + filename,
			std::string(filename)
		};
		for (const auto& p : paths)
		{
			std::ifstream file(p);
			if (file.is_open())
			{
				std::stringstream ss;
				ss << file.rdbuf();
				return ss.str();
			}
		}
		return "";
	}

	GLuint CreateBatchProgram(const char* vsSrc, const char* fsSrc)
	{
		GLuint vs = CompileBatchShader(GL_VERTEX_SHADER, vsSrc);
		if (!vs) return 0;
		GLuint fs = CompileBatchShader(GL_FRAGMENT_SHADER, fsSrc);
		if (!fs) { glDeleteShader(vs); return 0; }

		GLuint prog = glCreateProgram();
		glAttachShader(prog, vs);
		glAttachShader(prog, fs);
		glLinkProgram(prog);

		glDeleteShader(vs);
		glDeleteShader(fs);

		GLint success = GL_FALSE;
		glGetProgramiv(prog, GL_LINK_STATUS, &success);
		if (!success)
		{
			char infoLog[1024] = { 0 };
			glGetProgramInfoLog(prog, sizeof(infoLog), NULL, infoLog);
			g_ConsoleDebug->Write(5, "[BatchRenderer] Program link error: %s", infoLog);
			glDeleteProgram(prog);
			return 0;
		}
		return prog;
	}

	GLuint CreateBatchProgramFromFile(const char* vsFile, const char* fsFile, const char* fallbackVS, const char* fallbackFS)
	{
		GLuint prog = 0;
		std::string vsCode = ReadShaderSourceFile(vsFile);
		std::string fsCode = ReadShaderSourceFile(fsFile);
		if (!vsCode.empty() && !fsCode.empty())
		{
			prog = CreateBatchProgram(vsCode.c_str(), fsCode.c_str());
		}
		if (!prog && fallbackVS && fallbackFS)
		{
			prog = CreateBatchProgram(fallbackVS, fallbackFS);
		}
		return prog;
	}

	// Shaders (GLSL 4.30 / 3.30 compatible)
	const char* const kTerrainVS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

layout(std140) uniform VertUniforms {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
};

out vec2 vUV;
out vec4 vColor;

void main() {
    vUV = aTexCoord;
    vColor = aColor;
    gl_Position = uProjMatrix * uViewMatrix * vec4(aPos, 1.0);
}
)";

	const char* const kTerrainFS = R"(#version 330 core
precision highp float;

in vec2 vUV;
in vec4 vColor;
out vec4 FragColor;

uniform sampler2D uTexture;

void main() {
    vec4 texColor = texture(uTexture, vUV);
    float finalAlpha = vColor.a * texColor.a;
    if (finalAlpha < 0.01) discard;
    FragColor = vec4(texColor.rgb * vColor.rgb, finalAlpha);
}
)";

	const char* const kSpriteVS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

layout(std140) uniform SpriteBlock {
    mat4 uView;
    mat4 uProj;
    vec2 uScreenSize;
};

out vec2 vUV;
out vec4 vColor;

void main() {
    vUV = aUV;
    vColor = aColor;
    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
)";

	const char* const kSpriteFS = R"(#version 330 core
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uTexture;
out vec4 FragColor;

void main() {
    vec4 tex = texture(uTexture, vUV) * vColor;
    if (tex.a < 0.01) discard;
    FragColor = tex;
}
)";

	const char* const kImageVS = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec4 aExtra;

layout(std140) uniform ImageBlock {
    mat4 uProj;
    vec2 uScreenSize;
};

out vec2 vUV;
out vec4 vColor;
flat out vec4 vExtra;

void main() {
    vUV = aUV;
    vColor = aColor;
    vExtra = aExtra;
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
}
)";

	const char* const kImageFS = R"(#version 330 core
in vec2 vUV;
in vec4 vColor;
flat in vec4 vExtra;
uniform sampler2D uTexture;
out vec4 FragColor;

void main() {
    vec4 texColor = (vExtra.y > 0.5) ? vec4(1.0) : texture(uTexture, vUV);
    if (vExtra.z > 0.5) {
        texColor = vec4(1.0, 1.0, 1.0, texColor.r);
    }
    vec4 finalColor = texColor * vColor;
    if (vExtra.w > 0.5) {
        float gray = dot(finalColor.rgb, vec3(0.299, 0.587, 0.114));
        finalColor.rgb = vec3(gray);
    }
    if (finalColor.a < 0.01) discard;
    FragColor = finalColor;
}
)";

	static inline uint32_t PackRGBA8Helper(float r, float g, float b, float a)
	{
		auto q = [](float v) -> uint32_t {
			v = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
			return (uint32_t)(v * 255.f + 0.5f);
		};
		return q(r) | (q(g) << 8) | (q(b) << 16) | (q(a) << 24);
	}
}

// Vertex for 2D UI quads
struct ImageQuadVertex
{
	float pos[2];
	float uv[2];
	float color[4];
	float extra[4];
};

CBatchRenderer::CBatchRenderer()
	: m_TerrainProgram(0)
	, m_SpriteProgram(0)
	, m_ImageProgram(0)
	, m_GL43Supported(false)
	, m_TerrainVAO(0)
	, m_TerrainVBO(0)
	, m_TerrainIBO(0)
	, m_TerrainUBO(0)
	, m_SpriteVAO(0)
	, m_SpriteVBO(0)
	, m_SpriteUBO(0)
	, m_LastSpritePruneTime(0)
	, m_ImageVAO(0)
	, m_ImageVBO(0)
	, m_ImageUBO(0)
	, m_ScissorEnabled(false)
	, m_ScissorX(0), m_ScissorY(0), m_ScissorW(0), m_ScissorH(0)
	, m_ShadowBatchDirty(false)
	, m_LastProxyPruneTime(0)
	, m_Initialized(false)
{
}

CBatchRenderer::~CBatchRenderer()
{
	Shutdown();
}

bool CBatchRenderer::Initialize()
{
	if (m_Initialized) return true;

	m_GL43Supported = true;

	// 1. Compile Shaders using valid standard GLSL 3.30 programs
	m_TerrainProgram = CreateBatchProgram(kTerrainVS, kTerrainFS);
	m_SpriteProgram  = CreateBatchProgram(kSpriteVS, kSpriteFS);
	m_ImageProgram   = CreateBatchProgram(kImageVS, kImageFS);

	// 2. Setup Terrain VAO / VBO / IBO / UBO
	glGenVertexArrays(1, &m_TerrainVAO);
	glGenBuffers(1, &m_TerrainVBO);
	glGenBuffers(1, &m_TerrainIBO);
	glGenBuffers(1, &m_TerrainUBO);

	glBindVertexArray(m_TerrainVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_TerrainVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_TerrainIBO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex_t), (void*)offsetof(TerrainVertex_t, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex_t), (void*)offsetof(TerrainVertex_t, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TerrainVertex_t), (void*)offsetof(TerrainVertex_t, color));

	glBindBuffer(GL_UNIFORM_BUFFER, m_TerrainUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(TerrainVertUBO), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_TerrainUBO);

	glBindVertexArray(0);

	// 3. Setup Sprite VAO / VBO / UBO
	glGenVertexArrays(1, &m_SpriteVAO);
	glGenBuffers(1, &m_SpriteVBO);
	glGenBuffers(1, &m_SpriteUBO);

	glBindVertexArray(m_SpriteVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_SpriteVBO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex_t), (void*)offsetof(TerrainVertex_t, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex_t), (void*)offsetof(TerrainVertex_t, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TerrainVertex_t), (void*)offsetof(TerrainVertex_t, color));

	glBindBuffer(GL_UNIFORM_BUFFER, m_SpriteUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SpriteVertUBO), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_SpriteUBO);

	glBindVertexArray(0);

	// 4. Setup Image VAO / VBO / UBO
	glGenVertexArrays(1, &m_ImageVAO);
	glGenBuffers(1, &m_ImageVBO);
	glGenBuffers(1, &m_ImageUBO);

	glBindVertexArray(m_ImageVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_ImageVBO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImageQuadVertex), (void*)offsetof(ImageQuadVertex, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImageQuadVertex), (void*)offsetof(ImageQuadVertex, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ImageQuadVertex), (void*)offsetof(ImageQuadVertex, color));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ImageQuadVertex), (void*)offsetof(ImageQuadVertex, extra));

	glBindBuffer(GL_UNIFORM_BUFFER, m_ImageUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ImageVertUBO), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, m_ImageUBO);

	glBindVertexArray(0);

	m_Initialized = true;
	g_ConsoleDebug->Write(5, "[BatchRenderer] Initialized successfully. OpenGL 4.3 Support: %d", m_GL43Supported ? 1 : 0);
	return true;
}

void CBatchRenderer::Shutdown()
{
	if (!m_Initialized) return;
	ClearAllBatchMaps();

	if (m_TerrainProgram) { glDeleteProgram(m_TerrainProgram); m_TerrainProgram = 0; }
	if (m_SpriteProgram)  { glDeleteProgram(m_SpriteProgram);  m_SpriteProgram = 0; }
	if (m_ImageProgram)   { glDeleteProgram(m_ImageProgram);   m_ImageProgram = 0; }

	if (m_TerrainVAO) { glDeleteVertexArrays(1, &m_TerrainVAO); m_TerrainVAO = 0; }
	if (m_TerrainVBO) { glDeleteBuffers(1, &m_TerrainVBO); m_TerrainVBO = 0; }
	if (m_TerrainIBO) { glDeleteBuffers(1, &m_TerrainIBO); m_TerrainIBO = 0; }
	if (m_TerrainUBO) { glDeleteBuffers(1, &m_TerrainUBO); m_TerrainUBO = 0; }

	if (m_SpriteVAO) { glDeleteVertexArrays(1, &m_SpriteVAO); m_SpriteVAO = 0; }
	if (m_SpriteVBO) { glDeleteBuffers(1, &m_SpriteVBO); m_SpriteVBO = 0; }
	if (m_SpriteUBO) { glDeleteBuffers(1, &m_SpriteUBO); m_SpriteUBO = 0; }

	if (m_ImageVAO) { glDeleteVertexArrays(1, &m_ImageVAO); m_ImageVAO = 0; }
	if (m_ImageVBO) { glDeleteBuffers(1, &m_ImageVBO); m_ImageVBO = 0; }
	if (m_ImageUBO) { glDeleteBuffers(1, &m_ImageUBO); m_ImageUBO = 0; }

	m_Initialized = false;
}

// ============================================================================
// Terrain Batching Implementation
// ============================================================================
extern vec3_t TerrainVertex[4];
extern float TerrainTextureCoord[4][2];
extern vec3_t PrimaryTerrainLight[TERRAIN_SIZE * TERRAIN_SIZE];
extern vec3_t BackTerrainLight[TERRAIN_SIZE * TERRAIN_SIZE];
extern float TerrainMappingAlpha[TERRAIN_SIZE * TERRAIN_SIZE];
extern int TerrainIndex1, TerrainIndex2, TerrainIndex3, TerrainIndex4;
extern int CachTexture;

void CBatchRenderer::AddTerrainFaceToBatch(int batchType, int textureIndex, int renderFlags)
{
	if (batchType < 0 || batchType >= TERRAIN_BATCH_COUNT)
		return;

	TerrainBatchKey key = { (uint32_t)renderFlags, textureIndex };

	std::vector<TerrainVertex_t>* pVertices = nullptr;
	std::vector<uint32_t>* pIndices = nullptr;

	if (m_TerrainMRU.batchType == batchType && m_TerrainMRU.key == key)
	{
		pVertices = m_TerrainMRU.vertices;
		pIndices = m_TerrainMRU.indices;
	}
	else
	{
		pVertices = &m_TerrainVerticesMap[batchType][key];
		pIndices  = &m_TerrainIndicesMap[batchType][key];
		m_TerrainMRU.batchType = batchType;
		m_TerrainMRU.key = key;
		m_TerrainMRU.vertices = pVertices;
		m_TerrainMRU.indices = pIndices;
	}

	uint32_t base = (uint32_t)pVertices->size();
	TerrainVertex_t v[4];
	for (int i = 0; i < 4; ++i) {
		memcpy(v[i].pos, TerrainVertex[i], sizeof(v[i].pos));
		memcpy(v[i].uv, TerrainTextureCoord[i], sizeof(v[i].uv));
	}

	uint32_t idx[4] = { (uint32_t)TerrainIndex1, (uint32_t)TerrainIndex2, (uint32_t)TerrainIndex3, (uint32_t)TerrainIndex4 };

	const uint32_t maxIdx = TERRAIN_SIZE * TERRAIN_SIZE;
	for (int i = 0; i < 4; ++i)
	{
		uint32_t safeIdx = (idx[i] < maxIdx) ? idx[i] : 0;
		if (batchType == TERRAIN_BATCH_BLEND)
		{
			float l = TerrainMappingAlpha[safeIdx];
			v[i].color = PackRGBA8Helper(l, l, l, 1.0f);
		}
		else if (batchType == TERRAIN_BATCH_ALPHA)
		{
#if CBMu_ENABLE_GPU_DYNAMIC_LIGHTING
			float* l = BackTerrainLight[safeIdx];
#else
			float* l = PrimaryTerrainLight[safeIdx];
#endif
			v[i].color = PackRGBA8Helper(l[0], l[1], l[2], (std::min)(TerrainMappingAlpha[safeIdx], 1.0f));
		}
		else
		{
#if CBMu_ENABLE_GPU_DYNAMIC_LIGHTING
			float* l = BackTerrainLight[safeIdx];
#else
			float* l = PrimaryTerrainLight[safeIdx];
#endif
			v[i].color = PackRGBA8Helper(l[0], l[1], l[2], 1.0f);
		}
	}

	pVertices->insert(pVertices->end(), v, v + 4);
	const uint32_t newIdx[6] = { base + 0, base + 1, base + 2, base + 0, base + 2, base + 3 };
	pIndices->insert(pIndices->end(), newIdx, newIdx + 6);
}


void CBatchRenderer::AddTerrainCustomQuad(int batchType, int textureIndex, int renderFlags, const vec3_t verts[4], const vec3_t uvs[4], const vec4_t colors[4])
{
	if (batchType < 0 || batchType >= TERRAIN_BATCH_COUNT)
		return;

	TerrainBatchKey key = { (uint32_t)renderFlags, textureIndex };

	std::vector<TerrainVertex_t>* pVertices = nullptr;
	std::vector<uint32_t>* pIndices = nullptr;

	if (m_TerrainMRU.batchType == batchType && m_TerrainMRU.key == key)
	{
		pVertices = m_TerrainMRU.vertices;
		pIndices = m_TerrainMRU.indices;
	}
	else
	{
		pVertices = &m_TerrainVerticesMap[batchType][key];
		pIndices  = &m_TerrainIndicesMap[batchType][key];
		m_TerrainMRU.batchType = batchType;
		m_TerrainMRU.key = key;
		m_TerrainMRU.vertices = pVertices;
		m_TerrainMRU.indices = pIndices;
	}

	uint32_t base = (uint32_t)pVertices->size();
	TerrainVertex_t v[4];
	for (int i = 0; i < 4; ++i) {
		memcpy(v[i].pos, verts[i], sizeof(v[i].pos));
		v[i].uv[0] = uvs[i][0];
		v[i].uv[1] = uvs[i][1];
		v[i].color = PackRGBA8Helper(colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
	}
	pVertices->insert(pVertices->end(), v, v + 4);

	const uint32_t newIdx[6] = { base + 0, base + 1, base + 2, base + 0, base + 2, base + 3 };
	pIndices->insert(pIndices->end(), newIdx, newIdx + 6);
}

void CBatchRenderer::FlushTerrainBatches()
{
	struct TerrainDrawCmd {
		int textureIndex;
		uint32_t indexOffset;
		uint32_t indexCount;
		int batchType;
		uint32_t renderFlags;
	};
	static std::vector<TerrainDrawCmd> drawCmds;
	drawCmds.clear();

	if (GPUContext::Instance().IsFrameActive())
	{
		uint32_t totalVerts = 0;
		uint32_t totalIndices = 0;
		for (int bType = 0; bType < TERRAIN_BATCH_COUNT; ++bType)
		{
			for (const auto& [key, vertices] : m_TerrainVerticesMap[bType])
			{
				const auto& indices = m_TerrainIndicesMap[bType][key];
				if (!vertices.empty() && !indices.empty())
				{
					totalVerts += static_cast<uint32_t>(vertices.size());
					totalIndices += static_cast<uint32_t>(indices.size());
				}
			}
		}

		if (totalVerts > 0 && totalIndices > 0)
		{
			uint32_t vertOffset = 0;
			uint32_t idxOffset = 0;
			TerrainVertex_t* dstVerts = GPUContext::Instance().AllocateTerrainVertexBuffer(totalVerts, vertOffset);
			uint32_t* dstIndices = GPUContext::Instance().AllocateTerrainIndexBuffer(totalIndices, idxOffset);
			if (dstVerts && dstIndices)
			{
				uint32_t curVertCount = 0;
				uint32_t curIdxCount = 0;

				for (int bType = 0; bType < TERRAIN_BATCH_COUNT; ++bType)
				{
					for (const auto& [key, vertices] : m_TerrainVerticesMap[bType])
					{
						const auto& indices = m_TerrainIndicesMap[bType][key];
						if (vertices.empty() || indices.empty()) continue;

						uint32_t baseVertex = curVertCount;
						uint32_t baseIndex = curIdxCount;

						memcpy(dstVerts + curVertCount, vertices.data(), vertices.size() * sizeof(TerrainVertex_t));
						curVertCount += static_cast<uint32_t>(vertices.size());

						for (size_t i = 0; i < indices.size(); ++i)
						{
							dstIndices[curIdxCount++] = baseVertex + indices[i];
						}

						TerrainDrawCmd cmd;
						cmd.textureIndex = key.textureIndex;
						cmd.indexOffset = baseIndex;
						cmd.indexCount = static_cast<uint32_t>(indices.size());
						cmd.batchType = bType;
						cmd.renderFlags = key.renderFlags;
						drawCmds.push_back(cmd);
					}
				}

				GPUContext::TerrainVertUBO vkUbo;
				GetActiveViewMatrix(&vkUbo.viewMatrix[0][0]);
				GetActiveProjectionMatrix(&vkUbo.projMatrix[0][0]);

				// Invert row 1 (Y) for Vulkan NDC (OpenGL Y-up → Vulkan Y-down)
				vkUbo.projMatrix[0][1] = -vkUbo.projMatrix[0][1];
				vkUbo.projMatrix[1][1] = -vkUbo.projMatrix[1][1];
				vkUbo.projMatrix[2][1] = -vkUbo.projMatrix[2][1];
				vkUbo.projMatrix[3][1] = -vkUbo.projMatrix[3][1];

				// Remap depth row 2 (Z) from OpenGL [-1,1] to Vulkan [0,1]: (row 2 + row 3) * 0.5
				vkUbo.projMatrix[0][2] = (vkUbo.projMatrix[0][2] + vkUbo.projMatrix[0][3]) * 0.5f;
				vkUbo.projMatrix[1][2] = (vkUbo.projMatrix[1][2] + vkUbo.projMatrix[1][3]) * 0.5f;
				vkUbo.projMatrix[2][2] = (vkUbo.projMatrix[2][2] + vkUbo.projMatrix[2][3]) * 0.5f;
				vkUbo.projMatrix[3][2] = (vkUbo.projMatrix[3][2] + vkUbo.projMatrix[3][3]) * 0.5f;

				const auto& dynLights = GPUContext::Instance().GetDynamicLights();
				uint32_t numLights = (std::min)(static_cast<uint32_t>(dynLights.size()), 32u);
				if (dynLights.size() <= 32)
				{
					for (uint32_t li = 0; li < numLights; ++li) {
						vkUbo.lights[li] = dynLights[li];
					}
				}
				else
				{
					extern vec3_t CameraPosition;
					std::vector<GPUDynamicPointLight> sortedLights = dynLights;
					std::partial_sort(sortedLights.begin(), sortedLights.begin() + 32, sortedLights.end(),
						[](const GPUDynamicPointLight& a, const GPUDynamicPointLight& b) {
							extern vec3_t CameraPosition;
							float dxA = a.posRadius.x - CameraPosition[0];
							float dyA = a.posRadius.y - CameraPosition[1];
							float dzA = a.posRadius.z - CameraPosition[2];
							float distSqA = dxA * dxA + dyA * dyA + dzA * dzA;

							float dxB = b.posRadius.x - CameraPosition[0];
							float dyB = b.posRadius.y - CameraPosition[1];
							float dzB = b.posRadius.z - CameraPosition[2];
							float distSqB = dxB * dxB + dyB * dyB + dzB * dzB;

							return distSqA < distSqB;
						});
					for (uint32_t li = 0; li < 32; ++li) {
						vkUbo.lights[li] = sortedLights[li];
					}
					numLights = 32;
				}
				vkUbo.numLights = numLights;
				for (uint32_t li = numLights; li < 32; ++li) {
					vkUbo.lights[li].posRadius = glm::vec4(0.0f);
					vkUbo.lights[li].colorIntensity = glm::vec4(0.0f);
				}

				std::vector<GPUContext::TerrainMergedBatch> vkBatches;
				for (const auto& cmd : drawCmds)
				{
					GPUContext::TerrainMergedBatch batch;
					batch.batchType = cmd.batchType;
					BITMAP_t* pBitmap = (cmd.textureIndex >= 0) ? Bitmaps.FindTexture(static_cast<GLuint>(cmd.textureIndex)) : nullptr;
					batch.textureIndex = (pBitmap && pBitmap->TextureNumber > 0) ? static_cast<int>(pBitmap->TextureNumber)
						: (cmd.textureIndex < 0 ? -cmd.textureIndex : 0);
					batch.renderFlags = cmd.renderFlags;
					GPUContext::TerrainDrawCmd tCmd;
					tCmd.firstIndex = cmd.indexOffset;
					tCmd.indexCount = cmd.indexCount;
					tCmd.vertexOffset = 0;
					batch.cmds.push_back(tCmd);
					vkBatches.push_back(std::move(batch));
				}

				GPUContext::Instance().DrawTerrainMergedPreallocated(
					vertOffset, totalVerts * sizeof(TerrainVertex_t),
					idxOffset, totalIndices * sizeof(uint32_t),
					vkBatches, vkUbo);
			}
		}
	}
	else if (m_TerrainProgram != 0)
	{
		static std::vector<TerrainVertex_t> mergedVerts;
		static std::vector<uint32_t> mergedIndices;
		mergedVerts.clear();
		mergedIndices.clear();

		for (int bType = 0; bType < TERRAIN_BATCH_COUNT; ++bType)
		{
			for (const auto& [key, vertices] : m_TerrainVerticesMap[bType])
			{
				const auto& indices = m_TerrainIndicesMap[bType][key];
				if (vertices.empty() || indices.empty()) continue;

				uint32_t baseVertex = (uint32_t)mergedVerts.size();
				uint32_t baseIndex = (uint32_t)mergedIndices.size();

				mergedVerts.insert(mergedVerts.end(), vertices.begin(), vertices.end());
				for (auto idx : indices)
					mergedIndices.push_back(baseVertex + idx);

				TerrainDrawCmd cmd;
				cmd.textureIndex = key.textureIndex;
				cmd.indexOffset = baseIndex;
				cmd.indexCount = (uint32_t)indices.size();
				cmd.batchType = bType;
				cmd.renderFlags = key.renderFlags;
				drawCmds.push_back(cmd);
			}
		}

		if (!mergedVerts.empty())
		{
			TerrainVertUBO ubo;
			GetActiveViewMatrix(&ubo.viewMatrix[0][0]);
			GetActiveProjectionMatrix(&ubo.projMatrix[0][0]);
			glBindBuffer(GL_UNIFORM_BUFFER, m_TerrainUBO);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(TerrainVertUBO), &ubo);
			glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_TerrainUBO);

			glBindVertexArray(m_TerrainVAO);
			glBindBuffer(GL_ARRAY_BUFFER, m_TerrainVBO);
			glBufferData(GL_ARRAY_BUFFER, mergedVerts.size() * sizeof(TerrainVertex_t), mergedVerts.data(), GL_STREAM_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_TerrainIBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, mergedIndices.size() * sizeof(uint32_t), mergedIndices.data(), GL_STREAM_DRAW);

			glUseProgram(m_TerrainProgram);
			GLuint blockIndex = glGetUniformBlockIndex(m_TerrainProgram, "VertUniforms");
			if (blockIndex != GL_INVALID_INDEX) {
				glUniformBlockBinding(m_TerrainProgram, blockIndex, 1);
			}
			glUniform1i(glGetUniformLocation(m_TerrainProgram, "uTexture"), 0);

			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);

			for (const auto& cmd : drawCmds)
			{
				if (cmd.batchType == TERRAIN_BATCH_BLEND || cmd.batchType == TERRAIN_BATCH_GRASS_ADD)
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE);
					glDepthMask(GL_FALSE);
				}
				else if (cmd.batchType == TERRAIN_BATCH_ALPHA)
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glDepthMask(GL_FALSE);
				}
				else if (cmd.batchType == TERRAIN_BATCH_GRASS)
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glDepthMask(GL_TRUE);
				}
				else
				{
					glDisable(GL_BLEND);
					glDepthMask(GL_TRUE);
				}

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, cmd.textureIndex >= 0 ? Bitmaps[cmd.textureIndex].TextureNumber : 0);

				glDrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, (const void*)(cmd.indexOffset * sizeof(uint32_t)));
			}

			glUseProgram(0);
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			CachTexture = -1;
		}
	}

	for (int bType = 0; bType < TERRAIN_BATCH_COUNT; ++bType)
	{
		for (auto& [key, vec] : m_TerrainVerticesMap[bType]) vec.clear();
		for (auto& [key, vec] : m_TerrainIndicesMap[bType]) vec.clear();
	}
	m_TerrainMRU.Reset();
}

// ============================================================================
// Sprite & Particle Batching Implementation
// ============================================================================
void CBatchRenderer::AddSprite(int texture, vec3_t pos, float width, float height, vec3_t light, float Angle,
	float u, float v, float uWidth, float vHeight, float alpha, float scale, int renderFlags)
{
	SpriteInstance_t inst{};
	memcpy(inst.position, pos, sizeof(float) * 3);
	inst.size[0] = width;
	inst.size[1] = height;
	inst.rotation = Angle;
	memcpy(inst.light, light, sizeof(float) * 3);
	inst.light[3] = 1.0f;
	inst.uv[0] = u;
	inst.uv[1] = v;
	inst.uv[2] = uWidth;
	inst.uv[3] = vHeight;
	inst.alpha = alpha;
	inst.depth = pos[2];

	if (renderFlags == -1)
	{
		renderFlags = GetSpriteRenderFlags(texture, 0);
	}

	bool isTranslucent = alpha < 0.99f || (renderFlags & RENDER_ALPHA_BLEND_MASK) != 0;
	SpriteBatchKey key = { renderFlags, isTranslucent, scale, 0 };

	SpriteBatchGroup* pGroup = nullptr;
	if (m_SpriteMRU.group && m_SpriteMRU.key == key)
	{
		pGroup = m_SpriteMRU.group;
	}
	else
	{
		pGroup = &m_SpriteBatch[key];
		m_SpriteMRU.key = key;
		m_SpriteMRU.group = pGroup;
	}

	int texIdx = -1;
	for (;;) {
		for (int i = 0; i < (int)pGroup->textureIDs.size(); ++i) {
			if (pGroup->textureIDs[i] == texture) {
				texIdx = i;
				break;
			}
		}
		if (texIdx != -1) break;
		if (pGroup->textureIDs.size() < 16) {
			texIdx = (int)pGroup->textureIDs.size();
			pGroup->textureIDs.push_back(texture);
			break;
		}
		key.batchPage++;
		pGroup = &m_SpriteBatch[key];
		m_SpriteMRU.key = key;
		m_SpriteMRU.group = pGroup;
	}

	inst.textureIndex = (float)texIdx;
	pGroup->instances.push_back(inst);
}

int CBatchRenderer::GetSpriteRenderFlags(int texture, int baseFlags)
{
	int flags = baseFlags;
	if (Bitmaps[texture].Components == 3)
	{
		flags &= ~RENDER_ALPHA_BLEND_MASK;
		flags |= RENDER_ALPHA_BLEND_TYPE_ADD;
	}
	else
	{
		flags &= ~RENDER_ALPHA_BLEND_MASK;
		flags |= RENDER_ALPHA_BLEND_TYPE_NORMAL;
	}
	return flags;
}

void CBatchRenderer::RenderSpriteBatch()
{
	bool hasSprites = false;
	uint32_t totalSprites = 0;
	for (const auto& [key, group] : m_SpriteBatch)
	{
		if (!group.instances.empty())
		{
			hasSprites = true;
			totalSprites += static_cast<uint32_t>(group.instances.size());
		}
	}
	if (!hasSprites || totalSprites == 0)
		return;

	uint32_t firstInstIndex = 0;
	uint32_t instSSBOOffset = 0;
	GPUContext::GPUSpriteInstance* dstSprites = GPUContext::Instance().AllocateSpriteInstanceBuffer(totalSprites, firstInstIndex, instSSBOOffset);
	if (!dstSprites)
		return;

	std::vector<GPUContext::SpriteBatch> gpuBatches;
	uint32_t currentInstIndex = 0;

	for (auto& [key, group] : m_SpriteBatch)
	{
		if (group.instances.empty()) continue;

		const int blendType = (key.renderFlags & RENDER_ALPHA_BLEND_MASK) >> RENDER_ALPHA_BLEND_SHIFT;

		for (size_t t = 0; t < group.textureIDs.size(); ++t)
		{
			int texId = group.textureIDs[t];
			uint32_t batchFirstInst = currentInstIndex;

			for (const auto& inst : group.instances)
			{
				if ((int)inst.textureIndex != (int)t) continue;

				GPUContext::GPUSpriteInstance& s = dstSprites[currentInstIndex++];
				s.position = glm::vec4(inst.position[0], inst.position[1], inst.position[2], 0.0f);
				s.size = glm::vec2(inst.size[0] * 0.5f * key.scale, inst.size[1] * 0.5f * key.scale);
				s.rotation = inst.rotation;
				s.light = glm::vec4(inst.light[0], inst.light[1], inst.light[2], inst.light[3]);
				s.uv = glm::vec4(inst.uv[0], inst.uv[1], inst.uv[2], inst.uv[3]);
				s.alpha = inst.alpha;
				s.depth = inst.depth;
			}

			uint32_t count = currentInstIndex - batchFirstInst;
			if (count > 0)
			{
				GPUContext::SpriteBatch b;
				b.texture = texId >= 0 ? Bitmaps[texId].TextureNumber : 0;
				b.blendType = blendType;
				b.scale = key.scale;
				b.firstVertex = batchFirstInst;
				b.vertexCount = count;
				gpuBatches.push_back(b);
			}
		}
	}

	if (currentInstIndex > 0)
	{
		GPUContext::SpriteVertUBO ubo;

		glm::mat4 view = glm::mat4(1.0f);
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				view[c][r] = CameraMatrix[r][c];
			}
		}
		ubo.viewMatrix = view;

		float fov = CameraFOV > 0.0f ? CameraFOV : 35.0f;
		float aspect = (float)WindowWidth / (float)WindowHeight;
		float zNear = CameraViewNear > 0.0f ? CameraViewNear : 20.0f;
		float zFar = (CameraViewFar > 0.0f ? CameraViewFar : 4000.0f) * 1.4f;
		glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
		// Flip Y for Vulkan projection
		proj[1][1] = -proj[1][1];
		// Remap depth from OpenGL [-1,1] to Vulkan [0,1]
		proj[2][0] = (proj[2][0] + proj[3][0]) * 0.5f;
		proj[2][1] = (proj[2][1] + proj[3][1]) * 0.5f;
		proj[2][2] = (proj[2][2] + proj[3][2]) * 0.5f;
		proj[2][3] = (proj[2][3] + proj[3][3]) * 0.5f;
		ubo.projMatrix = proj;
		ubo.screenSize = glm::vec2((float)WindowWidth, (float)WindowHeight);
		ubo.scale = 1.0f;
		ubo.instanceBase = 0;

		GPUContext::Instance().DrawSpritesPreallocated(
			instSSBOOffset, (uint32_t)(currentInstIndex * sizeof(GPUContext::GPUSpriteInstance)),
			firstInstIndex, gpuBatches, ubo);
	}

	for (auto& [key, group] : m_SpriteBatch)
	{
		group.instances.clear();
		group.textureIDs.clear();
	}
	m_SpriteMRU.Reset();
}

// ============================================================================
// 2D Image / UI Batching Implementation (with AABB Lookback Merge)
// ============================================================================
void CBatchRenderer::SetScissor(bool enable, int x, int y, int w, int h)
{
	m_ScissorEnabled = enable;
	m_ScissorX = x; m_ScissorY = y; m_ScissorW = w; m_ScissorH = h;
}

void CBatchRenderer::AddImage(const ImageInstance_t& s)
{
	GPUImageInstance gi;
	gi.xywh[0] = s.x;
	gi.xywh[1] = s.y;
	gi.xywh[2] = s.width;
	gi.xywh[3] = s.height;
	gi.uvwh[0] = s.u;
	gi.uvwh[1] = s.v;
	gi.uvwh[2] = s.uWidth;
	gi.uvwh[3] = s.vHeight;

	auto clampF = [](float f) -> float { return f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f); };
	gi.color[0] = clampF(s.color[0]);
	gi.color[1] = clampF(s.color[1]);
	gi.color[2] = clampF(s.color[2]);
	gi.color[3] = clampF(s.color[3]);

	gi.extra[0] = s.rotation;
	gi.extra[1] = (s.Texture < 0) ? 1.0f : 0.0f;
	gi.extra[2] = (s.Texture >= BITMAP_GLYPH_ATLAS && s.Texture < BITMAP_GLYPH_ATLAS_END) ? 1.0f : 0.0f;
	gi.extra[3] = s.grayscale ? 1.0f : 0.0f;

	ImageBatchKey key{};
	key.layer = s.layer;
	key.Texture = s.Texture;
	key.RenderFlags = s.RenderFlags;
	key.scissorEnabled = m_ScissorEnabled;
	key.scissorX = m_ScissorX; key.scissorY = m_ScissorY;
	key.scissorW = m_ScissorW; key.scissorH = m_ScissorH;

	const float ix0 = s.x;
	const float iy0 = s.y;
	const float ix1 = s.x + s.width;
	const float iy1 = s.y + s.height;

	auto pushNewBatch = [&]() {
		m_ImageBatch.emplace_back(key, std::vector<GPUImageInstance>{});
		m_ImageBatch.back().second.reserve(64);
		m_ImageBatchBbox.push_back({ ix0, iy0, ix1, iy1 });
		m_ImageBatch.back().second.push_back(gi);
	};

	auto appendTo = [&](size_t i) {
		m_ImageBatch[i].second.push_back(gi);
		auto& bb = m_ImageBatchBbox[i];
		if (ix0 < bb.x0) bb.x0 = ix0;
		if (iy0 < bb.y0) bb.y0 = iy0;
		if (ix1 > bb.x1) bb.x1 = ix1;
		if (iy1 > bb.y1) bb.y1 = iy1;
	};

	if (!m_ImageBatch.empty() && m_ImageBatch.back().first == key) {
		appendTo(m_ImageBatch.size() - 1);
		return;
	}

	// Lookback merge: scan up to 16 previous batches
	if (s.rotation == 0.0f) {
		constexpr int kMaxLookback = 16;
		const int n = (int)m_ImageBatch.size();
		const int start = (n > kMaxLookback) ? (n - kMaxLookback) : 0;
		for (int i = n - 2; i >= start; --i) {
			if (!(m_ImageBatch[i].first == key))
				continue;
			bool blocked = false;
			for (int j = i + 1; j < n; ++j) {
				const auto& jb = m_ImageBatchBbox[j];
				if (ix0 < jb.x1 && jb.x0 < ix1 && iy0 < jb.y1 && jb.y0 < iy1) {
					blocked = true;
					break;
				}
			}
			if (!blocked) {
				appendTo(i);
				return;
			}
		}
	}

	pushNewBatch();
}

void CBatchRenderer::RenderImageBatches()
{
	FlushImageBatchesNow();
}

void CBatchRenderer::FlushImageBatchesNow()
{
	if (m_ImageBatch.empty())
		return;

	if (GPUContext::Instance().IsFrameActive())
	{
		uint32_t totalInstances = 0;
		for (auto& b : m_ImageBatch)
		{
			if (b.first.Texture < 0)
			{
				b.first.Texture = 0;
			}
			totalInstances += static_cast<uint32_t>(b.second.size());
		}

		if (totalInstances > 0)
		{
			uint32_t baseInstance = 0;
			uint32_t instanceSSBOOffset = 0;
			GPUImageInstance* dst = GPUContext::Instance().AllocateImageInstanceBuffer(totalInstances, baseInstance, instanceSSBOOffset);
			if (dst)
			{
				std::vector<GPUContext::ImageBatchRun> batchRuns;
				batchRuns.reserve(m_ImageBatch.size());

				uint32_t written = 0;
				for (const auto& b : m_ImageBatch)
				{
					uint32_t bCount = static_cast<uint32_t>(b.second.size());
					if (bCount > 0)
					{
						memcpy(dst + written, b.second.data(), bCount * sizeof(GPUImageInstance));
						written += bCount;
						batchRuns.push_back({ b.first, bCount });
					}
				}

				GPUContext::Instance().DrawImagesPreallocated(baseInstance, instanceSSBOOffset, batchRuns);
			}
		}

		m_ImageBatch.clear();
		m_ImageBatchBbox.clear();
		return;
	}

	if (m_ImageProgram == 0)
	{
		m_ImageBatch.clear();
		m_ImageBatchBbox.clear();
		return;
	}

	static std::vector<ImageQuadVertex> quadVertices;

	quadVertices.clear();

	struct ImageDrawCall {
		int textureID;
		bool scissor;
		int sx, sy, sw, sh;
		uint32_t vertexOffset;
		uint32_t vertexCount;
	};
	static std::vector<ImageDrawCall> drawCalls;
	drawCalls.clear();

	for (const auto& [key, instances] : m_ImageBatch)
	{
		if (instances.empty()) continue;

		uint32_t startVert = (uint32_t)quadVertices.size();

		for (const auto& inst : instances)
		{
			float x = inst.xywh[0];
			float y = inst.xywh[1];
			float w = inst.xywh[2];
			float h = inst.xywh[3];

			float u = inst.uvwh[0];
			float v = inst.uvwh[1];
			float uw = inst.uvwh[2];
			float vh = inst.uvwh[3];

			ImageQuadVertex v0 = { { x, y }, { u, v }, { inst.color[0], inst.color[1], inst.color[2], inst.color[3] }, { inst.extra[0], inst.extra[1], inst.extra[2], inst.extra[3] } };
			ImageQuadVertex v1 = { { x + w, y }, { u + uw, v }, { inst.color[0], inst.color[1], inst.color[2], inst.color[3] }, { inst.extra[0], inst.extra[1], inst.extra[2], inst.extra[3] } };
			ImageQuadVertex v2 = { { x + w, y + h }, { u + uw, v + vh }, { inst.color[0], inst.color[1], inst.color[2], inst.color[3] }, { inst.extra[0], inst.extra[1], inst.extra[2], inst.extra[3] } };
			ImageQuadVertex v3 = { { x, y + h }, { u, v + vh }, { inst.color[0], inst.color[1], inst.color[2], inst.color[3] }, { inst.extra[0], inst.extra[1], inst.extra[2], inst.extra[3] } };

			quadVertices.push_back(v0);
			quadVertices.push_back(v1);
			quadVertices.push_back(v2);
			quadVertices.push_back(v0);
			quadVertices.push_back(v2);
			quadVertices.push_back(v3);
		}

		uint32_t count = (uint32_t)quadVertices.size() - startVert;
		if (count > 0)
		{
			ImageDrawCall dc;
			dc.textureID = key.Texture;
			dc.scissor = key.scissorEnabled;
			dc.sx = key.scissorX; dc.sy = key.scissorY; dc.sw = key.scissorW; dc.sh = key.scissorH;
			dc.vertexOffset = startVert;
			dc.vertexCount = count;
			drawCalls.push_back(dc);
		}
	}

	if (!quadVertices.empty())
	{
		// 2D Orthographic projection
		glm::mat4 orthoProj = glm::ortho(0.0f, (float)WindowWidth, (float)WindowHeight, 0.0f, -1.0f, 1.0f);

		ImageVertUBO ubo;
		ubo.projMatrix = orthoProj;
		ubo.screenSize = glm::vec2((float)WindowWidth, (float)WindowHeight);

		glBindBuffer(GL_UNIFORM_BUFFER, m_ImageUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ImageVertUBO), &ubo);

		glBindVertexArray(m_ImageVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_ImageVBO);
		glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(ImageQuadVertex), quadVertices.data(), GL_STREAM_DRAW);

		glUseProgram(m_ImageProgram);
		GLuint blockIndex = glGetUniformBlockIndex(m_ImageProgram, "ImageBlock");
		if (blockIndex != GL_INVALID_INDEX) {
			glUniformBlockBinding(m_ImageProgram, blockIndex, 3);
		}
		glUniform1i(glGetUniformLocation(m_ImageProgram, "uTexture"), 0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		for (const auto& dc : drawCalls)
		{
			if (dc.scissor) {
				glEnable(GL_SCISSOR_TEST);
				glScissor(dc.sx, dc.sy, dc.sw, dc.sh);
			} else {
				glDisable(GL_SCISSOR_TEST);
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, dc.textureID >= 0 ? Bitmaps[dc.textureID].TextureNumber : 0);

			glDrawArrays(GL_TRIANGLES, dc.vertexOffset, dc.vertexCount);
		}

		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glUseProgram(0);
		glBindVertexArray(0);
	}

	m_ImageBatch.clear();
	m_ImageBatchBbox.clear();
}

// ============================================================================
// 3D Model / Mesh & Shadow Batching Implementation
// ============================================================================
uint32_t CBatchRenderer::FindMeshProxy(const MeshBatchKey& key, Mesh_t* mesh, int bucket)
{
	MeshProxyKey pk{ key, bucket };
	auto it = m_ProxyMap.find(pk);
	if (it != m_ProxyMap.end())
		return it->second;

	if (m_Proxies.size() >= MAX_MESH_PROXY)
		return (uint32_t)-1;

	uint32_t index = (uint32_t)m_Proxies.size();
	MeshProxy p;
	p.key = key;
	p.mesh = mesh;
	p.bucket = bucket;
	p.enabled = true;
	p.instanceCount = 0;
	m_Proxies.push_back(p);
	m_ProxyMap.emplace(pk, index);
	m_ProxyPending.resize(m_Proxies.size());
	return index;
}

uint32_t CBatchRenderer::FindShadowProxy(const MeshBatchKey& key, Mesh_t* mesh)
{
	auto it = m_ShadowProxyMap.find(key);
	if (it != m_ShadowProxyMap.end())
		return it->second;

	if (m_ShadowProxies.size() >= MAX_MESH_PROXY)
		return (uint32_t)-1;

	uint32_t index = (uint32_t)m_ShadowProxies.size();
	ShadowProxy p;
	p.key = key;
	p.mesh = mesh;
	p.enabled = true;
	p.instanceCount = 0;
	m_ShadowProxies.push_back(p);
	m_ShadowProxyMap.emplace(key, index);
	m_ShadowPending.resize(m_ShadowProxies.size());
	return index;
}

void CBatchRenderer::AddMeshToBatch(const MeshBatchKey& key, Mesh_t* mesh,
	const MeshInstanceData_t& instanceTemplate,
	int boneOffset, vec3_t ShadowAngle, vec3_t LightVector, float depth, int meshIndex)
{
	if (boneOffset < 0) return;

	int bucket = MESH_BUCKET_OPAQUE;
	if (depth < -99998) bucket = MESH_BUCKET_HIGHLIGHT;
	else if (instanceTemplate.alpha < 0.99f) bucket = MESH_BUCKET_ALPHA_BLEND;

	uint32_t proxyIndex = FindMeshProxy(key, mesh, bucket);
	if (proxyIndex == (uint32_t)-1) return;

	MeshProxy& proxy = m_Proxies[proxyIndex];
	proxy.mesh = mesh;
	proxy.enabled = true;
	VectorCopy(LightVector, proxy.LightVector);
	VectorCopy(ShadowAngle, proxy.ShadowAngle);

	MeshInstanceData_t inst = instanceTemplate;
	inst.boneOffset = boneOffset;

	m_ProxyPending[proxyIndex].push_back(inst);
	++proxy.instanceCount;
}

void CBatchRenderer::AddShadowToBatch(const MeshBatchKey& key, Mesh_t* mesh,
	const MeshInstanceData_t& instanceTemplate,
	int boneOffset, float depth, int meshIndex)
{
	if (boneOffset < 0) return;

	uint32_t proxyIndex = FindShadowProxy(key, mesh);
	if (proxyIndex == (uint32_t)-1) return;

	ShadowProxy& proxy = m_ShadowProxies[proxyIndex];
	proxy.mesh = mesh;
	proxy.enabled = true;

	MeshInstanceData_t inst = instanceTemplate;
	inst.boneOffset = boneOffset;

	m_ShadowPending[proxyIndex].push_back(inst);
	++proxy.instanceCount;
	m_ShadowBatchDirty = true;
}

void CBatchRenderer::RenderMeshBatch(bool clear)
{
	// Delegate to OGL330MODEL command queue flush
	GMMeshShader->FlushAllMesh();

	if (WorldTime - m_LastProxyPruneTime > 60000.0)
	{
		ClearMeshProxies();
		m_LastProxyPruneTime = WorldTime;
	}
	else
	{
		for (size_t i = 0; i < m_Proxies.size(); ++i) {
			m_Proxies[i].enabled = false;
			m_Proxies[i].instanceCount = 0;
			m_ProxyPending[i].clear();
		}
		for (size_t i = 0; i < m_ShadowProxies.size(); ++i) {
			m_ShadowProxies[i].enabled = false;
			m_ShadowProxies[i].instanceCount = 0;
			m_ShadowPending[i].clear();
		}
	}
	m_ShadowBatchDirty = false;
}

void CBatchRenderer::FlushShadowBatch()
{
	m_ShadowBatchDirty = false;
}

void CBatchRenderer::ClearMeshProxies()
{
	m_Proxies.clear();
	m_ProxyMap.clear();
	m_ActiveProxies.clear();
	m_ProxyPending.clear();

	m_ShadowProxies.clear();
	m_ShadowProxyMap.clear();
	m_ActiveShadowProxies.clear();
	m_ShadowPending.clear();

	m_ShadowBatchDirty = false;
}

void CBatchRenderer::ClearAllBatchMaps()
{
	ClearMeshProxies();
	m_SpriteBatch.clear();
	m_ImageBatch.clear();
	m_ImageBatchBbox.clear();
	for (int b = 0; b < TERRAIN_BATCH_COUNT; ++b) {
		m_TerrainVerticesMap[b].clear();
		m_TerrainIndicesMap[b].clear();
	}
	m_TerrainMRU.Reset();
	m_SpriteMRU.Reset();
	m_LastSpritePruneTime = WorldTime;
	m_LastProxyPruneTime = WorldTime;
}

// ============================================================================
// Frame Orchestration
// ============================================================================
void CBatchRenderer::FlushAllBatches()
{
	FlushTerrainBatches();
	RenderMeshBatch();
	FlushShadowBatch();
	RenderSpriteBatch();
	FlushImageBatchesNow();
}
