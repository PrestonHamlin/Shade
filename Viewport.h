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

    void Init(std::string name);    // configure and initialize self
    void SubmitWork();              // request engine do requisite prep work
    void DrawUI();                  // draw ImGui window with contents and UI

    const std::string GetName() const {return m_name;}
    ComPtr<ID3D12Resource> GetResource() {return m_pResource;}

    static uint NumViewports;
private:
    int m_viewportId;
    ViewportType m_viewportType;
    bool m_isValid;
    std::string m_name;

    Dx12RenderEngine* m_pEngine;
    ComPtr<ID3D12Resource> m_pResource;
};
