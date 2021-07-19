// Viewport class - UI construct for visualizing and manipulating Scene elements.
//
// TODO: ImGui pixel shaders all use DXGI_FORMAT_R8G8B8A8_UNORM by default. Consider making modifications.
// TODO: Have option to create pinned view via resource copy

#pragma once

#include "Util.h"
#include "Dx12RenderEngine.h"


enum class ViewportType
{
    Unknown,
    Text,           // probably not all that useful
    Buffer1D,       // 1D array of data elements
    Buffer2D,       // 2D array
    BufferCustom,   // parsed with lambdas
    DepthTexture,   // single channel R32 depth
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture3D
};


// viewport display scene content such as RTV, depth stencil, textures, etc...
class Viewport
{
public:
    Viewport();
    ~Viewport();

    void Init(std::string name, DXGI_FORMAT format);
    virtual void SubmitWork();              // request engine do requisite prep work
    virtual void DrawUI();                  // draw ImGui window with contents and UI

    const std::string GetName() const           {return m_name;}
    ComPtr<ID3D12Resource> GetResource()        {return m_pResource;}
    ComPtr<ID3D12Resource> GetResourcePinned()  {return m_pResourcePinned;}

    static uint NumViewports;

protected:
    void DrawReferencedResource();
    void DrawPinnedResource();

    // components
    Dx12RenderEngine* m_pEngine;
    ComPtr<ID3D12Resource> m_pResource;
    ComPtr<ID3D12Resource> m_pResourcePinned;   // for creating a copy of target resource
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandlePinned;

    // state
    int m_viewportId;
    ViewportType m_viewportType;
    bool m_isValid;
    bool m_isPinnedView;
    std::string m_name;
};


// viewport for displaying render targets
class ViewportRenderTarget : public Viewport
{
public:
    ViewportRenderTarget();
    ~ViewportRenderTarget();

    void Init(std::string name, ComPtr<ID3D12Resource> pResource);
    void DrawUI();

protected:
};


// viewport for visualizing depth textures
class ViewportDepthTexture : public Viewport
{
public:
    ViewportDepthTexture();
    ~ViewportDepthTexture();

    void Init(std::string name, ComPtr<ID3D12Resource> pResource);
    void DrawUI();

protected:
};
