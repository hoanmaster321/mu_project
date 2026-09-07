// =============================================================================
// AndroidGDI.cpp
// Android GDI text-rendering engine built on FreeType.
// Provides CreateFont / CreateDIBSection / TextOut / GetTextExtentPoint32 etc.
// =============================================================================
#ifdef __ANDROID__

#include "AndroidGDI.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <android/log.h>
#include <vector>
#include <string>
#include <fstream>
#include <limits>
#include <cstring>
#include <cstdlib>

#define LOG_TAG "AndroidGDI"
#if defined(MU_ANDROID_DISABLE_LOG)
#define LOGI(...) ((void)0)
#define LOGE(...) ((void)0)
#else
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

static FT_Library s_FTLib = nullptr;
static int s_DefaultFontSize = 14;

// Cache font files in memory so FT_New_Memory_Face is fast and avoids repeated file I/O
static std::vector<unsigned char> s_FontBytesNormal;
static std::vector<unsigned char> s_FontBytesBold;

static bool ReadFontFile(const char* path, std::vector<unsigned char>& outBytes) {
    if (!path || !path[0]) return false;
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) return false;
    std::streamsize sz = stream.tellg();
    if (sz <= 0) return false;
    outBytes.resize((size_t)sz);
    stream.seekg(0, std::ios::beg);
    if (!stream.read(reinterpret_cast<char*>(outBytes.data()), sz)) {
        outBytes.clear();
        return false;
    }
    return true;
}

static void LoadFontBytes() {
    static const char* sNormalPaths[] = {
        "Data/fonts/font_.ttf",
        "/sdcard/Android/data/com.muonline.client/files/Data/fonts/font_.ttf",
        "/storage/emulated/0/Android/data/com.muonline.client/files/Data/fonts/font_.ttf",
        "/data/data/com.muonline.client/files/Data/fonts/font_.ttf",
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/NotoSans-Regular.ttf",
        "/system/fonts/DroidSans.ttf",
        nullptr
    };
    static const char* sBoldPaths[] = {
        "Data/fonts/font_2.ttf",
        "/sdcard/Android/data/com.muonline.client/files/Data/fonts/font_2.ttf",
        "/storage/emulated/0/Android/data/com.muonline.client/files/Data/fonts/font_2.ttf",
        "/data/data/com.muonline.client/files/Data/fonts/font_2.ttf",
        "/system/fonts/Roboto-Bold.ttf",
        "/system/fonts/NotoSans-Bold.ttf",
        "/system/fonts/DroidSans-Bold.ttf",
        nullptr
    };

    s_FontBytesNormal.clear();
    for (int i = 0; sNormalPaths[i]; ++i) {
        if (ReadFontFile(sNormalPaths[i], s_FontBytesNormal)) {
            LOGI("Loaded normal font from %s", sNormalPaths[i]);
            break;
        }
    }

    s_FontBytesBold.clear();
    for (int i = 0; sBoldPaths[i]; ++i) {
        if (ReadFontFile(sBoldPaths[i], s_FontBytesBold)) {
            LOGI("Loaded bold font from %s", sBoldPaths[i]);
            break;
        }
    }

    if (s_FontBytesBold.empty() && !s_FontBytesNormal.empty()) {
        s_FontBytesBold = s_FontBytesNormal;
    }
}

// ── Public API ────────────────────────────────────────────────────────────

void AndroidGDI_Init(int defaultFontSizePx) {
    s_DefaultFontSize = (defaultFontSizePx > 0) ? defaultFontSizePx : 14;
    if (!s_FTLib) {
        if (FT_Init_FreeType(&s_FTLib) != 0) {
            LOGE("Failed to initialize FreeType library!");
            s_FTLib = nullptr;
            return;
        }
    }
    if (s_FontBytesNormal.empty()) {
        LoadFontBytes();
    }
    LOGI("AndroidGDI initialized with FreeType, defaultFontSize=%d", s_DefaultFontSize);
}

void AndroidGDI_Shutdown() {
    if (s_FTLib) {
        FT_Done_FreeType(s_FTLib);
        s_FTLib = nullptr;
    }
    s_FontBytesNormal.clear();
    s_FontBytesBold.clear();
}

// ── CreateDIBSection ──────────────────────────────────────────────────────
struct RawBITMAPINFOHEADER { int biSize; int biWidth; int biHeight; short biPlanes; short biBitCount; };
HBITMAP AndroidCreateDIBSection(const void* bmiPtr, void** ppvBits) {
    if (!bmiPtr || !ppvBits) { if (ppvBits) *ppvBits = nullptr; return nullptr; }
    const RawBITMAPINFOHEADER* hdr = reinterpret_cast<const RawBITMAPINFOHEADER*>(bmiPtr);
    int w = hdr->biWidth;
    int h = hdr->biHeight < 0 ? -hdr->biHeight : hdr->biHeight;
    int bpp = hdr->biBitCount;
    int pitch = ((w * bpp + 31) & ~31) >> 3;
    size_t sz = static_cast<size_t>(pitch) * static_cast<size_t>(h > 0 ? h : 1);

    AndroidBitmap* bmp = new AndroidBitmap;
    bmp->type   = ANDROID_GDI_OBJECT_BITMAP;
    bmp->data   = reinterpret_cast<uint8_t*>(calloc(sz, 1));
    bmp->width  = w;
    bmp->height = h;
    bmp->pitch  = pitch;
    *ppvBits    = bmp->data;
    return bmp;
}

HDC AndroidCreateCompatibleDC(HDC /*src*/) {
    AndroidDC* dc = new AndroidDC;
    dc->bmp       = nullptr;
    dc->font      = nullptr;
    dc->textColor = 0xFFFFFF00;
    dc->bgColor   = 0x00000000;
    return dc;
}

HFONT AndroidCreateFont(int height, int weight) {
    if (height <= 0) height = s_DefaultFontSize;
    bool bold = (weight >= 600);

    if (!s_FTLib) {
        AndroidGDI_Init(s_DefaultFontSize);
    }
    if (s_FontBytesNormal.empty()) {
        LoadFontBytes();
    }

    const auto& fontBytes = (bold && !s_FontBytesBold.empty()) ? s_FontBytesBold : s_FontBytesNormal;
    if (fontBytes.empty()) {
        LOGE("AndroidCreateFont: no font bytes loaded!");
        return nullptr;
    }

    FT_Face face = nullptr;
    FT_Error err = FT_New_Memory_Face(s_FTLib, fontBytes.data(), static_cast<FT_Long>(fontBytes.size()), 0, &face);
    if (err != 0 || !face) {
        LOGE("FT_New_Memory_Face failed: %d", err);
        return nullptr;
    }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, height);

    AndroidFont* f = new AndroidFont;
    f->type    = ANDROID_GDI_OBJECT_FONT;
    f->ttfFont = face;
    f->size    = height;
    f->bold    = bold;
    return f;
}

HGDIOBJ AndroidSelectObject(HDC hdc, HGDIOBJ obj) {
    if (!hdc || !obj) return nullptr;
    const auto type = *(reinterpret_cast<const AndroidGDIObjectType*>(obj));
    if (type == ANDROID_GDI_OBJECT_BITMAP) {
        hdc->bmp = reinterpret_cast<AndroidBitmap*>(obj);
    } else if (type == ANDROID_GDI_OBJECT_FONT) {
        hdc->font = reinterpret_cast<AndroidFont*>(obj);
    }
    return obj;
}

void AndroidSelectBitmap(HDC hdc, HBITMAP bmp) {
    if (hdc) hdc->bmp = bmp;
}

void AndroidSelectFont(HDC hdc, HFONT font) {
    if (hdc) hdc->font = font;
}

bool AndroidTextOut(HDC hdc, int x, int y, const wchar_t* text, int len) {
    if (!hdc || !hdc->bmp || !hdc->bmp->data || !text || len <= 0) return false;

    AndroidFont* af = hdc->font;
    FT_Face face = af ? reinterpret_cast<FT_Face>(af->ttfFont) : nullptr;
    int fontSize = af ? af->size : s_DefaultFontSize;

    if (!face) {
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    AndroidBitmap* bmp = hdc->bmp;
    int ascent = static_cast<int>(face->size->metrics.ascender >> 6);
    int baseline = y + ascent;

    // Measure total advance width to clear background area
    int strW = 0;
    for (int i = 0; i < len && text[i]; ++i) {
        if (FT_Load_Char(face, text[i], FT_LOAD_DEFAULT) == 0) {
            strW += static_cast<int>(face->glyph->advance.x >> 6);
        }
    }
    int fontH = static_cast<int>(face->size->metrics.height >> 6);
    if (fontH <= 0) fontH = fontSize;

    // Clear background to 0 (black / transparent) in bmp->data
    for (int r = 0; r < fontH; ++r) {
        int dstY = y + r;
        if (dstY < 0 || dstY >= bmp->height) continue;
        uint8_t* row = bmp->data + dstY * bmp->pitch + x * 3;
        int clearW = strW + 4;
        if (x + clearW > bmp->width) clearW = bmp->width - x;
        if (clearW > 0) memset(row, 0, clearW * 3);
    }

    // Render glyphs
    int penX = x;
    for (int i = 0; i < len && text[i]; ++i) {
        wchar_t ch = text[i];
        if (FT_Load_Char(face, ch, FT_LOAD_RENDER) != 0) {
            penX += (fontSize / 2);
            continue;
        }

        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap* gBmp = &slot->bitmap;

        int gx = penX + slot->bitmap_left;
        int gy = baseline - slot->bitmap_top;

        for (unsigned int row = 0; row < gBmp->rows; ++row) {
            int dstY = gy + static_cast<int>(row);
            if (dstY < 0 || dstY >= bmp->height) continue;

            uint8_t* srcRow = gBmp->buffer + row * gBmp->pitch;
            uint8_t* dstRow = bmp->data + dstY * bmp->pitch;

            for (unsigned int col = 0; col < gBmp->width; ++col) {
                int dstX = gx + static_cast<int>(col);
                if (dstX < 0 || dstX >= bmp->width) continue;

                uint8_t coverage = srcRow[col];
                if (coverage > 0) {
                    uint8_t* dst = dstRow + dstX * 3;
                    if (coverage > dst[0]) {
                        dst[0] = coverage;
                        dst[1] = coverage;
                        dst[2] = coverage;
                    }
                }
            }
        }

        penX += static_cast<int>(slot->advance.x >> 6);
    }

    return true;
}

bool AndroidGetTextExtentPoint32(HDC hdc, const wchar_t* text, int len, int* outW, int* outH) {
    if (outW) *outW = 0;
    if (outH) *outH = 0;
    if (!text || len <= 0) return false;

    AndroidFont* af = hdc ? hdc->font : nullptr;
    FT_Face face = af ? reinterpret_cast<FT_Face>(af->ttfFont) : nullptr;
    int fontSize = af ? af->size : s_DefaultFontSize;

    if (!face) {
        if (outW) *outW = len * fontSize / 2;
        if (outH) *outH = fontSize;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    int totalW = 0;
    for (int i = 0; i < len && text[i]; ++i) {
        wchar_t ch = text[i];
        if (FT_Load_Char(face, ch, FT_LOAD_DEFAULT) == 0) {
            totalW += static_cast<int>(face->glyph->advance.x >> 6);
        } else {
            totalW += (fontSize / 2);
        }
    }

    int fontH = static_cast<int>(face->size->metrics.height >> 6);
    if (fontH <= 0) fontH = fontSize;

    if (outW) *outW = totalW;
    if (outH) *outH = fontH;
    return true;
}

void AndroidSetTextColor(HDC hdc, uint32_t colorref) {
    if (hdc) hdc->textColor = colorref;
}

void AndroidSetBkColor(HDC hdc, uint32_t colorref) {
    if (hdc) hdc->bgColor = colorref;
}

bool AndroidDeleteDC(HDC hdc) {
    if (!hdc) return false;
    delete hdc;
    return true;
}

bool AndroidDeleteObject(HGDIOBJ obj) {
    if (!obj) return false;
    const auto type = *(reinterpret_cast<const AndroidGDIObjectType*>(obj));
    if (type == ANDROID_GDI_OBJECT_BITMAP) {
        AndroidBitmap* bmp = reinterpret_cast<AndroidBitmap*>(obj);
        if (bmp->data) {
            free(bmp->data);
            bmp->data = nullptr;
        }
        delete bmp;
        return true;
    }
    if (type == ANDROID_GDI_OBJECT_FONT) {
        AndroidFont* font = reinterpret_cast<AndroidFont*>(obj);
        if (font->ttfFont) {
            FT_Done_Face(reinterpret_cast<FT_Face>(font->ttfFont));
            font->ttfFont = nullptr;
        }
        delete font;
        return true;
    }
    return false;
}

#endif // __ANDROID__
