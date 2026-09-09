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
	static inline uint32_t PackRGBA8Helper(float r, float g, float b, float a)
	{
		auto q = [](float v) -> uint32_t {
			v = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
			return (uint32_t)(v * 255.f + 0.5f);
		};
		return q(r) | (q(g) << 8) | (q(b) << 16) | (q(a) << 24);
	}
}

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
	m_Initialized = true;
	return true;
}

void CBatchRenderer::Shutdown()
{
	ClearAllBatchMaps();
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
	m_TerrainBatchDirty = true;
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
	m_TerrainBatchDirty = true;
}

void CBatchRenderer::FlushTerrainBatches()
{
	if (!m_TerrainBatchDirty)
		return;

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


	for (int bType = 0; bType < TERRAIN_BATCH_COUNT; ++bType)
	{
		for (auto& [key, vec] : m_TerrainVerticesMap[bType]) vec.clear();
		for (auto& [key, vec] : m_TerrainIndicesMap[bType]) vec.clear();
	}
	m_TerrainMRU.Reset();
	m_TerrainBatchDirty = false;
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
	m_SpriteBatchDirty = true;
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
	if (!m_SpriteBatchDirty)
		return;

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
	m_SpriteBatchDirty = false;
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

void CBatchRenderer::AddMeshTriangles(int batchType, int textureIndex, int renderFlags, const TerrainVertex_t* verts, uint32_t vertCount)
{
	if (batchType < 0 || batchType >= TERRAIN_BATCH_COUNT || !verts || vertCount == 0)
		return;

	TerrainBatchKey key = { (uint32_t)renderFlags, textureIndex };

	MeshBatchData* pBatch = nullptr;

	if (m_MeshMRU.batchType == batchType && m_MeshMRU.key == key && m_MeshMRU.batch)
	{
		pBatch = m_MeshMRU.batch;
	}
	else
	{
		pBatch = &m_MeshBatchesMap[batchType][key];
		m_MeshMRU.batchType = batchType;
		m_MeshMRU.key = key;
		m_MeshMRU.batch = pBatch;
	}

	uint32_t base = (uint32_t)pBatch->vertices.size();
	pBatch->vertices.insert(pBatch->vertices.end(), verts, verts + vertCount);

	size_t oldIdxSize = pBatch->indices.size();
	pBatch->indices.resize(oldIdxSize + vertCount);
	uint32_t* dstIdx = pBatch->indices.data() + oldIdxSize;
	for (uint32_t k = 0; k < vertCount; ++k)
	{
		dstIdx[k] = base + k;
	}
	m_MeshBatchDirty = true;
}

void CBatchRenderer::FlushMeshBatches()
{
	if (!m_MeshBatchDirty)
		return;

	struct MeshDrawCmd {
		int textureIndex;
		uint32_t indexOffset;
		uint32_t indexCount;
		int batchType;
		uint32_t renderFlags;
	};
	static std::vector<MeshDrawCmd> drawCmds;
	drawCmds.clear();

	if (GPUContext::Instance().IsFrameActive())
	{
		uint32_t totalVerts = 0;
		uint32_t totalIndices = 0;
		for (int bType = 0; bType < TERRAIN_BATCH_COUNT; ++bType)
		{
			for (const auto& [key, batch] : m_MeshBatchesMap[bType])
			{
				if (!batch.vertices.empty() && !batch.indices.empty())
				{
					totalVerts += static_cast<uint32_t>(batch.vertices.size());
					totalIndices += static_cast<uint32_t>(batch.indices.size());
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
					for (const auto& [key, batch] : m_MeshBatchesMap[bType])
					{
						if (batch.vertices.empty() || batch.indices.empty()) continue;

						uint32_t baseVertex = curVertCount;
						uint32_t baseIndex = curIdxCount;

						memcpy(dstVerts + curVertCount, batch.vertices.data(), batch.vertices.size() * sizeof(TerrainVertex_t));
						curVertCount += static_cast<uint32_t>(batch.vertices.size());

						for (size_t i = 0; i < batch.indices.size(); ++i)
						{
							dstIndices[curIdxCount++] = baseVertex + batch.indices[i];
						}

						MeshDrawCmd cmd;
						cmd.textureIndex = key.textureIndex;
						cmd.indexOffset = baseIndex;
						cmd.indexCount = static_cast<uint32_t>(batch.indices.size());
						cmd.batchType = bType;
						cmd.renderFlags = key.renderFlags;
						drawCmds.push_back(cmd);
					}
				}

				GPUContext::TerrainVertUBO vkUbo;
				GetActiveViewMatrix(&vkUbo.viewMatrix[0][0]);
				GetActiveProjectionMatrix(&vkUbo.projMatrix[0][0]);

				// Invert row 1 (Y) for Vulkan NDC
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
				for (uint32_t li = 0; li < numLights; ++li) {
					vkUbo.lights[li] = dynLights[li];
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
						: (cmd.textureIndex < 0 ? -cmd.textureIndex : (cmd.textureIndex > 0 ? cmd.textureIndex : 0));
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

	for (int bType = 0; bType < TERRAIN_BATCH_COUNT; ++bType)
	{
		for (auto& [key, batch] : m_MeshBatchesMap[bType])
		{
			batch.vertices.clear();
			batch.indices.clear();
		}
	}
	m_MeshMRU.Reset();
	m_MeshBatchDirty = false;
}

void CBatchRenderer::RenderMeshBatch(bool clear)
{
	FlushMeshBatches();

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
		m_MeshBatchesMap[b].clear();
	}
	m_TerrainMRU.Reset();
	m_MeshMRU.Reset();
	m_SpriteMRU.Reset();
	m_TerrainBatchDirty = false;
	m_MeshBatchDirty = false;
	m_SpriteBatchDirty = false;
	m_ShadowBatchDirty = false;
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
