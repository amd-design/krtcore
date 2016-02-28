#pragma once

#ifdef DrawText
#undef DrawText
#endif

#include <fonts/Rect.h>
#include <fonts/RGBA.h>
#include <fonts/FontRendererAbstract.h>

namespace krt
{
class FontRenderer
{
public:
	virtual void Initialize(FontRendererGameInterface* gameInterface) = 0;

	virtual void DrawText(const std::string& text, const Rect& rect, const RGBA& color, float fontSize, float fontScale, const std::string& fontRef) = 0;

	virtual void DrawRectangle(const Rect& rect, const RGBA& color) = 0;

	virtual void DrawPerFrame() = 0;

	virtual bool GetStringMetrics(const std::string& characterString, float fontSize, float fontScale, const std::string& fontRef, Rect* outRect) = 0;
};

class GameWindow;

FontRendererGameInterface* CreateGameInterface(GameWindow* gameWindow);

extern FontRenderer* TheFonts;
}