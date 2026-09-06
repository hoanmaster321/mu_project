#pragma once
// =============================================================================
// AndroidGDI.h
// Android-side GDI stub types and declarations.
// Replaces void* handles with real structs so TextOut/GetTextExtentPoint32 work.
// Implemented in AndroidGDI.cpp using SDL2_ttf.
// =============================================================================
#if defined(__ANDROID__) || defined(MU_IOS)

#include <stdint.h>
#include <stdlib.h>

// ── Internal GDI handle structs ───────────────────────────────────────────

enum AndroidGDIObjectType : uint32_t {
    ANDROID_GDI_OBJECT_UNKNOWN = 0,
    ANDROID_GDI_OBJECT_BITMAP  = 1,
    ANDROID_GDI_OBJECT_FONT    = 2
};

struct AndroidBitmap {
    AndroidGDIObjectType type;
    uint8_t* data;      // RGB24 pixel data (calloc'd)
    int      width;
    int      height;
    int      pitch;     // bytes per scanline = ((width*24+31)&~31)/8
};

struct AndroidFont {
    AndroidGDIObjectType type;
    void*  ttfFont;     // TTF_Font*
    int    size;        // requested pixel height
    bool   bold;
};

struct AndroidDC {
    AndroidBitmap* bmp;            // currently selected bitmap
    AndroidFont*   font;           // currently selected font
    uint32_t       textColor;      // 0xRRGGBB00  (matches COLORREF RGB() on Windows)
    uint32_t       bgColor;
};

// ── Typedefs that replace void* in the Android build ─────────────────────
// These MUST match the typedefs in PlatformDefs.h.
// We change PlatformDefs.h to use these instead.

typedef AndroidDC*     HDC;
typedef AndroidBitmap* HBITMAP;
typedef AndroidFont*   HFONT;
typedef void*          HGDIOBJ;   // generic GDI object (HFONT or HBITMAP)
typedef void*          HBRUSH;
typedef void*          HPEN;
typedef void*          HINSTANCE;
typedef void*          HMENU;

// ── Function declarations (inline stubs - zero SDL_ttf dependency) ────────────────────
inline void    AndroidGDI_Init(int /*defaultFontSizePx*/) {}
inline void    AndroidGDI_Shutdown() {}

inline HBITMAP AndroidCreateDIBSection(const void* /*bmiPtr*/, void** ppvBits) { if (ppvBits) *ppvBits = nullptr; return nullptr; }
inline HDC     AndroidCreateCompatibleDC(HDC /*src*/) { static AndroidDC dummyDC{}; return &dummyDC; }
inline HFONT   AndroidCreateFont(int /*height*/, int /*weight*/) { static AndroidFont dummyFont{}; return &dummyFont; }
inline HGDIOBJ AndroidSelectObject(HDC /*hdc*/, HGDIOBJ obj) { return obj; }
inline void    AndroidSelectBitmap(HDC hdc, HBITMAP bmp) { if (hdc) hdc->bmp = bmp; }
inline void    AndroidSelectFont(HDC hdc, HFONT font) { if (hdc) hdc->font = font; }
inline bool    AndroidTextOut(HDC /*hdc*/, int /*x*/, int /*y*/, const wchar_t* /*text*/, int /*len*/) { return true; }
inline bool    AndroidGetTextExtentPoint32(HDC /*hdc*/, const wchar_t* /*text*/, int len, int* outW, int* outH) {
    if (outW) *outW = (len > 0 ? len : 1) * 8;
    if (outH) *outH = 14;
    return true;
}
inline void    AndroidSetTextColor(HDC hdc, uint32_t colorref) { if (hdc) hdc->textColor = colorref; }
inline void    AndroidSetBkColor(HDC hdc, uint32_t colorref) { if (hdc) hdc->bgColor = colorref; }
inline bool    AndroidDeleteDC(HDC /*hdc*/) { return true; }
inline bool    AndroidDeleteObject(HGDIOBJ /*obj*/) { return true; }

#endif // __ANDROID__
