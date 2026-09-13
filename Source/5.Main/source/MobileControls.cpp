#include "MobileControls.h"
#include "NewUIMainFrameMobile.h"
#include "UIMobile/UIMobileInventory.h"
#include "UIMobile/UIMobileSystem.h"

#if defined(__ANDROID__) || defined(MU_IOS)
namespace MobileControls
{
    void Init() {}
    void Destroy() {}
    bool OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (g_pMobileSystem && g_pMobileSystem->IsAnyUIVisible() && g_pMobileSystem->OnFingerDown(ev))
            return true;
        return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerDown(ev) : false;
    }
    bool OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (g_pMobileSystem && g_pMobileSystem->IsAnyUIVisible() && g_pMobileSystem->OnFingerMotion(ev))
            return true;
        return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerMotion(ev) : false;
    }
    bool OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (g_pMobileSystem && g_pMobileSystem->IsAnyUIVisible() && g_pMobileSystem->OnFingerUp(ev))
            return true;
        return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerUp(ev) : false;
    }
    void Update()
    {
        if (g_pMainFrameMobile) g_pMainFrameMobile->Update();
        if (g_pMobileSystem) g_pMobileSystem->Update();
    }
    void Render()
    {
        if (g_pMainFrameMobile) g_pMainFrameMobile->Render();
        if (g_pMobileSystem && g_pMobileSystem->IsAnyUIVisible())
        {
            g_pMobileSystem->Render();
        }
    }
    bool IsActive() { return false; }
    bool IsJoystickActive() { return false; }
}
#endif
