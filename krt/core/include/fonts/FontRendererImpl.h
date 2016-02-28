#pragma once

#include <dwrite.h>
#include <dwrite_2.h>

#include <wrl.h>
#include "FontRenderer.h"
#include "FontRendererAbstract.h"

#include <mutex>

using Microsoft::WRL::ComPtr;

namespace std
{
	template<typename T1, typename T2>
	struct hash<std::pair<T1, T2>>
	{
		std::size_t operator()(const std::pair<T1, T2>& x) const
		{
			return (3 * std::hash<T1>()(x.first)) ^ std::hash<T2>()(x.second);
		}
	};
}

namespace krt
{
class CachedFont;

class CachedFontPage
{
private:
	struct FontCharacter
	{
		Rect address;

		Rect affect;

		uint8_t* pixelData;
	};

private:
	CachedFont* m_owningFont;

	uint32_t m_minCodePoint;
	uint32_t m_maxCodePoint;

	bool m_enqueued;

	std::vector<FontCharacter> m_characterAddresses;

	void* m_pixelData;

	FontRendererTexture* m_targetTexture;

private:
	void CreateFontPage();

	void EnqueueCreateFontPage();

	void CreateNow();

	void CreateCharacter(uint32_t codePoint);

public:
	CachedFontPage(CachedFont* parent, uint32_t minCodePoint, uint32_t maxCodePoint);

	bool EnsureFontPageCreated();

	inline FontRendererTexture* GetTexture() { return m_targetTexture; }

	const Rect& GetCharacterAddress(uint32_t codePoint);

	const Rect& GetCharacterSize(uint32_t codePoint);
};

typedef FontRendererVertex ResultingVertex;
typedef uint16_t ResultingIndex;

struct ResultingSubGlyphRun : public FrpUseSequentialAllocator
{
	FontRendererTexture* texture;

	uint32_t numVertices;
	uint32_t numIndices;

	ResultingVertex* vertices;
	ResultingIndex* indices;

	ResultingSubGlyphRun();

	~ResultingSubGlyphRun();
};

struct ResultingGlyphRun : public FrpUseSequentialAllocator
{
	uint32_t numSubRuns;

	ResultingSubGlyphRun* subRuns;

	~ResultingGlyphRun();
};

class CachedFont
{
friend class CachedFontPage;

private:
	std::map<uint32_t, std::shared_ptr<CachedFontPage>> m_pages;

	std::vector<bool> m_presentPages;

	ComPtr<IDWriteFontFace> m_dwFace;

	float m_emSize;

private:
	void CreateFace();

	void TargetGlyphRunInternal(float originX, float originY, const DWRITE_GLYPH_RUN* glyphRun, ResultingSubGlyphRun* initialRuns, ResultingVertex* initialVertices, ResultingIndex* initialIndices, RGBA color, int& runCount);

public:
	CachedFont(ComPtr<IDWriteFontFace> fontFace, float emSize);

	bool EnsureFaceCreated();

	ResultingGlyphRun* TargetGlyphRun(float originX, float originY, const DWRITE_GLYPH_RUN* glyphRun, const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription, RGBA color);
};

struct FrDrawingContext
{
	std::vector<ResultingGlyphRun*> glyphRuns;

	float fontScale;

	~FrDrawingContext();
};

class FrRenderable
{
public:
	virtual ~FrRenderable() = default;

	virtual void Render() = 0;
};

class FontRendererImpl : public FontRenderer
{
private:
	ComPtr<IDWriteFactory> m_dwFactory;

	ComPtr<IDWriteFactory2> m_dwFactory2;

	ComPtr<IDWriteTextRenderer> m_textRenderer;

	FontRendererGameInterface* m_gameInterface;

	std::vector<std::unique_ptr<FrRenderable>> m_queuedRenderables;

	//std::vector<ResultingGlyphRun*> m_queuedGlyphRuns;

	//std::vector<ResultingRectangle> m_queuedRectangles;

	std::unordered_map<std::string, std::shared_ptr<CachedFont>> m_fontCache;

	std::unordered_map<std::pair<std::string, float>, ComPtr<IDWriteTextFormat>> m_textFormatCache;

	std::unordered_map<std::pair<IDWriteTextFormat*, std::pair<uint32_t, std::string>>, ComPtr<IDWriteTextLayout>> m_textLayoutCache;

	uint32_t m_lastLayoutClean;

	std::recursive_mutex m_mutex;

private:
	void CreateTextRenderer();

	std::string GetFontKey(ComPtr<IDWriteFontFace> fontFace, float emSize);

public:
	virtual void Initialize(FontRendererGameInterface* gameInterface);

	virtual void DrawPerFrame();

	std::shared_ptr<CachedFont> GetFontInstance(ComPtr<IDWriteFontFace> fontFace, float emSize);

	inline FontRendererGameInterface* GetGameInterface() { return m_gameInterface; }

	inline ComPtr<IDWriteFactory> GetDWriteFactory() { return m_dwFactory; }

	inline ComPtr<IDWriteFactory2> GetDWriteFactory2() { return m_dwFactory2; }

	virtual void DrawText(const std::string& text, const Rect& rect, const RGBA& color, float fontSize, float fontScale, const std::string& fontRef) override;

	virtual void DrawRectangle(const Rect& rect, const RGBA& color) override;

	virtual bool GetStringMetrics(const std::string& characterString, float fontSize, float fontScale, const std::string& fontRef, Rect* outRect) override;
};

// not entirely COM calling convention, but we'll only use it internally
// {71246052-4EEA-4339-BBC0-D2246A3F5CE3}
DEFINE_GUID(IID_IFrDrawingEffect,
			0x71246052, 0x4eea, 0x4339, 0xbb, 0xc0, 0xd2, 0x24, 0x6a, 0x3f, 0x5c, 0xe3);

interface DECLSPEC_UUID("71246052-4EEA-4339-BBC0-D2246A3F5CE3") IFrDrawingEffect;

struct IFrDrawingEffect : public IUnknown
{
	virtual RGBA GetColor() = 0;

	virtual void SetColor(RGBA color) = 0;
};

class FrDrawingEffect : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IFrDrawingEffect>
{
private:
	RGBA m_color;

public:
	virtual RGBA GetColor() { return m_color; }

	virtual void SetColor(RGBA color) { m_color = color; }
};

FontRendererGameInterface* CreateGameInterface();

extern FontRendererImpl g_fontRenderer;
}