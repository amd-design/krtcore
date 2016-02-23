#include <StdInc.h>

#include <windows.h>
#include <d3d9.h>

#define RW_D3D9

#include "src/rwd3d.h"
#include "src/rwd3d9.h"

#include <wrl.h>

#pragma comment(lib, "d3d9.lib")

using Microsoft::WRL::ComPtr;

namespace krt
{
void* InitializeD3D(HWND hWnd, int width, int height)
{
	// initialize the D3D9 factory
	ComPtr<IDirect3D9> d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	// create presentation parameters
	D3DPRESENT_PARAMETERS pp = { 0 };
	pp.BackBufferWidth = width;
	pp.BackBufferHeight = height;
	pp.BackBufferCount = 1;
	pp.BackBufferFormat = D3DFMT_A8R8G8B8;

	pp.MultiSampleQuality = 0;
	pp.MultiSampleType = D3DMULTISAMPLE_NONE;

	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	pp.hDeviceWindow = hWnd;
	pp.Windowed = TRUE;

	pp.EnableAutoDepthStencil = TRUE;
	pp.AutoDepthStencilFormat = D3DFMT_D24S8;

	pp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	// create device
	IDirect3DDevice9* devicePtr = nullptr;
	check(SUCCEEDED(d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &pp, &devicePtr)));

	// store device
	rw::d3d::device = devicePtr;

	return devicePtr;
}
}