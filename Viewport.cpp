#include "Viewport.h"

uint Viewport::NumViewports = 0;

// TODO: use Resource class


Viewport::Viewport() :
    m_viewportId(NumViewports++),
    m_isValid(false),
    m_pEngine(Dx12RenderEngine::pCurrentEngine)
{

}
Viewport::~Viewport()
{

}



// default initialization creates new dedicated resource
void Viewport::Init(std::string name)
{
    m_name = name;

    D3D12_RESOURCE_DESC textureDesc = {};
    //textureDesc.MipLevels = 1;
    //textureDesc.Format = DXGI_FORMAT_R32G32B32A32_TYPELESS;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    textureDesc.Width = 800;
    textureDesc.Height = 800;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    //textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    CheckResult(m_pEngine->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        //D3D12_RESOURCE_STATE_COPY_DEST,
        //D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&m_pResource)));

    SetDebugName(m_pResource.Get(), name);
}




void Viewport::DrawUI()
{
    // since this is a UI element, the engine's SRV heap is used
    void* pGpuMem = m_pEngine->SetSrv(m_pResource);

    ImGui::Begin(m_name.c_str());

    ImGui::Text("resource view should go here...");
    ImGui::Image((ImTextureID)pGpuMem, {200.0f, 200});

    ImGui::End();
}
