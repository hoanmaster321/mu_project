#pragma once

#if defined(__ANDROID__) || defined(MU_IOS)

#include <SDL3/SDL.h>
#include <string>

inline void MU_MobilePlatformInit() {}
inline void MU_MobilePlatformShutdown() {}

inline const bool* MU_MobileGetKeyboardState() { return SDL_GetKeyboardState(nullptr); }
inline void MU_MobileSetKeyState(SDL_Scancode scancode, bool isDown) {}
inline void MU_MobileClearKeyboardState() {}

inline void MU_MobileStartTextInput() {
    SDL_Window* w = SDL_GetKeyboardFocus();
    if (w) SDL_StartTextInput(w);
}
inline void MU_MobileStopTextInput() {
    SDL_Window* w = SDL_GetKeyboardFocus();
    if (w) SDL_StopTextInput(w);
}
inline bool MU_MobileIsTextInputActive() {
    SDL_Window* w = SDL_GetKeyboardFocus();
    return w ? SDL_TextInputActive(w) : false;
}
inline void MU_MobileSetTextInputRect(const SDL_Rect* rect) {
    SDL_Window* w = SDL_GetKeyboardFocus();
    if (w && rect) SDL_SetTextInputArea(w, rect, 0);
}

inline std::string MU_MobileGetExternalDataPath() { return "/sdcard/Android/data/com.muonline.client/files"; }
inline std::string MU_MobileGetInternalDataPath() { return "/data/data/com.muonline.client/files"; }

inline const void* MU_MobileGetNativeWindow() { return nullptr; }
inline const void* MU_MobileGetEglDisplay() { return nullptr; }
inline const void* MU_MobileGetEglContext() { return nullptr; }

#endif // defined(__ANDROID__) || defined(MU_IOS)
