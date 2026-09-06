#include "StdAfx.h"
#include "New_RenderBMD.h"
#include "CBMu/Render/CBMu_CPUHotspotProfiler.h"

#if CB_SHADER330_TEST
#include "ZzzBMD.h"
#include "ZzzTexture.h"
#include "TextureScript.h"
#include "GPUContext.h"
#include "Utilities/Log/muConsoleDebug.h"
#include "CBMu/Render/CBMu_GLObjectDebugLog.h"
#include "ZzzScene.h"
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

CGMNewRenderBMD* g_NewRenderBMD = NULL;

void CBMu_ResetBMDVAOBindCache()
{
}

void CBMu_ResetBMDBoneTextureBufferCache()
{
}

void CBMu_ResetBMDWearableCompositeTextureUnits()
{
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

bool CGMShaderBMD::RenderInstanced(const OGL330MODEL::MeshVAO& data, std::size_t start, std::size_t count)
{
#if CBMu_ENABLE_GL_BMD_SKINNED_INSTANCED_BATCH
	if (count < 2 || start >= data.size())
	{
		return false;
	}

	const OGL330MODEL::RenderMeshVAO& first = data[start];
	BMD& rModel = *first.m_OldBMD;
	VAOMesh& rNewMesh = rModel.New_Meshs[first.m_IndexMesh];
	if ((!rNewMesh.VAO && rNewMesh.vkVertexBuffer == VK_NULL_HANDLE) || !first.m_HasPackedBones || first.m_PackedBoneRows <= 0)
	{
		return false;
	}

#if CBMu_USE_VULKAN_NATIVE_MESH
	if (GPUContext::Instance().IsFrameActive() && rNewMesh.vkVertexBuffer != VK_NULL_HANDLE && rNewMesh.vkIndexBuffer != VK_NULL_HANDLE)
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
#endif
	return false;
}

void CGMShaderBMD::RenderOLD(OGL330MODEL::RenderMeshVAO& r)
{
}

void CGMShaderBMD::Render(OGL330MODEL::RenderMeshVAO& r)
{
	BMD& rModel = *r.m_OldBMD;
	VAOMesh& rNewMesh = rModel.New_Meshs[r.m_IndexMesh];
	if (!rNewMesh.VAO && rNewMesh.vkVertexBuffer == VK_NULL_HANDLE) return;

	RenderUniformBlock blk;
	FillRenderBlock(blk, r);

#if CBMu_USE_VULKAN_NATIVE_MESH
	if (GPUContext::Instance().IsFrameActive() && rNewMesh.vkVertexBuffer != VK_NULL_HANDLE && rNewMesh.vkIndexBuffer != VK_NULL_HANDLE)
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
		}
		else
#endif
		{
			uint32_t rowsNeeded = (r.m_PackedBoneRows > 0) ? static_cast<uint32_t>(r.m_PackedBoneRows) : static_cast<uint32_t>(rNewMesh.BoneContainer.size() * 3);
			if (rowsNeeded == 0) rowsNeeded = 3;
			boneBytes = static_cast<uint32_t>(rowsNeeded * 4 * sizeof(float));
			if (boneBytes > 0)
			{
				dstBones = static_cast<float*>(GPUContext::Instance().AllocateBoneBuffer(boneBytes, boneBaseRows, boneSSBOOffset));
				if (dstBones)
				{
					if (!rNewMesh.BoneContainer.empty())
					{
						bool isScale = false;
						float Scale = r.m_OldBMD->m_fRequestScale;
						if (Scale != 1.0f && Scale != 0.0f) isScale = true;
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
	}
#endif
}

#endif
