#pragma once

#if defined(__ANDROID__) || defined(MU_IOS)

#include <cstdint>
#include <SDL3/SDL.h>

inline void     MU_MobileTimeInit() {}
inline uint32_t MU_MobileGetTicks() { return static_cast<uint32_t>(SDL_GetTicks()); }
inline uint64_t MU_MobilePerfNow()  { return SDL_GetPerformanceCounter(); }
inline uint64_t MU_MobilePerfFrequency() { return SDL_GetPerformanceFrequency(); }
inline double   MU_MobilePerfToSeconds(uint64_t ticks) {
    uint64_t freq = SDL_GetPerformanceFrequency();
    return freq > 0 ? (static_cast<double>(ticks) / static_cast<double>(freq)) : 0.0;
}
inline double   MU_MobilePerfToMilliseconds(uint64_t ticks) {
    uint64_t freq = SDL_GetPerformanceFrequency();
    return freq > 0 ? ((static_cast<double>(ticks) * 1000.0) / static_cast<double>(freq)) : 0.0;
}
inline void     MU_MobileSleep(uint32_t ms) { SDL_Delay(ms); }

#endif // defined(__ANDROID__) || defined(MU_IOS)
