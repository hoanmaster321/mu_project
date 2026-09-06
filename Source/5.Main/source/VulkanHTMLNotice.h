#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <windows.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H

#define BITMAP_HTML_NOTICE_ATLAS 60005

namespace SEASON3B
{
	enum EHTMLNoticeAlign
	{
		ALIGN_LEFT = 0,
		ALIGN_CENTER,
		ALIGN_RIGHT,
		ALIGN_JUSTIFY
	};

	struct HTMLTextSpan
	{
		std::wstring text;
		DWORD color;
		bool bold;
		int fontSize;
	};

	struct HTMLNoticeLine
	{
		std::vector<HTMLTextSpan> spans;
		EHTMLNoticeAlign align;
		float lineWidth;
		float lineHeight;
	};

	struct HTMLNoticeItem
	{
		std::vector<HTMLNoticeLine> lines;
		float currentY;
		float targetY;
		float alpha;
		float lifeTime;
		float totalDuration;
		bool isMoving;
		float currentX;
		float moveSpeed;
	};

	struct NoticeGlyph
	{
		float u0, v0, u1, v1;
		int width, height;
		int bearingX, bearingY;
		int advanceX;
	};

	class VulkanHTMLNotice
	{
	public:
		static VulkanHTMLNotice& Instance();

		bool Init();
		void Release();

		void Update();
		void Render();

		void AddNoticeText(const char* text, DWORD style);
		void AddNoticeHTML(const char* htmlText);
		void AddEventMapText(const char* text);
		void AddLongMovementText(const char* text, int size, int yPos, int duration);

		// Item Tooltip rendering via FreeType Vector Font
		bool RenderTooltipText(
			float iPos_x, float iPos_y,
			const char* pszText,
			DWORD dwTextColor,
			DWORD dwBgColor,
			bool bBold,
			float fBoxWidth,
			int iSort,
			OUT SIZE* lpTextSize,
			int iFontSize = 0);

		SIZE MeasureTooltipText(const char* pszText, bool bBold, int iFontSize = 0);

		void Clear();

	private:
		VulkanHTMLNotice();
		~VulkanHTMLNotice();

		bool InitFreeType();
		NoticeGlyph* GetGlyph(wchar_t ch, bool bold, int fontSize);
		void UploadAtlasToVulkan();

		void ParseHTML(const char* htmlText, DWORD defaultColor, int defaultSize, bool defaultBold, std::vector<HTMLNoticeLine>& outLines);
		float MeasureLine(const HTMLNoticeLine& line);

	private:
		bool m_initialized;
		FT_Library m_ftLibrary;
		FT_Face m_ftFaceRegular;
		FT_Face m_ftFaceBold;

		// Atlas texture
		static const int ATLAS_SIZE = 2048;
		std::vector<uint8_t> m_atlasBuffer; // 2048x2048x4 RGBA
		int m_atlasPenX;
		int m_atlasPenY;
		int m_atlasRowHeight;
		bool m_atlasDirty;
		ULONGLONG m_lastAtlasResetTime; // Rate-limit atlas resets to avoid thrashing

		// Glyph cache: key = (fontSize << 33) | (bold ? (1ULL << 32) : 0) | (uint32_t)ch
		std::unordered_map<uint64_t, NoticeGlyph> m_glyphCache;

		// Active notices
		std::vector<HTMLNoticeItem> m_activeNotices;
		std::vector<HTMLNoticeItem> m_movingNotices;

		ULONGLONG m_lastUpdateTime;
	};
}

#define g_VulkanHTMLNotice (SEASON3B::VulkanHTMLNotice::Instance())
