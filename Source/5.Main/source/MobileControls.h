#pragma once

#if defined(__ANDROID__) || defined(MU_IOS)
#include <SDL3/SDL.h>

namespace MobileControls
{
    void Init();
    void Destroy();

    // Touch event handlers from SDL event loop (return true if consumed)
    bool OnFingerDown(const SDL_TouchFingerEvent& ev);
    bool OnFingerMotion(const SDL_TouchFingerEvent& ev);
    bool OnFingerUp(const SDL_TouchFingerEvent& ev);

    // Per-frame logic (steers Hero based on joystick state)
    void Update();

    // Render virtual joystick & MOBA combat cluster
    void Render();

    // Check if joystick or mobile combat button is currently active
    bool IsActive();
    bool IsJoystickActive();
}
#else
namespace MobileControls
{
    inline void Init() {}
    inline void Destroy() {}
    inline void Update() {}
    inline void Render() {}
    inline bool IsActive() { return false; }
    inline bool IsJoystickActive() { return false; }
}
#endif
