// CBMu: Tệp này đo CPU hotspot trong renderer OpenGL legacy.
// Phạm vi: Gom thời gian các phase render lớn, không đo từng draw call.
// Lưu ý: Chỉ bật khi cần tìm nút thắt FPS, tắt bằng define để đo FPS sạch.

#pragma once

#include "CBMu/CBMu_RenderConfig.h"

enum class CBMu_CPUHotspotPhase
{
	BMDFlush = 0,
	MoveCharacters,
	MoveEffects,
	MoveJoints,
	MoveParticles,
	CheckSprites,
	RenderTerrain,
	RenderObjects,
	RenderCharacters,
	RenderCharacterAnimation,
	RenderCharacterMainObject,
	RenderCharacterVisual,
	RenderCharacterVisualPreBody,
	RenderCharacterVisualBodyPart,
	RenderCharacterVisualBodyFullSet,
	RenderCharacterVisualBodyMesh,
	RenderCharacterVisualCloth,
	RenderCharacterVisualBackItem,
	RenderCharacterVisualWeapon,
	RenderCharacterVisualTypeSwitch,
	RenderCharacterNextGrade,
	RenderCharacterFullsetEquip,
	RenderCharacterAttachment,
	RenderCharacterLinkObject,
	RenderCharacterLight,
	RenderEffects,
	RenderJoints,
	RenderSprites,
	RenderParticles,
	Count
};

enum class CBMu_CPUHotspotPool
{
	Characters = 0,
	Effects,
	Joints,
	Particles,
	Sprites,
	Count
};

enum class CBMu_BMDBatchSplitReason
{
	BaseNotCandidate = 0,
	NextNotCandidate,
	BoneRowLimit,
	Model,
	Mesh,
	Texture,
	Flag,
	Shader,
	LightFlag,
	BoneRows,
	Setting1,
	Setting2,
	MeshUV,
	Color,
	BodyLight,
	LightPosition,
	Composite,
	Matrix,
	RenderFallback,
	BaseOrderSensitive,
	BaseNullModel,
	BaseShadow,
	BaseNoPackedBones,
	BaseBoneRows,
	BaseAlpha,
	BaseNoTexture,
	BaseAdditiveFlag,
	BaseUnsafeFlag,
	BaseNoTextureFlag,
	BaseRenderColor,
	BaseRenderDark,
	BaseRenderChrome,
	BaseRenderMetal,
	BaseRenderLightmap,
	BaseRenderOil,
	BaseNoBonePointer,
	BaseEmptyBoneContainer,
	BaseBoneRowOverflow,
	BaseInvalidBoneIndex,
	BaseDeferredFlushInactive,
	BasePackedBoneUnknown,
	Count
};

#if CBMu_ENABLE_CPU_HOTSPOT_PROFILER

void CBMu_CPUHotspotBeginFrame();
void CBMu_CPUHotspotEndFrame();
void CBMu_CPUHotspotRecord(CBMu_CPUHotspotPhase phase, unsigned long long elapsedTicks);
void CBMu_CPUHotspotRecordBMDFlushQueue(unsigned int commandCount);
void CBMu_CPUHotspotRecordBMDSkippedEmptyFlush();
void CBMu_CPUHotspotRecordBMDDraw(bool instanced, unsigned int instanceCount);
void CBMu_CPUHotspotRecordBMDTBOUpload(unsigned long long bytes);
void CBMu_CPUHotspotRecordBMDBatchSplit(CBMu_BMDBatchSplitReason reason);
void CBMu_CPUHotspotRecordBMDBatchSplitDetail(CBMu_BMDBatchSplitReason reason, unsigned long long modelAddress, int meshIndex, int textureId, int renderFlag, unsigned int shader, const char* modelFile);
void CBMu_CPUHotspotRecordPool(CBMu_CPUHotspotPool pool, unsigned int scanEnd, unsigned int liveCount);

#if CBMu_ENABLE_BODYMESH_DETAIL_COUNTERS
void CBMu_CPUHotspotSetBodyMeshScope(bool active);
bool CBMu_CPUHotspotIsBodyMeshScope();
void CBMu_CPUHotspotRecordBodyMeshPartObject(unsigned long long elapsedTicks);
void CBMu_CPUHotspotRecordBodyMeshEdgePass();
void CBMu_CPUHotspotRecordBodyMeshTransform(bool skippedCpuSkin);
void CBMu_CPUHotspotRecordBodyMeshBoneSerial(bool cacheHit);
#endif

void CBMu_RenderProfilerOverlay();

class CBMu_CPUHotspotScope
{
public:
	explicit CBMu_CPUHotspotScope(CBMu_CPUHotspotPhase phase);
	~CBMu_CPUHotspotScope();

private:
	CBMu_CPUHotspotPhase m_Phase;
	long long m_Start;
};

#define CBMu_CPU_HOTSPOT_SCOPE(name, phase) CBMu_CPUHotspotScope name(phase)

#else

inline void CBMu_CPUHotspotBeginFrame() {}
inline void CBMu_CPUHotspotEndFrame() {}
inline void CBMu_CPUHotspotRecordBMDFlushQueue(unsigned int) {}
inline void CBMu_CPUHotspotRecordBMDSkippedEmptyFlush() {}
inline void CBMu_CPUHotspotRecordBMDDraw(bool, unsigned int) {}
inline void CBMu_CPUHotspotRecordBMDTBOUpload(unsigned long long) {}
inline void CBMu_CPUHotspotRecordBMDBatchSplit(CBMu_BMDBatchSplitReason) {}
inline void CBMu_CPUHotspotRecordBMDBatchSplitDetail(CBMu_BMDBatchSplitReason, unsigned long long, int, int, int, unsigned int, const char*) {}
inline void CBMu_CPUHotspotRecordPool(CBMu_CPUHotspotPool, unsigned int, unsigned int) {}
inline void CBMu_RenderProfilerOverlay() {}
#define CBMu_CPU_HOTSPOT_SCOPE(name, phase) (void)0

#endif

// Stub BodyMesh detail khi profiler tắt — hook gọi vẫn compile sạch.
#if !CBMu_ENABLE_CPU_HOTSPOT_PROFILER || !CBMu_ENABLE_BODYMESH_DETAIL_COUNTERS
#ifndef CBMu_BODYMESH_DETAIL_STUBS_DEFINED
#define CBMu_BODYMESH_DETAIL_STUBS_DEFINED
inline void CBMu_CPUHotspotSetBodyMeshScope(bool) {}
inline bool CBMu_CPUHotspotIsBodyMeshScope() { return false; }
inline void CBMu_CPUHotspotRecordBodyMeshPartObject(unsigned long long) {}
inline void CBMu_CPUHotspotRecordBodyMeshEdgePass() {}
inline void CBMu_CPUHotspotRecordBodyMeshTransform(bool) {}
inline void CBMu_CPUHotspotRecordBodyMeshBoneSerial(bool) {}
#endif
#endif
