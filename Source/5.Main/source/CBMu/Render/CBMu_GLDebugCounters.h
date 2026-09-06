// CBMu: Tệp này bổ sung bộ đếm debug cho renderer OpenGL legacy.
// Phạm vi: Lưu số liệu theo pass/frame để đo trước khi tối ưu.
// Lưu ý: Module này không thay đổi logic gameplay hoặc format resource.

#pragma once

#include "../CBMu_RenderConfig.h"

struct CBMu_GLDebugCounters
{
	int DrawCalls;
	int TextureBinds;
	int ShaderChanges;
	int BlendChanges;
	int DepthStateChanges;
	int CullStateChanges;
	int SpritesSubmitted;
	int ParticlesAlive;
	int ParticlesSubmitted;
	int JointsSubmitted;
	int SpriteDrawCalls;
	int ParticleDrawCalls;
	int JointDrawCalls;
	int BatchesFlushed;
	int ParticleRenderDepth;

	unsigned long long WindowFrames;
	unsigned long long SumDrawCalls;
	unsigned long long SumTextureBinds;
	unsigned long long SumSpritesSubmitted;
	unsigned long long SumParticlesAlive;
	unsigned long long SumParticlesSubmitted;
	unsigned long long SumJointsSubmitted;
	unsigned long long SumSpriteDrawCalls;
	unsigned long long SumParticleDrawCalls;
	unsigned long long SumJointDrawCalls;
	unsigned long long SumBatchesFlushed;
	int MaxSpritesSubmitted;
	int MaxParticlesSubmitted;
	int MaxJointsSubmitted;
	int MaxSpriteDrawCalls;
	int MaxParticleDrawCalls;
	int MaxJointDrawCalls;

	void ResetFrame();
	void ResetWindow();
	void AccumulateFrame();
	void BeginParticleRenderPass();
	void EndParticleRenderPass();
	void AddDrawCall();
	void AddTextureBind();
	void AddShaderChange();
	void AddBlendChange();
	void AddDepthStateChange();
	void AddCullStateChange();
	void AddSpriteSubmitted();
	void AddParticleAlive();
	void AddParticleSubmitted();
	void AddJointSubmitted();
	void AddSpriteDrawCall();
	void AddParticleDrawCall();
	void AddJointDrawCall();
	void AddBatchFlushed();
	void AddRenderSpriteDrawCall();
	void LogEveryNFrames(int frameInterval) const;
	void WriteWindowToFile(FILE* file) const;
};

#if CBMu_ENABLE_GL_DEBUG_COUNTERS
extern CBMu_GLDebugCounters g_CBMuGLDebugCounters;
#else
struct CBMu_GLDebugCountersStub
{
	void ResetFrame() {}
	void ResetWindow() {}
	void AccumulateFrame() {}
	void BeginParticleRenderPass() {}
	void EndParticleRenderPass() {}
	void AddDrawCall() {}
	void AddTextureBind() {}
	void AddShaderChange() {}
	void AddBlendChange() {}
	void AddDepthStateChange() {}
	void AddCullStateChange() {}
	void AddSpriteSubmitted() {}
	void AddParticleAlive() {}
	void AddParticleSubmitted() {}
	void AddJointSubmitted() {}
	void AddSpriteDrawCall() {}
	void AddParticleDrawCall() {}
	void AddJointDrawCall() {}
	void AddBatchFlushed() {}
	void AddRenderSpriteDrawCall() {}
	void LogEveryNFrames(int) const {}
	void WriteWindowToFile(FILE*) const {}
};
inline CBMu_GLDebugCountersStub g_CBMuGLDebugCounters;
#endif
