#pragma once

#include "Dx12RenderEngine.h"
#include "GeometryManager.h"
#include "Shader.h"


struct CbvData
{
    void* pData;
    uint  size;
};

struct PipelineCreateInfo
{
    uint RtvCount;
    uint SrvCount;

    uint UploadBufferSize;
    uint GeometryBufferSize;
};


// TODO: allow sharing/referencing of resources across pipeline objects
class PipelineState
{
public:
    PipelineState();
    PipelineState(PipelineCreateInfo createInfo);
    ~PipelineState();

    // common usage
    void Init(PipelineCreateInfo createInfo);
    void Render();
    void Execute() {Dx12RenderEngine::pCurrentEngine->ExecuteCommandList(m_pCommandList.Get());}

    // geometry and draws
    void RegisterGeometryManager(GeometryManager* pGeometryManager) {m_pGeometryManager = pGeometryManager;}
    void DrawAllGeometry();
    void DrawStaticMesh(const Drawable& drawable);

    void SetConstantBufferData(CbvData data);
    void UpdateConstantBufferData();

    // getters/setters
    ComPtr<ID3D12PipelineState> GetPipelineState()  {return m_pPipelineState;}
    ComPtr<ID3D12RootSignature> GetRootSignature()  {return m_pRootSignature;}
    ComPtr<ID3D12Resource> GetRenderTarget()        {return m_pRenderTarget;}
    ComPtr<ID3D12Resource> GetDepthStencil()        {return m_pDepthStencil;}
    ComPtr<ID3D12DescriptorHeap> GetRtvHeap()       {return m_pRtvHeap;}
    void SetClearColor(float* pColor)               {m_pClearColor = pColor;}
    void SetViewport(CD3DX12_VIEWPORT viewport)     {m_viewport = viewport;}
    void SetReverseDepth(bool reverse)              {m_reverseDepth = reverse;}

    // state queries
    const bool isInitialized() const {m_initialized;}
    const bool isCompiled() const {m_compiled;}

private:
    // instance metadata
    bool                                m_initialized;
    bool                                m_compiled;
    static uint                         m_pipelineIdCounter;
    const uint                          m_pipelineId;

    // Shade constructs
    GeometryManager*                    m_pGeometryManager;
    std::vector<Drawable>               m_drawList;

    // API constructs
    ComPtr<ID3D12CommandAllocator>      m_pCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   m_pCommandList;

    // pipeline state
    ComPtr<ID3D12RootSignature>         m_pRootSignature;
    ComPtr<ID3D12DescriptorHeap>        m_pRtvHeap;
    ComPtr<ID3D12DescriptorHeap>        m_pSrvHeap;
    ComPtr<ID3D12DescriptorHeap>        m_pDsvHeap;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC  psoDesc;
    ComPtr<ID3D12PipelineState>         m_pPipelineState;
    ComPtr<ID3D12PipelineState>         m_pPipelineStateReverseDepth;
    CD3DX12_VIEWPORT                    m_viewport;
    CD3DX12_RECT                        m_scissorRect;

    // constant buffer
    ComPtr<ID3D12Resource>              m_pConstantBuffer;
    void*                               m_pConstantBufferData;
    uint                                m_constantBufferDataSize;
    UINT8*                              m_pConstandBufferDataDataBegin;

    // depth
    ComPtr<ID3D12Resource>              m_pDepthStencil;        // R32 depth texture
    bool                                m_reverseDepth;         // whether we are using reversed-Z depth

    // outputs
    ComPtr<ID3D12Resource>              m_pRenderTarget;        // output primary render target
    float*                              m_pClearColor;          // value to clear render target to, if provided
    float                               m_clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
};
