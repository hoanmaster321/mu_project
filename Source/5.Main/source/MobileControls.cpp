#include "MobileControls.h"
#include "NewUIMainFrameMobile.h"
#include "UIMobile/UIMobileInventory.h"

#if defined(__ANDROID__) || defined(MU_IOS)
namespace MobileControls
{
    void Init() {}
    void Destroy() {}
    bool OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (g_pMobileInventory && g_pMobileInventory->IsOpen() && g_pMobileInventory->OnFingerDown(ev))
            return true;
        return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerDown(ev) : false;
    }
    bool OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (g_pMobileInventory && g_pMobileInventory->IsOpen() && g_pMobileInventory->OnFingerMotion(ev))
            return true;
        return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerMotion(ev) : false;
    }
    bool OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (g_pMobileInventory && g_pMobileInventory->IsOpen() && g_pMobileInventory->OnFingerUp(ev))
            return true;
        return g_pMainFrameMobile ? g_pMainFrameMobile->OnFingerUp(ev) : false;
    }
    void Update()
    {
        if (g_pMainFrameMobile) g_pMainFrameMobile->Update();
        if (g_pMobileInventory) g_pMobileInventory->Update();
    }
    void Render()
    {
        if (g_pMainFrameMobile) g_pMainFrameMobile->Render();
        if (g_pMobileInventory && g_pMobileInventory->IsOpen())
        {
            g_pMobileInventory->Render();
            g_pMobileInventory->Render3D();
        }
    }
    bool IsActive() { return false; }
    bool IsJoystickActive() { return false; }
}
#endif
