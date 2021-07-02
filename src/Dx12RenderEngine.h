#pragma once

#include "RenderEngine.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include "Mesh.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


// The engine owns the device, adapter, swapchain and UI pipeline/heaps/resources. It schedules work and manages
//  resources at the request of client Scenes. Ideally clients would know nothing about the engine internals, simply
//  operating with an abstracted interface to create and manage resources and request work be done on said resources.
//  Eventual Vulkan support will need this abstraction, but for the time being it is partial and rather haphazard.
class Dx12RenderEngine : public RenderEngine
{
public:
    Dx12RenderEngine(UINT width, UINT height, std::wstring name);

    // primary interfaces for render loop
    virtual void Init(const HWND window);   // initialize API and other state
    virtual void OnUpdate();                // process inputs (physics, user, network, etc...)
    virtual void PreRender();               // do some work at dawn of new frame
    virtual void OnRender();                // issue Present()
    virtual void PostRender();              // work and clenup after frame presented
    void Flush();
    virtual void OnDestroy();

    // other controls
    void BuildEngineUi();
    virtual void OnResize(RECT* pRect);
    void ToggleFullscreen();
    virtual void OnKeyDown(UINT8 key);

    // API access provided to clients
    HRESULT CreateCommandQueue(ID3D12CommandQueue**                     ppCommandQueue);
    HRESULT CreateCommandAllocator(ID3D12CommandAllocator**             ppCommandAllocator);
    HRESULT CreateCommandList(ID3D12GraphicsCommandList**               ppCommandList);
    HRESULT CreateRootSignature(CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC*  pDesc,
                                ID3D12RootSignature**                   ppRootSignature);
    HRESULT CreatePipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC*     pDesc,
                                ID3D12PipelineState**                   ppPipelineState);
    HRESULT CreateResource(ID3D12Resource** ppResource);
    void ExecuteCommandList(ID3D12GraphicsCommandList*                  pCommandList);

    // utility functions provided to clients
    const uint UploadGeometryData(Mesh* pMesh);
    void CopyResource(ComPtr<ID3D12Resource> pDst, ComPtr<ID3D12Resource> pSrc);

    // getters/setters
    ID3D12Device8* GetDevice() {return m_pDevice.Get();}
    uint GetRtvDescriptorSize() {m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);}
    uint GetCbvSrvUavDescriptorSize() {m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}

    // debug helpers
    void* Dx12RenderEngine::SetSrv(ComPtr<ID3D12Resource> pResource);


    // have this be a single static globally-accessible instance
    // TODO: proper Singleton restrictions?
    static Dx12RenderEngine* pCurrentEngine;

private:
    static constexpr UINT FrameCount = 2;


    // internal helpers
    void PopulateCommandList();
    void ResizeSwapchain(uint width, uint height);
    void SetDebugData();


    // API constructs and interfaces
    ComPtr<IDXGIFactory4>               m_pFactory;
    ComPtr<IDXGIAdapter1>               m_pAdapter;
    ComPtr<ID3D12Device8>               m_pDevice;
    ComPtr<ID3D12CommandQueue>          m_pCommandQueue;
    ComPtr<ID3D12CommandAllocator>      m_pCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   m_pCommandList;

    // rendering resources
    ComPtr<IDXGISwapChain3>             m_pSwapChain;
    ComPtr<ID3D12Resource>              m_pRenderTargets[FrameCount];
    ComPtr<ID3D12DescriptorHeap>        m_pRtvHeap;
    ComPtr<ID3D12DescriptorHeap>        m_pSrvHeap;
    ComPtr<ID3D12Resource>              m_pUploadBuffer;        // generic CPU->GPU uploads
    uint                                m_uploadBufferOffset;   // offset to next free spot in upload buffer
    UINT8*                              m_pUploadBufferBegin;   // start of mapped region
    UINT8*                              m_pUploadBufferEnd;     // end of last added element
    ComPtr<ID3D12Resource>              m_pGeometryBuffer;      // committed resource for scene geometry data
    uint                                m_geometryBufferOffset; // offset to next free spot
    UINT                                m_rtvDescriptorSize;    // offset into RTV descriptor heap for next RTV
    float                               m_clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    // UI
    ImGuiContext*                       m_pImGuiContext;
    bool                                m_fullscreen;

    // synchronization objects
    UINT                                m_frameIndex;
    HANDLE                              m_pFenceEvent;
    ComPtr<ID3D12Fence>                 m_pFence;
    UINT64                              m_fenceValue;
};
