#pragma once

#include "Dx12RenderEngine.h"
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
    //PipelineState(Dx12RenderEngine* pEngine);
    PipelineState(PipelineCreateInfo createInfo);
    ~PipelineState();

    // helpers
    void Init(PipelineCreateInfo createInfo);
    void SetShader(std::string file, ShaderKind type, std::string entry);
    void AddMesh(Mesh* pMesh);
    void Render();
    //void Execute() {m_pEngine->ExecuteCommandList(m_pCommandList.Get());}
    void Execute() {Dx12RenderEngine::pCurrentEngine->ExecuteCommandList(m_pCommandList.Get());}

    // getters/setters
    ComPtr<ID3D12PipelineState> GetPipelineState()  {return m_pPipelineState;}
    ComPtr<ID3D12RootSignature> GetRootSignature()  {return m_pRootSignature;}
    ComPtr<ID3D12Resource> GetRenderTarget()        {return m_pRenderTarget;}
    ComPtr<ID3D12DescriptorHeap> GetRtvHeap()       {return m_pRtvHeap;}

    void SetClearColor(float* pColor)               {m_pClearColor = pColor;}
    void SetViewport(CD3DX12_VIEWPORT viewport)     {m_viewport = viewport;}
    void SetConstantBufferData(CbvData data);

    // state queries
    const bool isInitialized() const {m_initialized;}
    const bool isCompiled() const {m_compiled;}






private:
    PipelineCreateInfo                  m_createInfo;
    bool                                m_initialized;
    bool                                m_compiled;

    static uint                         m_pipelineIdCounter;
    const uint                          m_pipelineId;

    //Dx12RenderEngine*                   m_pEngine;
    ComPtr<ID3D12CommandAllocator>      m_pCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   m_pCommandList;

    ComPtr<ID3D12RootSignature>         m_pRootSignature;
    ComPtr<ID3D12DescriptorHeap>        m_pRtvHeap;
    ComPtr<ID3D12DescriptorHeap>        m_pSrvHeap;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC  psoDesc;
    ComPtr<ID3D12PipelineState>         m_pPipelineState;

    std::vector<Mesh*>                  m_meshes;

    ComPtr<ID3D12Resource>              m_pRenderTarget;
    CD3DX12_VIEWPORT                    m_viewport;
    CD3DX12_RECT                        m_scissorRect;

    // these should be optional/referenced when there is greater modularity so resources are not duplicated/wasted
    ComPtr<ID3D12Resource>              m_pUploadBuffer;        // generic CPU->GPU uploads
    uint                                m_uploadBufferOffset;   // offset to next free spot in upload buffer
    UINT8*                              m_pUploadBufferBegin;   // start of mapped region
    UINT8*                              m_pUploadBufferEnd;     // end of last added element
    ComPtr<ID3D12Resource>              m_pGeometryBuffer;      // committed resource for scene geometry data
    uint                                m_geometryBufferOffset; // offset to next free spot
    D3D12_VERTEX_BUFFER_VIEW            m_vertexBufferView;
    D3D12_VERTEX_BUFFER_VIEW            m_colorBufferView;
    D3D12_VERTEX_BUFFER_VIEW            m_normalBufferView;
    D3D12_INDEX_BUFFER_VIEW             m_indexBufferView;

    ComPtr<ID3D12Resource>              m_pConstantBuffer;
    void*                               m_pConstantBufferData;
    uint                                m_constantBufferDataSize;
    UINT8*                              m_pConstandBufferDataDataBegin;

    // UI
    float* m_pClearColor;
    float m_clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
};
