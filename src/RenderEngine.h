#pragma once

#include "Shade.h"

#include "Util.h"

class RenderEngine
{
public:
	RenderEngine(UINT width, UINT height, std::wstring name);
	virtual ~RenderEngine();

	virtual void Init(const HWND window) = 0;
	virtual void SetDebugData() = 0;

	virtual void OnUpdate() = 0;
	virtual void PreRender() = 0;
	virtual void OnRender() = 0;
	virtual void PostRender() = 0;
	virtual void OnDestroy() = 0;

	// Samples override the event handlers to handle specific messages.
	virtual void OnKeyDown(UINT8 key)	{}
	virtual void OnKeyUp(UINT8 key)     {}
	virtual void OnResize(RECT* pRect) = 0;


	// Accessors.
	UINT GetWidth() const           { return m_width; }
	UINT GetHeight() const          { return m_height; }
	const WCHAR* GetTitle() const   { return m_title.c_str(); }

	void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

	//static RenderEngine* pCurrentEngine;

protected:
	void GetHardwareAdapter(_In_ IDXGIFactory2* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter);
	void SetCustomWindowText(LPCWSTR text);

	// Viewport dimensions.
	UINT m_top;
	UINT m_left;
	UINT m_width;
	UINT m_height;

	float m_aspectRatio;

	// Adapter info.
	bool m_useWarpDevice;

	HWND m_window;

private:
	// Window title.
	std::wstring m_title;
};
