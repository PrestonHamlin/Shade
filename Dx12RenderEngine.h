#pragma once

#include "RenderEngine.h"

#include "imported/imgui/imgui.h"
#include "imported/imgui/imgui_impl_dx12.h"
#include "imported/imgui/imgui_impl_win32.h"

#include "Mesh.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

// TODO: refactor and cleanup
// The engine owns the device, adapter, swapchain and UI pipeline/heaps/resources. It schedules work and manages
//  resources at the request of client Scenes. Ideally clients would know nothing about the engine internals, simply
//  operating with an abstracted interface to create and manage resources and request work be done on said resources.
//  Eventual Vulkan support will need this abstraction, but for the time being it is partial and rather haphazard.
class Dx12RenderEngine : public RenderEngine
{
public:
    Dx12RenderEngine(UINT width, UINT height, std::wstring name);

    virtual void Init(const HWND window);
    virtual void SetDebugData();

    virtual void OnResize(RECT* pRect);
    void ToggleFullscreen();
    void NewFrame();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    void WaitForPreviousFrame();


    virtual void OnKeyDown(UINT8 key);


    HRESULT CreateCommandQueue(ID3D12CommandQueue**                     ppCommandQueue);
    HRESULT CreateCommandAllocator(ID3D12CommandAllocator**             ppCommandAllocator);
    HRESULT CreateCommandList(ID3D12GraphicsCommandList**               ppCommandList);
    HRESULT CreateRootSignature(CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC*  pDesc,
                                ID3D12RootSignature**                   ppRootSignature);
    HRESULT CreatePipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC*     pDesc,
                                ID3D12PipelineState**                   ppPipelineState);

    HRESULT CreateResource(ID3D12Resource** ppResource);


    const uint UploadGeometryData(Mesh* pMesh);

    void ExecuteCommandList(ID3D12GraphicsCommandList*                  pCommandList);



    UINT8* GetCbvDataBegin() {return m_pCbvDataBegin;}



    ID3D12Device8* GetDevice() {return m_device.Get();}
    uint GetRtvDescriptorSize() {m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);}
    uint GetCbvSrvUavDescriptorSize() {m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}


    void CopyResource(ComPtr<ID3D12Resource> dst, ComPtr<ID3D12Resource> src);
    void CopyResourceDescriptor(ComPtr<ID3D12DescriptorHeap> dst, ComPtr<ID3D12DescriptorHeap> src);

    void* Dx12RenderEngine::SetSrv(ComPtr<ID3D12Resource> resource);

    void DrawDebugUi();

    static Dx12RenderEngine* pCurrentEngine;


private:
    static constexpr UINT FrameCount = 2;

    struct Vertex
    {
        XMFLOAT3 position;
        //XMFLOAT4 color;
        XMFLOAT3 color;
    };


    // API constructs and interfaces
    ComPtr<IDXGIFactory4>               m_factory;
    ComPtr<IDXGIAdapter1>               m_adapter;
    //ComPtr<ID3D12Device>                m_device;
    ComPtr<ID3D12Device8>               m_device;
    ComPtr<ID3D12CommandQueue>          m_commandQueue;
    ComPtr<ID3D12CommandAllocator>      m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   m_pCommandList;

    // pipeline state
    ComPtr<ID3D12PipelineState>         m_pipelineState;
    ComPtr<ID3D12RootSignature>         m_rootSignature;
    CD3DX12_VIEWPORT                    m_viewport;
    CD3DX12_RECT                        m_scissorRect;
    ComPtr<IDXGISwapChain3>             m_swapChain;
    ComPtr<ID3D12Resource>              m_renderTargets[FrameCount];

    // rendering resources
    ComPtr<ID3D12DescriptorHeap>        m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap>        m_srvHeap;
    ComPtr<ID3D12Resource>              m_pUploadBuffer;        // generic CPU->GPU uploads
    uint                                m_uploadBufferOffset;   // offset to next free spot in upload buffer
    UINT8*                              m_pUploadBufferBegin;   // start of mapped region
    UINT8*                              m_pUploadBufferEnd;     // end of last added element
    ComPtr<ID3D12Resource>              m_pGeometryBuffer;      // committed resource for scene geometry data
    uint                                m_geometryBufferOffset; // offset to next free spot
    //ComPtr<ID3D12Resource>              m_vertexBuffer;
    //D3D12_VERTEX_BUFFER_VIEW            m_vertexBufferView;
    ComPtr<ID3D12Resource>              m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW             m_indexBufferView;
    ComPtr<ID3D12Resource>              m_constantBuffer;
    //SceneConstantBuffer                 m_constantBufferData;
    UINT                                m_rtvDescriptorSize;   // offset into RTV descriptor heap for next RTV
    UINT8*                              m_pCbvDataBegin;       // pointer to CBV data for mapping

    ComPtr<ID3D12Resource>              m_renderTargetTexture; // copy destination to use RTV as texture


    // UI
    ImGuiContext*                       m_ImGuiContext;
    bool                                m_fullscreen;
    bool                                m_showDebugConsole;
    bool                                m_showImGuiDemoWindow;
    bool                                m_showImGuiMetrics;
    bool                                m_showImGuiStyleEditor;
    RECT                                m_windowPosition;

    // synchronization objects
    UINT                                m_frameIndex;
    HANDLE                              m_fenceEvent;
    ComPtr<ID3D12Fence>                 m_fence;
    UINT64                              m_fenceValue;



    void LoadPipeline();
    void LoadAssets();

    void BuildUi();
    void ProcessUiEvents();

    void ResizeSwapchain(uint width, uint height);

    void PopulateCommandList();
    //void WaitForPreviousFrame();

    float m_clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    //float m_clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};
