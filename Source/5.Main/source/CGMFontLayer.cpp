#include "stdafx.h"
#include "ZzzOpenglUtil.h"
#include "UIControls.h"
#include "CGMFontLayer.h"
#include "MultiLanguage.h"
#include "./Utilities/Log/muConsoleDebug.h"
#include "VulkanTextureManager.h"
#include "GPUContext.h"
#include "BatchRenderer.h"

extern void BindTexture(int tex);

CGMFontLayer::CGMFontLayer()
{
	//BitmapIndex = -1;
	//output_width = 0;
	//output_hight = 0;
	//metrics_height = 0;

	BitmapFontIndex = -1;
}

CGMFontLayer::~CGMFontLayer()
{
	Characters.clear();
	hFontBuffer.clear();

	if (BitmapFontIndex != -1)
	{
		if (GPUContext::Instance().IsInitialized())
		{
			VulkanTextureManager::Instance().DestroyTexture(BitmapFontIndex);
		}
	}

	for (int i = 0; i < MAX_LINE_FONT; i++)
	{
		if (NormalFont[i].BitmapIndex != -1)
		{
			if (GPUContext::Instance().IsInitialized())
			{
				VulkanTextureManager::Instance().DestroyTexture(NormalFont[i].BitmapIndex);
			}
		}
		NormalFont[i].PakBuffer.clear();
	}
}

_FT_Bitmap* CGMFontLayer::GetULongChar(FT_ULong charcode)
{
	type_map_bitmap::iterator it = Characters.find(charcode);

	if (it != Characters.end())
	{
		return &it->second;
	}

	return NULL;
}

void CGMFontLayer::runtime_load_bitmap(GLuint* textures, GLsizei _width, GLsizei _height, BYTE* data)
{
	static GLuint s_nextFontTexId = BITMAP_GLYPH_ATLAS;
	if (*textures == (GLuint)-1 || *textures == 0)
	{
		*textures = s_nextFontTexId++;
	}

	if (GPUContext::Instance().IsInitialized() && data && _width > 0 && _height > 0)
	{
		VulkanTextureManager::Instance().CreateTextureWithId(*textures, _width, _height, 1, data, true, true);
	}
}

bool isChineseChar(FT_ULong charcode)
{
	return (charcode >= 0x4E00 && charcode <= 0x9FFF) ||
		(charcode >= 0x3400 && charcode <= 0x4DBF) ||
		(charcode >= 0x20000 && charcode <= 0x2A6DF) ||
		(charcode >= 0x2A700 && charcode <= 0x2B73F) ||
		(charcode >= 0x2B740 && charcode <= 0x2B81F) ||
		(charcode >= 0x2B820 && charcode <= 0x2CEAF) ||
		(charcode >= 0x2CEB0 && charcode <= 0x2EBEF) ||
		(charcode >= 0xF900 && charcode <= 0xFAFF);
}

bool isKoreanChar(FT_ULong charcode)
{
	return (charcode >= 0xAC00 && charcode <= 0xD7AF) ||
		(charcode >= 0x1100 && charcode <= 0x11FF) ||
		(charcode >= 0xA960 && charcode <= 0xA97F) ||
		(charcode >= 0xD7B0 && charcode <= 0xD7FF);
}

void CGMFontLayer::runtime_font_property(HDC hdc, HFONT hFont, DWORD dwTable, FT_Library library, BitmapFont* FontType, int FontIndex, int PixelSize)
{
#ifdef __ANDROID__
	FT_Face face = nullptr;
	const char* fontFile = (FontIndex == 1) ? "Data/fonts/font_2.ttf" : "Data/fonts/font_.ttf";
	if (FT_New_Face(library, fontFile, 0, &face) != 0)
	{
		if (FT_New_Face(library, "/system/fonts/Roboto-Regular.ttf", 0, &face) != 0)
		{
			return;
		}
	}

	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE))
	{
		FT_Done_Face(face);
		return;
	}
#else
	TEXTMETRICW tm;

	SelectObject(hdc, hFont);

	if (GetTextMetricsW(hdc, &tm))
	{
		PixelSize = tm.tmHeight;

		if (tm.tmCharSet == GB2312_CHARSET)
		{
			dwTable = 0x66637474;
		}
	}

	DWORD fontSize = GetFontData(hdc, 0, 0, NULL, 0);

	if (fontSize == GDI_ERROR)
	{
		return;
	}

	std::vector<BYTE> fontData(fontSize, 0);

	if (GetFontData(hdc, dwTable, 0, fontData.data(), fontSize) == GDI_ERROR)
	{
		return;
	}

	FT_Face face;

	if (FT_New_Memory_Face(library, fontData.data(), fontSize, 0, &face))
	{
		return;
	}

	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE))
	{
		return;
	}
#endif

	FT_Set_Pixel_Sizes(face, 0, PixelSize);

	FontType->metrics_height = (face->size->metrics.height >> 6);

	int pen_x = 0, pen_y = 0;
	FT_UInt max_dim = (FontType->metrics_height) * ceilf(sqrtf(face->num_glyphs));

	FontType->output_width = 1;

	while (FontType->output_width < max_dim)
	{
		FontType->output_width <<= 1;
	}
	FontType->output_hight = FontType->output_width;

	int max_size = (FontType->output_width * FontType->output_hight);

	FontType->PakBuffer.resize(max_size, 0);

	int num_glyphs = 0;

	FT_UInt glyph_index = 1;
	FT_ULong Charcode = FT_Get_First_Char(face, &glyph_index);

	while (glyph_index != 0)
	{
		if (GetULongChar(Charcode) == NULL)
		{
			FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING);

			FT_Render_Glyph(face->glyph, ft_render_mode_normal);

			FT_Bitmap* pBitmap = &face->glyph->bitmap;

			if (pen_x + pBitmap->width >= FontType->output_width)
			{
				pen_x = 0;
				pen_y += FontType->metrics_height;
			}

			for (FT_UInt row = 0; row < pBitmap->rows; ++row)
			{
				for (FT_UInt col = 0; col < pBitmap->width; ++col)
				{
					int x = pen_x + col;
					int y = pen_y + row;
					FontType->PakBuffer[y * FontType->output_width + x] = pBitmap->buffer[row * pBitmap->pitch + col];
				}
			}

			_FT_Bitmap pNewBitmap;
			pNewBitmap.FontType = FontIndex;
			pNewBitmap.RenderPosX = pen_x;
			pNewBitmap.RenderPosY = pen_y;
			pNewBitmap.RenderSizeX = pBitmap->width;
			pNewBitmap.RenderSizeY = pBitmap->rows;
			pNewBitmap.bitmap_left = face->glyph->bitmap_left;
			pNewBitmap.bitmap_top = face->glyph->bitmap_top;
			pNewBitmap.advance = face->glyph->advance.x >> 6;

			Characters.insert(type_map_bitmap::value_type(Charcode, pNewBitmap));

			pen_x += pBitmap->width + 1;
		}
		Charcode = FT_Get_Next_Char(face, Charcode, &glyph_index);
		num_glyphs++;
	}

	runtime_load_bitmap(&FontType->BitmapIndex, FontType->output_width, FontType->output_hight, FontType->PakBuffer.data());

	FT_Done_Face(face);
}

void CGMFontLayer::runtime_font_property(const char* file_base, FT_Library library, BitmapFont* FontType, int FontIndex, int PixelSize, FT_Encoding encoding)
{
	FT_Face face;

	if (FT_New_Face(library, file_base, 0, &face))
	{
		return;
	}

	if (FT_Select_Charmap(face, encoding))
	{
		return;
	}

	FT_Set_Pixel_Sizes(face, 0, PixelSize);

	FontType->metrics_height = (face->size->metrics.height >> 6);

	int pen_x = 0, pen_y = 0;
	FT_UInt max_dim = (FontType->metrics_height) * ceilf(sqrtf(face->num_glyphs));

	FontType->output_width = 1;

	while (FontType->output_width < max_dim)
	{
		FontType->output_width <<= 1;
	}
	FontType->output_hight = FontType->output_width;

	int max_size = (FontType->output_width * FontType->output_hight);

	FontType->PakBuffer.resize(max_size, 0);

	int num_glyphs = 0;

	FT_UInt glyph_index = 1;
	FT_ULong Charcode = FT_Get_First_Char(face, &glyph_index);

	while (glyph_index != 0)
	{
		if (GetULongChar(Charcode) == NULL)
		{
			FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING);

			FT_Render_Glyph(face->glyph, ft_render_mode_normal);

			FT_Bitmap* pBitmap = &face->glyph->bitmap;

			if (pen_x + pBitmap->width >= FontType->output_width)
			{
				pen_x = 0;
				pen_y += FontType->metrics_height;
			}

			for (FT_UInt row = 0; row < pBitmap->rows; ++row)
			{
				for (FT_UInt col = 0; col < pBitmap->width; ++col)
				{
					int x = pen_x + col;
					int y = pen_y + row;
					FontType->PakBuffer[y * FontType->output_width + x] = pBitmap->buffer[row * pBitmap->pitch + col];
				}
			}

			_FT_Bitmap pNewBitmap;
			pNewBitmap.FontType = FontIndex;
			pNewBitmap.RenderPosX = pen_x;
			pNewBitmap.RenderPosY = pen_y;
			pNewBitmap.RenderSizeX = pBitmap->width;
			pNewBitmap.RenderSizeY = pBitmap->rows;
			pNewBitmap.bitmap_left = face->glyph->bitmap_left;
			pNewBitmap.bitmap_top = face->glyph->bitmap_top;
			pNewBitmap.advance = face->glyph->advance.x >> 6;

			Characters[Charcode] = pNewBitmap;

			pen_x += pBitmap->width + 1;
		}
		Charcode = FT_Get_Next_Char(face, Charcode, &glyph_index);
		num_glyphs++;
	}

	runtime_load_bitmap(&FontType->BitmapIndex, FontType->output_width, FontType->output_hight, FontType->PakBuffer.data());

	FT_Done_Face(face);
}

void CGMFontLayer::runtime_font_property(HFONT hFont, int PixelSize)
{
	FT_Library library;

	if (FT_Init_FreeType(&library))
	{
		return;
	}
	//HFONT _hFont;
	bool font_korean = GetPrivateProfileInt("SettingsFont", "font-korean", 0, ".\\config.ini");

	bool font_chinase = GetPrivateProfileInt("SettingsFont", "font-chinase", 0, ".\\config.ini");

	HDC hdc = GetDC(NULL);

	Characters.clear();

	bool hasClientFont = false;
	FILE* fpFont = fopen("Data\\Interface\\HUD\\fonts\\fonts_all.ttf", "rb");
	if (fpFont)
	{
		fclose(fpFont);
		hasClientFont = true;
	}

	if (hasClientFont || font_chinase)
		runtime_font_property("Data\\Interface\\HUD\\fonts\\fonts_all.ttf", library, &NormalFont[0], 0, PixelSize);
	else
		runtime_font_property(hdc, hFont, 0, library, &NormalFont[0], 0, PixelSize);


	if (font_chinase)
	{
		runtime_font_property("Data\\Interface\\HUD\\fonts\\fonts_ch.ttf", library, &NormalFont[1], 1, PixelSize);
		//_hFont = CreateFont(PixelSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Microsoft YaHei UI");
		//runtime_font_property(hdc, _hFont, 0x66637474, library, &NormalFont[1], 1, PixelSize);
	}

	if (font_korean)
	{
		runtime_font_property("Data\\Interface\\HUD\\fonts\\fonts_kr.ttf", library, &NormalFont[2], 2, PixelSize);
		//_hFont = CreateFont(PixelSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Malgun Gothic");
		//runtime_font_property(hdc, _hFont, 0, library, &NormalFont[2], 2, PixelSize);
	}

	BYTE* buffer = new BYTE[512 * 32];

	runtime_load_bitmap(&BitmapFontIndex, 512, 32, buffer);

	SAFE_DELETE_ARRAY(buffer);

	FT_Done_FreeType(library);

	DeleteDC(hdc);
}

BOOL CGMFontLayer::_GetTextExtentPoint32(std::wstring wstrText, LPSIZE lpSize)
{
	if (lpSize)
	{
		lpSize->cx = 0;
		lpSize->cy = 0;

		for (auto wstr = wstrText.begin(); wstr != wstrText.end(); wstr++)
		{
			_FT_Bitmap* pBitmap;

			if (*wstr == 32)
				pBitmap = GetULongChar('|');
			else
				pBitmap = GetULongChar(*wstr);

			if (!pBitmap)
				pBitmap = GetULongChar(1);

			if (!pBitmap)
				continue;

			int metrics = ((NormalFont[pBitmap->FontType].metrics_height - pBitmap->bitmap_top) + pBitmap->RenderSizeY);

			if (lpSize->cy < metrics)
			{
				lpSize->cy = metrics;
			}

			lpSize->cx += pBitmap->advance;
		}
	}

	return 0;
}

BOOL CGMFontLayer::_GetTextExtentPoint32(LPCSTR lpString, int cbString, LPSIZE lpSize)
{
	std::wstring wstrText = L"";
	g_pMultiLanguage->ConvertCharToWideStr(wstrText, lpString);
	_GetTextExtentPoint32(wstrText, lpSize);
	return 0;
}

void CGMFontLayer::_TextOut(std::wstring wstrText, int& pen_x, int& pen_y)
{
	int Next_x = 0;
	for (auto wstr = wstrText.begin(); wstr != wstrText.end(); wstr++)
	{
		_FT_Bitmap* pBitmap;

		if (*wstr == 32)
			pBitmap = GetULongChar('|');
		else
			pBitmap = GetULongChar(*wstr);

		if (!pBitmap)
			pBitmap = GetULongChar(1);

		if (!pBitmap)
			continue;

		int metrics = ((NormalFont[pBitmap->FontType].metrics_height - pBitmap->bitmap_top) + pBitmap->RenderSizeY);
		if (pen_y < metrics)
		{
			pen_y = metrics;
		}

		Next_x += pBitmap->advance;
	}
	pen_x = Next_x;
}

void CGMFontLayer::runtime_writebuffer(int off_x, int off_y, _FT_Bitmap* pBitmap)
{
}

void CGMFontLayer::runtime_render_map(int pen_x, int pen_y, int RealTextX, int RealTextY, int Width, int Height, bool background)
{
}

void CGMFontLayer::FlushFontBatch()
{
	bool hasVertices = false;
	for (int f = 0; f < MAX_LINE_FONT; ++f)
	{
		if (!m_FontBatchVerts[f].empty())
		{
			hasVertices = true;
			break;
		}
	}
	if (!hasVertices)
		return;

	if (GPUContext::Instance().IsFrameActive())
	{
		for (int f = 0; f < MAX_LINE_FONT; ++f)
		{
			if (m_FontBatchVerts[f].empty())
				continue;

			int texIndex = static_cast<int>(NormalFont[f].BitmapIndex);
			for (size_t vi = 0; vi + 3 < m_FontBatchVerts[f].size(); vi += 4)
			{
				const auto& v0 = m_FontBatchVerts[f][vi + 0];
				const auto& v1 = m_FontBatchVerts[f][vi + 1];
				const auto& v2 = m_FontBatchVerts[f][vi + 2];

				float qx = v0.x;
				float qy = (float)WindowHeight - v0.y;
				float qw = v2.x - v0.x;
				float qh = v0.y - v1.y;

				ImageInstance_t img{};
				img.Texture = texIndex;
				img.x = qx;
				img.y = qy;
				img.width = qw;
				img.height = qh;
				img.u = v0.u;
				img.v = v0.v;
				img.uWidth = v2.u - v0.u;
				img.vHeight = v1.v - v0.v;
				img.color[0] = v0.r / 255.0f;
				img.color[1] = v0.g / 255.0f;
				img.color[2] = v0.b / 255.0f;
				img.color[3] = v0.a / 255.0f;
				img.rotation = 0.0f;
				img.layer = 0;
				img.RenderFlags = RENDER_ALPHA_BLEND_TYPE_NORMAL;
				img.grayscale = false;

				g_BatchRenderer.AddImage(img);
			}
			m_FontBatchVerts[f].clear();
		}
	}
}

void CGMFontLayer::RenderText(int iPos_x, int iPos_y, const unicode::t_char* pszText, int iWidth, int iHeight, int iSort, OUT SIZE* lpTextSize, bool background)
{
	if (pszText == NULL || (pszText[0] == '\0' && iHeight == 0))
		return;

	if (strlen(pszText) <= 0 && iHeight == 0)
		return;

	SIZE RealTextSize;

	std::wstring wstrText = L"";

	g_pMultiLanguage->ConvertCharToWideStr(wstrText, pszText);

	for (size_t i = 0; i < wstrText.size(); i++)
	{
		wstrText[i] = g_pMultiLanguage->ConvertFulltoHalfWidthChar(wstrText[i]);
	}

	_GetTextExtentPoint32(wstrText, &RealTextSize);

	POINT RealBoxPos = { iPos_x, iPos_y };
	SIZE RealBoxSize = { iWidth, iHeight };
	SIZE RealRenderingSize = { RealTextSize.cx, RealTextSize.cy };

	if (RealBoxSize.cx == 0)
		RealBoxSize.cx = RealTextSize.cx;

	if (RealBoxSize.cy == 0)
		RealBoxSize.cy = RealTextSize.cy;
	else
		RealBoxPos.y += (int)((RealBoxSize.cy - RealTextSize.cy) * 0.5);

	int iTab = 0;
	int iClipMove = 0;

	if (iSort == RT3_SORT_LEFT_CLIP)
	{
		if (RealRenderingSize.cx > RealBoxSize.cx)
		{
			iClipMove = RealRenderingSize.cx - RealBoxSize.cx;
			RealRenderingSize.cx = RealBoxSize.cx;
		}
	}
	else if (iSort == RT3_SORT_LEFT)
	{
		if (RealRenderingSize.cx > RealBoxSize.cx)
			RealRenderingSize.cx = RealBoxSize.cx;
	}
	else if (iSort == RT3_SORT_CENTER)
	{
		if (RealRenderingSize.cx > RealBoxSize.cx)
		{
			iClipMove = (RealRenderingSize.cx - RealBoxSize.cx) / 2;
			RealRenderingSize.cx = RealBoxSize.cx;
		}
		else
		{
			iTab = (RealBoxSize.cx - RealRenderingSize.cx) / 2;
		}
	}
	else if (iSort == RT3_SORT_RIGHT)
	{
		if (RealRenderingSize.cx > RealBoxSize.cx)
		{
			iClipMove = RealRenderingSize.cx - RealBoxSize.cx;
			RealRenderingSize.cx = RealBoxSize.cx;
		}
		else
		{
			iTab = RealBoxSize.cx - RealRenderingSize.cx;
		}
	}
	else if (iSort == RT3_WRITE_RIGHT_TO_LEFT)
	{
		if (RealRenderingSize.cx > RealBoxSize.cx)
		{
			iClipMove = RealRenderingSize.cx - RealBoxSize.cx;
			RealRenderingSize.cx = RealBoxSize.cx;
		}
		else
		{
			iTab = RealBoxSize.cx - RealRenderingSize.cx;
		}
		RealBoxPos.x -= RealBoxSize.cx;
	}
	else if (iSort == RT3_WRITE_CENTER)
	{
		if (RealRenderingSize.cx > RealBoxSize.cx)
		{
			iClipMove = (RealRenderingSize.cx - RealBoxSize.cx) / 2;
			RealRenderingSize.cx = RealBoxSize.cx;
		}
		else
		{
			iTab = (RealBoxSize.cx - RealRenderingSize.cx) / 2;
		}
		RealBoxPos.x -= (RealBoxSize.cx / 2);
	}

	DWORD dwTextColor = g_pRenderText->GetTextColor();
	if (dwTextColor != 0)
	{
		unsigned char textR = (unsigned char)GetRed(dwTextColor);
		unsigned char textG = (unsigned char)GetGreen(dwTextColor);
		unsigned char textB = (unsigned char)GetBlue(dwTextColor);
		unsigned char textA = (unsigned char)GetAlpha(dwTextColor);

		int Next_x = iTab - iClipMove;
		int Next_y = 0;

		for (int f = 0; f < MAX_LINE_FONT; ++f)
		{
			m_FontBatchVerts[f].clear();
		}

		for (auto wstr = wstrText.begin(); wstr != wstrText.end(); wstr++)
		{
			_FT_Bitmap* pBitmap;

			if (*wstr == 32)
				pBitmap = GetULongChar('|');
			else
				pBitmap = GetULongChar(*wstr);

			if (!pBitmap)
				pBitmap = GetULongChar(1);

			if (!pBitmap)
				continue;

			if (*wstr != 32)
			{
				int fontType = pBitmap->FontType;
				if (fontType >= 0 && fontType < MAX_LINE_FONT)
				{
					float atlasW = (float)NormalFont[fontType].output_width;
					float atlasH = (float)NormalFont[fontType].output_hight;
					if (atlasW > 0.0f && atlasH > 0.0f)
					{
						float u0 = (float)pBitmap->RenderPosX / atlasW;
						float v0 = (float)pBitmap->RenderPosY / atlasH;
						float u1 = (float)(pBitmap->RenderPosX + pBitmap->RenderSizeX) / atlasW;
						float v1 = (float)(pBitmap->RenderPosY + pBitmap->RenderSizeY) / atlasH;

						float gx = (float)(RealBoxPos.x + Next_x + pBitmap->bitmap_left);
						float gy = (float)(RealBoxPos.y + Next_y + (NormalFont[fontType].metrics_height - pBitmap->bitmap_top));
						float gw = (float)pBitmap->RenderSizeX;
						float gh = (float)pBitmap->RenderSizeY;

						float screenY0 = (float)WindowHeight - gy;
						float screenY1 = (float)WindowHeight - (gy + gh);

						if (background)
						{
							float sx0 = gx + 1.0f;
							float sx1 = gx + gw + 1.0f;
							float sy0 = screenY0 - 1.0f;
							float sy1 = screenY1 - 1.0f;

							FontBatchVertex sv0 = { sx0, sy0, u0, v0, 0, 0, 0, textA };
							FontBatchVertex sv1 = { sx0, sy1, u0, v1, 0, 0, 0, textA };
							FontBatchVertex sv2 = { sx1, sy1, u1, v1, 0, 0, 0, textA };
							FontBatchVertex sv3 = { sx1, sy0, u1, v0, 0, 0, 0, textA };

							m_FontBatchVerts[fontType].push_back(sv0);
							m_FontBatchVerts[fontType].push_back(sv1);
							m_FontBatchVerts[fontType].push_back(sv2);
							m_FontBatchVerts[fontType].push_back(sv3);
						}

						float fx0 = gx;
						float fx1 = gx + gw;
						float fy0 = screenY0;
						float fy1 = screenY1;

						FontBatchVertex fv0 = { fx0, fy0, u0, v0, textR, textG, textB, textA };
						FontBatchVertex fv1 = { fx0, fy1, u0, v1, textR, textG, textB, textA };
						FontBatchVertex fv2 = { fx1, fy1, u1, v1, textR, textG, textB, textA };
						FontBatchVertex fv3 = { fx1, fy0, u1, v0, textR, textG, textB, textA };

						m_FontBatchVerts[fontType].push_back(fv0);
						m_FontBatchVerts[fontType].push_back(fv1);
						m_FontBatchVerts[fontType].push_back(fv2);
						m_FontBatchVerts[fontType].push_back(fv3);
					}
				}
			}

			Next_x += pBitmap->advance;
		}

		FlushFontBatch();
	}

	if (lpTextSize)
	{
		lpTextSize->cx = RealBoxPos.x;
		lpTextSize->cy = RealBoxPos.y;
	}
}

void CGMFontLayer::RenderWave(int iPos_x, int iPos_y, const unicode::t_char* pszText, int iWidth, int iHeight, int iSort, OUT SIZE* lpTextSize)
{
	char filter[10];
	char bufer[255];
	DWORD dwTextColor = g_pRenderText->GetTextColor();

	SIZE TextSize;
	g_pRenderText->SetTextColor(0);

	this->RenderText(iPos_x, iPos_y, pszText, iWidth, iHeight, iSort, &TextSize, true);

	if (lpTextSize)
	{
		lpTextSize->cx = TextSize.cx;
		lpTextSize->cy = TextSize.cy;
	}

	int Size = strlen(pszText);
	iPos_x = TextSize.cx;
	for (BYTE i = 0; i < Size; i++)
	{
		BYTE step = (BYTE)(i + GetTickCount() / 100 % 255);
		BYTE r = (BYTE)(std::sin(0.5f * step + 0) * 127 + 128);
		BYTE g = (BYTE)(std::sin(0.5f * step + 2) * 127 + 128);
		BYTE b = (BYTE)(std::sin(0.5f * step + 4) * 127 + 128);
		DWORD Color = (255 << 24) | (b << 16) | (g << 8) | r;

		g_pRenderText->SetTextColor(Color);
		sprintf(filter, "%%.%ds", Size - i);
		sprintf(bufer, filter, pszText);

		this->RenderText(iPos_x, iPos_y, bufer, iWidth, iHeight, RT3_SORT_LEFT, NULL, i == 0);
	}

	glColor4f(1.f, 1.f, 1.f, 1.f);
	g_pRenderText->SetTextColor(dwTextColor);
}

void CGMFontLayer::runtime_render_map()
{
	//RenderColor(0, 100, 2048, 512.f, 0.9, 1, false);
	//EndRenderColor();
	//RenderBitmap(-((int)BitmapIndex), 0, 100, 2048, 512.f, 0.0, 0.0, 1.f, 512.f /2048.f, false, false);
}

int CGMFontLayer::getmetrics()
{
	return NormalFont[0].metrics_height;
}

CGMFontLayer* CGMFontLayer::Instance()
{
	static CGMFontLayer s_Instance;
	return &s_Instance;
}
