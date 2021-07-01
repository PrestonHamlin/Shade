#include "RenderEngine.h"

using namespace Microsoft::WRL;

//RenderEngine* RenderEngine::pCurrentEngine = nullptr;

RenderEngine::RenderEngine(UINT width, UINT height, std::wstring name) :
	m_width(width),
	m_height(height),
	m_title(name),
	m_useWarpDevice(false)
{
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

RenderEngine::~RenderEngine()
{
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
_Use_decl_annotations_
void RenderEngine::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> pAdapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = pAdapter.Detach();
}

// Helper function for setting the window's title text.
void RenderEngine::SetCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = m_title + L": " + text;
	SetWindowText(m_window, windowText.c_str());
}

// Helper function for parsing any supplied command line args.
_Use_decl_annotations_
void RenderEngine::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
	for (int i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
		{
			m_useWarpDevice = true;
			m_title = m_title + L" (WARP)";
		}
	}
}
