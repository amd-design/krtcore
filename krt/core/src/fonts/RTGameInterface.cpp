#include "StdInc.h"
#include "fonts/FontRendererImpl.h"

#include "GameWindow.h"

#include "src/rwd3d.h"
#include "src/rwd3d9.h"

namespace krt
{
class KrtGameInterface : public FontRendererGameInterface
{
public:
	KrtGameInterface(GameWindow* gameWindow);

	virtual FontRendererTexture* CreateTexture(int width, int height, FontRendererTextureFormat format, void* pixelData) override;

	virtual void SetTexture(FontRendererTexture* texture) override;

	virtual void UnsetTexture() override;

	virtual void DrawIndexedVertices(int numVertices, int numIndices, FontRendererVertex* vertex, uint16_t* indices) override;

	virtual void InvokeOnRender(void (*cb)(void*), void* arg) override;

	virtual void DrawRectangles(int numRectangles, const ResultingRectangle* rectangles) override;

private:
	GameWindow* m_gameWindow;
};

class KrtFontTexture : public FontRendererTexture
{
public:
	KrtFontTexture(rw::Texture* texture)
	    : m_texture(texture)
	{
	}

	~KrtFontTexture()
	{
		m_texture->destroy();
	}

	inline rw::Texture* GetTexture() { return m_texture; }

private:
	rw::Texture* m_texture;
};

KrtGameInterface::KrtGameInterface(GameWindow* gameWindow)
    : m_gameWindow(gameWindow)
{
}

FontRendererTexture* KrtGameInterface::CreateTexture(int width, int height, FontRendererTextureFormat format, void* pixelData)
{
	rw::Raster* raster      = rw::Raster::create(width, height, 32, 0x80, rw::PLATFORM_D3D9);
	rw::d3d::D3dRaster* ras = PLUGINOFFSET(rw::d3d::D3dRaster, raster, rw::d3d::nativeRasterOffset);
	rw::d3d::allocateDXT(raster, 5, 1, true);
	ras->customFormat = true;

	void* buf = raster->lock(0);
	memcpy(buf, pixelData, width * height);
	raster->unlock(0);

	return new KrtFontTexture(rw::Texture::create(raster));
}

void KrtGameInterface::SetTexture(FontRendererTexture* texture)
{
	if (texture)
	{
		KrtFontTexture* krtTexture = static_cast<KrtFontTexture*>(texture);
		rw::d3d::setTexture(0, krtTexture->GetTexture());
	}
	else
	{
		rw::d3d::setTexture(0, nullptr);
	}

	rw::d3d::setRenderState(D3DRS_ZENABLE, FALSE);
	rw::d3d::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
	rw::d3d::setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	rw::d3d::setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	rw::d3d::setRenderState(D3DRS_COLORVERTEX, TRUE);
	rw::d3d::setRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);

	rw::Matrix viewMatrix;
	viewMatrix.setIdentity();

	// right up at pos
	int width, height;
	m_gameWindow->GetMetrics(width, height);

	rw::Matrix projMatrix;
	projMatrix.setIdentity();
	projMatrix.right = rw::V3d(2.0f / width, 0.0f, 0.0f);
	projMatrix.up    = rw::V3d(0.0f, -2.0f / height, 0.0f);
	projMatrix.at    = rw::V3d(0.0f, 0.0f, 1.0f);
	projMatrix.pos   = rw::V3d(-1.0f, 1.0f, 0.0f);

	rw::d3d::device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&viewMatrix);
	rw::d3d::device->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&viewMatrix);
	rw::d3d::device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&projMatrix);
}

void KrtGameInterface::UnsetTexture()
{
	rw::d3d::setTexture(0, nullptr);
}

void KrtGameInterface::DrawIndexedVertices(int numVertices, int numIndices, FontRendererVertex* vertex, uint16_t* indices)
{
	rw::d3d::flushCache();

	static IDirect3DVertexDeclaration9* vertexDecl;

	if (!vertexDecl)
	{
		D3DVERTEXELEMENT9 vertexElements[] =
		    {
		        {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		        {0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		        {0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
		        D3DDECL_END()};

		check(SUCCEEDED(rw::d3d::device->CreateVertexDeclaration(vertexElements, &vertexDecl)));
	}

	for (int i = 0; i < numVertices; i++)
	{
		uint8_t oldRed       = vertex[i].color.red;
		vertex[i].color.red  = vertex[i].color.blue;
		vertex[i].color.blue = oldRed;
	}

	rw::d3d::device->SetVertexDeclaration(vertexDecl);
	rw::d3d::device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, numVertices, numIndices / 3, indices, D3DFMT_INDEX16, vertex, sizeof(*vertex));
}

void KrtGameInterface::DrawRectangles(int numRectangles, const ResultingRectangle* rectangles)
{
	SetTexture(nullptr);

	rw::d3d::flushCache();

	static IDirect3DVertexDeclaration9* vertexDecl;

	if (!vertexDecl)
	{
		D3DVERTEXELEMENT9 vertexElements[] =
		    {
		        {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		        {0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		        {0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
		        D3DDECL_END()};

		check(SUCCEEDED(rw::d3d::device->CreateVertexDeclaration(vertexElements, &vertexDecl)));
	}

	for (size_t i : irange<size_t>(numRectangles))
	{
		auto rectangle = &rectangles[i];
		auto& rect     = rectangle->rectangle;

		auto color = rectangle->color;

		// swap RGB/BGR
		uint8_t oldRed = color.red;
		color.red      = color.blue;
		color.blue     = oldRed;

		FontRendererVertex vertices[4] = {
		    {rect.fX1, rect.fY1, 0.0f, 0.0f, color},
		    {rect.fX2, rect.fY1, 0.0f, 0.0f, color},
		    {rect.fX1, rect.fY2, 0.0f, 0.0f, color},
		    {rect.fX2, rect.fY2, 0.0f, 0.0f, color}};

		rw::d3d::device->SetVertexDeclaration(vertexDecl);
		rw::d3d::device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(vertices[0]));
	}

	UnsetTexture();
}

void KrtGameInterface::InvokeOnRender(void (*cb)(void*), void* arg)
{
	// we're not a threaded renderer
	cb(arg);
}

FontRendererGameInterface* CreateGameInterface(GameWindow* gameWindow)
{
	return new KrtGameInterface(gameWindow);
}
}