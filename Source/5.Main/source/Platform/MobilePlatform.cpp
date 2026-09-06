#if defined(__ANDROID__) || defined(MU_IOS)

#include "MobilePlatform.h"

#include <sokol_app.h>
#include <SDL_system.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <initializer_list>

#if defined(__ANDROID__)
#include <jni.h>
#include <unistd.h>
#endif

namespace
{
std::array<Uint8, SDL_NUM_SCANCODES> g_keyboardState = {};
SDL_Rect g_textInputRect = {};
bool g_textInputActive = false;

bool MU_PathExists(const char* path)
{
#if defined(__ANDROID__)
    return (path != nullptr) && (path[0] != '\0') && (access(path, F_OK) == 0);
#else
    (void)path;
    return false;
#endif
}

std::string MU_GetFirstExistingPath(std::initializer_list<const char*> candidates)
{
    for (const char* candidate : candidates)
    {
        if (MU_PathExists(candidate))
        {
            return candidate;
        }
    }
    return {};
}

#if defined(__ANDROID__)
jclass g_keyboardBridgeClass = nullptr;
jmethodID g_showKeyboardMethod = nullptr;
jmethodID g_hideKeyboardMethod = nullptr;

void MU_ClearKeyboardBridge(JNIEnv* env)
{
    if (env == nullptr)
    {
        return;
    }

    if (g_keyboardBridgeClass != nullptr)
    {
        env->DeleteGlobalRef(g_keyboardBridgeClass);
        g_keyboardBridgeClass = nullptr;
    }

    g_showKeyboardMethod = nullptr;
    g_hideKeyboardMethod = nullptr;
}

void MU_RegisterKeyboardBridge(JNIEnv* env, jobject activity)
{
    if ((env == nullptr) || (activity == nullptr))
    {
        return;
    }

    jclass localClass = env->GetObjectClass(activity);
    if (localClass == nullptr)
    {
        env->ExceptionClear();
        return;
    }

    jclass globalClass = static_cast<jclass>(env->NewGlobalRef(localClass));
    env->DeleteLocalRef(localClass);

    if (globalClass == nullptr)
    {
        env->ExceptionClear();
        return;
    }

    MU_ClearKeyboardBridge(env);
    g_keyboardBridgeClass = globalClass;
    g_showKeyboardMethod =
        env->GetStaticMethodID(g_keyboardBridgeClass, "showKeyboardFromNative", "()V");
    g_hideKeyboardMethod =
        env->GetStaticMethodID(g_keyboardBridgeClass, "hideKeyboardFromNative", "()V");

    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
    }
}

void MU_CallKeyboardBridge(const char* methodName)
{
    if (methodName == nullptr || methodName[0] == '\0')
    {
        return;
    }

    JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    if (env == nullptr)
    {
        return;
    }

    if (g_keyboardBridgeClass == nullptr)
    {
        return;
    }

    jmethodID method = nullptr;
    if (std::strcmp(methodName, "showKeyboardFromNative") == 0)
    {
        method = g_showKeyboardMethod;
    }
    else if (std::strcmp(methodName, "hideKeyboardFromNative") == 0)
    {
        method = g_hideKeyboardMethod;
    }

    if (method == nullptr)
    {
        return;
    }

    env->CallStaticVoidMethod(g_keyboardBridgeClass, method);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
    }
}
#endif
} // namespace

#if defined(__ANDROID__)
extern "C" JNIEXPORT void JNICALL
Java_com_muonline_client_MuMainNativeActivity_nativeSetKeyboardBridge(
    JNIEnv* env,
    jobject activity)
{
    MU_RegisterKeyboardBridge(env, activity);
}

extern "C" JNIEXPORT void JNICALL
Java_com_muonline_client_MuMainNativeActivity_nativeClearKeyboardBridge(
    JNIEnv* env,
    jobject)
{
    MU_ClearKeyboardBridge(env);
}
#endif

void MU_MobilePlatformInit()
{
    MU_MobileClearKeyboardState();
    g_textInputRect = {};
    g_textInputActive = false;
}

void MU_MobilePlatformShutdown()
{
    g_textInputActive = false;
    g_textInputRect = {};
    MU_MobileClearKeyboardState();
}

const Uint8* MU_MobileGetKeyboardState()
{
    return g_keyboardState.data();
}

void MU_MobileSetKeyState(SDL_Scancode scancode, bool isDown)
{
    if ((scancode >= 0) && (static_cast<size_t>(scancode) < g_keyboardState.size()))
    {
        g_keyboardState[static_cast<size_t>(scancode)] = isDown ? 1u : 0u;
    }
}

void MU_MobileClearKeyboardState()
{
    std::fill(g_keyboardState.begin(), g_keyboardState.end(), static_cast<Uint8>(0));
}

void MU_MobileStartTextInput()
{
    g_textInputActive = true;
#if defined(__ANDROID__)
    MU_CallKeyboardBridge("showKeyboardFromNative");
#endif
    sapp_show_keyboard(true);
}

void MU_MobileStopTextInput()
{
    g_textInputActive = false;
#if defined(__ANDROID__)
    MU_CallKeyboardBridge("hideKeyboardFromNative");
#endif
    sapp_show_keyboard(false);
}

bool MU_MobileIsTextInputActive()
{
    return g_textInputActive || sapp_keyboard_shown();
}

void MU_MobileSetTextInputRect(const SDL_Rect* rect)
{
    if (rect)
    {
        g_textInputRect = *rect;
    }
    else
    {
        g_textInputRect = {};
    }
}

std::string MU_MobileGetExternalDataPath()
{
    return MU_GetFirstExistingPath({
        "/sdcard/Android/data/com.muonline.client/files",
        "/storage/emulated/0/Android/data/com.muonline.client/files"
    });
}

std::string MU_MobileGetInternalDataPath()
{
    return MU_GetFirstExistingPath({
        "/data/user/0/com.muonline.client/files",
        "/data/data/com.muonline.client/files"
    });
}

#endif // defined(__ANDROID__) || defined(MU_IOS)
