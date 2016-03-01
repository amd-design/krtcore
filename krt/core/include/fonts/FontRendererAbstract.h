#pragma once

#include "FontRendererAllocator.h"

#include <fonts/Rect.h>
#include <fonts/RGBA.h>

namespace krt
{
enum class FontRendererTextureFormat
{
	DXT5,
	ARGB
};

class FontRendererTexture
{
public:
	virtual ~FontRendererTexture() = 0;
};

struct FontRendererVertex : public FrpUseSequentialAllocator
{
	float x;
	float y;
	float u;
	float v;
	RGBA color;

	FontRendererVertex()
	{
	}

	FontRendererVertex(float x, float y, float u, float v, RGBA color)
	    : x(x), y(y), u(u), v(v), color(color)
	{
	}
};

struct ResultingRectangle : public FrpUseSequentialAllocator
{
	Rect rectangle;
	RGBA color;
};

struct ResultingRectangles : public FrpUseSequentialAllocator
{
	int count;
	ResultingRectangle* rectangles;
};

class FontRendererGameInterface
{
public:
	virtual FontRendererTexture* CreateTexture(int width, int height, FontRendererTextureFormat format, void* pixelData) = 0;

	virtual void SetTexture(FontRendererTexture* texture) = 0;

	virtual void DrawIndexedVertices(int numVertices, int numIndices, FontRendererVertex* vertices, uint16_t* indices) = 0;

	virtual void DrawRectangles(int numRectangles, const ResultingRectangle* rectangles) = 0;

	virtual void UnsetTexture() = 0;

	virtual void InvokeOnRender(void (*cb)(void*), void* arg) = 0;
};
}