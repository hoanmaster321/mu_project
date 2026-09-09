#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

// Forward declarations
struct _Mesh_t;
typedef struct _Mesh_t Mesh_t;
class BMD;

// ============================================================================
// Render State & Flag Definitions
// ============================================================================
#define RENDER_ALPHA_BLEND_MASK        0x00000038
#define RENDER_ALPHA_BLEND_SHIFT       3
#define RENDER_ALPHA_BLEND_TYPE_NONE   (0 << RENDER_ALPHA_BLEND_SHIFT)
#define RENDER_ALPHA_BLEND_TYPE_NORMAL (1 << RENDER_ALPHA_BLEND_SHIFT)
#define RENDER_ALPHA_BLEND_TYPE_ADD    (2 << RENDER_ALPHA_BLEND_SHIFT)
#define RENDER_ALPHA_BLEND_TYPE_SUB    (3 << RENDER_ALPHA_BLEND_SHIFT)

#define RENDER_DEPTH_MASK_ENABLE       0x00000040
#define RENDER_DEPTH_MASK_DISABLE      0x00000080
#define RENDER_CULL_FACE_ENABLE        0x00000100
#define RENDER_TILE_REPEAT             0x00000200
#define RENDER_ALPHA_TEST_ENABLE       0x00000400

#define GET_BLEND_TYPE(flags) (((flags) & RENDER_ALPHA_BLEND_MASK) >> RENDER_ALPHA_BLEND_SHIFT)

// ============================================================================
// Terrain Batching Definitions
// ============================================================================
enum TerrainBatchType
{
	TERRAIN_BATCH_OPAQUE = 0,
	TERRAIN_BATCH_BLEND,
	TERRAIN_BATCH_ALPHA,
	TERRAIN_BATCH_GRASS,
	TERRAIN_BATCH_GRASS_ADD,
	TERRAIN_BATCH_COUNT
};

struct TerrainVertex_t
{
	float pos[3];
	float uv[2];
	uint32_t color; // Packed RGBA8
};

static inline uint32_t PackRGBA8(float r, float g, float b, float a)
{
	uint8_t ur = (uint8_t)(std::min)(255.0f, (std::max)(0.0f, r * 255.0f));
	uint8_t ug = (uint8_t)(std::min)(255.0f, (std::max)(0.0f, g * 255.0f));
	uint8_t ub = (uint8_t)(std::min)(255.0f, (std::max)(0.0f, b * 255.0f));
	uint8_t ua = (uint8_t)(std::min)(255.0f, (std::max)(0.0f, a * 255.0f));
	return (uint32_t)ur | ((uint32_t)ug << 8) | ((uint32_t)ub << 16) | ((uint32_t)ua << 24);
}

struct TerrainBatchKey
{
	uint32_t renderFlags;
	int textureIndex;

	bool operator==(const TerrainBatchKey& o) const
	{
		return renderFlags == o.renderFlags && textureIndex == o.textureIndex;
	}
};

namespace std
{
	template<>
	struct hash<TerrainBatchKey>
	{
		std::size_t operator()(const TerrainBatchKey& k) const noexcept
		{
			return std::hash<uint32_t>()(k.renderFlags) ^ (std::hash<int>()(k.textureIndex) << 1);
		}
	};
}

struct GPUDynamicPointLight
{
	glm::vec4 posRadius;      // xyz = world pos, w = radius
	glm::vec4 colorIntensity; // rgb = light color, w = intensity
};

struct TerrainVertUBO
{
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	GPUDynamicPointLight lights[32];
	uint32_t numLights;
	uint32_t pad[3];

	TerrainVertUBO() {
		memset(this, 0, sizeof(*this));
	}
};

// ============================================================================
// Sprite & Particle Batching Definitions
// ============================================================================
struct SpriteInstance_t
{
	float position[3];
	float size[2];
	float rotation;
	float light[4];
	float uv[4]; // u, v, uWidth, vHeight
	float alpha;
	float depth;
	float textureIndex;
};

struct SpriteBatchKey
{
	int renderFlags;
	bool isTranslucent;
	float scale;
	int batchPage;

	bool operator==(const SpriteBatchKey& o) const
	{
		return renderFlags == o.renderFlags &&
			isTranslucent == o.isTranslucent &&
			scale == o.scale &&
			batchPage == o.batchPage;
	}
};

namespace std
{
	template<>
	struct hash<SpriteBatchKey>
	{
		std::size_t operator()(const SpriteBatchKey& k) const noexcept
		{
			std::size_t h1 = std::hash<int>()(k.renderFlags);
			std::size_t h2 = std::hash<bool>()(k.isTranslucent);
			std::size_t h3 = std::hash<float>()(k.scale);
			std::size_t h4 = std::hash<int>()(k.batchPage);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
		}
	};
}

struct SpriteBatchGroup
{
	std::vector<SpriteInstance_t> instances;
	std::vector<int> textureIDs;
};

struct SpriteVertUBO
{
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	glm::vec2 screenSize;
	float padding[2];
};

// ============================================================================
// 2D Image / UI Batching Definitions
// ============================================================================
#define BITMAP_GLYPH_ATLAS     50000
#define BITMAP_GLYPH_ATLAS_END 60000

struct ImageInstance_t
{
	int Texture;
	float x, y, width, height;
	float u, v, uWidth, vHeight;
	float color[4];
	float rotation;
	int layer;
	int RenderFlags;
	bool grayscale;
};

struct GPUImageInstance
{
	float xywh[4];  // x, y, width, height
	float uvwh[4];  // u, v, uWidth, vHeight
	float color[4]; // r, g, b, a
	float extra[4]; // [0]=rotation, [1]=(Texture<0?1:0), [2]=alphaOnly, [3]=grayscale
};

struct ImageBatchKey
{
	int layer;
	int Texture;
	int RenderFlags;
	bool scissorEnabled;
	int scissorX, scissorY, scissorW, scissorH;

	bool operator==(const ImageBatchKey& o) const
	{
		return layer == o.layer &&
			Texture == o.Texture &&
			RenderFlags == o.RenderFlags &&
			scissorEnabled == o.scissorEnabled &&
			scissorX == o.scissorX && scissorY == o.scissorY &&
			scissorW == o.scissorW && scissorH == o.scissorH;
	}
};

struct ImageBatchBbox
{
	float x0, y0, x1, y1;
};

struct ImageVertUBO
{
	glm::mat4 projMatrix;
	glm::vec2 screenSize;
	float padding[2];
};

// ============================================================================
// 3D Mesh & Shadow Batching Definitions
// ============================================================================
enum MeshBucketType
{
	MESH_BUCKET_HIGHLIGHT = 0,
	MESH_BUCKET_OPAQUE,
	MESH_BUCKET_CHROME,
	MESH_BUCKET_METAL,
	MESH_BUCKET_ALPHA_TEST,
	MESH_BUCKET_COLOR,
	MESH_BUCKET_ALPHA_BLEND,
	MESH_BUCKET_BRIGHT,
	MESH_BUCKET_COUNT
};

#define MAX_MESH_PROXY 4096

struct MeshInstanceData_t
{
	int boneOffset;
	uint32_t sourceIndex;
	float alpha;
	float bodyLight[4];
	float lightPosition[4];
};

struct MeshBatchKey
{
	int textureID;
	int renderFlags;
	int renderFlagsOrigin;

	bool operator==(const MeshBatchKey& o) const
	{
		return textureID == o.textureID &&
			renderFlags == o.renderFlags &&
			renderFlagsOrigin == o.renderFlagsOrigin;
	}
};

struct MeshProxyKey
{
	MeshBatchKey key;
	int bucket;

	bool operator==(const MeshProxyKey& o) const
	{
		return key == o.key && bucket == o.bucket;
	}
};

namespace std
{
	template<>
	struct hash<MeshProxyKey>
	{
		std::size_t operator()(const MeshProxyKey& pk) const noexcept
		{
			std::size_t h1 = std::hash<int>()(pk.key.textureID);
			std::size_t h2 = std::hash<int>()(pk.key.renderFlags);
			std::size_t h3 = std::hash<int>()(pk.bucket);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	template<>
	struct hash<MeshBatchKey>
	{
		std::size_t operator()(const MeshBatchKey& k) const noexcept
		{
			return std::hash<int>()(k.textureID) ^ (std::hash<int>()(k.renderFlags) << 1);
		}
	};
}

struct MeshProxy
{
	MeshBatchKey key;
	Mesh_t* mesh;
	int bucket;
	bool enabled;
	uint32_t instanceCount;
	vec3_t LightVector;
	vec3_t ShadowAngle;
};

struct ShadowProxy
{
	MeshBatchKey key;
	Mesh_t* mesh;
	bool enabled;
	uint32_t instanceCount;
};

struct GeneralVertUBO
{
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	glm::vec3 shadowAngle;
	float worldTime;
	glm::vec3 lightVector;
	uint32_t instanceBase;
};

struct GeneralFragUBO
{
	float batchTexture;
	float brightness;
	float alphaTestThreshold;
	float padding;
};

struct ShadowVertUBO
{
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
};

// ============================================================================
// BatchRenderer Class Interface
// ============================================================================
class CBatchRenderer
{
public:
	CBatchRenderer();
	~CBatchRenderer();

	static CBatchRenderer& Instance();

	bool Initialize();
	void Shutdown();

	// Terrain Batching
	void AddTerrainFaceToBatch(int batchType, int textureIndex, int renderFlags);
	void AddTerrainCustomQuad(int batchType, int textureIndex, int renderFlags, const vec3_t verts[4], const vec3_t uvs[4], const vec4_t colors[4]);
	void AddTerrainQuadsDirect(int batchType, int textureIndex, int renderFlags, const float* vData, int quadCount);
	void FlushTerrainBatches();

	// Sprite & Particle Batching
	void AddSprite(int texture, vec3_t pos, float width, float height, vec3_t light, float Angle,
		float u = 0.0f, float v = 0.0f, float uWidth = 1.0f, float vHeight = 1.0f, float alpha = 1.0f, float scale = 1.0f, int renderFlags = -1);
	int  GetSpriteRenderFlags(int texture, int baseFlags);
	void RenderSpriteBatch();

	// 2D Image / UI Batching
	void AddImage(const ImageInstance_t& s);
	void RenderImageBatches();
	void FlushImageBatchesNow();

	// 3D Model / Mesh & Shadow Batching
	void AddMeshTriangles(int batchType, int textureIndex, int renderFlags, const TerrainVertex_t* verts, uint32_t vertCount);
	void FlushMeshBatches();
	void AddMeshToBatch(const MeshBatchKey& key, Mesh_t* mesh,
		const MeshInstanceData_t& instanceTemplate,
		int boneOffset, vec3_t ShadowAngle, vec3_t LightVector, float depth, int meshIndex);
	void RenderMeshBatch(bool clear = true);
	void AddShadowToBatch(const MeshBatchKey& key, Mesh_t* mesh,
		const MeshInstanceData_t& instanceTemplate,
		int boneOffset, float depth, int meshIndex);
	void FlushShadowBatch();

	// Frame Orchestration
	void FlushAllBatches();
	void ClearAllBatchMaps();

	// Scissor management for UI
	void SetScissor(bool enable, int x = 0, int y = 0, int w = 0, int h = 0);

private:
	uint32_t FindMeshProxy(const MeshBatchKey& key, Mesh_t* mesh, int bucket);
	uint32_t FindShadowProxy(const MeshBatchKey& key, Mesh_t* mesh);
	void ClearMeshProxies();

	// Shaders
	uint32_t m_TerrainProgram;
	uint32_t m_SpriteProgram;
	uint32_t m_ImageProgram;

	// GL 4.3 State
	bool m_GL43Supported;

	// Terrain Buffers
	uint32_t m_TerrainVAO;
	uint32_t m_TerrainVBO;
	uint32_t m_TerrainIBO;
	uint32_t m_TerrainUBO;
	struct TerrainBatchData
	{
		std::vector<TerrainVertex_t> vertices;
		std::vector<uint32_t> indices;
	};
	std::unordered_map<TerrainBatchKey, TerrainBatchData> m_TerrainBatchesMap[TERRAIN_BATCH_COUNT];

	struct MeshBatchData
	{
		std::vector<TerrainVertex_t> vertices;
	};
	std::unordered_map<TerrainBatchKey, MeshBatchData> m_MeshBatchesMap[TERRAIN_BATCH_COUNT];

	struct TerrainMRUCache
	{
		int batchType = -1;
		TerrainBatchKey key = { 0xFFFFFFFF, -1 };
		TerrainBatchData* batch = nullptr;

		void Reset()
		{
			batchType = -1;
			key = { 0xFFFFFFFF, -1 };
			batch = nullptr;
		}
	} m_TerrainMRU;

	struct MeshMRUCache
	{
		int batchType = -1;
		TerrainBatchKey key = { 0xFFFFFFFF, -1 };
		MeshBatchData* batch = nullptr;

		void Reset()
		{
			batchType = -1;
			key = { 0xFFFFFFFF, -1 };
			batch = nullptr;
		}
	} m_MeshMRU;

	// Sprite Buffers
	uint32_t m_SpriteVAO;
	uint32_t m_SpriteVBO;
	uint32_t m_SpriteUBO;
	std::unordered_map<SpriteBatchKey, SpriteBatchGroup> m_SpriteBatch;
	double m_LastSpritePruneTime;

	struct SpriteMRUCache
	{
		SpriteBatchKey key = { -1, false, 0.0f, -1 };
		SpriteBatchGroup* group = nullptr;

		void Reset()
		{
			key = { -1, false, 0.0f, -1 };
			group = nullptr;
		}
	} m_SpriteMRU;

	// Image Buffers
	uint32_t m_ImageVAO;
	uint32_t m_ImageVBO;
	uint32_t m_ImageUBO;
	std::vector<std::pair<ImageBatchKey, std::vector<GPUImageInstance>>> m_ImageBatch;
	std::vector<ImageBatchBbox> m_ImageBatchBbox;
	bool m_ScissorEnabled;
	int m_ScissorX, m_ScissorY, m_ScissorW, m_ScissorH;

	// Mesh & Shadow Proxy Buffers
	std::vector<MeshProxy> m_Proxies;
	std::unordered_map<MeshProxyKey, uint32_t> m_ProxyMap;
	std::vector<uint32_t> m_ActiveProxies;
	std::vector<std::vector<MeshInstanceData_t>> m_ProxyPending;

	std::vector<ShadowProxy> m_ShadowProxies;
	std::unordered_map<MeshBatchKey, uint32_t> m_ShadowProxyMap;
	std::vector<uint32_t> m_ActiveShadowProxies;
	std::vector<std::vector<MeshInstanceData_t>> m_ShadowPending;

	bool m_TerrainBatchDirty = false;
	bool m_MeshBatchDirty = false;
	bool m_SpriteBatchDirty = false;
	bool m_ShadowBatchDirty = false;
	double m_LastProxyPruneTime;
	bool m_Initialized;
};

#define g_BatchRenderer (CBatchRenderer::Instance())
