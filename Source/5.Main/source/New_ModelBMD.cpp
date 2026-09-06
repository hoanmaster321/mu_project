#include "StdAfx.h"
#include "New_ModelBMD.h"
#include "CBMu/CBMu_RenderConfig.h"

#if CB_SHADER330_TEST
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include "ZzzBMD.h"
#include "ZzzObject.h"
#include "New_RenderBMD.h"
#include "CBMu/Render/CBMu_CPUHotspotProfiler.h"
#include "CBMu/Render/CBMu_GLObjectDebugLog.h"
#include "ZzzScene.h"
#include "ZzzOpenglUtil.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "MU_OpenGL.h"
#include "GlobalBitmap.h"
#include "Util.h"
#include "Shaders330.h"
#include "Utilities/Log/muConsoleDebug.h"
#include "MapManager.h"
#include "UniformLocationCache.h"

#ifndef BITMAP_HQSKIN
#define BITMAP_HQSKIN   (BITMAP_HAIR_END + 1)
#define BITMAP_HQSKIN2  (BITMAP_HAIR_END + 2)
#define BITMAP_HQSKIN3  (BITMAP_HAIR_END + 3)
#define BITMAP_HQHAIR   (BITMAP_HAIR_END + 4)
#endif

using namespace OGL330;
extern int WaterTextureNumber;
extern float g_ProjectionMatrix[16];
extern float g_ViewMatrix[16];
namespace OGL330MODEL
{
	std::unordered_map<int, GLuint> shaderProgramMap;
	GLuint g_CurrentShaderID = 0;
#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
	static int g_CBMuGoldenOverlayPassDepth = 0;

	void PushGoldenOverlayPass()
	{
		++g_CBMuGoldenOverlayPassDepth;
	}

	void PopGoldenOverlayPass()
	{
		if (g_CBMuGoldenOverlayPassDepth > 0)
		{
			--g_CBMuGoldenOverlayPassDepth;
		}
	}
#endif
#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
	// CBMu_BEGIN: Giữ bộ overlay wearable trong lúc producer tạo command BMD body.
	static BmdCompositeOverlay g_activeBmdCompositeOverlays[3];
	static int g_activeBmdCompositeOverlayCount = 0;

	void PushBmdCompositeOverlays(const BmdCompositeOverlay* overlays, int count)
	{
		g_activeBmdCompositeOverlayCount = 0;
		if (overlays == NULL || count <= 0)
		{
			return;
		}

		const int copyCount = count > 3 ? 3 : count;
		for (int i = 0; i < copyCount; ++i)
		{
			g_activeBmdCompositeOverlays[i] = overlays[i];
		}
		g_activeBmdCompositeOverlayCount = copyCount;
	}

	void PopBmdCompositeOverlays()
	{
		g_activeBmdCompositeOverlayCount = 0;
	}

	// CBMu_END: Giữ bộ overlay wearable trong lúc producer tạo command BMD body.
#endif
}


#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
namespace
{
	using OGL330MODEL::CBMu_BMDPackedBoneRejectReason;
	using OGL330MODEL::CBMu_BMD_PACKED_BONE_REJECT_DEFER_INACTIVE;
	using OGL330MODEL::CBMu_BMD_PACKED_BONE_REJECT_EMPTY_BONE_CONTAINER;
	using OGL330MODEL::CBMu_BMD_PACKED_BONE_REJECT_INVALID_BONE_INDEX;
	using OGL330MODEL::CBMu_BMD_PACKED_BONE_REJECT_NO_BONE_POINTER;
	using OGL330MODEL::CBMu_BMD_PACKED_BONE_REJECT_NONE;
	using OGL330MODEL::CBMu_BMD_PACKED_BONE_REJECT_ROW_OVERFLOW;
	using OGL330MODEL::CBMu_BMD_PACKED_BONE_REJECT_UNKNOWN;

	static unsigned int CBMu_FloatBits(float value)
	{
		unsigned int bits = 0;
		memcpy(&bits, &value, sizeof(bits));
		return bits;
	}

	static std::size_t CBMu_HashCombine(std::size_t seed, std::size_t value)
	{
		return seed ^ (value + 0x9e3779b9u + (seed << 6) + (seed >> 2));
	}

	static bool CBMu_IsLegacyScratchBoneBuffer(const float* bone)
	{
		return bone == &BoneTransform[0][0][0];
	}

#if CBMu_ENABLE_GL_BMD_TRANSFORMED_BONE_ROW_CACHE
	struct CBMu_BMDTransformedBoneRowsCacheKey
	{
		std::uintptr_t ModelAddress;
		std::uintptr_t BoneAddress;
		int BoneCount;
		unsigned int Flags;
		unsigned int ScaleBits;
		unsigned int RequestScaleBits;
		unsigned int OriginBits[3];

		bool operator==(const CBMu_BMDTransformedBoneRowsCacheKey& rhs) const
		{
			return ModelAddress == rhs.ModelAddress &&
				BoneAddress == rhs.BoneAddress &&
				BoneCount == rhs.BoneCount &&
				Flags == rhs.Flags &&
				ScaleBits == rhs.ScaleBits &&
				RequestScaleBits == rhs.RequestScaleBits &&
				OriginBits[0] == rhs.OriginBits[0] &&
				OriginBits[1] == rhs.OriginBits[1] &&
				OriginBits[2] == rhs.OriginBits[2];
		}
	};

	struct CBMu_BMDTransformedBoneRowsCacheKeyHash
	{
		std::size_t operator()(const CBMu_BMDTransformedBoneRowsCacheKey& key) const
		{
			std::size_t hash = std::hash<std::uintptr_t>()(key.ModelAddress);
			hash = CBMu_HashCombine(hash, std::hash<std::uintptr_t>()(key.BoneAddress));
			hash = CBMu_HashCombine(hash, std::hash<int>()(key.BoneCount));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.Flags));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.ScaleBits));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.RequestScaleBits));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.OriginBits[0]));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.OriginBits[1]));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.OriginBits[2]));
			return hash;
		}
	};

#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
	static std::unordered_map<CBMu_BMDTransformedBoneRowsCacheKey, std::vector<float>, CBMu_BMDTransformedBoneRowsCacheKeyHash> s_CBMu_BMDTransformedBoneRowsCache;
#else
	static std::unordered_map<CBMu_BMDTransformedBoneRowsCacheKey, std::shared_ptr<std::vector<float> >, CBMu_BMDTransformedBoneRowsCacheKeyHash> s_CBMu_BMDTransformedBoneRowsCache;
#endif

	static void CBMu_ClearBMDTransformedBoneRowsCache()
	{
		s_CBMu_BMDTransformedBoneRowsCache.clear();
	}

	static const std::vector<float>* CBMu_GetBMDTransformedBoneRowsCached(BMD* model, const float* bone, bool transEnabled, vec3_t transValue, float scale, bool applyScale, float requestScale)
	{
		if (!model || !bone || CBMu_IsLegacyScratchBoneBuffer(bone))
		{
			return NULL;
		}

		float resultScale = transEnabled ? scale : 1.0f;
		vec3_t trans = { 0.f, 0.f, 0.f };
		if (transEnabled)
		{
			VectorCopy(transValue, trans);
		}

		const float preTransScale = resultScale;
		if (applyScale)
		{
			resultScale = requestScale * resultScale;
		}
		const float translateScale = applyScale ? preTransScale : resultScale;

		if (s_CBMu_BMDTransformedBoneRowsCache.bucket_count() == 0)
		{
			// CBMu: Cache này sống trong một flush BMD, đủ lớn cho pass đông NPC nhưng không giữ qua frame.
			s_CBMu_BMDTransformedBoneRowsCache.reserve(256);
		}

		const int boneCount = MAX_BONES;
		CBMu_BMDTransformedBoneRowsCacheKey key = {};
		key.ModelAddress = reinterpret_cast<std::uintptr_t>(model);
		key.BoneAddress = reinterpret_cast<std::uintptr_t>(bone);
		key.BoneCount = boneCount;
		key.Flags = (transEnabled ? 1u : 0u) | (applyScale ? 2u : 0u);
		key.ScaleBits = CBMu_FloatBits(scale);
		key.RequestScaleBits = CBMu_FloatBits(requestScale);
		key.OriginBits[0] = CBMu_FloatBits(trans[0]);
		key.OriginBits[1] = CBMu_FloatBits(trans[1]);
		key.OriginBits[2] = CBMu_FloatBits(trans[2]);

#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
		std::unordered_map<CBMu_BMDTransformedBoneRowsCacheKey, std::vector<float>, CBMu_BMDTransformedBoneRowsCacheKeyHash>::iterator found = s_CBMu_BMDTransformedBoneRowsCache.find(key);
#else
		std::unordered_map<CBMu_BMDTransformedBoneRowsCacheKey, std::shared_ptr<std::vector<float> >, CBMu_BMDTransformedBoneRowsCacheKeyHash>::iterator found = s_CBMu_BMDTransformedBoneRowsCache.find(key);
#endif
		if (found != s_CBMu_BMDTransformedBoneRowsCache.end())
		{
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
			return &found->second;
#else
			return found->second.get();
#endif
		}

#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
		std::vector<float>& rows = s_CBMu_BMDTransformedBoneRowsCache[key];
		rows.resize(static_cast<std::size_t>(boneCount) * 12u);
#else
		std::shared_ptr<std::vector<float> > rows(new std::vector<float>());
		rows->resize(static_cast<std::size_t>(boneCount) * 12u);
#endif
		const int validBones = (model != NULL && model->NumBones > 0) ? min(model->NumBones, boneCount) : 0;
		for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
			float* dst = rows.data() + static_cast<std::size_t>(boneIndex) * 12u;
#else
			float* dst = rows->data() + static_cast<std::size_t>(boneIndex) * 12u;
#endif
			if (boneIndex < validBones)
			{
				const float* src = bone + static_cast<std::size_t>(boneIndex) * 12u;
				for (int row = 0; row < 3; ++row)
				{
					const float* srcRow = src + static_cast<std::size_t>(row) * 4u;
					dst[0] = srcRow[0] * resultScale;
					dst[1] = srcRow[1] * resultScale;
					dst[2] = srcRow[2] * resultScale;
					dst[3] = srcRow[3] * translateScale + trans[row];
					dst += 4;
				}
			}
			else
			{
				for (int row = 0; row < 3; ++row)
				{
					dst[0] = (row == 0) ? 1.f : 0.f;
					dst[1] = (row == 1) ? 1.f : 0.f;
					dst[2] = (row == 2) ? 1.f : 0.f;
					dst[3] = trans[row];
					dst += 4;
				}
			}
		}

#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
		return &rows;
#else
		const std::vector<float>* result = rows.get();
		s_CBMu_BMDTransformedBoneRowsCache.emplace(key, rows);
		return result;
#endif
	}
#else
	static void CBMu_ClearBMDTransformedBoneRowsCache()
	{
	}
#endif

	bool CBMu_PackBMDCommandBones(BMD* model, VAOMesh& mesh, const float* bone, bool transEnabled, vec3_t transValue, float scale, bool applyScale, float requestScale, std::vector<float>& packed, GLsizei& rows, CBMu_BMDPackedBoneRejectReason* rejectReason)
	{
		if (rejectReason)
		{
			*rejectReason = CBMu_BMD_PACKED_BONE_REJECT_NONE;
		}

		if (!bone)
		{
			rows = 0;
			packed.clear();
			if (rejectReason)
			{
				*rejectReason = CBMu_BMD_PACKED_BONE_REJECT_NO_BONE_POINTER;
			}
			return false;
		}

		if (mesh.BoneContainer.empty())
		{
			rows = 0;
			packed.clear();
			if (rejectReason)
			{
				*rejectReason = CBMu_BMD_PACKED_BONE_REJECT_EMPTY_BONE_CONTAINER;
			}
			return false;
		}

		const size_t rowsNeeded = mesh.BoneContainer.size() * 3;
		const size_t maxRows =
#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER && CBMu_ENABLE_GL_BMD_TBO_EXTENDED_BONE_ROWS
			static_cast<size_t>(CBMu_GL_BMD_BONE_TEXTURE_BUFFER_MAX_ROWS);
#else
			256u;
#endif
		if (rowsNeeded > maxRows)
		{
			rows = 0;
			packed.clear();
			if (rejectReason)
			{
				*rejectReason = CBMu_BMD_PACKED_BONE_REJECT_ROW_OVERFLOW;
			}
			return false;
		}

		float resultScale = transEnabled ? scale : 1.0f;
		vec3_t trans = { 0.f, 0.f, 0.f };
		if (transEnabled)
		{
			VectorCopy(transValue, trans);
		}

		const float preTransScale = resultScale;
		if (applyScale)
		{
			resultScale = requestScale * resultScale;
		}
		const float translateScale = applyScale ? preTransScale : resultScale;

		packed.resize(rowsNeeded * 4);
		size_t writeIndex = 0;
		for (size_t i = 0; i < mesh.BoneContainer.size(); ++i)
		{
			const int boneIndex = static_cast<int>(mesh.BoneContainer[i]);
			const bool invalid = (boneIndex < 0) || (model != NULL && boneIndex >= model->NumBones) || (boneIndex >= MAX_BONES);
			const int matrixBase = invalid ? 0 : boneIndex * 12;

			for (int row = 0; row < 3; ++row)
			{
				if (invalid)
				{
					packed[writeIndex + 0] = (row == 0) ? 1.f : 0.f;
					packed[writeIndex + 1] = (row == 1) ? 1.f : 0.f;
					packed[writeIndex + 2] = (row == 2) ? 1.f : 0.f;
					packed[writeIndex + 3] = trans[row];
				}
				else
				{
					const float* matrixRow = bone + matrixBase + row * 4;
					packed[writeIndex + 0] = matrixRow[0] * resultScale;
					packed[writeIndex + 1] = matrixRow[1] * resultScale;
					packed[writeIndex + 2] = matrixRow[2] * resultScale;
					packed[writeIndex + 3] = matrixRow[3] * translateScale + trans[row];
				}
				writeIndex += 4;
			}
		}

		rows = static_cast<GLsizei>(rowsNeeded);
		return true;
	}

#if CBMu_ENABLE_GL_BMD_PACKED_BONE_CACHE
	struct CBMu_BMDPackedBoneCacheKey
	{
		std::uintptr_t ModelAddress;
		std::uintptr_t MeshAddress;
		std::uintptr_t BoneAddress;
		std::uint64_t BoneSnapshotSerial;
		std::uintptr_t BoneContainerAddress;
		std::size_t BoneContainerSize;
		unsigned int Flags;
		unsigned int ScaleBits;
		unsigned int RequestScaleBits;
		unsigned int OriginBits[3];

		bool operator==(const CBMu_BMDPackedBoneCacheKey& rhs) const
		{
			return ModelAddress == rhs.ModelAddress &&
				MeshAddress == rhs.MeshAddress &&
				BoneAddress == rhs.BoneAddress &&
				BoneSnapshotSerial == rhs.BoneSnapshotSerial &&
				BoneContainerAddress == rhs.BoneContainerAddress &&
				BoneContainerSize == rhs.BoneContainerSize &&
				Flags == rhs.Flags &&
				ScaleBits == rhs.ScaleBits &&
				RequestScaleBits == rhs.RequestScaleBits &&
				OriginBits[0] == rhs.OriginBits[0] &&
				OriginBits[1] == rhs.OriginBits[1] &&
				OriginBits[2] == rhs.OriginBits[2];
		}
	};

	struct CBMu_BMDPackedBoneCacheKeyHash
	{
		std::size_t operator()(const CBMu_BMDPackedBoneCacheKey& key) const
		{
			std::size_t hash = std::hash<std::uintptr_t>()(key.ModelAddress);
			hash = CBMu_HashCombine(hash, std::hash<std::uintptr_t>()(key.MeshAddress));
			hash = CBMu_HashCombine(hash, std::hash<std::uintptr_t>()(key.BoneAddress));
			hash = CBMu_HashCombine(hash, std::hash<std::uint64_t>()(key.BoneSnapshotSerial));
			hash = CBMu_HashCombine(hash, std::hash<std::uintptr_t>()(key.BoneContainerAddress));
			hash = CBMu_HashCombine(hash, std::hash<std::size_t>()(key.BoneContainerSize));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.Flags));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.ScaleBits));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.RequestScaleBits));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.OriginBits[0]));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.OriginBits[1]));
			hash = CBMu_HashCombine(hash, std::hash<unsigned int>()(key.OriginBits[2]));
			return hash;
		}
	};

#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
	static std::unordered_map<CBMu_BMDPackedBoneCacheKey, std::vector<float>, CBMu_BMDPackedBoneCacheKeyHash> s_CBMu_BMDPackedBoneCache;
#else
	static std::unordered_map<CBMu_BMDPackedBoneCacheKey, std::shared_ptr<std::vector<float> >, CBMu_BMDPackedBoneCacheKeyHash> s_CBMu_BMDPackedBoneCache;
#endif

	static void CBMu_ClearBMDPackedBoneCache()
	{
		s_CBMu_BMDPackedBoneCache.clear();
		CBMu_ClearBMDTransformedBoneRowsCache();
	}

	static bool CBMu_PackBMDCommandBonesCached(BMD* model, VAOMesh& mesh, const float* bone, std::uint64_t boneSnapshotSerial, bool transEnabled, vec3_t transValue, float scale, bool applyScale, float requestScale, std::vector<float>& fallbackPacked,
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
		const std::vector<float>*& cachedPacked,
#else
		std::shared_ptr<const std::vector<float> >& sharedPacked,
#endif
		GLsizei& rows,
		CBMu_BMDPackedBoneRejectReason* rejectReason)
	{
		if (rejectReason)
		{
			*rejectReason = CBMu_BMD_PACKED_BONE_REJECT_NONE;
		}
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
		cachedPacked = NULL;
#else
		sharedPacked.reset();
#endif
		fallbackPacked.clear();
		if (!bone)
		{
			rows = 0;
			if (rejectReason)
			{
				*rejectReason = CBMu_BMD_PACKED_BONE_REJECT_NO_BONE_POINTER;
			}
			return false;
		}

		if (mesh.BoneContainer.empty())
		{
			rows = 0;
			if (rejectReason)
			{
				*rejectReason = CBMu_BMD_PACKED_BONE_REJECT_EMPTY_BONE_CONTAINER;
			}
			return false;
		}

		if (s_CBMu_BMDPackedBoneCache.bucket_count() == 0)
		{
			// CBMu: Reserve nhỏ đủ cho một pass character đông, cache bị xóa sau FlushAllMesh nên không giữ qua frame.
			s_CBMu_BMDPackedBoneCache.reserve(512);
		}

		CBMu_BMDPackedBoneCacheKey key = {};
		key.ModelAddress = reinterpret_cast<std::uintptr_t>(model);
		key.MeshAddress = reinterpret_cast<std::uintptr_t>(&mesh);
		key.BoneAddress = reinterpret_cast<std::uintptr_t>(bone);
		key.BoneSnapshotSerial = boneSnapshotSerial;
		key.BoneContainerAddress = reinterpret_cast<std::uintptr_t>(mesh.BoneContainer.empty() ? NULL : &mesh.BoneContainer[0]);
		key.BoneContainerSize = mesh.BoneContainer.size();
		key.Flags = (transEnabled ? 1u : 0u) | (applyScale ? 2u : 0u);
		key.ScaleBits = CBMu_FloatBits(scale);
		key.RequestScaleBits = CBMu_FloatBits(requestScale);
		key.OriginBits[0] = CBMu_FloatBits(transValue[0]);
		key.OriginBits[1] = CBMu_FloatBits(transValue[1]);
		key.OriginBits[2] = CBMu_FloatBits(transValue[2]);

#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
		std::unordered_map<CBMu_BMDPackedBoneCacheKey, std::vector<float>, CBMu_BMDPackedBoneCacheKeyHash>::iterator found = s_CBMu_BMDPackedBoneCache.find(key);
#else
		std::unordered_map<CBMu_BMDPackedBoneCacheKey, std::shared_ptr<std::vector<float> >, CBMu_BMDPackedBoneCacheKeyHash>::iterator found = s_CBMu_BMDPackedBoneCache.find(key);
#endif
		if (found != s_CBMu_BMDPackedBoneCache.end())
		{
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
			cachedPacked = &found->second;
			rows = static_cast<GLsizei>(found->second.size() / 4u);
#else
			sharedPacked = found->second;
			rows = static_cast<GLsizei>(found->second->size() / 4u);
#endif
			return rows > 0;
		}

#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
		std::vector<float>& packed = s_CBMu_BMDPackedBoneCache[key];
		if (!CBMu_PackBMDCommandBones(model, mesh, bone, transEnabled, transValue, scale, applyScale, requestScale, packed, rows, rejectReason))
		{
			s_CBMu_BMDPackedBoneCache.erase(key);
			return false;
		}

		cachedPacked = &packed;
#else
		std::shared_ptr<std::vector<float> > packed(new std::vector<float>());
		if (!CBMu_PackBMDCommandBones(model, mesh, bone, transEnabled, transValue, scale, applyScale, requestScale, *packed, rows, rejectReason))
		{
			return false;
		}

		sharedPacked = packed;
		s_CBMu_BMDPackedBoneCache.emplace(key, packed);
#endif
		return true;
	}
#else
	static void CBMu_ClearBMDPackedBoneCache()
	{
		CBMu_ClearBMDTransformedBoneRowsCache();
	}
#endif
}
#endif

GLuint LoadShaderProgramFromFiles(const char* vertexPath, const char* fragmentPath)
{
	auto LoadShaderSource = [](const char* path) -> std::string {
		std::ifstream file(path);
		if (!file.is_open())
		{
			ErrorMessageBox("Failed to open shader: %s", path);
			std::cerr << "Failed to open shader: " << path << std::endl;

			return "";
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
		};

	std::string vertCode = LoadShaderSource(vertexPath);
	const char* vertSrc = vertCode.c_str();
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, &vertSrc, nullptr);
	glCompileShader(vertShader);

	GLint success;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[512];
		glGetShaderInfoLog(vertShader, 512, nullptr, log);
		ErrorMessageBox("Vertex shader error [ %s ] %s", vertexPath, log);
		std::cerr << "Vertex shader error [" << vertexPath << "]: " << log << std::endl;
	}

	std::string fragCode = LoadShaderSource(fragmentPath);
	const char* fragSrc = fragCode.c_str();
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &fragSrc, nullptr);
	glCompileShader(fragShader);

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[512];
		glGetShaderInfoLog(fragShader, 512, nullptr, log);
		ErrorMessageBox("Fragment shader error [ %s ] %s", fragmentPath, log);
		std::cerr << "Fragment shader error [" << fragmentPath << "]: " << log << std::endl;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char log[512];
		glGetProgramInfoLog(program, 512, nullptr, log);
		ErrorMessageBox("Program link error %s", log);
		std::cerr << "Program link error: " << log << std::endl;
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return program;
}
int CountLoad = 0;
GLuint LoadShaderProgramFromSource(const char* vertexSource, const char* fragmentSource, const char* debugName = NULL)
{
	// Compile vertex shader
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, &vertexSource, nullptr);
	glCompileShader(vertShader);

	GLint success;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[4096];
		GLsizei logLength = 0;
		glGetShaderInfoLog(vertShader, sizeof(log), &logLength, log);
		ErrorMessageBox("(%d) Vertex shader error %s", CountLoad, log);
		std::cerr << "Vertex shader error: " << log << std::endl;
	}

	// Compile fragment shader
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &fragmentSource, nullptr);
	glCompileShader(fragShader);

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[4096];
		GLsizei logLength = 0;
		glGetShaderInfoLog(fragShader, sizeof(log), &logLength, log);
		ErrorMessageBox("Fragment shader error %s", log);
		std::cerr << "Fragment shader error: " << log << std::endl;
	}

	// Link program
	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char log[4096];
		GLsizei logLength = 0;
		glGetProgramInfoLog(program, sizeof(log), &logLength, log);
		ErrorMessageBox("Program link error %s", log);
		std::cerr << "Program link error: " << log << std::endl;
	}

	// Cleanup
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return program;
}

GLuint LoadShaderProgramFromSource(const char* vertexSource, const char* fragmentSource)
{
	return LoadShaderProgramFromSource(vertexSource, fragmentSource, NULL);
}

void OGL330MODEL::Init()
{
	int count = sizeof(shaderSources) / sizeof(ShaderSource);
	for (int i = 0; i < count; ++i)
	{
		CountLoad = i;
		char vsPath[128], fsPath[128];
		snprintf(vsPath, sizeof(vsPath), "Shaders\\%s.vert.glsl", shaderSources[i].name);
		snprintf(fsPath, sizeof(fsPath), "Shaders\\%s.frag.glsl", shaderSources[i].name);
		
		std::ifstream testFile(vsPath);
		if (testFile.is_open())
		{
			testFile.close();
			shaderProgramMap[i] = LoadShaderProgramFromFiles(vsPath, fsPath);
		}
		else
		{
			shaderProgramMap[i] = LoadShaderProgramFromSource(shaderSources[i].vs, shaderSources[i].fs, shaderSources[i].name);
		}
	}
}

GLuint OGL330MODEL::GetShader(int idNumber)
{
	auto it = shaderProgramMap.find(idNumber);
	if (it != shaderProgramMap.end()) {
		return it->second;
	}
	return 0;
}
void OGL330MODEL::ConvertOldMeshToVaoMesh(GLuint iModel, bool DelMesh)
{
	if (!OGL330::IsShader()) return;

	BMD* pModel = (iModel < (GLuint)MAX_MODELS && Models) ? &Models[iModel] : nullptr;

	if (pModel)
	{
		pModel->LoadMeshToVAO();

		pModel->UploadAllToGPU();
	}
}

void OGL330MODEL::UseShader(GLuint shaderID)
{

	if (g_CurrentShaderID != shaderID)
	{
		glUseProgram(shaderID);
		g_CurrentShaderID = shaderID;

	}
}

void OGL330MODEL::UnUseShader()
{
	glUseProgram(0);
	g_CurrentShaderID = -1;
}

void OGL330MODEL::SetTargetRender(OBJECT* pObj)
{
	if (!OGL330::IsShader()) return;

	if (pObj != NULL)
	{
		GMMeshShader->Lock(true);
	}
	else
	{
#if CBMu_ENABLE_GL_BMD_SKIP_EMPTY_FLUSH
		// Tránh gọi flush khi scope BMD không sinh mesh, vì cảnh đông quái tạo rất nhiều scope rỗng.
		if (!GMMeshShader->HasPendingMesh())
		{
			CBMu_CPUHotspotRecordBMDSkippedEmptyFlush();
			GMMeshShader->Lock(false);
			return;
		}
#endif
		if (GMMeshShader->IsDeferredFlushActive())
		{
			GMMeshShader->Lock(false);
			return;
		}
		GMMeshShader->FlushAllMesh();
		GMMeshShader->Lock(false);
	}
}
// Khai báo global hoặc static
static UniformLocationCache uniformCache;

void OGL330MODEL::SendUniform(GLuint shaderID,
	const mvec4& bodyLight, const mvec4& lightPosition,
	const mvec4& meshUV, const mvec4& setting1,
	const mvec4& setting2, const bool enableLight)
{
	float ProjMatrix[16];
	GetActiveProjectionMatrix(ProjMatrix);
	float ViewMatrix[16];
	GetActiveViewMatrix(ViewMatrix);

	GLint loc;
	if ((loc = uniformCache.GetLocation(shaderID, "uProj")) != -1)
		glUniformMatrix4fv(loc, 1, GL_FALSE, ProjMatrix);
	if ((loc = uniformCache.GetLocation(shaderID, "uView")) != -1)
		glUniformMatrix4fv(loc, 1, GL_FALSE, ViewMatrix);
	if ((loc = uniformCache.GetLocation(shaderID, "u_bodyLight")) != -1)
		glUniform4f(loc, bodyLight.x, bodyLight.y, bodyLight.z, bodyLight.w);
	if ((loc = uniformCache.GetLocation(shaderID, "u_lightPosition")) != -1)
		glUniform4f(loc, lightPosition.x, lightPosition.y, lightPosition.z, lightPosition.w);
	if ((loc = uniformCache.GetLocation(shaderID, "u_meshUV")) != -1)
		glUniform4f(loc, meshUV.x, meshUV.y, meshUV.z, meshUV.w);
	if ((loc = uniformCache.GetLocation(shaderID, "u_setting1")) != -1)
		glUniform4f(loc, setting1.x, setting1.y, setting1.z, setting1.w);
	if ((loc = uniformCache.GetLocation(shaderID, "u_setting2")) != -1)
		glUniform4f(loc, setting2.x, setting2.y, setting2.z, setting2.w);
	if ((loc = uniformCache.GetLocation(shaderID, "u_enableLight")) != -1)
		glUniform1i(loc, enableLight ? 1 : 0);
	if ((loc = uniformCache.GetLocation(shaderID, "uTexture")) != -1)
		glUniform1i(loc, 0);
}

//void OGL330MODEL::SendUniform(GLuint shaderID, const mvec4& bodyLight, const mvec4& lightPosition, const mvec4& meshUV, const mvec4& setting1, const mvec4& setting2, const bool enableLight)
//{
//	float ProjMatrix[16];
//	glGetFloatv(GL_PROJECTION_MATRIX, ProjMatrix);
//	float ViewMatrix[16];
//	glGetFloatv(GL_MODELVIEW_MATRIX, ViewMatrix);
//
//	GLint loc;
//	if ((loc = glGetUniformLocation(shaderID, "uProj")) != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, ProjMatrix);
//	if ((loc = glGetUniformLocation(shaderID, "uView")) != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, ViewMatrix);
//	if ((loc = glGetUniformLocation(shaderID, "u_bodyLight")) != -1) glUniform4f(loc, bodyLight.x, bodyLight.y, bodyLight.z, bodyLight.w);
//	if ((loc = glGetUniformLocation(shaderID, "u_lightPosition")) != -1) glUniform4f(loc, lightPosition.x, lightPosition.y, lightPosition.z, lightPosition.w);
//	if ((loc = glGetUniformLocation(shaderID, "u_meshUV")) != -1) glUniform4f(loc, meshUV.x, meshUV.y, meshUV.z, meshUV.w);
//	if ((loc = glGetUniformLocation(shaderID, "u_setting1")) != -1) glUniform4f(loc, setting1.x, setting1.y, setting1.z, setting1.w);
//	if ((loc = glGetUniformLocation(shaderID, "u_setting2")) != -1) glUniform4f(loc, setting2.x, setting2.y, setting2.z, setting2.w);
//	if ((loc = glGetUniformLocation(shaderID, "u_enableLight")) != -1) glUniform1i(loc, enableLight ? 1 : 0);
//	if ((loc = glGetUniformLocation(shaderID, "uTexture")) != -1) glUniform1i(loc, 0);
//}

using namespace OGL330MODEL;

bool OGL330MODEL::CGMMeshShader::m_Transfrom = false;
float* OGL330MODEL::CGMMeshShader::m_finalBone = NULL;
std::uint64_t OGL330MODEL::CGMMeshShader::m_BoneSnapshotSerial = 0;
#if CBMu_ENABLE_GL_BMD_BONE_SNAPSHOT_POINTER_CACHE
float* OGL330MODEL::CGMMeshShader::m_CBMuLastBoneMatrixPointer = NULL;
bool OGL330MODEL::CGMMeshShader::m_CBMuLastBoneTransform = false;
#endif

CGMMeshShader::CGMMeshShader()
{
	m_Transfrom = false;
	m_BoneSnapshotSerial = 0;
	m_DataCount = 0;
#if CBMu_ENABLE_GL_BMD_COMMAND_POOL_REUSE
	// CBMu: Cảnh nhiều NPC cùng loại tạo nhiều command BMD; reserve lớn hơn giúp giảm cấp phát lúc vào bãi đông.
	m_Data.reserve(128);
#else
	m_Data.reserve(10);
#endif
	m_Lock = false;
	m_Enabled = true;
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
	m_DeferFlushDepth = 0;
#endif
#if CBMu_ENABLE_GL_BMD_BONE_SNAPSHOT_POINTER_CACHE
	m_CBMuLastBoneMatrixPointer = NULL;
	m_CBMuLastBoneTransform = false;
#endif
	memset(m_vLightPosOrg, 0, sizeof(vec3_t));
	memset(m_vLightDirOrg, 0, sizeof(vec3_t));
	memset(m_vLightPos, 0, sizeof(vec3_t));
	memset(m_vLightDir, 0, sizeof(vec3_t));
	m_finalBone = NULL;
}

CGMMeshShader::~CGMMeshShader() {
	Release();
}

void CGMMeshShader::Release()
{
	m_Data.clear();
	m_DataCount = 0;
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
	CBMu_ClearBMDPackedBoneCache();
#endif
}

bool CGMMeshShader::IsAlpha(int iType)
{
	if (iType & RENDER_BRIGHT) return true;

	if (iType & RENDER_CHROME3) return true;
	else if (iType & RENDER_CHROME4) return true;
	else if (iType & RENDER_CHROME5) return true;
	else if (iType & RENDER_CHROME7) return true;

	return false;
}

void CGMMeshShader::MakeShaderType(int iShaderType, bool enableLight, bool bAlphaNoUse, float BlendU, float BlendV, RenderMeshVAO& r)
{
	mvec4 bodyLight;

	if (bAlphaNoUse)
	{
		bodyLight.x = r.m_isColor.x * r.m_isAlpha;
		bodyLight.y = r.m_isColor.y * r.m_isAlpha;
		bodyLight.z = r.m_isColor.z * r.m_isAlpha;
		bodyLight.w = 1.f;
	}
	else
	{
		bodyLight.x = r.m_isColor.x;
		bodyLight.y = r.m_isColor.y;
		bodyLight.z = r.m_isColor.z;
		bodyLight.w = r.m_isAlpha;
	}

	mvec4 lightPosition;

	lightPosition.w = r.m_isAlpha;
	if (enableLight)
	{
		lightPosition.x = m_vLightDir[0];
		lightPosition.y = m_vLightDir[1];
		lightPosition.z = m_vLightDir[2];
	}
	else
	{
		lightPosition.x = 0.f;
		lightPosition.y = 0.f;
		lightPosition.z = 0.f;
	}

	// Composite đọc các uniform này cả ở base pass; khởi tạo để màu/UV không phụ thuộc dữ liệu stack mỗi lần chạy.
	mvec4 meshUV = {};
	mvec4 setting1 = {};
	mvec4 setting2 = {};

	switch (iShaderType)
	{
	case SHADER_330_BLENDMESH:
	{
		meshUV.x = BlendU;
		meshUV.y = BlendV;
		meshUV.z = 1.f;
		meshUV.w = 0.f;
	}
	break;
	case SHADER_330_CHROME1:
	{
		setting1.x = 1.f;
		setting1.y = 0.f;
		setting1.z = (int)WorldTime % 10000 * 0.0001f;
		setting1.w = 0.f;

		setting2.x = 0.5f;
		setting2.y = 0.5f;
		setting2.z = 2.0f;
		setting2.w = 1.f;
	}
	break;
	case SHADER_330_CHROME2:
	{
		setting1.x = 1.f;
		setting1.y = 0.f;
		setting1.z = 0.f;
		setting1.w = (int)WorldTime % 5000 * 0.00024f - 0.4f;

		setting2.x = 0.8f;
		setting2.y = 2.f;
		setting2.z = 1.f;
		setting2.w = 3.f;
	}
	break;
	case SHADER_330_CHROME3:
	{
		setting2.x = 0.0;
		setting2.y = -0.1f;
		setting2.z = -0.8f;
		setting2.w = 1.f;
	}
	break;
	case SHADER_330_CHROME4:
	{
		setting1.x = cosf(WorldTime * 0.001f);
		setting1.y = sinf(WorldTime * 0.002f);
		setting1.z = 1.f;
		setting1.w = (int)WorldTime % 10000 * 0.0001f;

		meshUV.x = BlendU;
		meshUV.y = BlendV;
		meshUV.z = 1.f;
		meshUV.w = 0.f;

		setting2.x = 0.5f;
		setting2.y = 3.f;
		setting2.z = 0.5f;
		setting2.w = 3.f;
	}
	break;
	case SHADER_330_CHROME5:
	{
		setting1.x = cosf(WorldTime * 0.001f);
		setting1.y = sinf(WorldTime * 0.002f);
		setting1.z = 1.f;
		setting1.w = (int)WorldTime % 10000 * 0.0001f;

		setting2.x = 2.5f;
		setting2.y = 1.f;
		setting2.z = 3.f;
		setting2.w = 5.f;
	}
	break;
	case SHADER_330_CHROME6:
	{
		setting1.x = 0.8f;
		setting1.y = 2.f;
		setting1.z = (int)WorldTime % 5000 * 0.00024f - 0.4f;
		setting1.w = 0.f;
		if (r.m_TextureID == -1)
			r.m_TextureID = BITMAP_CHROME6;
	}
	break;
	case SHADER_330_CHROME7:
	{
		setting1.x = 0.8f;
		setting1.y = 0.8f;
		setting1.z = WorldTime;
		setting1.w = 0.00006f;
	}
	break;
	case SHADER_330_CHROME8:
	{
		setting1.x = 0.8f;
		setting1.y = 0.8f;
		setting1.z = WorldTime;
		setting1.w = 0.00006f;
	}
	break;
	case SHADER_330_METAL:
	{
		setting2.x = 0.5f;
		setting2.y = 0.2f;
		setting2.z = 0.5f;
		setting2.w = 0.5f;
		if (r.m_TextureID == -1)
			r.m_TextureID = BITMAP_SHINY;
	}
	break;
	case SHADER_330_OIL:
	{
		meshUV.x = BlendU;
		meshUV.y = BlendV;
		meshUV.z = 1.f;
		meshUV.w = 0.f;
	}
	break;
	}

	r.m_bodyLight = bodyLight;
	r.m_lightPosition = lightPosition;
	r.m_meshUV = meshUV;
	r.m_setting1 = setting1;
	r.m_setting2 = setting2;

	r.m_Shader = shaderProgramMap[iShaderType];
}

void CGMMeshShader::SetHighLight(bool bHighLight, bool bBattleCastle)
{
	if (bHighLight)
	{
		Vector(1.3f, 0.f, 2.f, m_vLightPosOrg);
	}
	else if (bBattleCastle)
	{
		Vector(0.5f, -1.f, 1.f, m_vLightPosOrg);
	}
	else
	{
		Vector(1.3f, 0.f, 2.f, m_vLightPosOrg);
	}

	VectorCopy(m_vLightPosOrg, m_vLightDirOrg);
}
inline float Distance3D(const mvec3& a, const mvec3& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

static bool CBMu_IsBMDTransparentForSort(const RenderMeshVAO& item)
{
	BITMAP_t* pBitmap = (item.m_TextureID >= 0) ? Bitmaps.FindTexture(static_cast<GLuint>(item.m_TextureID)) : nullptr;
	const bool is4Comp = pBitmap && pBitmap->Components == 4;
	return item.m_isAlpha < 0.99f
		|| is4Comp
		|| (item.m_FlagRender & (RENDER_BRIGHT | RENDER_DARK |
			RENDER_CHROME | RENDER_CHROME2 | RENDER_CHROME3 | RENDER_CHROME4 |
			RENDER_CHROME5 | RENDER_CHROME6 | RENDER_CHROME7 | RENDER_CHROME8 |
			RENDER_METAL | RENDER_LIGHTMAP | RENDER_OIL)) != 0;
}

static bool CBMu_IsBMDLayeredMaterialPass(const RenderMeshVAO& item)
{
	// Mọi pass blend/special làm cả bone-snapshot group nhạy thứ tự; opaque-first sort sẽ che detail legacy.
	return (item.m_FlagRender & (RENDER_BRIGHT | RENDER_DARK |
		RENDER_CHROME | RENDER_CHROME2 | RENDER_CHROME3 | RENDER_CHROME4 |
		RENDER_CHROME5 | RENDER_CHROME6 | RENDER_CHROME7 | RENDER_CHROME8 |
		RENDER_METAL | RENDER_LIGHTMAP | RENDER_OIL)) != 0;
}

#if CBMu_ENABLE_GL_BMD_SKINNED_INSTANCED_BATCH
static bool CBMu_IsBMDSafeChromeBrightInstancedPass(const RenderMeshVAO& item)
{
#if CBMu_ENABLE_GL_BMD_CHROME_BRIGHT_INSTANCED_BATCH
	const int safeFlags = RENDER_BRIGHT | RENDER_CHROME;
	const int safeChrome4Flags = RENDER_BRIGHT | RENDER_CHROME4;
	const bool safeFlag = item.m_FlagRender == safeFlags || item.m_FlagRender == safeChrome4Flags;
	return safeFlag
		&& item.m_isAlpha >= 0.99f
		&& item.m_TextureID != -1;
#else
	(void)item;
	return false;
#endif
}

static bool CBMu_IsBMDInstancedBatchCandidate(const RenderMeshVAO& item)
{
#if CBMu_USE_VULKAN_NATIVE_MESH
	return item.m_OldBMD != NULL
		&& !item.m_Shadow
		&& item.m_HasPackedBones
		&& item.m_PackedBoneRows > 0;
#else
	const int unsupportedNativeMaterialFlags = RENDER_COLOR | RENDER_DARK |
		RENDER_CHROME | RENDER_CHROME2 | RENDER_CHROME3 | RENDER_CHROME4 |
		RENDER_CHROME5 | RENDER_CHROME6 | RENDER_CHROME7 | RENDER_CHROME8 |
		RENDER_METAL | RENDER_OIL | RENDER_LIGHTMAP;
	return item.m_OldBMD != NULL
		&& !item.m_Shadow
		&& item.m_HasPackedBones
		&& item.m_PackedBoneRows > 0
		&& (item.m_FlagRender & unsupportedNativeMaterialFlags) == 0;
#endif
}

// CBMu_BEGIN: Cache key sort BMD để giảm CPU trong stable_sort ở queue lớn.
#if CBMu_ENABLE_GL_BMD_SORT_KEY_CACHE
static void CBMu_PrepareBMDSortKey(RenderMeshVAO& item)
{
	item.m_CBMuInstancedCandidate = CBMu_IsBMDInstancedBatchCandidate(item);
	item.m_CBMuTransparentForSort =
#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
		item.m_CBMuGoldenOverlay ||
#endif
		CBMu_IsBMDTransparentForSort(item);
}

static bool CBMu_IsBMDInstancedBatchCandidateFast(const RenderMeshVAO& item)
{
	return !item.m_CBMuOrderSensitiveGroup && item.m_CBMuInstancedCandidate;
}
#else
static bool CBMu_IsBMDInstancedBatchCandidateFast(const RenderMeshVAO& item)
{
	return !item.m_CBMuOrderSensitiveGroup && CBMu_IsBMDInstancedBatchCandidate(item);
}
#endif
// CBMu_END: Cache key sort BMD để giảm CPU trong stable_sort ở queue lớn.

static bool CBMu_SameVec4(const mvec4& lhs, const mvec4& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

static bool CBMu_SameVec3(const mvec3& lhs, const mvec3& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
static bool CBMu_SameBMDCompositeSet(const RenderMeshVAO& lhs, const RenderMeshVAO& rhs)
{
	if (lhs.m_CompositeOverlayCount != rhs.m_CompositeOverlayCount)
	{
		return false;
	}

	for (int i = 0; i < lhs.m_CompositeOverlayCount; ++i)
	{
		if (lhs.m_CompositeTextureID[i] != rhs.m_CompositeTextureID[i] ||
			lhs.m_CompositeMaterialMode[i] != rhs.m_CompositeMaterialMode[i])
		{
			return false;
		}
#if !CBMu_ENABLE_GL_BMD_INSTANCE_COMPOSITE_COLOR
		if (!CBMu_SameVec3(lhs.m_CompositeColor[i], rhs.m_CompositeColor[i]))
		{
			return false;
		}
#endif
	}

	return true;
}
#endif

static bool CBMu_SameBMDInstancedBatchKey(const RenderMeshVAO& lhs, const RenderMeshVAO& rhs)
{
	if (!CBMu_IsBMDInstancedBatchCandidateFast(lhs) || !CBMu_IsBMDInstancedBatchCandidateFast(rhs))
	{
		return false;
	}

	if (lhs.m_OldBMD != rhs.m_OldBMD ||
		lhs.m_IndexMesh != rhs.m_IndexMesh ||
		lhs.m_TextureID != rhs.m_TextureID ||
		lhs.m_FlagRender != rhs.m_FlagRender ||
		lhs.m_Shader != rhs.m_Shader ||
		lhs.m_isLight != rhs.m_isLight ||
		lhs.m_PackedBoneRows != rhs.m_PackedBoneRows)
	{
		return false;
	}

	if (!CBMu_SameVec4(lhs.m_setting1, rhs.m_setting1) ||
		!CBMu_SameVec4(lhs.m_setting2, rhs.m_setting2) ||
		!CBMu_SameVec4(lhs.m_meshUV, rhs.m_meshUV)
#if !CBMu_USE_VULKAN_NATIVE_MESH
#if !CBMu_ENABLE_GL_BMD_INSTANCE_PARAM_KEY_RELAX
		|| !CBMu_SameVec3(lhs.m_isColor, rhs.m_isColor)
#endif
#if !CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER
#if !CBMu_ENABLE_GL_BMD_INSTANCE_PARAM_KEY_RELAX
		|| !CBMu_SameVec4(lhs.m_bodyLight, rhs.m_bodyLight) ||
		!CBMu_SameVec4(lhs.m_lightPosition, rhs.m_lightPosition)
#endif
#endif
#endif
		)
	{
		return false;
	}

#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
	if (!CBMu_SameBMDCompositeSet(lhs, rhs))
	{
		return false;
	}
#endif

#if CBMu_ENABLE_GL_BMD_MATRIX_CACHE
	if (lhs.m_HasCachedMatrices != rhs.m_HasCachedMatrices)
	{
		return false;
	}
	if (lhs.m_HasCachedMatrices &&
		(memcmp(lhs.m_ProjectionMatrix, rhs.m_ProjectionMatrix, sizeof(lhs.m_ProjectionMatrix)) != 0 ||
			memcmp(lhs.m_ViewMatrix, rhs.m_ViewMatrix, sizeof(lhs.m_ViewMatrix)) != 0))
	{
		return false;
	}
#endif
	return true;
}


#if CBMu_ENABLE_GL_BMD_SPLIT_COUNTERS
static CBMu_BMDBatchSplitReason CBMu_MapBMDPackedBoneRejectReason(CBMu_BMDPackedBoneRejectReason reason)
{
	switch (reason)
	{
	case CBMu_BMD_PACKED_BONE_REJECT_NO_BONE_POINTER:
		return CBMu_BMDBatchSplitReason::BaseNoBonePointer;
	case CBMu_BMD_PACKED_BONE_REJECT_EMPTY_BONE_CONTAINER:
		return CBMu_BMDBatchSplitReason::BaseEmptyBoneContainer;
	case CBMu_BMD_PACKED_BONE_REJECT_ROW_OVERFLOW:
		return CBMu_BMDBatchSplitReason::BaseBoneRowOverflow;
	case CBMu_BMD_PACKED_BONE_REJECT_INVALID_BONE_INDEX:
		return CBMu_BMDBatchSplitReason::BaseInvalidBoneIndex;
	case CBMu_BMD_PACKED_BONE_REJECT_DEFER_INACTIVE:
		return CBMu_BMDBatchSplitReason::BaseDeferredFlushInactive;
	case CBMu_BMD_PACKED_BONE_REJECT_UNKNOWN:
		return CBMu_BMDBatchSplitReason::BasePackedBoneUnknown;
	default:
		return CBMu_BMDBatchSplitReason::BaseNoPackedBones;
	}
}

static CBMu_BMDBatchSplitReason CBMu_GetBMDUnsafeRenderFlagReason(int renderFlag)
{
	const int chromeFlags = RENDER_CHROME | RENDER_CHROME2 | RENDER_CHROME3 | RENDER_CHROME4 |
		RENDER_CHROME5 | RENDER_CHROME6 | RENDER_CHROME7 | RENDER_CHROME8;
	if ((renderFlag & RENDER_COLOR) == RENDER_COLOR) return CBMu_BMDBatchSplitReason::BaseRenderColor;
	if ((renderFlag & RENDER_DARK) == RENDER_DARK) return CBMu_BMDBatchSplitReason::BaseRenderDark;
	if ((renderFlag & chromeFlags) != 0) return CBMu_BMDBatchSplitReason::BaseRenderChrome;
	if ((renderFlag & RENDER_METAL) == RENDER_METAL) return CBMu_BMDBatchSplitReason::BaseRenderMetal;
	if ((renderFlag & RENDER_LIGHTMAP) == RENDER_LIGHTMAP) return CBMu_BMDBatchSplitReason::BaseRenderLightmap;
	if ((renderFlag & RENDER_OIL) == RENDER_OIL) return CBMu_BMDBatchSplitReason::BaseRenderOil;
	return CBMu_BMDBatchSplitReason::BaseUnsafeFlag;
}

static CBMu_BMDBatchSplitReason CBMu_GetBMDInstancedRejectReasonCode(const RenderMeshVAO& item, bool baseCommand)
{
	if (!baseCommand)
	{
		return CBMu_BMDBatchSplitReason::NextNotCandidate;
	}
	if (item.m_CBMuOrderSensitiveGroup) return CBMu_BMDBatchSplitReason::BaseOrderSensitive;
	if (item.m_OldBMD == NULL) return CBMu_BMDBatchSplitReason::BaseNullModel;
	if (item.m_Shadow) return CBMu_BMDBatchSplitReason::BaseShadow;
	if (!item.m_HasPackedBones) return CBMu_MapBMDPackedBoneRejectReason(item.m_CBMuPackedBoneRejectReason);
	if (item.m_PackedBoneRows <= 0) return CBMu_BMDBatchSplitReason::BaseBoneRows;
	if (item.m_isAlpha < 0.99f) return CBMu_BMDBatchSplitReason::BaseAlpha;
	if ((item.m_FlagRender & RENDER_TEXTURE) != RENDER_TEXTURE) return CBMu_BMDBatchSplitReason::BaseNoTextureFlag;
#if CBMu_ENABLE_GL_BMD_ADDITIVE_INSTANCED_BATCH
	const bool additive = (item.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT;
	if (additive && (item.m_FlagRender & (RENDER_NODEPTH | RENDER_TEXTURE | RENDER_BRIGHT)) != item.m_FlagRender)
	{
		return CBMu_BMDBatchSplitReason::BaseAdditiveFlag;
	}
#else
	if ((item.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT) return CBMu_BMDBatchSplitReason::BaseAdditiveFlag;
#endif
	return CBMu_GetBMDUnsafeRenderFlagReason(item.m_FlagRender);
}

static void CBMu_RecordBMDInstancedRejectDetail(const RenderMeshVAO& item, CBMu_BMDBatchSplitReason reason)
{
	// Stage B: kèm FileName BMD để định danh DeferInactive/OrderSensitive trên log top-N.
	const char* modelFile = NULL;
	if (item.m_OldBMD != NULL)
	{
		modelFile = item.m_OldBMD->Name;
	}
	CBMu_CPUHotspotRecordBMDBatchSplitDetail(
		reason,
		static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(item.m_OldBMD)),
		item.m_IndexMesh,
		item.m_TextureID,
		item.m_FlagRender,
		static_cast<unsigned int>(item.m_Shader),
		modelFile);
}

static CBMu_BMDBatchSplitReason CBMu_GetBMDBatchSplitReasonCode(const RenderMeshVAO& lhs, const RenderMeshVAO& rhs, GLsizei totalRows, GLsizei maxRows)
{
	if (!CBMu_IsBMDInstancedBatchCandidateFast(lhs)) return CBMu_GetBMDInstancedRejectReasonCode(lhs, true);
	if (!CBMu_IsBMDInstancedBatchCandidateFast(rhs)) return CBMu_GetBMDInstancedRejectReasonCode(rhs, false);
	if (lhs.m_PackedBoneRows > 0 && totalRows + rhs.m_PackedBoneRows > maxRows) return CBMu_BMDBatchSplitReason::BoneRowLimit;
	if (lhs.m_OldBMD != rhs.m_OldBMD) return CBMu_BMDBatchSplitReason::Model;
	if (lhs.m_IndexMesh != rhs.m_IndexMesh) return CBMu_BMDBatchSplitReason::Mesh;
	if (lhs.m_TextureID != rhs.m_TextureID) return CBMu_BMDBatchSplitReason::Texture;
	if (lhs.m_FlagRender != rhs.m_FlagRender) return CBMu_BMDBatchSplitReason::Flag;
	if (lhs.m_Shader != rhs.m_Shader) return CBMu_BMDBatchSplitReason::Shader;
	if (lhs.m_isLight != rhs.m_isLight) return CBMu_BMDBatchSplitReason::LightFlag;
	if (lhs.m_PackedBoneRows != rhs.m_PackedBoneRows) return CBMu_BMDBatchSplitReason::BoneRows;
	if (!CBMu_SameVec4(lhs.m_setting1, rhs.m_setting1)) return CBMu_BMDBatchSplitReason::Setting1;
	if (!CBMu_SameVec4(lhs.m_setting2, rhs.m_setting2)) return CBMu_BMDBatchSplitReason::Setting2;
	if (!CBMu_SameVec4(lhs.m_meshUV, rhs.m_meshUV)) return CBMu_BMDBatchSplitReason::MeshUV;
#if !CBMu_ENABLE_GL_BMD_INSTANCE_PARAM_KEY_RELAX
	if (!CBMu_SameVec3(lhs.m_isColor, rhs.m_isColor)) return CBMu_BMDBatchSplitReason::Color;
#endif
#if !CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER && !CBMu_ENABLE_GL_BMD_INSTANCE_PARAM_KEY_RELAX
	if (!CBMu_SameVec4(lhs.m_bodyLight, rhs.m_bodyLight)) return CBMu_BMDBatchSplitReason::BodyLight;
	if (!CBMu_SameVec4(lhs.m_lightPosition, rhs.m_lightPosition)) return CBMu_BMDBatchSplitReason::LightPosition;
#endif
#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
	if (!CBMu_SameBMDCompositeSet(lhs, rhs)) return CBMu_BMDBatchSplitReason::Composite;
#endif
#if CBMu_ENABLE_GL_BMD_MATRIX_CACHE
	if (lhs.m_HasCachedMatrices != rhs.m_HasCachedMatrices) return CBMu_BMDBatchSplitReason::Matrix;
	if (lhs.m_HasCachedMatrices &&
		(memcmp(lhs.m_ProjectionMatrix, rhs.m_ProjectionMatrix, sizeof(lhs.m_ProjectionMatrix)) != 0 ||
			memcmp(lhs.m_ViewMatrix, rhs.m_ViewMatrix, sizeof(lhs.m_ViewMatrix)) != 0)) return CBMu_BMDBatchSplitReason::Matrix;
#endif
	return CBMu_BMDBatchSplitReason::RenderFallback;
}
#endif

#if CBMu_ENABLE_GL_BMD_INSTANCED_SORT_KEY
static bool CBMu_LessVec3(const mvec3& lhs, const mvec3& rhs)
{
	if (lhs.x != rhs.x) return lhs.x < rhs.x;
	if (lhs.y != rhs.y) return lhs.y < rhs.y;
	return lhs.z < rhs.z;
}

static bool CBMu_LessVec4(const mvec4& lhs, const mvec4& rhs)
{
	if (lhs.x != rhs.x) return lhs.x < rhs.x;
	if (lhs.y != rhs.y) return lhs.y < rhs.y;
	if (lhs.z != rhs.z) return lhs.z < rhs.z;
	return lhs.w < rhs.w;
}

static int CBMu_CompareBMDInstancedSortKey(const RenderMeshVAO& lhs, const RenderMeshVAO& rhs)
{
	if (lhs.m_OldBMD != rhs.m_OldBMD) return lhs.m_OldBMD < rhs.m_OldBMD ? -1 : 1;
	if (lhs.m_IndexMesh != rhs.m_IndexMesh) return lhs.m_IndexMesh < rhs.m_IndexMesh ? -1 : 1;
	if (lhs.m_TextureID != rhs.m_TextureID) return lhs.m_TextureID < rhs.m_TextureID ? -1 : 1;
	if (lhs.m_Shader != rhs.m_Shader) return lhs.m_Shader < rhs.m_Shader ? -1 : 1;
	if (lhs.m_FlagRender != rhs.m_FlagRender) return lhs.m_FlagRender < rhs.m_FlagRender ? -1 : 1;
	if (lhs.m_isLight != rhs.m_isLight) return lhs.m_isLight < rhs.m_isLight ? -1 : 1;
	if (lhs.m_PackedBoneRows != rhs.m_PackedBoneRows) return lhs.m_PackedBoneRows < rhs.m_PackedBoneRows ? -1 : 1;
	if (!CBMu_SameVec4(lhs.m_setting1, rhs.m_setting1)) return CBMu_LessVec4(lhs.m_setting1, rhs.m_setting1) ? -1 : 1;
	if (!CBMu_SameVec4(lhs.m_setting2, rhs.m_setting2)) return CBMu_LessVec4(lhs.m_setting2, rhs.m_setting2) ? -1 : 1;
	if (!CBMu_SameVec4(lhs.m_meshUV, rhs.m_meshUV)) return CBMu_LessVec4(lhs.m_meshUV, rhs.m_meshUV) ? -1 : 1;
#if !CBMu_ENABLE_GL_BMD_INSTANCE_PARAM_KEY_RELAX
	if (!CBMu_SameVec3(lhs.m_isColor, rhs.m_isColor)) return CBMu_LessVec3(lhs.m_isColor, rhs.m_isColor) ? -1 : 1;
#endif
#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
	if (lhs.m_CompositeOverlayCount != rhs.m_CompositeOverlayCount) return lhs.m_CompositeOverlayCount < rhs.m_CompositeOverlayCount ? -1 : 1;
	for (int i = 0; i < lhs.m_CompositeOverlayCount; ++i)
	{
		if (lhs.m_CompositeTextureID[i] != rhs.m_CompositeTextureID[i]) return lhs.m_CompositeTextureID[i] < rhs.m_CompositeTextureID[i] ? -1 : 1;
		if (lhs.m_CompositeMaterialMode[i] != rhs.m_CompositeMaterialMode[i]) return lhs.m_CompositeMaterialMode[i] < rhs.m_CompositeMaterialMode[i] ? -1 : 1;
#if !CBMu_ENABLE_GL_BMD_INSTANCE_COMPOSITE_COLOR
		if (!CBMu_SameVec3(lhs.m_CompositeColor[i], rhs.m_CompositeColor[i])) return CBMu_LessVec3(lhs.m_CompositeColor[i], rhs.m_CompositeColor[i]) ? -1 : 1;
#endif
	}
#endif
	return 0;
}
#endif
#endif

RenderMeshVAO& CGMMeshShader::AllocateMeshCommand()
{
#if CBMu_ENABLE_GL_BMD_COMMAND_POOL_REUSE
	if (m_DataCount >= m_Data.size())
	{
		m_Data.push_back(RenderMeshVAO());
	}

	RenderMeshVAO& command = m_Data[m_DataCount++];
	command.Reset();
	return command;
#else
	m_Data.push_back(RenderMeshVAO());
	return m_Data[m_Data.size() - 1];
#endif
}

void CGMMeshShader::DiscardLastMeshCommand()
{
#if CBMu_ENABLE_GL_BMD_COMMAND_POOL_REUSE
	if (m_DataCount > 0)
	{
		--m_DataCount;
	}
#else
	m_Data.pop_back();
#endif
}

void CGMMeshShader::AddMeshCommand(BMD* pSrc, int idx, int RFlag, float Alpha, int BlendMesh, float Light, float BlendU, float BlendV, int Texture)
{
	std::lock_guard<std::mutex> lock(m_CommandMutex);

	VAOMesh& rMesh = pSrc->New_Meshs[idx];
	Mesh_t& rOldMesh = pSrc->Meshs[idx];
	RenderMeshVAO& rNew = AllocateMeshCommand();
	rNew.m_OldBMD = pSrc;
	rNew.m_CBMuBoneSnapshotSerial = m_BoneSnapshotSerial;
	rNew.m_IndexMesh = idx;
	rNew.m_FlagRender = RFlag;
#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
	rNew.m_CBMuGoldenOverlay = g_CBMuGoldenOverlayPassDepth > 0;
#endif
	rNew.m_isAlpha = Alpha > 0.99 ? 1.f : Alpha;
	rNew.m_isLight = pSrc->LightEnable && !pSrc->bOffLight;
	memcpy(&rNew.m_isColor.x, pSrc->BodyLight, sizeof(float) * 3);

	int iShaderType = SHADER_330_NONE;
	int iSourceTex = pSrc->IndexTexture[rMesh.Texture];


	if (BITMAP_HIDE == iSourceTex && RFlag != (RENDER_SHADOWMAP | RENDER_TEXTURE))
	{
		DiscardLastMeshCommand();
		return;
	}
	else if (iSourceTex == BITMAP_SKIN)
	{
		if (pSrc->HideSkin)
		{
			DiscardLastMeshCommand();
			return;
		}
		iSourceTex = BITMAP_SKIN + pSrc->Skin;
	}
	else if (iSourceTex == BITMAP_HQSKIN)
	{
		if (pSrc->HideSkin)
		{
			DiscardLastMeshCommand();
			return;
		}
		iSourceTex = BITMAP_HQSKIN + pSrc->Skin;
	}
	else if (iSourceTex == BITMAP_HQSKIN2)
	{
		if (pSrc->HideSkin)
		{
			DiscardLastMeshCommand();
			return;
		}
		iSourceTex = BITMAP_HQSKIN2 + pSrc->Skin;
	}
	else if (iSourceTex == BITMAP_HQSKIN3)
	{
		if (pSrc->HideSkin)
		{
			DiscardLastMeshCommand();
			return;
		}
		iSourceTex = BITMAP_HQSKIN3 + pSrc->Skin;
	}
	else if (iSourceTex == BITMAP_WATER)
	{
		iSourceTex = BITMAP_WATER + WaterTextureNumber;
	}
	else if (iSourceTex == BITMAP_HAIR)
	{
		if (pSrc->HideSkin)
		{
			DiscardLastMeshCommand();
			return;
		}
		iSourceTex = BITMAP_HAIR + pSrc->Skin;
	}
	else if (iSourceTex == BITMAP_HQHAIR)
	{
		if (pSrc->HideSkin)
		{
			DiscardLastMeshCommand();
			return;
		}
		iSourceTex = BITMAP_HQHAIR + pSrc->Skin;
	}

	OGL330::GetTexturShader(rNew.m_FlagRender, rNew.m_TextureID, iShaderType);

	if (rNew.m_TextureID == -1) rNew.m_TextureID = iSourceTex;

	if (Texture != -1)
	{
		if (Texture >= 0 && Texture < pSrc->NumMeshs && pSrc->IndexTexture && pSrc->IndexTexture[Texture] > 0)
		{
			rNew.m_TextureID = pSrc->IndexTexture[Texture];
		}
		else
		{
			rNew.m_TextureID = Texture;
		}
	}

	if (iShaderType == SHADER_330_NONE)
	{
		if (rMesh.m_csTScript)
		{
			if (rMesh.m_csTScript->getStreamMesh())
			{
				iShaderType = SHADER_330_BLENDMESH;
			}
		}
		else if (idx == BlendMesh || idx == pSrc->StreamMesh)
		{
			iShaderType = SHADER_330_BLENDMESH;
			rNew.m_isLight = false;
		}

		if (BlendMesh <= -2 || rMesh.Texture == BlendMesh || idx == BlendMesh)
		{
			if ((rNew.m_FlagRender & RENDER_DARK) != RENDER_DARK)
			{
				if (!(rNew.m_FlagRender & RENDER_BRIGHT)) rNew.m_FlagRender |= RENDER_BRIGHT;

				if (BlendU != 0.f || BlendV != 0.f)
				{
					iShaderType = SHADER_330_BLENDMESH;
				}
				rNew.m_isColor.x *= Light;
				rNew.m_isColor.y *= Light;
				rNew.m_isColor.z *= Light;
				rNew.m_isAlpha = 1.f;
			}
		}
	}
	else
	{
		if (rOldMesh.NoneBlendMesh)
		{
			DiscardLastMeshCommand();
			return;
		}

		if (rMesh.m_csTScript)
		{
			if (rMesh.m_csTScript->getNoneBlendMesh())
			{
				DiscardLastMeshCommand();
				return;
			}
		}

		rNew.m_isLight = false;
	}

	if (rNew.m_isLight)
	{
		vec34_t mat;
		Vector(0.f, 0.f, -45.f, pSrc->ShadowAngle);
		AngleMatrix(pSrc->ShadowAngle, mat);
		VectorIRotate(m_vLightDirOrg, mat, m_vLightDir);
	}

	MakeShaderType(iShaderType, rNew.m_isLight, IsAlpha(RFlag), BlendU, BlendV, rNew);

#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
	// CBMu_BEGIN: Chụp overlay composite wearable vào command để renderer chỉ batch khi cùng bộ overlay.
	const int cbmuUnsafeCompositeFlags = RENDER_COLOR | RENDER_CHROME | RENDER_CHROME2 |
		RENDER_CHROME3 | RENDER_CHROME4 | RENDER_CHROME5 | RENDER_CHROME6 |
		RENDER_CHROME7 | RENDER_CHROME8 | RENDER_METAL | RENDER_OIL | RENDER_LIGHTMAP |
		RENDER_BRIGHT;
	if (g_activeBmdCompositeOverlayCount > 0 &&
		!rOldMesh.NoneBlendMesh &&
		(rMesh.m_csTScript == NULL || !rMesh.m_csTScript->getNoneBlendMesh()) &&
		(rNew.m_FlagRender & RENDER_TEXTURE) == RENDER_TEXTURE &&
		(rNew.m_FlagRender & cbmuUnsafeCompositeFlags) == 0)
	{
		rNew.m_CompositeOverlayCount = static_cast<unsigned char>(g_activeBmdCompositeOverlayCount);
		for (int i = 0; i < g_activeBmdCompositeOverlayCount; ++i)
		{
			rNew.m_CompositeTextureID[i] = g_activeBmdCompositeOverlays[i].textureId;
			rNew.m_CompositeMaterialMode[i] = g_activeBmdCompositeOverlays[i].materialMode;
			rNew.m_CompositeColor[i].x = g_activeBmdCompositeOverlays[i].color[0];
			rNew.m_CompositeColor[i].y = g_activeBmdCompositeOverlays[i].color[1];
			rNew.m_CompositeColor[i].z = g_activeBmdCompositeOverlays[i].color[2];
		}
	}
	// CBMu_END: Chụp overlay composite wearable vào command để renderer chỉ batch khi cùng bộ overlay.
#endif

#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
	// CBMu_BEGIN: Snapshot bone và ma trận cho command BMD khi defer flush theo character.
	float requestScale = pSrc->m_fRequestScale;
	const bool applyScale = requestScale != 1.0f && requestScale != 0.0f;
	if (IsDeferredFlushActive())
	{
#if CBMu_ENABLE_GL_BMD_PACKED_BONE_CACHE
		rNew.m_HasPackedBones = CBMu_PackBMDCommandBonesCached(pSrc, rMesh, m_finalBone, m_BoneSnapshotSerial, m_Transfrom, pSrc->BodyOrigin, pSrc->BodyScale, applyScale, requestScale, rNew.m_PackedBones,
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
			rNew.m_PackedBonesCached,
#else
			rNew.m_PackedBonesShared,
#endif
			rNew.m_PackedBoneRows,
			&rNew.m_CBMuPackedBoneRejectReason);
#else
		rNew.m_HasPackedBones = CBMu_PackBMDCommandBones(pSrc, rMesh, m_finalBone, m_Transfrom, pSrc->BodyOrigin, pSrc->BodyScale, applyScale, requestScale, rNew.m_PackedBones, rNew.m_PackedBoneRows, &rNew.m_CBMuPackedBoneRejectReason);
#endif
	}
	else
	{
		rNew.m_HasPackedBones = CBMu_PackBMDCommandBones(pSrc, rMesh, m_finalBone, m_Transfrom, pSrc->BodyOrigin, pSrc->BodyScale, applyScale, requestScale, rNew.m_PackedBones, rNew.m_PackedBoneRows, &rNew.m_CBMuPackedBoneRejectReason);
	}

#if CBMu_ENABLE_GL_BMD_MATRIX_CACHE
	GetActiveProjectionMatrix(rNew.m_ProjectionMatrix);
	GetActiveViewMatrix(rNew.m_ViewMatrix);
	rNew.m_HasCachedMatrices = true;
#endif
	// CBMu_END: Snapshot bone và ma trận cho command BMD khi defer flush theo character.
#endif

}

void CGMMeshShader::AddBoneTransform(BMD* model, float(*BoneMatrix)[3][4], bool trans)
{
	float* const bonePointer = (BoneMatrix != NULL) ? BoneMatrix[0][0] : NULL;
	m_Transfrom = trans;
	m_finalBone = bonePointer;
#if CBMu_ENABLE_GL_BMD_BONE_SNAPSHOT_POINTER_CACHE
	// CBMu_BEGIN: Cùng bone matrix pointer + transform flag → giữ serial, tránh pack bone lặp 6 body part.
	// Scratch BoneTransform (RenderLinkObject tay trái/phải) giữ địa chỉ, đổi nội dung mỗi lần.
	// Không reuse serial trên scratch — nếu không, 2 vũ khí (0,19) một cây tàng hình / chồng vị trí.
	const bool cbmuScratchBone = (bonePointer != NULL) && CBMu_IsLegacyScratchBoneBuffer(bonePointer);
	if (!cbmuScratchBone &&
		bonePointer != NULL &&
		bonePointer == m_CBMuLastBoneMatrixPointer &&
		trans == m_CBMuLastBoneTransform &&
		m_BoneSnapshotSerial != 0)
	{
#if CBMu_ENABLE_BODYMESH_DETAIL_COUNTERS
		CBMu_CPUHotspotRecordBodyMeshBoneSerial(true);
#endif
		return;
	}
	m_CBMuLastBoneMatrixPointer = bonePointer;
	m_CBMuLastBoneTransform = trans;
#if CBMu_ENABLE_BODYMESH_DETAIL_COUNTERS
	CBMu_CPUHotspotRecordBodyMeshBoneSerial(false);
#endif
	// CBMu_END: Cùng bone matrix pointer + transform flag → giữ serial, tránh pack bone lặp 6 body part.
#endif
	++m_BoneSnapshotSerial;
	if (m_BoneSnapshotSerial == 0)
	{
		++m_BoneSnapshotSerial;
	}
}
void CGMMeshShader::AddShadowCommand(BMD* bmd, int meshIndex, float sx, float sy, vec3_t target)
{
	//RenderMeshVAO cmd;
	//cmd.m_Shader = shaderProgramMap[SHADER_330_SHADOW];
	//cmd.m_OldBMD = bmd;
	//cmd.m_IndexMesh = meshIndex;
	//cmd.m_Shadow = true;
	//cmd.m_TextureID = -1;
	//cmd.m_isLight = false;
	//cmd.m_isAlpha = 1.0f;

	//// setting1
	//cmd.m_setting1.x = sx;
	//cmd.m_setting1.y = sy;
	//cmd.m_setting1.z = bmd->BodyOrigin[2];
	//cmd.m_setting1.w = 0.0f;

	//// setting2
	//cmd.m_setting2.x = bmd->BodyOrigin[0];
	//cmd.m_setting2.y = bmd->BodyOrigin[1];
	//cmd.m_setting2.z = bmd->BodyOrigin[2];
	//cmd.m_setting2.w = 0.0f;

	//// bodyLight = alpha bóng
	//cmd.m_bodyLight.x = bmd->BodyLight[0];
	//cmd.m_bodyLight.y = bmd->BodyLight[1];
	//cmd.m_bodyLight.z = bmd->BodyLight[2];
	//cmd.m_bodyLight.w = 0.20f;

	//// toa do
	//cmd.m_meshUV.x = target[0];
	//cmd.m_meshUV.y = target[1];
	//cmd.m_meshUV.z = target[2];
	//cmd.m_meshUV.w = 0;

	//cmd.m_FlagRender = RENDER_COLOR | RENDER_SHADOWMAP;

	//// ✅ add vào queue giống AddMeshCommand
	//m_Data.push_back(cmd);
}

void CGMMeshShader::BeginDeferredFlush()
{
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
	++m_DeferFlushDepth;
#endif
}

void CGMMeshShader::EndDeferredFlush()
{
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
	if (m_DeferFlushDepth <= 0)
	{
		return;
	}

	--m_DeferFlushDepth;
	if (m_DeferFlushDepth == 0 && HasPendingMesh())
	{
		FlushAllMesh();
		Lock(false);
	}
#endif
}

void CGMMeshShader::FlushDeferredNow()
{
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
	if (HasPendingMesh())
	{
		FlushAllMesh();
		Lock(false);
	}
#endif
}

std::mutex m_FlushMutex;
void CGMMeshShader::FlushAllMesh()
{
	CBMu_CPU_HOTSPOT_SCOPE(cbmuBMDFlushScope, CBMu_CPUHotspotPhase::BMDFlush);
	std::lock_guard<std::mutex> guard(m_FlushMutex);
	m_Lock = false;
	CBMu_LogObjectRenderQueue("CGMMeshShader::FlushAllMesh_begin", PendingMeshCount());
	if (!HasPendingMesh())
	{
		CBMu_LogObjectRenderQueue("CGMMeshShader::FlushAllMesh_end", PendingMeshCount());
		return;
	}
	CBMu_CPUHotspotRecordBMDFlushQueue(static_cast<unsigned int>(PendingMeshCount()));
	const std::size_t activeCount = PendingMeshCount();
#if CBMu_ENABLE_GL_BMD_COMMAND_POOL_REUSE
	MeshVAO::iterator activeEnd = m_Data.begin() + static_cast<MeshVAO::difference_type>(m_DataCount);
#else
	MeshVAO::iterator activeEnd = m_Data.end();
#endif

#if CBMu_ENABLE_GL_BMD_SKINNED_INSTANCED_BATCH && CBMu_ENABLE_GL_BMD_SORT_KEY_CACHE
	for (MeshVAO::iterator iter = m_Data.begin(); iter != activeEnd; ++iter)
	{
		CBMu_PrepareBMDSortKey(*iter);
	}
#endif

	// CBMu_BEGIN: Giữ producer order cho riêng các pass transparent/layered, cho phép các pass opaque cơ bản batch instancing.
	for (MeshVAO::iterator groupBegin = m_Data.begin(); groupBegin != activeEnd;)
	{
		MeshVAO::iterator groupEnd = groupBegin + 1;
		const std::uint64_t groupSerial = groupBegin->m_CBMuBoneSnapshotSerial;
		while (groupSerial != 0 && groupEnd != activeEnd && groupEnd->m_CBMuBoneSnapshotSerial == groupSerial)
		{
			++groupEnd;
		}
		groupBegin = groupEnd;
	}
	// CBMu_END: Cho phép các pass opaque cơ bản batch instancing.

#if CBMu_ENABLE_GL_BMD_COMMAND_SORT
	// CBMu_BEGIN: Sắp xếp command BMD một lần duy nhất tối ưu: nhóm cùng BMD, Submesh, Texture, Shader và giữ transparent ở cuối.
	std::stable_sort(m_Data.begin(), activeEnd, [](const RenderMeshVAO& lhs, const RenderMeshVAO& rhs)
		{
#if CBMu_ENABLE_GL_BMD_SKINNED_INSTANCED_BATCH && CBMu_ENABLE_GL_BMD_SORT_KEY_CACHE
			const bool lhsTransparent = lhs.m_CBMuTransparentForSort;
			const bool rhsTransparent = rhs.m_CBMuTransparentForSort;
#else
			const bool lhsTransparent = lhs.m_CBMuOrderSensitiveGroup ||
#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
				lhs.m_CBMuGoldenOverlay ||
#endif
				CBMu_IsBMDTransparentForSort(lhs);
			const bool rhsTransparent = rhs.m_CBMuOrderSensitiveGroup ||
#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
				rhs.m_CBMuGoldenOverlay ||
#endif
				CBMu_IsBMDTransparentForSort(rhs);
#endif

			if (lhsTransparent != rhsTransparent)
			{
				return !lhsTransparent && rhsTransparent;
			}
			if (lhs.m_OldBMD != rhs.m_OldBMD) return lhs.m_OldBMD < rhs.m_OldBMD;
			if (lhs.m_IndexMesh != rhs.m_IndexMesh) return lhs.m_IndexMesh < rhs.m_IndexMesh;
			if (lhs.m_TextureID != rhs.m_TextureID) return lhs.m_TextureID < rhs.m_TextureID;
			if (lhs.m_FlagRender != rhs.m_FlagRender) return lhs.m_FlagRender < rhs.m_FlagRender;
			return lhs.m_Shader < rhs.m_Shader;
		});
	// CBMu_END: Sắp xếp command BMD một lần duy nhất tối ưu.
#endif

#if CBMu_ENABLE_GL_BMD_MATRIX_CACHE
	// CBMu_BEGIN: Đọc ma trận BMD một lần ở biên flush thay vì từng mesh.
	float cbmuProjectionMatrix[16];
	float cbmuViewMatrix[16];
	GetActiveProjectionMatrix(cbmuProjectionMatrix);
	GetActiveViewMatrix(cbmuViewMatrix);

	for (MeshVAO::iterator iter = m_Data.begin(); iter != activeEnd; ++iter)
	{
		if (!iter->m_HasCachedMatrices)
		{
			memcpy(iter->m_ProjectionMatrix, cbmuProjectionMatrix, sizeof(cbmuProjectionMatrix));
			memcpy(iter->m_ViewMatrix, cbmuViewMatrix, sizeof(cbmuViewMatrix));
			iter->m_HasCachedMatrices = true;
		}
	}
	// CBMu_END: Đọc ma trận BMD một lần ở biên flush thay vì từng mesh.
#endif


#if CBMu_ENABLE_GL_BMD_SKINNED_INSTANCED_BATCH
	for (std::size_t i = 0; i < activeCount;)
	{
		std::size_t batchCount = 1;
		if (CBMu_IsBMDInstancedBatchCandidateFast(m_Data[i]))
		{
			GLsizei totalRows = m_Data[i].m_PackedBoneRows;
			const GLsizei maxRows =
#if CBMu_USE_VULKAN_NATIVE_MESH
				16384;
#elif CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER
				CBMu_GL_BMD_BONE_TEXTURE_BUFFER_MAX_ROWS;
#else
				256;
#endif
			while (i + batchCount < activeCount &&
				CBMu_SameBMDInstancedBatchKey(m_Data[i], m_Data[i + batchCount]) &&
				totalRows + m_Data[i + batchCount].m_PackedBoneRows <= maxRows)
			{
				totalRows += m_Data[i + batchCount].m_PackedBoneRows;
				++batchCount;
			}
#if CBMu_ENABLE_GL_BMD_SPLIT_COUNTERS
			if (i + batchCount < activeCount)
			{
				const CBMu_BMDBatchSplitReason cbmuSplitReason = CBMu_GetBMDBatchSplitReasonCode(m_Data[i], m_Data[i + batchCount], totalRows, maxRows);
				CBMu_CPUHotspotRecordBMDBatchSplit(cbmuSplitReason);
				CBMu_RecordBMDInstancedRejectDetail(m_Data[i + batchCount], cbmuSplitReason);
			}
#endif
		}
#if CBMu_ENABLE_GL_BMD_SPLIT_COUNTERS
		else
		{
			const CBMu_BMDBatchSplitReason cbmuSplitReason = CBMu_GetBMDInstancedRejectReasonCode(m_Data[i], true);
			CBMu_CPUHotspotRecordBMDBatchSplit(cbmuSplitReason);
			CBMu_RecordBMDInstancedRejectDetail(m_Data[i], cbmuSplitReason);
		}
#endif

		if (batchCount > 1 && g_NewRenderBMD->RenderInstanced(m_Data, i, batchCount))
		{
			CBMu_CPUHotspotRecordBMDDraw(true, static_cast<unsigned int>(batchCount));
			i += batchCount;
			continue;
		}

#if CBMu_ENABLE_GL_BMD_SPLIT_COUNTERS
		if (batchCount > 1)
		{
			CBMu_CPUHotspotRecordBMDBatchSplit(CBMu_BMDBatchSplitReason::RenderFallback);
		}
#endif
		g_NewRenderBMD->Render(m_Data[i]);
		CBMu_CPUHotspotRecordBMDDraw(false, 1);
		++i;
	}
#else
	for (MeshVAO::iterator iter = m_Data.begin(); iter != activeEnd; ++iter)
	{
		g_NewRenderBMD->Render(*iter);
		CBMu_CPUHotspotRecordBMDDraw(false, 1);
	}
#endif
#if CBMu_ENABLE_GL_BMD_SHADER_SCOPE_CACHE
	// Shader được giữ qua nhiều mesh trong một flush, nên chỉ unbind một lần ở biên pass.
	OGL330MODEL::UnUseShader();
#endif
	CBMu_ResetBMDVAOBindCache();
	CBMu_ResetBMDBoneTextureBufferCache();
	CBMu_ResetBMDWearableCompositeTextureUnits();
	CBMu_LogObjectRenderQueue("CGMMeshShader::FlushAllMesh_end", PendingMeshCount());
#if CBMu_ENABLE_GL_BMD_COMMAND_POOL_REUSE
	m_DataCount = 0;
#else
	m_Data.resize(0);
#endif
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
	CBMu_ClearBMDPackedBoneCache();
#endif
}
GLuint CGMMeshShader::GetShaderMap(int Type)
{
	return shaderProgramMap[Type];
}
#endif
