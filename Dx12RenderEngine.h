#pragma once

#include "RenderEngine.h"

#include "imported/imgui/imgui.h"
#include "imported/imgui/imgui_impl_dx12.h"
#include "imported/imgui/imgui_impl_win32.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

// TODO: refactor and cleanup
class Dx12RenderEngine : public RenderEngine
{
public:
    Dx12RenderEngine(UINT width, UINT height, std::wstring name);

    virtual void OnInit(const HWND window);
    virtual void SetDebugData();

    virtual void OnResize(RECT* pRect);
    void ToggleFullscreen();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

    virtual void OnKeyDown(UINT8 key);

private:
    static constexpr UINT FrameCount = 2;

    struct Vertex
    {
        XMFLOAT3 position;
        //XMFLOAT4 color;
        XMFLOAT3 color;
    };

    struct SceneConstantBuffer
    {
        XMFLOAT4 angles;
        XMMATRIX MVP;
    };



    // API constructs and interfaces
    ComPtr<IDXGIFactory4>               m_factory;
    ComPtr<IDXGIAdapter1>               m_adapter;
    ComPtr<ID3D12Device>                m_device;
    ComPtr<ID3D12CommandAllocator>      m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   m_commandList;
    ComPtr<ID3D12GraphicsCommandList>   m_commandList2;
    ComPtr<ID3D12CommandQueue>          m_commandQueue;

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
    ComPtr<ID3D12Resource>              m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW            m_vertexBufferView;
    ComPtr<ID3D12Resource>              m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW             m_indexBufferView;
    ComPtr<ID3D12Resource>              m_constantBuffer;
    SceneConstantBuffer                 m_constantBufferData;
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

    // state and data
    DirectX::XMMATRIX                   m_ModelMatrix;
    DirectX::XMMATRIX                   m_ViewMatrix;
    DirectX::XMMATRIX                   m_ProjectionMatrix;

    float m_fieldOfViewAngle = 45.0;
    float m_variableAspectRatio = 1;

    void LoadPipeline();
    void LoadAssets();

    void BuildUi();
    void ProcessUiEvents();

    void ResizeSwapchain(uint width, uint height);

    void PopulateCommandList();
    void WaitForPreviousFrame();

    float m_clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
};
