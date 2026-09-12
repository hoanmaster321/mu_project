#pragma once

#if defined(__ANDROID__) || defined(MU_IOS)

#include <SDL3/SDL.h>
#include <string>

#if defined(__ANDROID__)
#include <jni.h>
#include <android/native_activity.h>
#endif

extern "C" {
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((weak)) void sapp_show_keyboard(bool show);
    __attribute__((weak)) bool sapp_keyboard_shown(void);
    __attribute__((weak)) const void* sapp_android_get_native_activity(void);

    __attribute__((weak)) void sapp_show_keyboard(bool) {}
    __attribute__((weak)) bool sapp_keyboard_shown(void) { return false; }
    __attribute__((weak)) const void* sapp_android_get_native_activity(void) { return nullptr; }
#else
    void sapp_show_keyboard(bool show);
    bool sapp_keyboard_shown(void);
    const void* sapp_android_get_native_activity(void);
#endif
}

inline bool& MU_MobileTextInputActiveState() {
    static bool s_active = false;
    return s_active;
}

inline void MU_MobilePlatformInit() {
    MU_MobileTextInputActiveState() = false;
}
inline void MU_MobilePlatformShutdown() {
    MU_MobileTextInputActiveState() = false;
}

inline const bool* MU_MobileGetKeyboardState() { return SDL_GetKeyboardState(nullptr); }
inline void MU_MobileSetKeyState(SDL_Scancode scancode, bool isDown) {}
inline void MU_MobileClearKeyboardState() {}

inline void MU_MobilePlatformShowKeyboard(bool show) {
    MU_MobileTextInputActiveState() = show;

#if defined(__ANDROID__)
    const void* rawActivity = sapp_android_get_native_activity();
    if (rawActivity) {
        auto* activity = static_cast<const ANativeActivity*>(rawActivity);
        if (activity && activity->vm) {
            JNIEnv* env = nullptr;
            bool attached = false;
            jint res = activity->vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
            if (res == JNI_EDETACHED) {
                if (activity->vm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                    attached = true;
                }
            }
            if (env) {
                jclass actClass = env->FindClass("com/muonline/client/MuMainNativeActivity");
                if (actClass) {
                    jmethodID method = env->GetStaticMethodID(
                        actClass,
                        show ? "showKeyboardFromNative" : "hideKeyboardFromNative",
                        "()V");
                    if (method) {
                        env->CallStaticVoidMethod(actClass, method);
                    }
                    env->DeleteLocalRef(actClass);
                }
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                }
                if (attached) {
                    activity->vm->DetachCurrentThread();
                }
            }
        }
    }
#endif

    sapp_show_keyboard(show);

    SDL_Window* w = SDL_GetKeyboardFocus();
    if (!w) {
        int count = 0;
        SDL_Window** windows = SDL_GetWindows(&count);
        if (windows && count > 0) {
            w = windows[0];
        }
    }
    if (w) {
        if (show) SDL_StartTextInput(w);
        else SDL_StopTextInput(w);
    }
}

inline void MU_MobileStartTextInput() {
    MU_MobilePlatformShowKeyboard(true);
}
inline void MU_MobileStopTextInput() {
    MU_MobilePlatformShowKeyboard(false);
}
inline bool MU_MobileIsTextInputActive() {
    SDL_Window* w = SDL_GetKeyboardFocus();
    if (!w) {
        int count = 0;
        SDL_Window** windows = SDL_GetWindows(&count);
        if (windows && count > 0) {
            w = windows[0];
        }
    }
    if (w && SDL_TextInputActive(w)) return true;
    return MU_MobileTextInputActiveState() || sapp_keyboard_shown();
}
inline void MU_MobileSetTextInputRect(const SDL_Rect* rect) {
    SDL_Window* w = SDL_GetKeyboardFocus();
    if (!w) {
        int count = 0;
        SDL_Window** windows = SDL_GetWindows(&count);
        if (windows && count > 0) {
            w = windows[0];
        }
    }
    if (w && rect) SDL_SetTextInputArea(w, rect, 0);
}

inline std::string MU_MobileGetExternalDataPath() {
#if defined(MU_IOS)
    const char* basePath = SDL_GetBasePath();
    if (basePath) return std::string(basePath);
    char* prefPath = SDL_GetPrefPath("muonline", "client");
    if (prefPath) {
        std::string res(prefPath);
        SDL_free(prefPath);
        return res;
    }
    return "./";
#else
    return "/sdcard/Android/data/com.muonline.client/files";
#endif
}

inline std::string MU_MobileGetInternalDataPath() {
#if defined(MU_IOS)
    char* prefPath = SDL_GetPrefPath("muonline", "client");
    if (prefPath) {
        std::string res(prefPath);
        SDL_free(prefPath);
        return res;
    }
    const char* basePath = SDL_GetBasePath();
    if (basePath) return std::string(basePath);
    return "./";
#else
    return "/data/data/com.muonline.client/files";
#endif
}

inline const void* MU_MobileGetNativeWindow() { return nullptr; }
inline const void* MU_MobileGetEglDisplay() { return nullptr; }
inline const void* MU_MobileGetEglContext() { return nullptr; }

#endif // defined(__ANDROID__) || defined(MU_IOS)
