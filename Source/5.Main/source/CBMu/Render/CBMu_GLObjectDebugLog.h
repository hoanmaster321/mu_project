
#pragma once

#include "../CBMu_RenderConfig.h"
#include <cstddef>

inline void CBMu_LogObjectRenderEvent(
	const char* stage,
	const char* reason,
	int meshIndex,
	int meshCount,
	int renderFlag,
	int texture,
	int extraValue) {}

inline void CBMu_LogObjectRenderQueue(const char* stage, std::size_t count) {}
