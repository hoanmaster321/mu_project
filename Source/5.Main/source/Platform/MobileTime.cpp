#if defined(__ANDROID__) || defined(MU_IOS)

#include "MobileTime.h"

#include <chrono>
#include <cmath>
#include <thread>

namespace
{
using Clock = std::chrono::steady_clock;
const auto g_startTimePoint = Clock::now();
} // namespace

void MU_MobileTimeInit()
{
}

uint32_t MU_MobileGetTicks()
{
    const auto now = Clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_startTimePoint).count();
    return static_cast<uint32_t>(ms & 0xFFFFFFFFULL);
}

uint64_t MU_MobilePerfNow()
{
    const auto now = Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now - g_startTimePoint).count());
}

uint64_t MU_MobilePerfFrequency()
{
    return 1000000000ULL;
}

double MU_MobilePerfToSeconds(uint64_t ticks)
{
    return static_cast<double>(ticks) / 1000000000.0;
}

double MU_MobilePerfToMilliseconds(uint64_t ticks)
{
    return static_cast<double>(ticks) / 1000000.0;
}

void MU_MobileSleep(uint32_t ms)
{
    if (ms == 0)
    {
        std::this_thread::yield();
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#endif // defined(__ANDROID__) || defined(MU_IOS)
