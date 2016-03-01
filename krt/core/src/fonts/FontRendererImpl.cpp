#include "StdInc.h"
#include <fonts/FontRendererImpl.h>

#include <sstream>

using Microsoft::WRL::Make;

#pragma comment(lib, "dwrite.lib")

namespace krt
{
class FrGlyphRunRenderable : public FrRenderable
{
public:
	FrGlyphRunRenderable(ResultingGlyphRun* run)
		: m_run(run)
	{

	}

	virtual void Render() override;

private:
	ResultingGlyphRun* m_run;
};

class FrRectangleRenderable : public FrRenderable
{
public:
	FrRectangleRenderable(const ResultingRectangle& run)
		: m_run(run)
	{

	}

	virtual void Render() override;

private:
	ResultingRectangle m_run;
};

void FontRendererImpl::Initialize(FontRendererGameInterface* gameInterface)
{
	m_gameInterface = gameInterface;

	console::Printf("[FontRenderer] Initializing DirectWrite.\n");

	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)m_dwFactory.GetAddressOf());

	// attempt to get a IDWriteFactory2
	HRESULT hr = m_dwFactory.As(&m_dwFactory2);

	if (FAILED(hr))
	{
		console::PrintWarning("[FontRenderer] IDWriteFactory2 unavailable (hr=%08x), colored font rendering will not be used\n", hr);
	}

	CreateTextRenderer();
}

ResultingGlyphRun::~ResultingGlyphRun()
{
	delete[] subRuns;
}

FrDrawingContext::~FrDrawingContext() {}

std::string FontRendererImpl::GetFontKey(ComPtr<IDWriteFontFace> fontFace, float emSize)
{
	// get the file reference key
	uint32_t reqFiles = 1;
	IDWriteFontFile* file;

	fontFace->GetFiles(&reqFiles, &file);

	const void* referenceKey;
	uint32_t referenceKeySize;

	file->GetReferenceKey(&referenceKey, &referenceKeySize);

	// store in a buffer and append the size
	uint32_t referenceKeySizeTarget = std::min(referenceKeySize, (uint32_t)128);

	char refKeyBuffer[256];
	memcpy(refKeyBuffer, referenceKey, referenceKeySizeTarget);

	*(float*)&refKeyBuffer[referenceKeySizeTarget] = emSize;

	// release the file
	file->Release();

	return std::string(refKeyBuffer, referenceKeySizeTarget + 4);
}

std::shared_ptr<CachedFont> FontRendererImpl::GetFontInstance(ComPtr<IDWriteFontFace> fontFace, float emSize)
{
	auto key = GetFontKey(fontFace, emSize);

	auto it = m_fontCache.find(key);

	if (it != m_fontCache.end())
	{
		return it->second;
	}

	auto cachedFont = std::make_shared<CachedFont>(fontFace, emSize);
	m_fontCache[key] = cachedFont;

	return cachedFont;
}

void FontRendererImpl::DrawRectangle(const Rect& rect, const RGBA& color)
{
	ResultingRectangle resultRect;
	resultRect.rectangle = rect;
	resultRect.color = color;

	m_mutex.lock();
	m_queuedRenderables.push_back(std::make_unique<FrRectangleRenderable>(resultRect));
	m_mutex.unlock();
}

inline std::string StripColors(const std::string& text)
{
	std::stringstream strippedText;

	for (int i = 0; i < text.length(); i++)
	{
		if (text[i] == '^' && isdigit(text[i + 1]))
		{
			i += 1;
		}
		else
		{
			strippedText << text[i];
		}
	}

	return strippedText.str();
}

void FontRendererImpl::DrawText(const std::string& text, const Rect& rect, const RGBA& color, float fontSize, float fontScale, const std::string& fontRef)
{
	// wait for a swap to complete
	FrpSeqAllocatorWaitForSwap();

	m_mutex.lock();

	// create or find a text format
	ComPtr<IDWriteTextFormat> textFormat;

	auto formatKey = std::make_pair(fontRef, fontSize);
	auto formatIter = m_textFormatCache.find(formatKey);

	if (formatIter != m_textFormatCache.end())
	{
		textFormat = formatIter->second;
	}
	else
	{
		m_dwFactory->CreateTextFormat(ToWide(fontRef).c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", textFormat.GetAddressOf());

		m_textFormatCache[formatKey] = textFormat;
	}

	// create or find a cached text layout
	ComPtr<IDWriteTextLayout> textLayout;

	auto layoutKey = std::make_pair(textFormat.Get(), std::make_pair(color.AsARGB(), text));
	auto layoutIter = m_textLayoutCache.find(layoutKey);

	if (layoutIter != m_textLayoutCache.end())
	{
		textLayout = layoutIter->second;
	}
	else
	{
		std::wstring wideText = ToWide(text);
		
		// parse colors and the lot
		std::wstring noColorTextString;

		std::vector<DWRITE_TEXT_RANGE> textRanges;
		std::vector<RGBA> textColors;

		{
			std::wstringstream noColorText;
			int count = 0;

			static const RGBA colors[] = {
				RGBA(0, 0, 0),
				RGBA(255, 0, 0),
				RGBA(0, 255, 0),
				RGBA(255, 255, 0),
				RGBA(0, 0, 255),
				RGBA(0, 255, 255),
				RGBA(255, 0, 255),
				RGBA(255, 255, 255),
				RGBA(100, 0, 0),
				RGBA(0, 0, 100)
			};

			textRanges.reserve(50);
			textColors.reserve(50);

			textRanges.push_back({ 0, UINT32_MAX });
			textColors.push_back(color);

			for (int i = 0; i < wideText.length(); i++)
			{
				if (wideText[i] == '^' && (i + 1) < wideText.length() && isdigit(wideText[i + 1]))
				{
					textRanges.back().length = count - textRanges.back().startPosition;
					textRanges.push_back({ (UINT32)count, UINT32_MAX });

					textColors.push_back(colors[wideText[i + 1] - '0']);

					++i;
					continue;
				}

				noColorText << wideText[i];
				++count;
			}

			textRanges.back().length = count - textRanges.back().startPosition;

			noColorTextString = noColorText.str();
		}

		m_dwFactory->CreateTextLayout(noColorTextString.c_str(), static_cast<UINT32>(noColorTextString.length()), textFormat.Get(), rect.Width(), rect.Height(), textLayout.GetAddressOf());

		m_textLayoutCache[layoutKey] = textLayout;

		// set effect
		for (size_t i : irange(textRanges.size()))
		{
			DWRITE_TEXT_RANGE effectRange = textRanges[i];
			RGBA color = textColors[i];

			static thread_local std::map<uint32_t, ComPtr<FrDrawingEffect>> effects;
			auto it = effects.find(color.AsARGB());

			if (it == effects.end())
			{
				ComPtr<FrDrawingEffect> effect = Make<FrDrawingEffect>();

				effect->SetColor(textColors[i]);

				it = effects.insert({ color.AsARGB(), effect }).first;
			}

			check(SUCCEEDED(textLayout->SetDrawingEffect((IUnknown*)it->second.Get(), effectRange)));
		}
	}

	// draw
	auto drawingContext = new FrDrawingContext();
	textLayout->Draw(drawingContext, m_textRenderer.Get(), rect.Left(), rect.Top());

	auto numRuns = drawingContext->glyphRuns.size();

	if (numRuns)
	{
		for (auto& run : drawingContext->glyphRuns)
		{
			m_queuedRenderables.push_back(std::make_unique<FrGlyphRunRenderable>(run));
			//m_queuedGlyphRuns.push_back(run);
		}
	}

	delete drawingContext;

	m_mutex.unlock();
}

bool FontRendererImpl::GetStringMetrics(const std::string& characterString, float fontSize, float fontScale, const std::string& fontRef, Rect* outRect)
{
	ComPtr<IDWriteTextFormat> textFormat;
	m_dwFactory->CreateTextFormat(ToWide(fontRef).c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", textFormat.GetAddressOf());

	std::string stripped = StripColors(characterString);
	std::wstring wide = ToWide(stripped);

	ComPtr<IDWriteTextLayout> textLayout;
	m_dwFactory->CreateTextLayout(wide.c_str(), static_cast<UINT32>(wide.length()), textFormat.Get(), 8192.0, 8192.0, textLayout.GetAddressOf());

	DWRITE_TEXT_METRICS textMetrics;
	textLayout->GetMetrics(&textMetrics);

	*outRect = Rect();
	outRect->SetRect(textMetrics.left, textMetrics.top, textMetrics.left + textMetrics.width, textMetrics.top + textMetrics.height);

	return true;
}

void FrRectangleRenderable::Render()
{
	g_fontRenderer.GetGameInterface()->DrawRectangles(1, &m_run);
}

void FrGlyphRunRenderable::Render()
{
	auto glyphRun = m_run;

	for (uint32_t i = 0; i < glyphRun->numSubRuns; i++)
	{
		auto subRun = &glyphRun->subRuns[i];

		g_fontRenderer.GetGameInterface()->SetTexture(subRun->texture);
		g_fontRenderer.GetGameInterface()->DrawIndexedVertices(subRun->numVertices, subRun->numIndices, subRun->vertices, subRun->indices);
		g_fontRenderer.GetGameInterface()->UnsetTexture();
	}

	delete glyphRun;
}

void FontRendererImpl::DrawPerFrame()
{
	m_mutex.lock();

	for (auto& renderable : m_queuedRenderables)
	{
		renderable->Render();
	}

	m_queuedRenderables.clear();

	FrpSeqAllocatorSwapPage();

	m_gameInterface->InvokeOnRender([] (void* arg)
	{
		FrpSeqAllocatorUnlockSwap();
	}, nullptr);

	// clean the layout cache if needed
	// TODO: keep layouts for longer if they actually get recently used
	if ((GetTickCount() - m_lastLayoutClean) > 5000)
	{
		m_textLayoutCache.clear();

		m_lastLayoutClean = GetTickCount();
	}

	m_mutex.unlock();
}

FontRendererImpl g_fontRenderer;
FontRenderer* TheFonts = &g_fontRenderer;
}