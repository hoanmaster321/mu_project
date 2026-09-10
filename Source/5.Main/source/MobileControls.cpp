#include "MobileControls.h"
#include "NewUIMainFrameMobile.h"

#if defined(__ANDROID__) || defined(MU_IOS)
namespace MobileControls
{
    void Init() {}
    void Destroy() {}
    bool OnFingerDown(const SDL_TouchFingerEvent& ev) { return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerDown(ev) : false; }
    bool OnFingerMotion(const SDL_TouchFingerEvent& ev) { return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerMotion(ev) : false; }
    bool OnFingerUp(const SDL_TouchFingerEvent& ev) { return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerUp(ev) : false; }
    void Update() { if (g_pMainFrameMobile) g_pMainFrameMobile->Update(); }
    void Render() { if (g_pMainFrameMobile) g_pMainFrameMobile->Render(); }
    bool IsActive() { return false; }
    bool IsJoystickActive() { return false; }
}
#endif
