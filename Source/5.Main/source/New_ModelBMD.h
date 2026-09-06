#pragma once

#if CB_SHADER330_TEST
#include "CBMu/CBMu_RenderConfig.h"
#include <cstdint>
#include <memory>
#include <mutex>

class BMD;
class OBJECT;

static std::mutex gShaderMutex;

namespace OGL330MODEL
{
	typedef struct _mvec3
	{
		float x, y, z;
		_mvec3() : x(0.f), y(0.f), z(0.f) {}
		_mvec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
	}mvec3;

	typedef struct _mvec4
	{
		float x, y, z, w;
		_mvec4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
		_mvec4(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) {}
	}mvec4;

	void Init();
	void ConvertOldMeshToVaoMesh(GLuint iModel, bool DelMesh = true);
	void UseShader(GLuint shaderID);
	void UnUseShader();
	void SendUniform(GLuint shaderID, const mvec4& bodyLight, const mvec4& lightPosition, const mvec4& meshUV, const mvec4& setting1, const mvec4& setting2, const bool enableLight);
	void SetTargetRender(OBJECT*);
	GLuint GetShader(int idNumber);

#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
	// CBMu_BEGIN: Mô tả tối đa ba lớp material được gộp vào draw body BMD.
	struct BmdCompositeOverlay
	{
		int textureId;
		float materialMode;
		float color[3];

		BmdCompositeOverlay()
			: textureId(-1)
			, materialMode(0.0f)
			, color{ 1.0f, 1.0f, 1.0f }
		{
		}
	};

	void PushBmdCompositeOverlays(const BmdCompositeOverlay* overlays, int count);
	void PopBmdCompositeOverlays();
	// CBMu_END: Mô tả tối đa ba lớp material được gộp vào draw body BMD.
#endif

	// CBMu_BEGIN: Lưu lý do mesh không đóng gói được bone để profiler biết nên tối ưu nhánh nào.
	enum CBMu_BMDPackedBoneRejectReason : unsigned char
	{
		CBMu_BMD_PACKED_BONE_REJECT_NONE = 0,
		CBMu_BMD_PACKED_BONE_REJECT_NO_BONE_POINTER,
		CBMu_BMD_PACKED_BONE_REJECT_EMPTY_BONE_CONTAINER,
		CBMu_BMD_PACKED_BONE_REJECT_ROW_OVERFLOW,
		CBMu_BMD_PACKED_BONE_REJECT_INVALID_BONE_INDEX,
		CBMu_BMD_PACKED_BONE_REJECT_DEFER_INACTIVE,
		CBMu_BMD_PACKED_BONE_REJECT_UNKNOWN
	};
	// CBMu_END: Lưu lý do mesh không đóng gói được bone để profiler biết nên tối ưu nhánh nào.

}
class ShaderGuard {
public:

	ShaderGuard(GLuint shader) {
		gShaderMutex.lock();
		OGL330MODEL::UseShader(shader);
	}
	~ShaderGuard() {
#if CBMu_ENABLE_GL_BMD_SHADER_SCOPE_CACHE
		// Giữ shader đến cuối FlushAllMesh để tránh bind/unbind chương trình cho từng mesh.
#else
		OGL330MODEL::UnUseShader();
#endif
		gShaderMutex.unlock();
	}
};

class CGMNewRenderBMD;

namespace OGL330MODEL
{
#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
	void PushGoldenOverlayPass();
	void PopGoldenOverlayPass();
#endif

	typedef struct _RenderMeshVAO
	{
		BMD* m_OldBMD;
		int		m_IndexMesh;
		int		m_TextureID;
		int		m_FlagRender;
		int		m_ConstNumber;
		bool	m_isLight;
		GLuint  m_Shader;
		float	m_isAlpha;
		mvec3	m_isColor;
		mvec4   m_setting1;
		mvec4   m_setting2;
		mvec4   m_bodyLight;
		mvec4   m_meshUV;
		mvec4   m_lightPosition;
		bool	m_Shadow;
		std::uint64_t m_CBMuBoneSnapshotSerial;
		bool	m_CBMuOrderSensitiveGroup;
#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
		bool	m_CBMuGoldenOverlay;
#endif
#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
		unsigned char m_CompositeOverlayCount;
		int		m_CompositeTextureID[3];
		float	m_CompositeMaterialMode[3];
		mvec3	m_CompositeColor[3];
#endif
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
		std::vector<float> m_PackedBones;
#if CBMu_ENABLE_GL_BMD_PACKED_BONE_CACHE
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
		const std::vector<float>* m_PackedBonesCached;
#else
		std::shared_ptr<const std::vector<float> > m_PackedBonesShared;
#endif
#endif
		GLsizei m_PackedBoneRows;
		bool	m_HasPackedBones;
		CBMu_BMDPackedBoneRejectReason m_CBMuPackedBoneRejectReason;
#endif
#if CBMu_ENABLE_GL_BMD_MATRIX_CACHE
		float	m_ProjectionMatrix[16];
		float	m_ViewMatrix[16];
		bool	m_HasCachedMatrices;
#endif
#if CBMu_ENABLE_GL_BMD_SORT_KEY_CACHE
		bool	m_CBMuInstancedCandidate;
		bool	m_CBMuTransparentForSort;
#endif
	public:
		_RenderMeshVAO()
		{
			Reset();
		}

		void Reset()
		{
			m_OldBMD = NULL;
			m_IndexMesh = m_TextureID = m_FlagRender = -1;
			m_ConstNumber = 0;
			m_isLight = false;
			m_Shader = -1;
			m_isAlpha = 1.f;
			m_isColor.x = m_isColor.y = m_isColor.z = 1.f;
			m_Shadow = 0;
			m_CBMuBoneSnapshotSerial = 0;
			m_CBMuOrderSensitiveGroup = false;
#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
			m_CBMuGoldenOverlay = false;
#endif
#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
			m_CompositeOverlayCount = 0;
			for (int i = 0; i < 3; ++i)
			{
				m_CompositeTextureID[i] = -1;
				m_CompositeMaterialMode[i] = 0.0f;
				m_CompositeColor[i].x = 1.0f;
				m_CompositeColor[i].y = 1.0f;
				m_CompositeColor[i].z = 1.0f;
			}
#endif
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
			// Giữ capacity của vector để command BMD ở cảnh đông NPC không cấp phát lại mỗi frame.
			m_PackedBones.clear();
#if CBMu_ENABLE_GL_BMD_PACKED_BONE_CACHE
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
			m_PackedBonesCached = NULL;
#else
			m_PackedBonesShared.reset();
#endif
#endif
			m_PackedBoneRows = 0;
			m_HasPackedBones = false;
			m_CBMuPackedBoneRejectReason = CBMu_BMD_PACKED_BONE_REJECT_UNKNOWN;
#endif
#if CBMu_ENABLE_GL_BMD_MATRIX_CACHE
			memset(m_ProjectionMatrix, 0, sizeof(m_ProjectionMatrix));
			memset(m_ViewMatrix, 0, sizeof(m_ViewMatrix));
			m_HasCachedMatrices = false;
#endif
#if CBMu_ENABLE_GL_BMD_SORT_KEY_CACHE
			m_CBMuInstancedCandidate = false;
			m_CBMuTransparentForSort = false;
#endif
		}
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
		const std::vector<float>& PackedBones() const
		{
#if CBMu_ENABLE_GL_BMD_PACKED_BONE_CACHE
#if CBMu_ENABLE_GL_BMD_CACHE_VALUE_STORAGE
			return m_PackedBonesCached ? *m_PackedBonesCached : m_PackedBones;
#else
			return m_PackedBonesShared ? *m_PackedBonesShared : m_PackedBones;
#endif
#else
			return m_PackedBones;
#endif
		}
#endif
	}RenderMeshVAO;
	typedef std::vector<RenderMeshVAO> MeshVAO;

	class CGMMeshShader
	{
	private:
		MeshVAO	    m_Data;
		std::size_t m_DataCount;
		std::mutex  m_CommandMutex;
		bool		m_Lock;
		bool		m_Enabled;

		vec3_t		m_vLightPosOrg;
		vec3_t		m_vLightDirOrg;

		vec3_t		m_vLightPos;
		vec3_t		m_vLightDir;

		static bool		m_Transfrom;
		static float* m_finalBone;
		static std::uint64_t m_BoneSnapshotSerial;
#if CBMu_ENABLE_GL_BMD_BONE_SNAPSHOT_POINTER_CACHE
		static float* m_CBMuLastBoneMatrixPointer;
		static bool m_CBMuLastBoneTransform;
#endif
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
		int			m_DeferFlushDepth;
#endif

		CGMMeshShader(const CGMMeshShader&);
		CGMMeshShader& operator=(const CGMMeshShader&);

		bool IsAlpha(int iType);

		void MakeShaderType(int iShaderType, bool enableLight, bool bAlphaNoUse, float BlendU, float BlendV, RenderMeshVAO& r);
		RenderMeshVAO& AllocateMeshCommand();
		void DiscardLastMeshCommand();

	public:
		CGMMeshShader();
		~CGMMeshShader();

		void SetHighLight(bool bHighLight = false, bool bBattleCastle = false);

		inline bool Enabled() { return m_Enabled; }
		inline void Toggle(bool b) { m_Enabled = b; }
		inline void Lock(bool b) { m_Lock = b; }
#if CBMu_ENABLE_GL_BMD_COMMAND_POOL_REUSE
		inline bool HasPendingMesh() const { return m_DataCount > 0; }
		inline std::size_t PendingMeshCount() const { return m_DataCount; }
#else
		inline bool HasPendingMesh() const { return !m_Data.empty(); }
		inline std::size_t PendingMeshCount() const { return m_Data.size(); }
#endif
		inline bool IsDeferredFlushActive() const {
#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
			return m_DeferFlushDepth > 0;
#else
			return false;
#endif
		}
		inline void	SetTransfrom(bool b) { m_Transfrom = b; }
		inline bool GetTransfrom() { return m_Transfrom; }

		inline void SetLightPosition(vec3_t vPos, vec3_t vDir)
		{
			VectorCopy(vPos, m_vLightPos); VectorCopy(vDir, m_vLightDir);
		}

		void AddBoneTransform(BMD* model, float(*BoneMatrix)[3][4], bool trans);
		inline float* GetfinalBone() { return m_finalBone; }
		void AddMeshCommand(BMD* pSrc, int idx, int RFlag, float Alpha, int BlendMesh, float Light, float BlendU, float BlendV, int Texture);
		void BeginDeferredFlush();
		void EndDeferredFlush();
		void FlushDeferredNow();
		void FlushAllMesh();
		void Release();

		void AddShadowCommand(BMD* bmd, int meshIndex, float sx, float sy, vec3_t target);
		GLuint GetShaderMap(int Type);


	public:
		static CGMMeshShader* Instance()
		{
			static CGMMeshShader sInstance;
			return &sInstance;
		};
	};
}


class rRenderLayOut
{
public:
	rRenderLayOut(OBJECT* pObj) {
		OGL330MODEL::SetTargetRender(pObj);
	}
	~rRenderLayOut() {
		OGL330MODEL::SetTargetRender(NULL);
	}
};

#if CBMu_ENABLE_GL_BMD_GOLDEN_OVERLAY_SEMANTIC
class rGoldenOverlayPass
{
public:
	rGoldenOverlayPass() { OGL330MODEL::PushGoldenOverlayPass(); }
	~rGoldenOverlayPass() { OGL330MODEL::PopGoldenOverlayPass(); }
};
#endif

#define GMMeshShader (OGL330MODEL::CGMMeshShader::Instance())

#endif
