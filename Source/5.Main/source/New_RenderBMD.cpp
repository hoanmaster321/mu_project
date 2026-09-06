#include "StdAfx.h"
#include "New_RenderBMD.h"
#include "CBMu/Render/CBMu_CPUHotspotProfiler.h"

#if CB_SHADER330_TEST
#include "ZzzBMD.h"
#include "ZzzTexture.h"
#include "TextureScript.h"
#include "GPUContext.h"
#include "VulkanGLStub.h"
#include "Utilities/Log/muConsoleDebug.h"
#include "CBMu/Render/CBMu_GLObjectDebugLog.h"
#include "ZzzScene.h"
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

CGMNewRenderBMD* g_NewRenderBMD = NULL;

namespace
{
	void CBMu_LogShader330(const char*, ...)
	{
	}

#if CBMu_ENABLE_GL_BMD_BONE_UNIFORM_CACHE
	GLint CBMu_GetBMDBoneUniformLocation(GLuint shaderID)
	{
		static std::unordered_map<GLuint, GLint> s_BoneUniformLocations;
		std::unordered_map<GLuint, GLint>::iterator iter = s_BoneUniformLocations.find(shaderID);
		if (iter != s_BoneUniformLocations.end())
		{
			return iter->second;
		}


		const GLint location = glGetUniformLocation(shaderID, "u_Bones");
		s_BoneUniformLocations[shaderID] = location;
		return location;
	}
#endif

	GLint CBMu_GetBMDUniformLocationCached(GLuint shaderID, const char* name)
	{
		static std::unordered_map<GLuint, std::unordered_map<std::string, GLint> > s_Locations;
		std::unordered_map<std::string, GLint>& shaderLocations = s_Locations[shaderID];
		std::unordered_map<std::string, GLint>::iterator iter = shaderLocations.find(name);
		if (iter != shaderLocations.end())
		{
			return iter->second;
		}

		const GLint location = glGetUniformLocation(shaderID, name);
		shaderLocations[name] = location;
		return location;
	}

#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE

	struct CBMu_BMDCompositeUniformState
	{
		int CompositeOverlayCount = -1;
		int CompositeTextureSampler[3] = { -1, -1, -1 };
	};

	static std::unordered_map<GLuint, CBMu_BMDCompositeUniformState> s_CBMuBMDCompositeUniformStates;

	void CBMu_BindCompositeTextureUnit(GLenum unit, int textureId)
	{
		GLuint textureNumber = 0;
		if (textureId >= 0)
		{
			BITMAP_t* bitmap = Bitmaps.GetTexture(textureId);
			if (bitmap != NULL)
			{
				textureNumber = bitmap->TextureNumber;
			}
		}

		glActiveTexture(unit);
		glBindTexture(GL_TEXTURE_2D, textureNumber);
	}

	void CBMu_ApplyBMDWearableComposite(GLuint shaderID, const OGL330MODEL::RenderMeshVAO& command)
	{
		CBMu_BMDCompositeUniformState& state = s_CBMuBMDCompositeUniformStates[shaderID];
		const GLint countLoc = CBMu_GetBMDUniformLocationCached(shaderID, "uCompositeOverlayCount");
		const GLint timeLoc = CBMu_GetBMDUniformLocationCached(shaderID, "uCompositeTime");
		const GLint colorModeLoc[3] = {
			CBMu_GetBMDUniformLocationCached(shaderID, "uCompositeColorMode0"),
			CBMu_GetBMDUniformLocationCached(shaderID, "uCompositeColorMode1"),
			CBMu_GetBMDUniformLocationCached(shaderID, "uCompositeColorMode2"),
		};
		const GLint samplerLoc[3] = {
			CBMu_GetBMDUniformLocationCached(shaderID, "uCompositeTexture0"),
			CBMu_GetBMDUniformLocationCached(shaderID, "uCompositeTexture1"),
			CBMu_GetBMDUniformLocationCached(shaderID, "uCompositeTexture2"),
		};
		const GLint samplerUnits[3] = { 4, 5, 6 };
		const GLenum activeUnits[3] = { GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6 };
		const int overlayCount = static_cast<int>(command.m_CompositeOverlayCount);

		if (countLoc >= 0 && state.CompositeOverlayCount != overlayCount)
		{
			glUniform1i(countLoc, overlayCount);
			state.CompositeOverlayCount = overlayCount;
		}
		if (timeLoc >= 0)
		{
			glUniform1f(timeLoc, WorldTime);
		}

		for (int i = 0; i < 3; ++i)
		{
			if (samplerLoc[i] >= 0 && state.CompositeTextureSampler[i] != samplerUnits[i])
			{
				glUniform1i(samplerLoc[i], samplerUnits[i]);
				state.CompositeTextureSampler[i] = samplerUnits[i];
			}

			if (i < overlayCount)
			{
				if (colorModeLoc[i] >= 0)
				{
					glUniform4f(
						colorModeLoc[i],
						command.m_CompositeColor[i].x,
						command.m_CompositeColor[i].y,
						command.m_CompositeColor[i].z,
						command.m_CompositeMaterialMode[i]);
				}
				CBMu_BindCompositeTextureUnit(activeUnits[i], command.m_CompositeTextureID[i]);
			}
			else
			{
				if (colorModeLoc[i] >= 0)
				{
					glUniform4f(colorModeLoc[i], 1.0f, 1.0f, 1.0f, 0.0f);
				}
				CBMu_BindCompositeTextureUnit(activeUnits[i], -1);
			}
		}

		glActiveTexture(GL_TEXTURE0);
	}

#endif

	struct CBMu_BMDInstanceData
	{
		GLuint BoneBase;
		GLfloat Padding[3];
		GLfloat BodyLight[4];
		GLfloat LightPosition[4];
	};

#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER

	bool CBMu_LogBMDTBOFailure(const char* step, GLenum error, GLsizei rowCount, std::size_t floatCount, std::size_t byteCount)
	{
		CBMu_LogShader330("BMD TBO disabled: %s error=0x%04X rows=%d floats=%u bytes=%u",
			step,
			static_cast<unsigned int>(error),
			static_cast<int>(rowCount),
			static_cast<unsigned int>(floatCount),
			static_cast<unsigned int>(byteCount));
		return false;
	}

#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER
	struct CBMu_BMDBoneTextureShaderState
	{
		int BoneRowsSampler = -1;
		int UseBoneTexture = -1;
		int UseInstanceParams = -1;
	};

	std::unordered_map<GLuint, CBMu_BMDBoneTextureShaderState> s_CBMuBMDBoneTextureShaderStates;
	GLuint s_CBMuBMDBoundTextureBufferOnUnit3 = 0;

	void CBMu_SetBMDBoneRowsSampler(GLuint shaderID)
	{
		CBMu_BMDBoneTextureShaderState& state = s_CBMuBMDBoneTextureShaderStates[shaderID];
		if (state.BoneRowsSampler == 3)
		{
			return;
		}
		const GLint boneRowsLoc = CBMu_GetBMDUniformLocationCached(shaderID, "u_BoneRows");
		if (boneRowsLoc >= 0)
		{
			glUniform1i(boneRowsLoc, 3);
			state.BoneRowsSampler = 3;
		}
	}

	void CBMu_SetBMDUseBoneTexture(GLuint shaderID, int value)
	{
		CBMu_BMDBoneTextureShaderState& state = s_CBMuBMDBoneTextureShaderStates[shaderID];
		if (state.UseBoneTexture == value)
		{
			return;
		}
		const GLint useBoneTextureLoc = CBMu_GetBMDUniformLocationCached(shaderID, "uUseBoneTexture");
		if (useBoneTextureLoc >= 0)
		{
			glUniform1i(useBoneTextureLoc, value);
			state.UseBoneTexture = value;
		}
	}

	void CBMu_SetBMDUseInstanceParams(GLuint shaderID, int value)
	{
		CBMu_BMDBoneTextureShaderState& state = s_CBMuBMDBoneTextureShaderStates[shaderID];
		if (state.UseInstanceParams == value)
		{
			return;
		}
		const GLint useInstanceParamsLoc = CBMu_GetBMDUniformLocationCached(shaderID, "uUseInstanceParams");
		if (useInstanceParamsLoc >= 0)
		{
			glUniform1i(useInstanceParamsLoc, value);
			state.UseInstanceParams = value;
		}
	}
#endif

	bool CBMu_UploadBMDBoneTextureBuffer(GLuint shaderID, const std::vector<float>& rows, GLsizei rowCount)
	{
		if (rowCount <= 0 || rows.empty())
		{
			return false;
		}

		static bool s_TBODisabledForSession = false;
		if (s_TBODisabledForSession)
		{
			return false;
		}


#if CBMu_ENABLE_GL_BMD_BONE_TBO_RING
		static GLuint s_BoneTBO[3] = {};
		static GLuint s_BoneTexture[3] = {};
		static std::size_t s_BufferBytes[3] = {};
		static bool s_TextureBufferAttached[3] = {};
		static unsigned int s_BoneTBOCursor = 0;
		const unsigned int cbmuBoneTBOIndex = s_BoneTBOCursor++ % 3u;
		if (s_BoneTBO[cbmuBoneTBOIndex] == 0)
		{
			glGenBuffers(1, &s_BoneTBO[cbmuBoneTBOIndex]);
			glGenTextures(1, &s_BoneTexture[cbmuBoneTBOIndex]);
			CBMu_LogShader330("BMD TBO create buffer=%u texture=%u slot=%u", s_BoneTBO[cbmuBoneTBOIndex], s_BoneTexture[cbmuBoneTBOIndex], cbmuBoneTBOIndex);
		}
		GLuint cbmuBoneTBO = s_BoneTBO[cbmuBoneTBOIndex];
		GLuint cbmuBoneTexture = s_BoneTexture[cbmuBoneTBOIndex];
		std::size_t& cbmuBufferBytes = s_BufferBytes[cbmuBoneTBOIndex];
		bool& cbmuTextureBufferAttached = s_TextureBufferAttached[cbmuBoneTBOIndex];
#else
		static GLuint cbmuBoneTBO = 0;
		static GLuint cbmuBoneTexture = 0;
		static std::size_t cbmuBufferBytes = 0;
		static bool cbmuTextureBufferAttached = false;
		if (cbmuBoneTBO == 0)
		{
			glGenBuffers(1, &cbmuBoneTBO);
			glGenTextures(1, &cbmuBoneTexture);
			CBMu_LogShader330("BMD TBO create buffer=%u texture=%u", cbmuBoneTBO, cbmuBoneTexture);
		}
#endif


		const std::size_t byteCount = rows.size() * sizeof(float);
		glBindBuffer(GL_TEXTURE_BUFFER, cbmuBoneTBO);

		if (cbmuBufferBytes < byteCount)
		{

			glBufferData(GL_TEXTURE_BUFFER, byteCount, rows.data(), GL_STREAM_DRAW);
		}
		else
		{
			glBufferSubData(GL_TEXTURE_BUFFER, 0, byteCount, rows.data());
		}
		if (cbmuBufferBytes < byteCount)
		{
			cbmuBufferBytes = byteCount;
		}

		glActiveTexture(GL_TEXTURE3);

#if CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
		if (s_CBMuBMDBoundTextureBufferOnUnit3 != cbmuBoneTexture)
		{
			glBindTexture(GL_TEXTURE_BUFFER, cbmuBoneTexture);
			s_CBMuBMDBoundTextureBufferOnUnit3 = cbmuBoneTexture;
		}
#else
		glBindTexture(GL_TEXTURE_BUFFER, cbmuBoneTexture);
#endif

		if (!cbmuTextureBufferAttached)
		{
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, cbmuBoneTBO);
			cbmuTextureBufferAttached = true;
		}

		const GLint boneRowsLoc = CBMu_GetBMDUniformLocationCached(shaderID, "u_BoneRows");
		const GLint useBoneTextureLoc = CBMu_GetBMDUniformLocationCached(shaderID, "uUseBoneTexture");
		if (boneRowsLoc < 0 || useBoneTextureLoc < 0)
		{
			s_TBODisabledForSession = true;
			CBMu_LogShader330("BMD TBO upload FAIL: missing uniforms shader=%u u_BoneRows=%d uUseBoneTexture=%d",
				shaderID,
				boneRowsLoc,
				useBoneTextureLoc);
			glActiveTexture(GL_TEXTURE0);
			return false;
		}

#if CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
		CBMu_SetBMDBoneRowsSampler(shaderID);
		CBMu_SetBMDUseBoneTexture(shaderID, 1);
#else
		glUniform1i(boneRowsLoc, 3);
		glUniform1i(useBoneTextureLoc, 1);
#endif
		glActiveTexture(GL_TEXTURE0);
		CBMu_CPUHotspotRecordBMDTBOUpload(static_cast<unsigned long long>(byteCount));
		return true;
	}

	void CBMu_DisableBMDBoneTextureBuffer(GLuint shaderID)
	{
#if CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
		CBMu_SetBMDUseBoneTexture(shaderID, 0);
		CBMu_SetBMDUseInstanceParams(shaderID, 0);
#else
		const GLint useBoneTextureLoc = CBMu_GetBMDUniformLocationCached(shaderID, "uUseBoneTexture");
		if (useBoneTextureLoc >= 0)
		{
			glUniform1i(useBoneTextureLoc, 0);
		}
		const GLint useInstanceParamsLoc = CBMu_GetBMDUniformLocationCached(shaderID, "uUseInstanceParams");
		if (useInstanceParamsLoc >= 0)
		{
			glUniform1i(useInstanceParamsLoc, 0);
		}
#endif
		glActiveTexture(GL_TEXTURE3);
#if CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
		if (s_CBMuBMDBoundTextureBufferOnUnit3 != 0)
		{
			glBindTexture(GL_TEXTURE_BUFFER, 0);
			s_CBMuBMDBoundTextureBufferOnUnit3 = 0;
		}
#else
		glBindTexture(GL_TEXTURE_BUFFER, 0);
#endif
		glActiveTexture(GL_TEXTURE0);
	}
#endif
}

namespace
{
	GLuint s_CBMuBoundBMDVAO = 0;
}

void CBMu_BindBMDVAO(GLuint vao)
{
#if CBMu_ENABLE_GL_BMD_VAO_BIND_CACHE
	if (s_CBMuBoundBMDVAO != vao)
	{
		glBindVertexArray(vao);
		s_CBMuBoundBMDVAO = vao;
	}
#else
	glBindVertexArray(vao);
#endif
}

void CBMu_UnbindBMDVAOAfterDraw()
{
#if CBMu_ENABLE_GL_BMD_VAO_BIND_CACHE

#else
	glBindVertexArray(0);
#endif
}

void CBMu_ResetBMDVAOBindCache()
{
#if CBMu_ENABLE_GL_BMD_VAO_BIND_CACHE
	if (s_CBMuBoundBMDVAO != 0)
	{
		glBindVertexArray(0);
		s_CBMuBoundBMDVAO = 0;
	}
#endif
}

void CBMu_ResetBMDBoneTextureBufferCache()
{
#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER && CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
	s_CBMuBMDBoneTextureShaderStates.clear();
	if (s_CBMuBMDBoundTextureBufferOnUnit3 != 0)
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
		glActiveTexture(GL_TEXTURE0);
		s_CBMuBMDBoundTextureBufferOnUnit3 = 0;
	}
#endif
}

void CBMu_ResetBMDWearableCompositeTextureUnits()
{
#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE

	for (GLenum unit = GL_TEXTURE4; unit <= GL_TEXTURE6; ++unit)
	{
		glActiveTexture(unit);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glActiveTexture(GL_TEXTURE0);
#endif
}

bool MeshHasValidBone(VAOMesh& mesh, int numBones)
{
	for (short b : mesh.BoneContainer)
	{
		if (b < 0 || b >= numBones * 3)
			return false;
	}
	return true;
}
bool MeshIsTooLarge(VAOMesh& mesh)
{
	for (auto& v : mesh.VBuffer)
	{
		float dist = sqrtf(v.m_vPos[0] * v.m_vPos[0] + v.m_vPos[1] * v.m_vPos[1] + v.m_vPos[2] * v.m_vPos[2]);
		if (dist > 500.0f)
			return true;
	}
	return false;
}
bool MeshIndexTooLarge(VAOMesh& mesh)
{
	return mesh.IndexCount > 5000;
}
bool MeshHasValidUV(VAOMesh& mesh)
{
	for (auto& v : mesh.VBuffer)
	{
		if (v.m_vTex[0] != 0.0f || v.m_vTex[1] != 0.0f)
			return true;
	}
	return false;
}

bool CanRenderShadow(BMD* bmd, VAOMesh& mesh)
{

	if (!MeshHasValidBone(mesh, bmd->NumBones))
		return false;


	if (MeshIsTooLarge(mesh))
		return false;


	if (MeshIndexTooLarge(mesh))
		return false;


	if (!MeshHasValidUV(mesh))
		return false;

	return true;
}

static bool CBMu_RequiresLegacyMaterialPass(const OGL330MODEL::RenderMeshVAO& item)
{
	if (item.m_Shadow) return true;
	return false;
}

bool CGMShaderBMD::RenderInstanced(const OGL330MODEL::MeshVAO& data, std::size_t start, std::size_t count)
{
#if CBMu_ENABLE_GL_BMD_SKINNED_INSTANCED_BATCH
	if (count < 2 || start >= data.size())
	{
		return false;
	}

	const OGL330MODEL::RenderMeshVAO& first = data[start];
	if (CBMu_RequiresLegacyMaterialPass(first))
	{
		return false;
	}
	BMD& rModel = *first.m_OldBMD;
	VAOMesh& rNewMesh = rModel.New_Meshs[first.m_IndexMesh];
	if (!rNewMesh.VAO || !first.m_HasPackedBones || first.m_PackedBoneRows <= 0)
	{
		return false;
	}

#if CBMu_USE_VULKAN_NATIVE_MESH
	if (!CBMu_RequiresLegacyMaterialPass(first) && GPUContext::Instance().IsFrameActive() && rNewMesh.vkVertexBuffer != VK_NULL_HANDLE && rNewMesh.vkIndexBuffer != VK_NULL_HANDLE)
	{
		const std::size_t totalFloats = count * static_cast<std::size_t>(first.m_PackedBoneRows) * 4u;
		const uint32_t totalBytes = static_cast<uint32_t>(totalFloats * sizeof(float));

		uint32_t boneBaseRows = 0;
		uint32_t boneSSBOOffset = 0;
		float* dstBones = static_cast<float*>(GPUContext::Instance().AllocateBoneBuffer(totalBytes, boneBaseRows, boneSSBOOffset));
		if (!dstBones)
		{
			return false;
		}

		using GPUInstanceData = GPUContext::GPUInstanceData;

		uint32_t baseInstanceIndex = 0;
		uint32_t instanceSSBOStart = 0;
		GPUInstanceData* dstInstances = GPUContext::Instance().AllocateInstanceBuffer(static_cast<uint32_t>(count), baseInstanceIndex, instanceSSBOStart);
		if (!dstInstances)
		{
			return false;
		}

		std::size_t cbmuBoneRowFloatsWritten = 0;
		GLsizei currentBoneRow = 0;
		for (std::size_t i = 0; i < count; ++i)
		{
			const OGL330MODEL::RenderMeshVAO& item = data[start + i];
			if (!item.m_HasPackedBones || item.m_PackedBoneRows <= 0)
			{
				return false;
			}

			const std::vector<float>& packedBones = item.PackedBones();
			const std::size_t cbmuBoneRowFloats = packedBones.size();
			if (cbmuBoneRowFloatsWritten + cbmuBoneRowFloats > totalFloats)
			{
				return false;
			}
			if (cbmuBoneRowFloats > 0)
			{
				memcpy(dstBones + cbmuBoneRowFloatsWritten, packedBones.data(), cbmuBoneRowFloats * sizeof(float));
				cbmuBoneRowFloatsWritten += cbmuBoneRowFloats;
			}

			GPUInstanceData& inst = dstInstances[i];
			inst = {};
			inst.modelMatrix = glm::vec4(0.0f);
			inst.lightColor = glm::vec4(item.m_bodyLight.x, item.m_bodyLight.y, item.m_bodyLight.z, item.m_isAlpha);
			inst.sourceIndex = static_cast<int32_t>(item.m_TextureID);
			inst.blendUV = glm::vec2(item.m_meshUV.x, item.m_meshUV.y);
			inst.newScale = 0.0f;
			inst.alpha = item.m_isAlpha;
			inst.bodyScale = 1.0f;
			inst.boneScale = 1.0f;
			inst.boneOffset = static_cast<int32_t>(currentBoneRow + boneBaseRows);
			inst.customIntensity = 1.0f;
			inst.bodyLightColor = glm::vec3(item.m_bodyLight.x, item.m_bodyLight.y, item.m_bodyLight.z);

			uint32_t flags = 0x02u; // texture
			if (item.m_isLight) flags |= 0x04u;
			if (item.m_meshUV.z > 0.5f) flags |= 0x01u;
			if ((item.m_FlagRender & RENDER_COLOR) != 0) flags |= 0x08u;
			if ((item.m_FlagRender & RENDER_BRIGHT) != 0) flags |= 0x20u;

			int32_t chromeMode = 0;
			int32_t finalRenderMode = 0;

			if ((item.m_FlagRender & RENDER_METAL) != 0)
			{
				flags |= 0x10u;
				chromeMode = 1;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}
			else if ((item.m_FlagRender & RENDER_CHROME2) != 0)
			{
				flags |= 0x10u;
				chromeMode = 2;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}
			else if ((item.m_FlagRender & RENDER_CHROME3) != 0)
			{
				flags |= 0x10u;
				chromeMode = 3;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}
			else if ((item.m_FlagRender & RENDER_CHROME4) != 0 || (item.m_FlagRender & RENDER_OIL) != 0)
			{
				flags |= 0x10u;
				chromeMode = 4;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}
			else if ((item.m_FlagRender & RENDER_CHROME5) != 0)
			{
				flags |= 0x10u;
				chromeMode = 5;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}
			else if ((item.m_FlagRender & RENDER_CHROME6) != 0)
			{
				flags |= 0x10u;
				chromeMode = 6;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}
			else if ((item.m_FlagRender & RENDER_CHROME7) != 0)
			{
				flags |= 0x10u;
				chromeMode = 7;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}
			else if ((item.m_FlagRender & RENDER_CHROME8) != 0)
			{
				flags |= 0x10u;
				chromeMode = 8;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}
			else if ((item.m_FlagRender & RENDER_CHROME) != 0)
			{
				flags |= 0x10u;
				chromeMode = 0;
				finalRenderMode = (item.m_meshUV.z > 0.5f) ? 3 : 2;
			}

			inst.flags = flags;
			inst.chromeMode = chromeMode;
			inst.finalRenderMode = finalRenderMode;
			inst.renderMode = 0;

			currentBoneRow += item.m_PackedBoneRows;
		}

		BITMAP_t* pBitmap = (first.m_TextureID >= 0) ? Bitmaps.GetTexture(static_cast<GLuint>(first.m_TextureID)) : nullptr;
		const uint32_t vkTextureId = pBitmap ? pBitmap->TextureNumber : (first.m_TextureID < 0 ? static_cast<uint32_t>(-first.m_TextureID) : 0);

		int blendType = 0;
		if ((first.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT || (first.m_FlagRender & (RENDER_CHROME3 | RENDER_CHROME4 | RENDER_CHROME5 | RENDER_CHROME7)) != 0) blendType = 2;
		else if ((first.m_FlagRender & RENDER_DARK) == RENDER_DARK) blendType = 3;
		else if (first.m_isAlpha < 0.99f || (pBitmap && pBitmap->Components == 4)) blendType = 1;
		bool depthWrite = ((first.m_FlagRender & RENDER_NODEPTH) == 0) && (blendType == 0);
		bool depthTest = (first.m_FlagRender & RENDER_NODEPTH) == 0;

		RenderUniformBlock blk;
		FillRenderBlock(blk, first);

		GPUContext::Instance().DrawInstancedMeshPreallocated(
			rNewMesh.vkVertexBuffer, rNewMesh.vkIndexBuffer, rNewMesh.IndexCount,
			static_cast<uint32_t>(count), vkTextureId,
			blendType, depthTest, depthWrite,
			&blk, sizeof(RenderUniformBlock),
			instanceSSBOStart, baseInstanceIndex,
			boneSSBOOffset, static_cast<uint32_t>(cbmuBoneRowFloatsWritten * sizeof(float))
		);
		return true;
	}
#endif

	static std::vector<CBMu_BMDInstanceData> s_InstanceData;
	static std::vector<float> s_BoneRows;
	static bool s_BuffersInitialized = false;
	if (!s_BuffersInitialized)
	{
		s_InstanceData.reserve(1024);
		s_BoneRows.reserve(65536);
		s_BuffersInitialized = true;
	}
	s_InstanceData.clear();
	s_BoneRows.clear();
	s_InstanceData.reserve(count);
#if CBMu_ENABLE_GL_BMD_FAST_BONE_ROW_UPLOAD_PACK
	std::size_t cbmuBoneRowWrite = 0;
	s_BoneRows.resize(count * static_cast<std::size_t>(first.m_PackedBoneRows) * 4u);
#else
	s_BoneRows.reserve(count * static_cast<std::size_t>(first.m_PackedBoneRows) * 4u);
#endif

	GLsizei totalRows = 0;
	for (std::size_t i = 0; i < count; ++i)
	{
		const OGL330MODEL::RenderMeshVAO& item = data[start + i];
		const GLsizei maxRows =
#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER
			CBMu_GL_BMD_BONE_TEXTURE_BUFFER_MAX_ROWS;
#else
			256;
#endif
		if (!item.m_HasPackedBones || item.m_PackedBoneRows <= 0 || totalRows + item.m_PackedBoneRows > maxRows)
		{
			return false;
		}

		CBMu_BMDInstanceData instanceData = {};
		instanceData.BoneBase = static_cast<GLuint>(totalRows);
		instanceData.BodyLight[0] = item.m_bodyLight.x;
		instanceData.BodyLight[1] = item.m_bodyLight.y;
		instanceData.BodyLight[2] = item.m_bodyLight.z;
		instanceData.BodyLight[3] = item.m_bodyLight.w;
		instanceData.LightPosition[0] = item.m_lightPosition.x;
		instanceData.LightPosition[1] = item.m_lightPosition.y;
		instanceData.LightPosition[2] = item.m_lightPosition.z;
		instanceData.LightPosition[3] = item.m_lightPosition.w;
		s_InstanceData.push_back(instanceData);
		const std::vector<float>& packedBones = item.PackedBones();
#if CBMu_ENABLE_GL_BMD_FAST_BONE_ROW_UPLOAD_PACK
		const std::size_t cbmuBoneRowFloats = packedBones.size();
		if (cbmuBoneRowWrite + cbmuBoneRowFloats > s_BoneRows.size())
		{
			return false;
		}
		if (cbmuBoneRowFloats > 0)
		{
			memcpy(&s_BoneRows[cbmuBoneRowWrite], packedBones.data(), cbmuBoneRowFloats * sizeof(float));
			cbmuBoneRowWrite += cbmuBoneRowFloats;
		}
#else
		s_BoneRows.insert(s_BoneRows.end(), packedBones.begin(), packedBones.end());
#endif
		totalRows += item.m_PackedBoneRows;
	}
#if CBMu_ENABLE_GL_BMD_FAST_BONE_ROW_UPLOAD_PACK
	s_BoneRows.resize(cbmuBoneRowWrite);
#endif

	RenderUniformBlock blk;
	FillRenderBlock(blk, first);

	ShaderGuard guard(first.m_Shader);
	static UniformBlockCache ubCache;
	ubCache.Update(blk);
	ubCache.Bind(first.m_Shader, "RenderBlock", 0);
#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
	CBMu_ApplyBMDWearableComposite(first.m_Shader, first);
#endif

	glEnable(GL_TEXTURE_2D);
	BindTexture(first.m_TextureID);
	if ((first.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT)
	{
		EnableAlphaBlend();
	}
	else if (first.m_isAlpha < 0.99f || Bitmaps[first.m_TextureID].Components == 4)
	{
		EnableAlphaTest();
	}
	else
	{
		DisableAlphaBlend();
		glDisable(GL_ALPHA_TEST);
	}

	if ((first.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH)
	{
		DisableDepthTest();
	}
	else
	{
		EnableDepthTest();
	}

#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER
	if (!CBMu_UploadBMDBoneTextureBuffer(first.m_Shader, s_BoneRows, totalRows))
	{
		if (totalRows > 256)
		{
			return false;
		}

#if CBMu_ENABLE_GL_BMD_BONE_UNIFORM_CACHE
		const GLint fallbackBaseLoc = CBMu_GetBMDBoneUniformLocation(first.m_Shader);
#else
		const GLint fallbackBaseLoc = glGetUniformLocation(first.m_Shader, "u_Bones");
#endif
		if (fallbackBaseLoc < 0)
		{
			return false;
		}

		const GLint useBoneTextureLoc = CBMu_GetBMDUniformLocationCached(first.m_Shader, "uUseBoneTexture");
		if (useBoneTextureLoc >= 0)
		{
#if CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
			CBMu_SetBMDUseBoneTexture(first.m_Shader, 0);
#else
			glUniform1i(useBoneTextureLoc, 0);
#endif
		}
		glUniform4fv(fallbackBaseLoc, totalRows, s_BoneRows.data());
	}
	const GLint useInstanceParamsLoc = CBMu_GetBMDUniformLocationCached(first.m_Shader, "uUseInstanceParams");
	if (useInstanceParamsLoc >= 0)
	{
#if CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
		CBMu_SetBMDUseInstanceParams(first.m_Shader, 1);
#else
		glUniform1i(useInstanceParamsLoc, 1);
#endif
	}
#else
#if CBMu_ENABLE_GL_BMD_BONE_UNIFORM_CACHE
	const GLint baseLoc = CBMu_GetBMDBoneUniformLocation(first.m_Shader);
#else
	const GLint baseLoc = glGetUniformLocation(first.m_Shader, "u_Bones");
#endif
	if (baseLoc < 0)
	{
		return false;
	}
	glUniform4fv(baseLoc, totalRows, s_BoneRows.data());
#endif


#if CBMu_ENABLE_GL_BMD_INSTANCE_VBO_RING
	static GLuint s_InstanceVBO[3] = {};
	static std::size_t s_InstanceVBOBytes[3] = {};
	static unsigned int s_InstanceVBOCursor = 0;
	const unsigned int cbmuInstanceVBOIndex = s_InstanceVBOCursor++ % 3u;
	if (s_InstanceVBO[cbmuInstanceVBOIndex] == 0)
	{
		glGenBuffers(1, &s_InstanceVBO[cbmuInstanceVBOIndex]);
	}
	GLuint cbmuInstanceVBO = s_InstanceVBO[cbmuInstanceVBOIndex];
	std::size_t& cbmuInstanceVBOBytes = s_InstanceVBOBytes[cbmuInstanceVBOIndex];
#else
	static GLuint cbmuInstanceVBO = 0;
	static std::size_t cbmuInstanceVBOBytes = 0;
	if (cbmuInstanceVBO == 0)
	{
		glGenBuffers(1, &cbmuInstanceVBO);
	}
#endif


	CBMu_BindBMDVAO(rNewMesh.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, cbmuInstanceVBO);
	const std::size_t instanceBytes = s_InstanceData.size() * sizeof(CBMu_BMDInstanceData);
	if (cbmuInstanceVBOBytes < instanceBytes)
	{

		glBufferData(GL_ARRAY_BUFFER, instanceBytes, s_InstanceData.data(), GL_STREAM_DRAW);
		cbmuInstanceVBOBytes = instanceBytes;
	}
	else
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, instanceBytes, s_InstanceData.data());
	}
	glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(CBMu_BMDInstanceData), reinterpret_cast<const void*>(offsetof(CBMu_BMDInstanceData, BoneBase)));
	glVertexAttribDivisor(4, 1);
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(CBMu_BMDInstanceData), reinterpret_cast<const void*>(offsetof(CBMu_BMDInstanceData, BodyLight)));
	glVertexAttribDivisor(5, 1);
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(CBMu_BMDInstanceData), reinterpret_cast<const void*>(offsetof(CBMu_BMDInstanceData, LightPosition)));
	glVertexAttribDivisor(6, 1);

	glDrawElementsInstanced(GL_TRIANGLES, rNewMesh.IndexCount, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(count));
	const bool drawOk = true;

	glVertexAttribDivisor(4, 0);
	glDisableVertexAttribArray(4);
	glVertexAttribI1ui(4, 0);
	glVertexAttribDivisor(5, 0);
	glDisableVertexAttribArray(5);
	glVertexAttribDivisor(6, 0);
	glDisableVertexAttribArray(6);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	CBMu_UnbindBMDVAOAfterDraw();
#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER
#if !CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
	CBMu_DisableBMDBoneTextureBuffer(first.m_Shader);
#endif
#endif
	return drawOk;
#else
	return false;
#endif
}

void CGMShaderBMD::RenderOLD(OGL330MODEL::RenderMeshVAO& r)
{
	BMD& rModel = *r.m_OldBMD;

	VAOMesh& rNewMesh = rModel.New_Meshs[r.m_IndexMesh];
	if (!rNewMesh.VAO)
		return;


































	OGL330MODEL::UseShader(r.m_Shader);

	if ((r.m_FlagRender & RENDER_COLOR) == RENDER_COLOR)
	{
		if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT)
		{
			EnableAlphaBlend();
		}
		else if ((r.m_FlagRender & RENDER_DARK) == RENDER_DARK)
			EnableAlphaBlendMinus();
		else
			DisableAlphaBlend();

		if ((r.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH)
		{
			DisableDepthTest();
		}

		DisableTexture();

		if (r.m_isAlpha < 0.99f)
		{
			EnableAlphaTest();
		}

	}
	else if ((r.m_FlagRender & RENDER_CHROME) == RENDER_CHROME ||
		(r.m_FlagRender & RENDER_CHROME2) == RENDER_CHROME2 ||
		(r.m_FlagRender & RENDER_CHROME3) == RENDER_CHROME3 ||
		(r.m_FlagRender & RENDER_CHROME4) == RENDER_CHROME4 ||
		(r.m_FlagRender & RENDER_CHROME5) == RENDER_CHROME5 ||
		(r.m_FlagRender & RENDER_CHROME7) == RENDER_CHROME7 ||
		(r.m_FlagRender & RENDER_CHROME8) == RENDER_CHROME8 ||
		(r.m_FlagRender & RENDER_METAL) == RENDER_METAL ||
		(r.m_FlagRender & RENDER_OIL) == RENDER_OIL)
	{
		glEnable(GL_TEXTURE_2D);
		BindTexture(r.m_TextureID);

		if ((r.m_FlagRender & RENDER_CHROME3) == RENDER_CHROME3
			|| (r.m_FlagRender & RENDER_CHROME4) == RENDER_CHROME4
			|| (r.m_FlagRender & RENDER_CHROME5) == RENDER_CHROME5
			|| (r.m_FlagRender & RENDER_CHROME7) == RENDER_CHROME7)
		{
			EnableAlphaBlend();
		}
		else if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT)
			EnableAlphaBlend();
		else if ((r.m_FlagRender & RENDER_DARK) == RENDER_DARK)
			EnableAlphaBlendMinus();
		else if ((r.m_FlagRender & RENDER_LIGHTMAP) == RENDER_LIGHTMAP)
			EnableLightMap();
		else if (r.m_isAlpha >= 0.99f)
			DisableAlphaBlend();
		else
			EnableAlphaTest();

		if ((r.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH)
		{
			DisableDepthTest();
		}
	}
	else if ((r.m_FlagRender & RENDER_TEXTURE) == RENDER_TEXTURE)
	{
		glEnable(GL_TEXTURE_2D);
		BindTexture(r.m_TextureID);

		if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT)
		{
			EnableAlphaBlend();
		}
		else if ((r.m_FlagRender & RENDER_DARK) == RENDER_DARK)
		{
			EnableAlphaBlendMinus();
		}
		else if ((r.m_FlagRender & RENDER_LIGHTMAP) == RENDER_LIGHTMAP)
		{
			EnableLightMap();
		}
		else if (r.m_isAlpha < 0.99f || Bitmaps[r.m_TextureID].Components == 4)
		{
			EnableAlphaTest();
		}
		else
		{
			DisableAlphaBlend();
		}

		if ((r.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH)
		{
			DisableDepthTest();
		}
	}
	else if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT)
	{
		EnableAlphaBlend();
		if (r.m_isAlpha < 0.99f || Bitmaps[r.m_TextureID].Components == 4)
		{
			EnableAlphaTest();
		}
		else if (SceneFlag == MAIN_SCENE)
		{
			glEnable(GL_TEXTURE_2D);
			BindTexture(r.m_TextureID);
			glDisable(GL_ALPHA_TEST);
		}
		if ((r.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH)
		{
			DisableDepthTest();
		}
		DisableDepthMask();
	}


























































	if (r.m_Shadow)
	{



		DisableTexture();
		DisableDepthMask();

	}

	bool isScale = false;
	float Scale = 0.0f;
	Scale = r.m_OldBMD->m_fRequestScale;

	if (Scale != 1.0f && Scale != 0.0f)
	{
		isScale = true;
	}
	else
	{
		isScale = false;
	}

	SendUniform(r.m_Shader, r.m_bodyLight, r.m_lightPosition, r.m_meshUV, r.m_setting1, r.m_setting2, r.m_isLight);
	rNewMesh.SendIndexBone(r.m_Shader, GMMeshShader->GetfinalBone(), GMMeshShader->GetTransfrom(), r.m_OldBMD->BodyOrigin, r.m_OldBMD->BodyScale, isScale, Scale);

	glBindVertexArray(rNewMesh.VAO);
	glDrawRangeElements(GL_TRIANGLES, 0, rNewMesh.IndexCount - 1, rNewMesh.IndexCount, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
	if (r.m_Shadow)
	{

		EnableDepthMask();
		glDisable(GL_STENCIL_TEST);

	}
	OGL330MODEL::UnUseShader();
}
void CGMShaderBMD::Render(OGL330MODEL::RenderMeshVAO& r)
{
	BMD& rModel = *r.m_OldBMD;
	VAOMesh& rNewMesh = rModel.New_Meshs[r.m_IndexMesh];
	if (!rNewMesh.VAO && rNewMesh.vkVertexBuffer == VK_NULL_HANDLE) return;

	RenderUniformBlock blk;
	FillRenderBlock(blk, r);

#if CBMu_USE_VULKAN_NATIVE_MESH
	if (!CBMu_RequiresLegacyMaterialPass(r) && GPUContext::Instance().IsFrameActive() && rNewMesh.vkVertexBuffer != VK_NULL_HANDLE && rNewMesh.vkIndexBuffer != VK_NULL_HANDLE)
	{
		BITMAP_t* pBitmap = (r.m_TextureID >= 0) ? Bitmaps.GetTexture(static_cast<GLuint>(r.m_TextureID)) : nullptr;
		const uint32_t vkTextureId = pBitmap ? pBitmap->TextureNumber : (r.m_TextureID < 0 ? static_cast<uint32_t>(-r.m_TextureID) : 0);

		int blendType = 0;
		if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT || (r.m_FlagRender & (RENDER_CHROME3 | RENDER_CHROME4 | RENDER_CHROME5 | RENDER_CHROME7)) != 0) blendType = 2;
		else if ((r.m_FlagRender & RENDER_DARK) == RENDER_DARK) blendType = 3;
		else if (r.m_isAlpha < 0.99f || (pBitmap && pBitmap->Components == 4)) blendType = 1;
		bool depthWrite = ((r.m_FlagRender & RENDER_NODEPTH) == 0) && (blendType == 0);
		bool depthTest = (r.m_FlagRender & RENDER_NODEPTH) == 0;

		uint32_t boneBaseRows = 0;
		uint32_t boneSSBOOffset = 0;
		uint32_t boneBytes = 0;
		float* dstBones = nullptr;

#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
		if (r.m_HasPackedBones)
		{
			const std::vector<float>& packedBones = r.PackedBones();
			boneBytes = static_cast<uint32_t>(packedBones.size() * sizeof(float));
			if (boneBytes > 0)
			{
				dstBones = static_cast<float*>(GPUContext::Instance().AllocateBoneBuffer(boneBytes, boneBaseRows, boneSSBOOffset));
				if (dstBones)
				{
					memcpy(dstBones, packedBones.data(), boneBytes);
				}
			}
			else
			{
				boneBytes = 12 * sizeof(float);
				dstBones = static_cast<float*>(GPUContext::Instance().AllocateBoneBuffer(boneBytes, boneBaseRows, boneSSBOOffset));
				if (dstBones)
				{
					const float identityBones[12] = {
						1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f
					};
					memcpy(dstBones, identityBones, sizeof(identityBones));
				}
			}
		}
		else
#endif
		{
			bool isScale = false;
			float Scale = r.m_OldBMD->m_fRequestScale;
			if (Scale != 1.0f && Scale != 0.0f) isScale = true;

			size_t rowsNeeded = rNewMesh.BoneContainer.size() * 3;
			if (rowsNeeded == 0) rowsNeeded = 3;
			boneBytes = static_cast<uint32_t>(rowsNeeded * 4 * sizeof(float));
			if (boneBytes > 0)
			{
				dstBones = static_cast<float*>(GPUContext::Instance().AllocateBoneBuffer(boneBytes, boneBaseRows, boneSSBOOffset));
				if (dstBones)
				{
					if (!rNewMesh.BoneContainer.empty())
					{
						rNewMesh.GetPackedBonesDirect(GMMeshShader->GetfinalBone(),
							GMMeshShader->GetTransfrom(),
							r.m_OldBMD->BodyOrigin, r.m_OldBMD->BodyScale,
							isScale, Scale, dstBones);
					}
					else
					{
						const float identityBones[12] = {
							1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f
						};
						memcpy(dstBones, identityBones, sizeof(identityBones));
					}
				}
			}
		}

		if (!dstBones || boneBytes == 0)
		{
			return;
		}

		uint32_t baseInstanceIndex = 0;
		uint32_t instanceSSBOStart = 0;
		GPUContext::GPUInstanceData* dstInst = GPUContext::Instance().AllocateInstanceBuffer(1, baseInstanceIndex, instanceSSBOStart);
		if (!dstInst)
		{
			return;
		}

		GPUContext::GPUInstanceData& inst = *dstInst;
		inst = {};
		inst.modelMatrix = glm::vec4(0.0f);
		inst.lightColor = glm::vec4(r.m_bodyLight.x, r.m_bodyLight.y, r.m_bodyLight.z, r.m_isAlpha);
		inst.renderMode = 0;
		inst.finalRenderMode = 0;
		inst.sourceIndex = static_cast<int32_t>(vkTextureId);
		inst.chromeMode = 0;
		inst.blendUV = glm::vec2(r.m_meshUV.x, r.m_meshUV.y);
		inst.newScale = 0.0f;
		inst.alpha = r.m_isAlpha;
		inst.bodyScale = 1.0f;
		inst.boneScale = 1.0f;
		inst.boneOffset = static_cast<int32_t>(boneBaseRows);
		inst.customIntensity = 1.0f;
		inst.bodyLightColor = glm::vec3(r.m_bodyLight.x, r.m_bodyLight.y, r.m_bodyLight.z);

		uint32_t flags = 0x02u;
		if (r.m_isLight) flags |= 0x04u;
		if (r.m_meshUV.z > 0.5f) flags |= 0x01u;
		if ((r.m_FlagRender & RENDER_COLOR) != 0) flags |= 0x08u;
		if ((r.m_FlagRender & RENDER_BRIGHT) != 0) flags |= 0x20u;

		int32_t chromeMode = 0;
		int32_t finalRenderMode = 0;

		if ((r.m_FlagRender & RENDER_METAL) != 0)
		{
			flags |= 0x10u;
			chromeMode = 1;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}
		else if ((r.m_FlagRender & RENDER_CHROME2) != 0)
		{
			flags |= 0x10u;
			chromeMode = 2;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}
		else if ((r.m_FlagRender & RENDER_CHROME3) != 0)
		{
			flags |= 0x10u;
			chromeMode = 3;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}
		else if ((r.m_FlagRender & RENDER_CHROME4) != 0 || (r.m_FlagRender & RENDER_OIL) != 0)
		{
			flags |= 0x10u;
			chromeMode = 4;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}
		else if ((r.m_FlagRender & RENDER_CHROME5) != 0)
		{
			flags |= 0x10u;
			chromeMode = 5;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}
		else if ((r.m_FlagRender & RENDER_CHROME6) != 0)
		{
			flags |= 0x10u;
			chromeMode = 6;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}
		else if ((r.m_FlagRender & RENDER_CHROME7) != 0)
		{
			flags |= 0x10u;
			chromeMode = 7;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}
		else if ((r.m_FlagRender & RENDER_CHROME8) != 0)
		{
			flags |= 0x10u;
			chromeMode = 8;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}
		else if ((r.m_FlagRender & RENDER_CHROME) != 0)
		{
			flags |= 0x10u;
			chromeMode = 0;
			finalRenderMode = (r.m_meshUV.z > 0.5f) ? 3 : 2;
		}

		inst.flags = flags;
		inst.chromeMode = chromeMode;
		inst.finalRenderMode = finalRenderMode;
		inst.renderMode = 0;

		GPUContext::Instance().DrawInstancedMeshPreallocated(
			rNewMesh.vkVertexBuffer, rNewMesh.vkIndexBuffer, rNewMesh.IndexCount,
			1, vkTextureId, blendType, depthTest, depthWrite,
			&blk, sizeof(RenderUniformBlock),
			instanceSSBOStart, baseInstanceIndex,
			boneSSBOOffset, boneBytes);
		return;
	}
#endif

	ShaderGuard guard(r.m_Shader);
	static UniformBlockCache ubCache;
	ubCache.Update(blk);
	ubCache.Bind(r.m_Shader, "RenderBlock", 0);
#if CBMu_ENABLE_GL_BMD_WEARABLE_COMPOSITE
	CBMu_ApplyBMDWearableComposite(r.m_Shader, r);
#endif

#if CBMu_ENABLE_GL_BMD_SKINNED_INSTANCED_BATCH && CBMu_ENABLE_GL_BMD_BONE_UNIFORM_CACHE

	const GLint useInstanceParamsLoc = CBMu_GetBMDUniformLocationCached(r.m_Shader, "uUseInstanceParams");
	if (useInstanceParamsLoc >= 0)
	{
#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER && CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
		CBMu_SetBMDUseInstanceParams(r.m_Shader, 0);
#else
		glUniform1i(useInstanceParamsLoc, 0);
#endif
	}
#endif


	if ((r.m_FlagRender & RENDER_COLOR) == RENDER_COLOR)
	{
		if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT) {
			EnableAlphaBlend();
		}
		else if ((r.m_FlagRender & RENDER_DARK) == RENDER_DARK) {
			EnableAlphaBlendMinus();
		}
		else {
			DisableAlphaBlend();
		}

		if ((r.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH) {
			DisableDepthTest();
		}
		else {
			EnableDepthTest();
		}

		DisableTexture();

		if (r.m_isAlpha < 0.99f) {
			EnableAlphaTest();
		}
		else {
			glDisable(GL_ALPHA_TEST);
		}
	}
	else if ((r.m_FlagRender & RENDER_CHROME) == RENDER_CHROME ||
		(r.m_FlagRender & RENDER_CHROME2) == RENDER_CHROME2 ||
		(r.m_FlagRender & RENDER_CHROME3) == RENDER_CHROME3 ||
		(r.m_FlagRender & RENDER_CHROME4) == RENDER_CHROME4 ||
		(r.m_FlagRender & RENDER_CHROME5) == RENDER_CHROME5 ||
		(r.m_FlagRender & RENDER_CHROME7) == RENDER_CHROME7 ||
		(r.m_FlagRender & RENDER_CHROME8) == RENDER_CHROME8 ||
		(r.m_FlagRender & RENDER_METAL) == RENDER_METAL ||
		(r.m_FlagRender & RENDER_OIL) == RENDER_OIL)
	{
		glEnable(GL_TEXTURE_2D);
		BindTexture(r.m_TextureID);

		if ((r.m_FlagRender & (RENDER_CHROME3 | RENDER_CHROME4 | RENDER_CHROME5 | RENDER_CHROME7)) != 0) {
			EnableAlphaBlend();
		}
		else if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT) {
			EnableAlphaBlend();
		}
		else if ((r.m_FlagRender & RENDER_DARK) == RENDER_DARK) {
			EnableAlphaBlendMinus();
		}
		else if ((r.m_FlagRender & RENDER_LIGHTMAP) == RENDER_LIGHTMAP) {
			EnableLightMap();
		}
		else if (r.m_isAlpha >= 0.99f) {
			DisableAlphaBlend();
		}
		else {
			EnableAlphaTest();
		}

		if ((r.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH) {
			DisableDepthTest();
		}
		else {
			EnableDepthTest();
		}
	}
	else if ((r.m_FlagRender & RENDER_TEXTURE) == RENDER_TEXTURE)
	{
		glEnable(GL_TEXTURE_2D);
		BindTexture(r.m_TextureID);

		if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT) {
			EnableAlphaBlend();
		}
		else if ((r.m_FlagRender & RENDER_DARK) == RENDER_DARK) {
			EnableAlphaBlendMinus();
		}
		else if ((r.m_FlagRender & RENDER_LIGHTMAP) == RENDER_LIGHTMAP) {
			EnableLightMap();
		}
		else if (r.m_isAlpha < 0.99f || Bitmaps[r.m_TextureID].Components == 4) {
			EnableAlphaTest();
		}
		else {
			DisableAlphaBlend();
			glDisable(GL_ALPHA_TEST);
		}

		if ((r.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH) {
			DisableDepthTest();
		}
		else {
			EnableDepthTest();
		}
	}
	else if ((r.m_FlagRender & RENDER_BRIGHT) == RENDER_BRIGHT)
	{
		EnableAlphaBlend();
		if (r.m_isAlpha < 0.99f || Bitmaps[r.m_TextureID].Components == 4) {
			EnableAlphaTest();
		}
		else if (SceneFlag == MAIN_SCENE) {
			glEnable(GL_TEXTURE_2D);
			BindTexture(r.m_TextureID);
			glDisable(GL_ALPHA_TEST);
		}

		if ((r.m_FlagRender & RENDER_NODEPTH) == RENDER_NODEPTH) {
			DisableDepthTest();
		}
		else {
			EnableDepthTest();
		}
		DisableDepthMask();
	}


	if (r.m_Shadow)
	{
		DisableTexture();
		DisableDepthMask();
	}


	bool isScale = false;
	float Scale = r.m_OldBMD->m_fRequestScale;
	if (Scale != 1.0f && Scale != 0.0f) isScale = true;

	bool cbmuUsedBoneTextureBuffer = false;


#if CBMu_ENABLE_GL_BMD_DEFER_CHARACTER_FLUSH
	if (r.m_HasPackedBones)
	{
		const std::vector<float>& packedBones = r.PackedBones();
#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER && CBMu_ENABLE_GL_BMD_TBO_NON_INSTANCED
		if (SceneFlag != CHARACTER_SCENE)
		{

			glVertexAttribI1ui(4, 0);
			cbmuUsedBoneTextureBuffer = CBMu_UploadBMDBoneTextureBuffer(r.m_Shader, packedBones, r.m_PackedBoneRows);
		}
#endif
		if (!cbmuUsedBoneTextureBuffer)
		{
#if CBMu_ENABLE_GL_BMD_BONE_UNIFORM_CACHE
			const GLint baseLoc = CBMu_GetBMDBoneUniformLocation(r.m_Shader);
#else
			const GLint baseLoc = glGetUniformLocation(r.m_Shader, "u_Bones");
#endif
			if (baseLoc >= 0 && r.m_PackedBoneRows > 0)
			{
				glUniform4fv(baseLoc, r.m_PackedBoneRows, packedBones.data());
			}
		}
	}
	else
#endif
	{
#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER
#if CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
		CBMu_SetBMDUseBoneTexture(r.m_Shader, 0);
#else
		const GLint useBoneTextureLoc = CBMu_GetBMDUniformLocationCached(r.m_Shader, "uUseBoneTexture");
		if (useBoneTextureLoc >= 0)
		{
			glUniform1i(useBoneTextureLoc, 0);
		}
#endif
#endif
		rNewMesh.SendIndexBone(r.m_Shader, GMMeshShader->GetfinalBone(),
			GMMeshShader->GetTransfrom(),
			r.m_OldBMD->BodyOrigin, r.m_OldBMD->BodyScale,
			isScale, Scale);
	}


	if (SceneFlag == CHARACTER_SCENE)
	{

		DisableCullFace();
	}
	CBMu_BindBMDVAO(rNewMesh.VAO);
	CBMu_LogObjectRenderEvent("CGMShaderBMD::Render", "draw_shader_range_elements", r.m_IndexMesh, r.m_OldBMD ? r.m_OldBMD->NumMeshs : -1, r.m_FlagRender, r.m_TextureID, rNewMesh.IndexCount);
	glDrawRangeElements(GL_TRIANGLES, 0, rNewMesh.IndexCount - 1,
		rNewMesh.IndexCount, GL_UNSIGNED_INT, nullptr);
	CBMu_UnbindBMDVAOAfterDraw();


#if CBMu_ENABLE_GL_BMD_BONE_TEXTURE_BUFFER && CBMu_ENABLE_GL_BMD_TBO_NON_INSTANCED
	if (cbmuUsedBoneTextureBuffer)
	{
#if !CBMu_ENABLE_GL_BMD_TBO_STATE_CACHE
		CBMu_DisableBMDBoneTextureBuffer(r.m_Shader);
#endif
	}
#endif

	if (r.m_Shadow) {
		EnableDepthMask();
		glDisable(GL_STENCIL_TEST);
	}

}

#endif
