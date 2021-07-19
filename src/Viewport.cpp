#include "Viewport.h"

uint Viewport::NumViewports = 0;


//**********************************************************************************************************************
//                                              Viewport Virtual Class
//**********************************************************************************************************************
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
void Viewport::Init(std::string name, DXGI_FORMAT format)
{
    // TODO: rework
}

void Viewport::SubmitWork()
{
}

void Viewport::DrawUI()
{
    if (m_isPinnedView) DrawPinnedResource();
    else                DrawReferencedResource();
}

void Viewport::DrawReferencedResource()
{
    ImGui::Begin(m_name.c_str());
    ImGui::Text("referenced resource view");
    ImGui::Image((ImTextureID)m_srvHandle.ptr, {200.0f, 200});
    ImGui::End();
}

void Viewport::DrawPinnedResource()
{
    ImGui::Begin(m_name.c_str());
    ImGui::Text("pinned resource view");
    ImGui::Image((ImTextureID)m_srvHandlePinned.ptr, {200.0f, 200});
    ImGui::End();
}


//**********************************************************************************************************************
//                                              Render Target Viewport
//**********************************************************************************************************************
ViewportRenderTarget::ViewportRenderTarget() :
    Viewport()
{
}
ViewportRenderTarget::~ViewportRenderTarget()
{
}

void ViewportRenderTarget::Init(std::string name, ComPtr<ID3D12Resource> pResource)
{
    m_name = name;
    m_pResource = pResource;
    m_isPinnedView = false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    m_srvHandle = m_pEngine->AddSrvForResource(srvDesc, pResource);

    m_isValid = true;
}

void ViewportRenderTarget::DrawUI()
{
    Viewport::DrawUI();
}


//**********************************************************************************************************************
//                                              Depth Texture Viewport
//**********************************************************************************************************************
ViewportDepthTexture::ViewportDepthTexture() :
    Viewport()
{
}
ViewportDepthTexture::~ViewportDepthTexture()
{
}

void ViewportDepthTexture::Init(std::string name, ComPtr<ID3D12Resource> pResource)
{
    m_name = name;
    m_pResource = pResource;
    m_isPinnedView = false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    m_srvHandle = m_pEngine->AddSrvForResource(srvDesc, pResource);

    m_isValid = true;
}

void ViewportDepthTexture::DrawUI()
{
    Viewport::DrawUI();
}
