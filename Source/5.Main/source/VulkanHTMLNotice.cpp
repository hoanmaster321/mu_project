#include "stdafx.h"
#include "VulkanHTMLNotice.h"
#include "NewUICommon.h"
#include "BatchRenderer.h"
#include "VulkanTextureManager.h"
#include "GPUContext.h"
#include "MultiLanguage.h"
#include "ZzzOpenglUtil.h"

#include <algorithm>
#include <sstream>

extern int FontHeight;

namespace SEASON3B
{
	VulkanHTMLNotice& VulkanHTMLNotice::Instance()
	{
		static VulkanHTMLNotice s_instance;
		return s_instance;
	}

	VulkanHTMLNotice::VulkanHTMLNotice()
		: m_initialized(false)
		, m_ftLibrary(nullptr)
		, m_ftFaceRegular(nullptr)
		, m_ftFaceBold(nullptr)
		, m_atlasPenX(2)
		, m_atlasPenY(2)
		, m_atlasRowHeight(0)
		, m_atlasDirty(false)
		, m_lastUpdateTime(0)
	{
	}

	VulkanHTMLNotice::~VulkanHTMLNotice()
	{
		Release();
	}

	void VulkanHTMLNotice::Release()
	{
		if (m_ftFaceBold)
		{
			FT_Done_Face(m_ftFaceBold);
			m_ftFaceBold = nullptr;
		}
		if (m_ftFaceRegular)
		{
			FT_Done_Face(m_ftFaceRegular);
			m_ftFaceRegular = nullptr;
		}
		if (m_ftLibrary)
		{
			FT_Done_FreeType(m_ftLibrary);
			m_ftLibrary = nullptr;
		}
		m_glyphCache.clear();
		m_activeNotices.clear();
		m_movingNotices.clear();
		m_initialized = false;
	}

	bool VulkanHTMLNotice::Init()
	{
		if (m_initialized)
			return true;

		if (!InitFreeType())
			return false;

		// Initialize atlas buffer (1024x1024x4 RGBA8)
		m_atlasBuffer.assign(ATLAS_SIZE * ATLAS_SIZE * 4, 0);
		m_atlasPenX = 2;
		m_atlasPenY = 2;
		m_atlasRowHeight = 0;
		m_atlasDirty = true;
		m_glyphCache.clear();

		m_lastUpdateTime = GetTickCount64();
		m_lastAtlasResetTime = 0;
		m_initialized = true;
		return true;
	}

	bool VulkanHTMLNotice::InitFreeType()
	{
		if (FT_Init_FreeType(&m_ftLibrary) != 0)
		{
			return false;
		}

		// Font search candidates (prioritizing One Sans / AR One Sans for modern, balanced typography)
		const char* regularCandidates[] = {
			"Data\\Interface\\HUD\\fonts\\OneSans.ttf",
			"Data\\Interface\\HUD\\fonts\\AROneSans.ttf",
			"Data\\Interface\\HUD\\fonts\\OpenSans.ttf",
			"Data\\Interface\\HUD\\fonts\\tahoma.ttf",
			"C:\\Windows\\Fonts\\tahoma.ttf",
			"Data\\Interface\\HUD\\fonts\\fonts_all.ttf",
			"C:\\Windows\\Fonts\\arial.ttf",
			"Data\\Interface\\HUD\\fonts\\Roboto.ttf",
			"Roboto.ttf",
			nullptr
		};

		const char* boldCandidates[] = {
			"Data\\Interface\\HUD\\fonts\\OneSans-Bold.ttf",
			"Data\\Interface\\HUD\\fonts\\AROneSans-Bold.ttf",
			"Data\\Interface\\HUD\\fonts\\OpenSans-Bold.ttf",
			"Data\\Interface\\HUD\\fonts\\OneSans.ttf",
			"Data\\Interface\\HUD\\fonts\\AROneSans.ttf",
			"Data\\Interface\\HUD\\fonts\\tahomabd.ttf",
			"C:\\Windows\\Fonts\\tahomabd.ttf",
			"Data\\Interface\\HUD\\fonts\\fonts_all.ttf",
			"C:\\Windows\\Fonts\\arialbd.ttf",
			"Data\\Interface\\HUD\\fonts\\Roboto.ttf",
			"Roboto.ttf",
			nullptr
		};

		std::string regularPath = "";
		for (int i = 0; regularCandidates[i] != nullptr; ++i)
		{
			DWORD attr = GetFileAttributesA(regularCandidates[i]);
			if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
			{
				regularPath = regularCandidates[i];
				break;
			}
		}

		std::string boldPath = "";
		for (int i = 0; boldCandidates[i] != nullptr; ++i)
		{
			DWORD attr = GetFileAttributesA(boldCandidates[i]);
			if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
			{
				boldPath = boldCandidates[i];
				break;
			}
		}

		if (regularPath.empty())
		{
			FT_Done_FreeType(m_ftLibrary);
			m_ftLibrary = nullptr;
			return false;
		}

		if (FT_New_Face(m_ftLibrary, regularPath.c_str(), 0, &m_ftFaceRegular) != 0)
		{
			FT_Done_FreeType(m_ftLibrary);
			m_ftLibrary = nullptr;
			return false;
		}

		FT_Select_Charmap(m_ftFaceRegular, FT_ENCODING_UNICODE);

		if (!boldPath.empty() && boldPath != regularPath)
		{
			if (FT_New_Face(m_ftLibrary, boldPath.c_str(), 0, &m_ftFaceBold) == 0)
			{
				FT_Select_Charmap(m_ftFaceBold, FT_ENCODING_UNICODE);
			}
		}

		return true;
	}

	NoticeGlyph* VulkanHTMLNotice::GetGlyph(wchar_t ch, bool bold, int fontSize)
	{
		if (!m_initialized || !m_ftLibrary)
			return nullptr;

		if (fontSize <= 0)
			fontSize = 16;

		// Key: (fontSize << 33) | (bold ? (1ULL << 32) : 0) | (uint32_t)ch
		uint64_t key = (static_cast<uint64_t>(fontSize) << 33) |
			(bold ? (1ULL << 32) : 0ULL) |
			static_cast<uint64_t>(static_cast<uint32_t>(ch));

		auto it = m_glyphCache.find(key);
		if (it != m_glyphCache.end())
		{
			return &it->second;
		}

		FT_Face face = (bold && m_ftFaceBold) ? m_ftFaceBold : m_ftFaceRegular;
		if (!face)
			face = m_ftFaceRegular;

		FT_Set_Pixel_Sizes(face, 0, fontSize);

		FT_UInt glyphIndex = FT_Get_Char_Index(face, ch);
		if (glyphIndex == 0 && ch != L' ')
		{
			// Fallback to regular face if bold failed to find character
			if (face != m_ftFaceRegular && m_ftFaceRegular)
			{
				face = m_ftFaceRegular;
				FT_Set_Pixel_Sizes(face, 0, fontSize);
				glyphIndex = FT_Get_Char_Index(face, ch);
			}
		}

		if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_TARGET_LIGHT) != 0)
		{
			return nullptr;
		}

		if (bold && (!m_ftFaceBold || face == m_ftFaceRegular))
		{
			FT_GlyphSlot_Embolden(face->glyph);
		}

		if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
		{
			return nullptr;
		}

		FT_Bitmap& bmp = face->glyph->bitmap;
		int bmpW = bmp.width;
		int bmpH = bmp.rows;

		// Handle space character
		if (ch == L' ' && bmpW == 0)
		{
			bmpW = 1;
			bmpH = 1;
		}

		// Check atlas space
		if (m_atlasPenX + bmpW + 2 >= ATLAS_SIZE)
		{
			m_atlasPenX = 2;
			m_atlasPenY += m_atlasRowHeight + 2;
			m_atlasRowHeight = 0;
		}

		if (m_atlasPenY + bmpH + 2 >= ATLAS_SIZE)
		{
			// Rate-limit atlas resets to prevent thrashing (max once per 5 seconds)
			ULONGLONG now = GetTickCount64();
			if (m_lastAtlasResetTime != 0 && (now - m_lastAtlasResetTime) < 5000)
			{
				// Atlas is thrashing — skip this glyph to avoid CPU spike
				return nullptr;
			}
			m_lastAtlasResetTime = now;

			// Atlas full: reset dynamic cache
			m_glyphCache.clear();
			m_atlasPenX = 2;
			m_atlasPenY = 2;
			m_atlasRowHeight = 0;
			std::fill(m_atlasBuffer.begin(), m_atlasBuffer.end(), 0);
		}

		int dstX = m_atlasPenX;
		int dstY = m_atlasPenY;

		// Copy into RGBA buffer (White RGB, Alpha from glyph coverage)
		if (ch != L' ' && bmp.buffer)
		{
			for (int row = 0; row < bmpH; ++row)
			{
				for (int col = 0; col < bmpW; ++col)
				{
					uint8_t alphaVal = bmp.buffer[row * bmp.pitch + col];
					int dstIdx = ((dstY + row) * ATLAS_SIZE + (dstX + col)) * 4;
					m_atlasBuffer[dstIdx + 0] = 255;
					m_atlasBuffer[dstIdx + 1] = 255;
					m_atlasBuffer[dstIdx + 2] = 255;
					m_atlasBuffer[dstIdx + 3] = alphaVal;
				}
			}
		}

		NoticeGlyph glyph{};
		glyph.u0 = static_cast<float>(dstX) / static_cast<float>(ATLAS_SIZE);
		glyph.v0 = static_cast<float>(dstY) / static_cast<float>(ATLAS_SIZE);
		glyph.u1 = static_cast<float>(dstX + bmpW) / static_cast<float>(ATLAS_SIZE);
		glyph.v1 = static_cast<float>(dstY + bmpH) / static_cast<float>(ATLAS_SIZE);
		glyph.width = bmpW;
		glyph.height = bmpH;
		glyph.bearingX = face->glyph->bitmap_left;
		glyph.bearingY = face->glyph->bitmap_top;
		glyph.advanceX = face->glyph->advance.x >> 6;

		m_atlasPenX += bmpW + 2;
		if (bmpH > m_atlasRowHeight)
		{
			m_atlasRowHeight = bmpH;
		}
		m_atlasDirty = true;

		auto inserted = m_glyphCache.emplace(key, glyph);
		return &inserted.first->second;
	}

	void VulkanHTMLNotice::UploadAtlasToVulkan()
	{
		if (!m_atlasDirty || m_atlasBuffer.empty())
			return;

		if (GPUContext::Instance().IsInitialized())
		{
			VulkanTextureManager::Instance().CreateTextureWithId(
				BITMAP_HTML_NOTICE_ATLAS,
				ATLAS_SIZE,
				ATLAS_SIZE,
				4,
				m_atlasBuffer.data(),
				true,
				true
			);
			m_atlasDirty = false;
		}
	}

	static DWORD ParseHexColor(const std::string& hex)
	{
		std::string clean = hex;
		if (!clean.empty() && clean[0] == '#')
			clean = clean.substr(1);

		if (clean.size() < 6)
			return RGBA(255, 255, 255, 255);

		unsigned int r = 255, g = 255, b = 255;
		std::stringstream ss;
		ss << std::hex << clean.substr(0, 2);
		ss >> r;
		ss.clear();
		ss << std::hex << clean.substr(2, 2);
		ss >> g;
		ss.clear();
		ss << std::hex << clean.substr(4, 2);
		ss >> b;

		return RGBA(r, g, b, 255);
	}

	static std::wstring ToWideString(const std::string& str)
	{
		if (str.empty())
			return L"";

		// Try UTF-8 first
		int numWide = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
		if (numWide > 1)
		{
			std::vector<wchar_t> buf(numWide);
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf.data(), numWide);
			return std::wstring(buf.data());
		}

		// Fallback to ANSI / CP1258 / CP1252
		numWide = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
		if (numWide > 1)
		{
			std::vector<wchar_t> buf(numWide);
			MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buf.data(), numWide);
			return std::wstring(buf.data());
		}

		return L"";
	}

	void VulkanHTMLNotice::ParseHTML(const char* htmlText, DWORD defaultColor, int defaultSize, bool defaultBold, std::vector<HTMLNoticeLine>& outLines)
	{
		outLines.clear();
		if (!htmlText || htmlText[0] == '\0')
			return;

		std::string raw = htmlText;
		HTMLNoticeLine currentLine;
		currentLine.align = ALIGN_CENTER;
		currentLine.lineWidth = 0.0f;
		currentLine.lineHeight = 0.0f;

		DWORD curColor = defaultColor;
		int curSize = (defaultSize > 0) ? defaultSize : 12;
		bool curBold = defaultBold;
		EHTMLNoticeAlign curAlign = ALIGN_CENTER;

		size_t i = 0;
		std::string textAccum = "";

		auto flushSpan = [&]() {
			if (!textAccum.empty())
			{
				HTMLTextSpan span;
				span.text = ToWideString(textAccum);
				span.color = curColor;
				span.bold = curBold;
				span.fontSize = curSize;
				currentLine.spans.push_back(span);
				textAccum.clear();
			}
		};

		auto flushLine = [&]() {
			flushSpan();
			if (!currentLine.spans.empty())
			{
				currentLine.align = curAlign;
				currentLine.lineWidth = MeasureLine(currentLine);
				currentLine.lineHeight = static_cast<float>(curSize + 4);
				outLines.push_back(currentLine);
				currentLine.spans.clear();
			}
		};

		while (i < raw.size())
		{
			if (raw[i] == '<')
			{
				flushSpan();
				size_t closePos = raw.find('>', i);
				if (closePos != std::string::npos)
				{
					std::string tag = raw.substr(i + 1, closePos - i - 1);
					std::string tagLower = tag;
					std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);

					if (tagLower == "b")
					{
						curBold = true;
					}
					else if (tagLower == "/b")
					{
						curBold = defaultBold;
					}
					else if (tagLower == "br" || tagLower == "br/")
					{
						flushLine();
					}
					else if (tagLower.rfind("p ", 0) == 0 || tagLower == "p")
					{
						// Parse align attribute
						size_t alignPos = tagLower.find("align=");
						if (alignPos != std::string::npos)
						{
							size_t valStart = tagLower.find_first_of("\"'", alignPos);
							if (valStart != std::string::npos)
							{
								size_t valEnd = tagLower.find_first_of("\"'", valStart + 1);
								if (valEnd != std::string::npos)
								{
									std::string alignVal = tagLower.substr(valStart + 1, valEnd - valStart - 1);
									if (alignVal == "left") curAlign = ALIGN_LEFT;
									else if (alignVal == "right") curAlign = ALIGN_RIGHT;
									else if (alignVal == "center") curAlign = ALIGN_CENTER;
									else if (alignVal == "justify") curAlign = ALIGN_CENTER;
								}
							}
						}
					}
					else if (tagLower == "/p")
					{
						flushLine();
					}
					else if (tagLower.rfind("font", 0) == 0)
					{
						// Parse color attribute
						size_t colorPos = tagLower.find("color=");
						if (colorPos != std::string::npos)
						{
							size_t valStart = tag.find_first_of("\"'", colorPos);
							if (valStart != std::string::npos)
							{
								size_t valEnd = tag.find_first_of("\"'", valStart + 1);
								if (valEnd != std::string::npos)
								{
									std::string colVal = tag.substr(valStart + 1, valEnd - valStart - 1);
									curColor = ParseHexColor(colVal);
								}
							}
						}
						// Parse size attribute
						size_t sizePos = tagLower.find("size=");
						if (sizePos != std::string::npos)
						{
							size_t valStart = tag.find_first_of("\"'", sizePos);
							if (valStart != std::string::npos)
							{
								size_t valEnd = tag.find_first_of("\"'", valStart + 1);
								if (valEnd != std::string::npos)
								{
									std::string sizeVal = tag.substr(valStart + 1, valEnd - valStart - 1);
									int s = atoi(sizeVal.c_str());
									if (s > 0) curSize = s;
								}
							}
						}
					}
					else if (tagLower == "/font")
					{
						curColor = defaultColor;
						curSize = (defaultSize > 0) ? defaultSize : 12;
					}

					i = closePos + 1;
					continue;
				}
			}
			else if (raw[i] == '&')
			{
				// Decode HTML entities
				if (raw.compare(i, 4, "&lt;") == 0)
				{
					textAccum += '<';
					i += 4;
					continue;
				}
				else if (raw.compare(i, 4, "&gt;") == 0)
				{
					textAccum += '>';
					i += 4;
					continue;
				}
				else if (raw.compare(i, 5, "&amp;") == 0)
				{
					textAccum += '&';
					i += 5;
					continue;
				}
				else if (raw.compare(i, 6, "&quot;") == 0)
				{
					textAccum += '"';
					i += 6;
					continue;
				}
			}

			textAccum += raw[i];
			++i;
		}

		flushLine();

		// If no line parsed (e.g. plain text without <p>), ensure we have the line
		if (outLines.empty() && !currentLine.spans.empty())
		{
			currentLine.align = curAlign;
			currentLine.lineWidth = MeasureLine(currentLine);
			currentLine.lineHeight = static_cast<float>(curSize + 4);
			outLines.push_back(currentLine);
		}
	}

	float VulkanHTMLNotice::MeasureLine(const HTMLNoticeLine& line)
	{
		float totalW = 0.0f;
		for (const auto& span : line.spans)
		{
			for (wchar_t ch : span.text)
			{
				NoticeGlyph* g = GetGlyph(ch, span.bold, span.fontSize);
				if (g)
				{
					totalW += static_cast<float>(g->advanceX);
				}
				else
				{
					totalW += static_cast<float>(span.fontSize) * 0.5f;
				}
			}
		}
		return totalW;
	}

	void VulkanHTMLNotice::AddNoticeText(const char* text, DWORD style)
	{
		if (!text || text[0] == '\0')
			return;

		Init();

		DWORD hexColor = RGBA(0xff, 0xff, 0xff, 0xff);
		switch (style)
		{
		case 0:
			// Bright Gold / Yellow matching original ScaleForm Notice
			hexColor = RGBA(0xff, 0xc8, 0x32, 0xff);
			break;
		case 1:
			// Crisp White
			hexColor = RGBA(0xff, 0xff, 0xff, 0xff);
			break;
		case 2:
			// Emerald Green
			hexColor = RGBA(0x32, 0xe2, 0x86, 0xff);
			break;
		default:
			hexColor = RGBA(0xff, 0xc8, 0x32, 0xff);
			break;
		}

		int noticeFontSize = (FontHeight > 0) ? (FontHeight - 1) : 12;
		if (noticeFontSize < 10) noticeFontSize = 10;
		if (noticeFontSize > 24) noticeFontSize = 24;

		const char* html = SEASON3B::FontHTML(text, hexColor, static_cast<BYTE>(noticeFontSize), true, false, 3);
		AddNoticeHTML(html);
	}

	void VulkanHTMLNotice::AddNoticeHTML(const char* htmlText)
	{
		if (!htmlText || htmlText[0] == '\0')
			return;

		Init();

		int defaultFontSize = (FontHeight > 0) ? (FontHeight - 1) : 12;
		if (defaultFontSize < 10) defaultFontSize = 10;
		if (defaultFontSize > 24) defaultFontSize = 24;

		HTMLNoticeItem item{};
		ParseHTML(htmlText, RGBA(255, 200, 50, 255), defaultFontSize, true, item.lines);
		if (item.lines.empty())
			return;

		item.lifeTime = 6.5f;
		item.totalDuration = 6.5f;
		item.alpha = 0.0f;
		item.isMoving = false;

		// Calculate total height
		float totalH = 0.0f;
		for (const auto& l : item.lines)
		{
			totalH += l.lineHeight;
		}

		// Centered vertically in the lower-middle portion of the viewport (matching original PositionY_In_The_Mid(310))
		float screenH = static_cast<float>(WindowHeight);
		float baseTopY = (std::max)(0.0f, (screenH - 480.0f) * 0.5f) + 310.0f;
		float lineSpacing = (totalH > 0.0f) ? (totalH + 3.0f) : 18.0f;

		// Shift existing active notices downwards
		for (size_t i = 0; i < m_activeNotices.size(); ++i)
		{
			m_activeNotices[i].targetY += lineSpacing;
		}

		// Insert newest notice at top slot
		item.targetY = baseTopY;
		item.currentY = baseTopY - 8.0f; // Slide-in effect

		m_activeNotices.insert(m_activeNotices.begin(), item);

		// Limit maximum stacked notices to 5
		while (m_activeNotices.size() > 5)
		{
			m_activeNotices.pop_back();
		}
	}

	SIZE VulkanHTMLNotice::MeasureTooltipText(const char* pszText, bool bBold, int iFontSize)
	{
		SIZE sz{ 0, 0 };
		if (!pszText || pszText[0] == '\0')
			return sz;

		Init();

		int fontSize = (iFontSize > 0) ? iFontSize : ((FontHeight > 0) ? (FontHeight - 1) : 12);
		if (fontSize < 10) fontSize = 10;
		if (fontSize > 24) fontSize = 24;

		std::string textStr = pszText;
		std::vector<HTMLNoticeLine> lines;

		if (textStr.find('<') != std::string::npos && textStr.find('>') != std::string::npos)
		{
			ParseHTML(textStr.c_str(), 0xFFFFFFFF, fontSize, bBold, lines);
		}
		else
		{
			HTMLNoticeLine singleLine;
			HTMLTextSpan span;
			span.text = ToWideString(textStr);
			span.bold = bBold;
			span.fontSize = fontSize;
			singleLine.spans.push_back(span);
			singleLine.lineWidth = MeasureLine(singleLine);
			lines.push_back(singleLine);
		}

		if (!lines.empty())
		{
			sz.cx = static_cast<long>(lines[0].lineWidth);
			sz.cy = static_cast<long>(fontSize + 2);
		}

		return sz;
	}

	bool VulkanHTMLNotice::RenderTooltipText(
		float iPos_x, float iPos_y,
		const char* pszText,
		DWORD dwTextColor,
		DWORD dwBgColor,
		bool bBold,
		float fBoxWidth,
		int iSort,
		OUT SIZE* lpTextSize,
		int iFontSize)
	{
		if (!pszText || pszText[0] == '\0')
			return false;

		Init();

		int fontSize = (iFontSize > 0) ? iFontSize : ((FontHeight > 0) ? (FontHeight - 1) : 12);
		if (fontSize < 10) fontSize = 10;
		if (fontSize > 24) fontSize = 24;

		std::string textStr = pszText;
		std::vector<HTMLNoticeLine> lines;

		if (textStr.find('<') != std::string::npos && textStr.find('>') != std::string::npos)
		{
			ParseHTML(textStr.c_str(), dwTextColor, fontSize, bBold, lines);
		}
		else
		{
			HTMLNoticeLine singleLine;
			singleLine.align = ALIGN_LEFT;
			HTMLTextSpan span;
			span.text = ToWideString(textStr);
			span.color = dwTextColor;
			span.bold = bBold;
			span.fontSize = fontSize;
			singleLine.spans.push_back(span);
			singleLine.lineWidth = MeasureLine(singleLine);
			singleLine.lineHeight = static_cast<float>(fontSize + 2);
			lines.push_back(singleLine);
		}

		if (lines.empty())
			return false;

		float lineW = lines[0].lineWidth;
		float lineH = static_cast<float>(fontSize + 2);

		if (lpTextSize)
		{
			lpTextSize->cx = static_cast<long>(lineW);
			lpTextSize->cy = static_cast<long>(lineH);
		}

		// Render background quad if specified
		if (dwBgColor != 0 && fBoxWidth > 0.0f)
		{
			float bgR = static_cast<float>(GetRed(dwBgColor)) / 255.0f;
			float bgG = static_cast<float>(GetGreen(dwBgColor)) / 255.0f;
			float bgB = static_cast<float>(GetBlue(dwBgColor)) / 255.0f;
			float bgA = static_cast<float>(GetAlpha(dwBgColor)) / 255.0f;
			if (bgA <= 0.001f) bgA = 1.0f;

			glColor4f(bgR, bgG, bgB, bgA);
			RenderColor(iPos_x - 2.0f, iPos_y, fBoxWidth + 4.0f, lineH, 0.0f, 0, false);
			EndRenderColor();
		}

		// Calculate start X according to sort (snapped to whole integer pixels)
		float snapX = std::floor(iPos_x + 0.5f);
		float snapY = std::floor(iPos_y + 0.5f);
		float startX = snapX;
		if (iSort == RT3_SORT_CENTER)
		{
			if (fBoxWidth > lineW)
				startX = std::floor(snapX + (fBoxWidth - lineW) * 0.5f + 0.5f);
		}
		else if (iSort == RT3_SORT_RIGHT)
		{
			if (fBoxWidth > lineW)
				startX = std::floor(snapX + fBoxWidth - lineW + 0.5f);
		}
		else if (iSort == RT3_WRITE_CENTER)
		{
			startX = std::floor(snapX - lineW * 0.5f + 0.5f);
		}

		// Ensure any new glyphs are uploaded to the Vulkan atlas
		UploadAtlasToVulkan();

		// Render crisp, pure foreground vector text (no muddy shadow on dark tooltip background)
		for (const auto& line : lines)
		{
			float penX = startX;
			for (const auto& span : line.spans)
			{
				float r = static_cast<float>(GetRed(span.color)) / 255.0f;
				float gCol = static_cast<float>(GetGreen(span.color)) / 255.0f;
				float b = static_cast<float>(GetBlue(span.color)) / 255.0f;
				float a = (static_cast<float>(GetAlpha(span.color)) / 255.0f);
				if (a <= 0.001f) a = 1.0f;

				for (wchar_t ch : span.text)
				{
					NoticeGlyph* g = GetGlyph(ch, span.bold, span.fontSize);
					if (!g) continue;

					if (ch != L' ' && g->width > 0 && g->height > 0)
					{
						float gx = std::floor(penX + g->bearingX + 0.5f);
						float gy = std::floor(snapY + (span.fontSize - g->bearingY) + 0.5f);

						ImageInstance_t tImg{};
						tImg.Texture = BITMAP_HTML_NOTICE_ATLAS;
						tImg.x = gx;
						tImg.y = gy;
						tImg.width = static_cast<float>(g->width);
						tImg.height = static_cast<float>(g->height);
						tImg.u = g->u0;
						tImg.v = g->v0;
						tImg.uWidth = g->u1 - g->u0;
						tImg.vHeight = g->v1 - g->v0;
						tImg.color[0] = r;
						tImg.color[1] = gCol;
						tImg.color[2] = b;
						tImg.color[3] = a;
						tImg.rotation = 0.0f;
						tImg.layer = 0;
						tImg.RenderFlags = RENDER_ALPHA_BLEND_TYPE_NORMAL;
						tImg.grayscale = false;
						g_BatchRenderer.AddImage(tImg);
					}
					penX += g->advanceX;
				}
			}
		}

		return true;
	}

	void VulkanHTMLNotice::AddEventMapText(char const* text)
	{
		if (!text || text[0] == '\0')
			return;

		Init();
		const char* html = SEASON3B::FontHTML(text, RGBA(0xff, 0xcc, 0x19u, 0xffu), 0, false, true);
		AddNoticeHTML(html);
	}

	void VulkanHTMLNotice::AddLongMovementText(const char* text, int size, int yPos, int duration)
	{
		if (!text || text[0] == '\0')
			return;

		Init();
		const char* html = SEASON3B::FontHTML(text, RGBA(0xff, 0xcc, 0x19u, 0xffu), static_cast<BYTE>(size), false, true);

		HTMLNoticeItem item{};
		ParseHTML(html, RGBA(255, 204, 25, 255), (size > 0 ? size : 20), true, item.lines);
		if (item.lines.empty())
			return;

		float durationSec = (duration > 0) ? (duration / 1000.0f) : 2.5f;
		item.lifeTime = durationSec;
		item.totalDuration = durationSec;
		item.alpha = 0.0f;
		item.isMoving = true;
		item.targetY = (yPos > 0) ? (yPos * g_fScreenRate_y) : (150.0f * g_fScreenRate_y);
		item.currentY = item.targetY;
		item.currentX = (static_cast<float>(WindowWidth) - item.lines[0].lineWidth) * 0.5f;

		m_movingNotices.push_back(item);
	}

	void VulkanHTMLNotice::Clear()
	{
		m_activeNotices.clear();
		m_movingNotices.clear();
	}

	void VulkanHTMLNotice::Update()
	{
		ULONGLONG curTime = GetTickCount64();
		if (m_lastUpdateTime == 0)
		{
			m_lastUpdateTime = curTime;
			return;
		}

		float dt = static_cast<float>(curTime - m_lastUpdateTime) / 1000.0f;
		m_lastUpdateTime = curTime;

		if (dt <= 0.0f || dt > 0.5f)
			dt = 0.016f;

		// 1. Update active notices
		for (auto it = m_activeNotices.begin(); it != m_activeNotices.end(); )
		{
			it->lifeTime -= dt;

			// Fade in / Fade out
			if (it->lifeTime > 0.5f)
			{
				it->alpha = (std::min)(1.0f, it->alpha + dt * 4.0f);
			}
			else if (it->lifeTime > 0.0f)
			{
				it->alpha = (std::max)(0.0f, it->lifeTime / 0.5f);
			}
			else
			{
				it->alpha = 0.0f;
			}

			// Smooth position slide
			it->currentY += (it->targetY - it->currentY) * (std::min)(1.0f, dt * 10.0f);

			if (it->lifeTime <= 0.0f)
			{
				it = m_activeNotices.erase(it);
			}
			else
			{
				++it;
			}
		}

		// 2. Update moving notices
		for (auto it = m_movingNotices.begin(); it != m_movingNotices.end(); )
		{
			it->lifeTime -= dt;

			if (it->lifeTime > 0.5f)
			{
				it->alpha = (std::min)(1.0f, it->alpha + dt * 4.0f);
			}
			else if (it->lifeTime > 0.0f)
			{
				it->alpha = (std::max)(0.0f, it->lifeTime / 0.5f);
			}
			else
			{
				it->alpha = 0.0f;
			}

			if (it->lifeTime <= 0.0f)
			{
				it = m_movingNotices.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void VulkanHTMLNotice::Render()
	{
		if (m_activeNotices.empty() && m_movingNotices.empty())
			return;

		if (!GPUContext::Instance().IsFrameActive())
			return;

		UploadAtlasToVulkan();

		auto renderLine = [this](const HTMLNoticeLine& line, float startX, float lineY, float alpha) {
			startX = std::floor(startX + 0.5f);
			lineY = std::floor(lineY + 0.5f);

			float penX = startX;

			// Pass 1: Crisp single 1-pixel drop shadow (offset +1, +1)
			float shadowAlpha = alpha * 0.85f;
			const float shadowOffsets[1][2] = {
				{ 1.0f, 1.0f }
			};

			for (int pass = 0; pass < 1; ++pass)
			{
				float curPenX = penX;
				float offX = shadowOffsets[pass][0];
				float offY = shadowOffsets[pass][1];

				for (const auto& span : line.spans)
				{
					for (wchar_t ch : span.text)
					{
						NoticeGlyph* g = GetGlyph(ch, span.bold, span.fontSize);
						if (!g) continue;

						if (ch != L' ' && g->width > 0 && g->height > 0)
						{
							float gx = std::floor(curPenX + g->bearingX + offX + 0.5f);
							float gy = std::floor(lineY + (span.fontSize - g->bearingY) + offY + 0.5f);

							ImageInstance_t sImg{};
							sImg.Texture = BITMAP_HTML_NOTICE_ATLAS;
							sImg.x = gx;
							sImg.y = gy;
							sImg.width = static_cast<float>(g->width);
							sImg.height = static_cast<float>(g->height);
							sImg.u = g->u0;
							sImg.v = g->v0;
							sImg.uWidth = g->u1 - g->u0;
							sImg.vHeight = g->v1 - g->v0;
							sImg.color[0] = 0.0f;
							sImg.color[1] = 0.0f;
							sImg.color[2] = 0.0f;
							sImg.color[3] = shadowAlpha;
							sImg.rotation = 0.0f;
							sImg.layer = 0;
							sImg.RenderFlags = RENDER_ALPHA_BLEND_TYPE_NORMAL;
							sImg.grayscale = false;

							g_BatchRenderer.AddImage(sImg);
						}
						curPenX += g->advanceX;
					}
				}
			}

			// Pass 2: Main Foreground Vector Text
			float curPenX = penX;
			for (const auto& span : line.spans)
			{
				float r = static_cast<float>(GetRed(span.color)) / 255.0f;
				float gCol = static_cast<float>(GetGreen(span.color)) / 255.0f;
				float b = static_cast<float>(GetBlue(span.color)) / 255.0f;
				float textA = alpha * (static_cast<float>(GetAlpha(span.color)) / 255.0f);

				for (wchar_t ch : span.text)
				{
					NoticeGlyph* g = GetGlyph(ch, span.bold, span.fontSize);
					if (!g) continue;

					if (ch != L' ' && g->width > 0 && g->height > 0)
					{
						float gx = std::floor(curPenX + g->bearingX + 0.5f);
						float gy = std::floor(lineY + (span.fontSize - g->bearingY) + 0.5f);

						ImageInstance_t tImg{};
						tImg.Texture = BITMAP_HTML_NOTICE_ATLAS;
						tImg.x = gx;
						tImg.y = gy;
						tImg.width = static_cast<float>(g->width);
						tImg.height = static_cast<float>(g->height);
						tImg.u = g->u0;
						tImg.v = g->v0;
						tImg.uWidth = g->u1 - g->u0;
						tImg.vHeight = g->v1 - g->v0;
						tImg.color[0] = r;
						tImg.color[1] = gCol;
						tImg.color[2] = b;
						tImg.color[3] = textA;
						tImg.rotation = 0.0f;
						tImg.layer = 0;
						tImg.RenderFlags = RENDER_ALPHA_BLEND_TYPE_NORMAL;
						tImg.grayscale = false;

						g_BatchRenderer.AddImage(tImg);
					}
					curPenX += g->advanceX;
				}
			}
		};

		// 1. Render active stacked notices
		for (const auto& item : m_activeNotices)
		{
			if (item.alpha <= 0.001f)
				continue;

			float curY = std::floor(item.currentY + 0.5f);
			for (const auto& line : item.lines)
			{
				float lineX = 0.0f;
				if (line.align == ALIGN_CENTER)
				{
					lineX = std::floor((static_cast<float>(WindowWidth) - line.lineWidth) * 0.5f + 0.5f);
				}
				else if (line.align == ALIGN_RIGHT)
				{
					lineX = std::floor((static_cast<float>(WindowWidth) + 640.0f * g_fScreenRate_x) * 0.5f - 50.0f * g_fScreenRate_x - line.lineWidth + 0.5f);
				}
				else
				{
					lineX = std::floor((static_cast<float>(WindowWidth) - 640.0f * g_fScreenRate_x) * 0.5f + 50.0f * g_fScreenRate_x + 0.5f);
				}

				renderLine(line, lineX, curY, item.alpha);
				curY += std::floor(line.lineHeight + 0.5f);
			}
		}

		// 2. Render moving / event notices
		for (const auto& item : m_movingNotices)
		{
			if (item.alpha <= 0.001f)
				continue;

			float curY = std::floor(item.currentY + 0.5f);
			for (const auto& line : item.lines)
			{
				float lineX = item.currentX;
				if (lineX <= 0.0f)
				{
					lineX = std::floor((static_cast<float>(WindowWidth) - line.lineWidth) * 0.5f + 0.5f);
				}
				else
				{
					lineX = std::floor(lineX + 0.5f);
				}

				renderLine(line, lineX, curY, item.alpha);
				curY += std::floor(line.lineHeight + 0.5f);
			}
		}
	}
}
