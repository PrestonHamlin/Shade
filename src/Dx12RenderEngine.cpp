#include "Dx12RenderEngine.h"

#include <dwmapi.h>

#include "Shader.h"
#include "Mesh.h"
#include "ShaderToyScene.h"
#include "Widgets.h"

using namespace std;


Dx12RenderEngine* Dx12RenderEngine::pCurrentEngine = nullptr;


//**********************************************************************************************************************
//                                              Engine Primary Interfaces
//**********************************************************************************************************************
Dx12RenderEngine::Dx12RenderEngine(UINT width, UINT height) :
    RenderEngine(width, height, L"DX12 Render Engine"),
    m_uploadBufferOffset(0),
    m_pUploadBufferBegin(nullptr),
    m_pUploadBufferEnd(nullptr),
    m_geometryBufferOffset(0),
    m_rtvDescriptorSize(0),
    m_srvDescriptorSize(0),
    m_srvCount(0),
    m_frameIndex(0),
    m_pFenceEvent(nullptr),
    m_fenceValue(0),
    m_pImGuiContext(nullptr),
    m_fullscreen(false),
    m_showDebugConsole(false),
    m_showImGuiDemoWindow(false),
    m_showImGuiMetrics(false),
    m_showImGuiStyleEditor(false)
{
    pCurrentEngine = this;
}

void Dx12RenderEngine::Init(const HWND window)
{
    m_window = window;
    GetWindowRect(m_window, &m_windowPosition);

    // configure adapter and create device
    {
        UINT dxgiFactoryFlags = 0;

        // enable debug layer
#if defined(_DEBUG)
        {
            ComPtr<ID3D12Debug> pDebugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
            {
                pDebugController->EnableDebugLayer();
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif

        // enable experimental shader models prior to device creation
        D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModels, nullptr, 0);

        // select adapter and create device
        CheckResult(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_pFactory)));
        if (m_useWarpDevice)
        {
            CheckResult(m_pFactory->EnumWarpAdapter(IID_PPV_ARGS(&m_pAdapter)));
        }
        else
        {
            // try each adapter the runtime presents us with untii we fine one we like
            for (UINT adapterIndex = 0; m_pFactory->EnumAdapters1(adapterIndex, &m_pAdapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                m_pAdapter->GetDesc1(&desc);
                PrintMessage("Examining device: {}\n", ToNormalString(desc.Description));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) // ignore software adapters
                {
                    continue;
                }

                // test the adapter without actually creating a device
                if (SUCCEEDED(D3D12CreateDevice(m_pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }
        CheckResult(D3D12CreateDevice(m_pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice)));
    }

    // create primary submission queue, command list and its allocator
    CreateCommandQueue(&m_pCommandQueue);
    CreateCommandAllocator(&m_pCommandAllocator);
    CreateCommandList(&m_pCommandList);

    // resource management constructs and core resources
    {
        // descriptor heaps
        {
            // RTV descriptor heap
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = 4;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            CheckResult(m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRtvHeap)));

            m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            // SRV descriptor heap
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.NumDescriptors = 1024;
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            CheckResult(m_pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvHeap)));

            m_srvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        // upload heap (committed resource) for generic usage
        {
            constexpr uint uploadBufferSize = 8*1024*1024;
            CheckResult(m_pDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pUploadBuffer)));
        }

        // default heap (committed resource) for geometry data
        {
            constexpr uint geometryBufferSize = 8*1024*1024;
            CheckResult(m_pDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(geometryBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pGeometryBuffer)));

            // keep this buffer persistently mapped
            CD3DX12_RANGE readRange(0, 0);
            CheckResult(m_pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pUploadBufferBegin)));
            m_pUploadBufferEnd = m_pUploadBufferBegin;
            m_uploadBufferOffset = 0;
        }

        // describe and create swapchain
        {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.BufferCount = FrameCount;
            swapChainDesc.Width = m_width;
            swapChainDesc.Height = m_height;
            swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.SampleDesc.Count = 1;

            ComPtr<IDXGISwapChain1> pSwapChain;
            CheckResult(m_pFactory->CreateSwapChainForHwnd(
                m_pCommandQueue.Get(),
                m_window,
                &swapChainDesc,
                nullptr,
                nullptr,
                &pSwapChain
            ));

            // disable alt+enter fullscreen transition for now
            CheckResult(m_pFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));

            CheckResult(pSwapChain.As(&m_pSwapChain));
            m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
        }

        // frame resources
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (UINT n = 0; n < FrameCount; n++)
            {
                CheckResult(m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n])));
                m_pDevice->CreateRenderTargetView(m_pRenderTargets[n].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, m_rtvDescriptorSize);
            }
        }
    }

    // create fences and perform initial upload sync
    {
        CheckResult(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
        m_fenceValue = 1;

        // create CPU-side event for waiting on
        m_pFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_pFenceEvent == nullptr)
        {
            CheckResult(HRESULT_FROM_WIN32(GetLastError()), "fence creation failed");
        }

        // perrform initial flush so we are ready to begin afresh
        Flush();

        // start at frame in swapchain
        m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    }

    // label API constructs with debug names
    SetDebugData();

    // configure and initialize ImGui
    {
        m_pImGuiContext = ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        ImGui_ImplDX12_Init(m_pDevice.Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, m_pSrvHeap.Get(),
                            m_pSrvHeap->GetCPUDescriptorHandleForHeapStart(), m_pSrvHeap->GetGPUDescriptorHandleForHeapStart());
        ImGui_ImplWin32_Init(window);
        m_srvCount++; // entry 0 is used for text glyphs

        // configure fonts for ImGui
        {
            static const ImWchar rangesBase[] =
            {
                0x0020, 0x00FF, // Basic Latin + Latin Supplement
                0
            };
            static const ImWchar rangesJapanese[] =
            {
                0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
                0x31F0, 0x31FF, // Katakana Phonetic Extensions
                0xFF00, 0xFFEF, // Half-width characters
                0
            };
            static ImWchar rangesPrivateUse[] =
            {
                0xE000, 0xE0DE,
                0
            };

            auto& io = ImGui::GetIO();
            ImFontConfig config;
            config.MergeMode = true;    // merge each new font into first font entry, sharing the same atlas
            config.PixelSnapH = true;   // horizontal snap keeps following glyphs aligned, preventing blur

            io.Fonts->AddFontDefault();
            io.Fonts->AddFontFromFileTTF("./assets/fonts/NotoSansCJKjp-Regular.otf", 13.0f, &config, rangesJapanese);
            config.GlyphOffset = { 1, 1 }; // open-iconic and others tend to float up and left, so this re-centers them
            io.Fonts->AddFontFromFileTTF("./assets/fonts/open-iconic.ttf", 13.0f, &config, rangesPrivateUse);

            PrintMessage(Info, "Fonts set");
        }
    }
}

void Dx12RenderEngine::OnUpdate()
{
    m_pScene->OnUpdate();

    // TODO: update internal state
}

void Dx12RenderEngine::PreRender()
{
    // TODO: perform pre-render work for new frame
}

void Dx12RenderEngine::OnRender()
{
    // Inform ImGui backends and core that we are starting a new frame, and construct the immediate-mode UI. The UI
    //  will be drawn later via insertions into command list.
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // reset so that scene can make requests
    CheckResult(m_pCommandAllocator->Reset());
    CheckResult(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

    // execute scene pipelines
    m_pScene->OnRender();

    // build dockspace with main menu bar, then populate with engine and scene contents
    BuildEngineUi();
    m_pScene->BuildUI();
    ImGui::Render();

    // after UI is described, update the floating windows and re-draw their viewports
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(NULL, (void*)m_pCommandList.Get());

    // collect ImGui commands and execute them, drawing the UI
    PopulateCommandList();
    ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
    m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // present the frame we just generated and wait for all remaining work to complete
    CheckResult(m_pSwapChain->Present(1, 0));   // sync to next vertical blank
    //CheckResult(m_pSwapChain->Present(0, 0));   // present immediately (no vsync)
    Flush();

    // progress to next frame in swapchain
    m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}

void Dx12RenderEngine::PostRender()
{
    // TODO: perform post-render work for new frame
}

void Dx12RenderEngine::OnDestroy()
{
    // ensure GPU is idle and not using resources we want to free
    Flush();

    CloseHandle(m_pFenceEvent);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(m_pImGuiContext);
    m_pImGuiContext = nullptr;
}

void Dx12RenderEngine::Flush()
{
    // For the time being, we only expect one frame from a single pipeline in flight. When parallel work across multiple
    //  queues is wanted, we will need a proper synchronization mechanism and possibly a scheduler.

    // signal on on current fence value
    const UINT64 fence = m_fenceValue;
    CheckResult(m_pCommandQueue->Signal(m_pFence.Get(), fence));
    m_fenceValue++;

    if (m_pFence->GetCompletedValue() < fence)
    {
        // set CPU-side event to wait on
        CheckResult(m_pFence->SetEventOnCompletion(fence, m_pFenceEvent));
        WaitForSingleObject(m_pFenceEvent, INFINITE);
    }
}


//**********************************************************************************************************************
//                                                  Other Controls
//**********************************************************************************************************************
void Dx12RenderEngine::BuildEngineUi()
{
    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    m_menuBarText = "Hello!";

    // Create dockspace before anything else and fill main viewport. This should also supplant the "debug" viewport.
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    // main window menu bar containing drop-down menu dialogues and window controls
    {
        ImGui::BeginMainMenuBar();

        // drag clear portions of menu bar to move window
        if (ImGui::IsItemActive()) // while button is held, redraw with drag offset from original position
        {
            //PrintMessage("Menu Bar Active\n");
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);
            m_menuBarText += "\tDrag delta: " + to_string(dragDelta.x) + ", " + to_string(dragDelta.y);
            SetWindowPos(m_window, nullptr, m_windowPosition.left + dragDelta.x, m_windowPosition.top + dragDelta.y, m_width, m_height, 0);
        }
        if (ImGui::IsItemDeactivated()) // since we retained the original position, don't accumulate error or slip
        {
            //PrintMessage("Menu Bar Deactiveated\n");
            GetWindowRect(m_window, &m_windowPosition);
        }

        // menus and their contents
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit"))
            {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::Checkbox("Show Debug Console", &m_showDebugConsole);
            ImGui::Checkbox("Show ImGui Metrics/Debug", &m_showImGuiMetrics);
            ImGui::Checkbox("Show ImGui Style Editor", &m_showImGuiStyleEditor);
            ImGui::Separator();
            ImGui::MenuItem("Foo");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::Text("This is some menu text");
            ImGui::Separator();
            ImGui::MenuItem("Bar");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::Text("This is some menu text");
            ImGui::Separator();
            if (ImGui::MenuItem("ShowDemoWindow"))
            {
                m_showImGuiDemoWindow = true;
            }
            ImGui::EndMenu();
        }

        // title and status text
        ImGui::Separator();
        ImGui::Text("Shade");
        ImGui::Separator();
        ImGui::Text(m_menuBarText.c_str());

        // minimize, maximize, quit
        ImGui::SameLine(ImGui::GetWindowWidth()-50);
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button,        (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.3f));
        if (ImGui::Button("\xEE\x83\x9B"))
        {
            PostQuitMessage(0);
        }
        ImGui::PopStyleColor(3);

        ImGui::EndMainMenuBar();
    }

    // toggleable menus
    {
        if (m_showDebugConsole)
        {
            ImGui::Begin("Debug Console");
            ImGui::Text("this should be a debug console");
            ImGui::End();
        }
        if (m_showImGuiDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_showImGuiDemoWindow);
        }
        if (m_showImGuiMetrics)
        {
            ImGui::ShowMetricsWindow(&m_showImGuiMetrics);
        }
        if (m_showImGuiStyleEditor)
        {
            ImGui::ShowStyleEditor();
        }
    }

    // display mode info
    {
        ImGui::Begin("Display Info");
        ImGui::Separator();
        ImGui::Text("ImGui main viewport");
        ImGui::Text("Window: %.1f,%.1f, %.1fx%.1f", mainViewport->Pos.x, mainViewport->Pos.y, mainViewport->Size.x, mainViewport->Size.y);
        ImGui::Text("Client: %.1f,%.1f, %.1fx%.1f", mainViewport->WorkPos.x, mainViewport->WorkPos.y, mainViewport->WorkSize.x, mainViewport->WorkSize.y);
        ImGui::Text("Rect:   %i,%i,%i,%i %ix%i", m_windowPosition.left, m_windowPosition.top, m_windowPosition.right, m_windowPosition.bottom,
                                                 m_windowPosition.right - m_windowPosition.left, m_windowPosition.bottom - m_windowPosition.top);
        ImGui::Separator();
        ImGui::End();
    }

    // engine/environment configuration details
    {
        // adapter info
        DXGI_FRAME_STATISTICS frameStats;
        m_pSwapChain->GetFrameStatistics(&frameStats);
        DXGI_ADAPTER_DESC1 adapterDesc;
        m_pAdapter->GetDesc1(&adapterDesc);

        // winapi monitor info
        MONITORINFOEXA monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFOEXA);
        HMONITOR monitor = MonitorFromWindow(m_window, MONITOR_DEFAULTTONEAREST); // get majority contributor
        GetMonitorInfoA(monitor, &monitorInfo);
        const uint width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        const uint height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

        // system's name for device is something like "\\.\DISPLAY0" which we can use to get a user-friendly name
        DISPLAY_DEVICEA displayDevice = {};
        displayDevice.cb = sizeof(DISPLAY_DEVICEA);
        EnumDisplayDevicesA(monitorInfo.szDevice, 0, &displayDevice, 0);

        ImGui::Begin("Configuration Details");
        ImGui::Text(ToNormalString(adapterDesc.Description).c_str());
        ImGui::Text(monitorInfo.szDevice);
        ImGui::Text(displayDevice.DeviceString);
        ImGui::Text("Resolution: %ux%u", width, height);
        ImGui::Separator();
        ImGui::Text("Present #%u\n", frameStats.PresentRefreshCount);
        ImGui::End();
    }

    // display all SRV heap resources in use
    {
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_pSrvHeap->GetGPUDescriptorHandleForHeapStart());

        ImGui::Begin("Engine Srv Heap Contents");
        for (uint i = 0; i < m_srvCount; ++i)
        {
            ImGui::Image((ImTextureID)gpuHandle.ptr, { 200.0f, 200 });
            ImGui::Separator();
            gpuHandle.Offset(1, m_srvDescriptorSize);
        }
        ImGui::End();
    }
}

void Dx12RenderEngine::OnResize(RECT* pRect)
{
    // TODO: re-implement
    auto mainViewport = ImGui::GetMainViewport();

    // resize window, then place it where it is fully visible to prevent top bar from being stuck off-screen
    RECT windowRect = { 50, 50, 1280+50, 720+50 };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    *pRect = windowRect;
    m_width = 1280;
    m_height = 720;
    m_aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);

    ResizeSwapchain(1280, 720);
}

void Dx12RenderEngine::ToggleFullscreen()
{
    // TODO: re-implement
    auto mainViewport = ImGui::GetMainViewport();

    m_width = 1920;
    m_height = 1080;

    auto& IO = ImGui::GetIO();
    //IO.DisplaySize.x = 1920;
    //IO.DisplaySize.y = 1080;
    m_aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);

    ResizeSwapchain(1920, 1080);
}

void Dx12RenderEngine::OnKeyDown(UINT8 key)
{
    // TODO: flesh out
    switch (key)
    {
    case VK_F11:
    {
        ToggleFullscreen();
        break;
    }
    default:
    {
    }
    }
}


//**********************************************************************************************************************
//                                              Mediated API Access
//**********************************************************************************************************************
HRESULT Dx12RenderEngine::CreateCommandQueue(ID3D12CommandQueue** ppCommandQueue)
{
    HRESULT result = S_OK;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    result = m_pDevice->CreateCommandQueue(&queueDesc,
                                          __uuidof(ID3D12CommandQueue),
                                          (void**)ppCommandQueue);

    CheckResult(result, "command allocator creation");
    return result;
}

HRESULT Dx12RenderEngine::CreateCommandAllocator(ID3D12CommandAllocator** ppCommandAllocator)
{
    HRESULT result = S_OK;
    result = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                              __uuidof(ID3D12CommandAllocator),
                                              (void**)ppCommandAllocator);

    CheckResult(result, "command allocator creation");
    return result;
}

HRESULT Dx12RenderEngine::CreateCommandList(ID3D12GraphicsCommandList** ppCommandList)
{
    HRESULT result = S_OK;

    // Old interface created in opened state. The newer one is cleaner since it creates in closed state and does not
    //  require preexisting PSO and allocator, allowing for flexibility in setup.
    result = m_pDevice->CreateCommandList1(0,
                                          D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          D3D12_COMMAND_LIST_FLAG_NONE,
                                          __uuidof(ID3D12GraphicsCommandList),
                                          (void**)(ppCommandList));

    CheckResult(result, "command list creation");
    return result;
}

HRESULT Dx12RenderEngine::CreateRootSignature(CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC* pDesc, ID3D12RootSignature** ppRootSignature)
{
    ComPtr<ID3DBlob> pSignature;
    ComPtr<ID3DBlob> pError;
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    HRESULT result = S_OK;

    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    result = D3DX12SerializeVersionedRootSignature(pDesc, featureData.HighestVersion, &pSignature, &pError);
    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(ppRootSignature));
        if (FAILED(result))
        {
            PrintMessage(Error, "Root signature creation failed!\n");
        }
    }
    else
    {
        PrintMessage(Error, "Root signature serialization failed:\n{}", (char*)pError->GetBufferPointer());
    }

    return result;
}

HRESULT Dx12RenderEngine::CreatePipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc,
                                              ID3D12PipelineState**               ppPipelineState)
{
    HRESULT result = m_pDevice->CreateGraphicsPipelineState(pDesc, IID_PPV_ARGS(ppPipelineState));
    CheckResult(result, "PSO creation");

    return result;
}

HRESULT Dx12RenderEngine::CreateResource(ID3D12Resource** ppResource)
{

    return 0;
}

void Dx12RenderEngine::ExecuteCommandList(ID3D12GraphicsCommandList* pCommandList)
{
    ID3D12CommandList* ppCommandLists[] = { pCommandList };
    m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}


//**********************************************************************************************************************
//                                              Utility & Ease-Of-Use
//**********************************************************************************************************************
const uint Dx12RenderEngine::UploadGeometryData(Mesh* pMesh)
{
    const aiScene* pScene = pMesh->GetScene();
    const uint dataSize = pScene->mMeshes[0]->mNumVertices * sizeof(aiVector3D);
    const void* pDataBegin = pScene->mMeshes[0]->mVertices;

    // write to the end of the upload buffer
    memcpy(m_pUploadBufferEnd, pDataBegin, dataSize);

    // TODO: round offset?

    m_uploadBufferOffset += dataSize;
    m_pUploadBufferEnd = m_pUploadBufferBegin + m_uploadBufferOffset;

    return 0;
}

void Dx12RenderEngine::CopyResource(ComPtr<ID3D12Resource> pDst, ComPtr<ID3D12Resource> pSrc)
{
    m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pSrc.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));
    m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDst.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

    m_pCommandList->CopyResource(pDst.Get(), pSrc.Get());

    m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pSrc.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
    m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDst.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}


//**********************************************************************************************************************
//                                                  Debug & Global UI
//**********************************************************************************************************************
D3D12_GPU_DESCRIPTOR_HANDLE Dx12RenderEngine::AddSrvForResource(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, ComPtr<ID3D12Resource> pResource)
{
    const uint index = m_srvCount++;
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(m_pSrvHeap->GetCPUDescriptorHandleForHeapStart(), index, m_srvDescriptorSize);
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_pSrvHeap->GetGPUDescriptorHandleForHeapStart(), index, m_srvDescriptorSize);

    m_pDevice->CreateShaderResourceView(pResource.Get(), &srvDesc, cpuHandle);

    return gpuHandle;
}


//**********************************************************************************************************************
//                                              Engine Internal Helpers
//**********************************************************************************************************************
void Dx12RenderEngine::PopulateCommandList()
{
    // ImGui uses the heaps we provide, so we need to set them
    ID3D12DescriptorHeap* ppHeaps[] = { m_pSrvHeap.Get() };
    m_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // use back buffer of swapchain as render target
    m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    m_pCommandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);

    // insert ImGui draw commands into engine command list
    // TODO: transition viewport resources to READ
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCommandList.Get());
    // TODO: transition viewport resources back

    // transition back buffer to present mode prior to present
    m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    CheckResult(m_pCommandList->Close());
}

void Dx12RenderEngine::ResizeSwapchain(uint width, uint height)
{
    // TODO: re-implement
    // update swapchain and RTV state
    {
        // wait for render targets to not be in use, then remove references to back buffers
        Flush();
        m_pRenderTargets[0].Reset();
        m_pRenderTargets[1].Reset();

        // resize the swapchain buffers
        DXGI_MODE_DESC newMode = {0};
        newMode.Width = width;
        newMode.Height = height;
        newMode.RefreshRate.Numerator = 60;
        newMode.RefreshRate.Denominator = 1;
        m_pSwapChain->ResizeTarget(&newMode);
        m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0); // resize to client area

        // re-bind swapchain back buffers to render target resources, then update RTVs
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
        CheckResult(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&m_pRenderTargets[0])));
        m_pDevice->CreateRenderTargetView(m_pRenderTargets[0].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);
        CheckResult(m_pSwapChain->GetBuffer(1, IID_PPV_ARGS(&m_pRenderTargets[1])));
        m_pDevice->CreateRenderTargetView(m_pRenderTargets[1].Get(), nullptr, rtvHandle);

        // re-configure render state
        m_pRenderTargets[0]->SetName(L"Updated RTV 0");
        m_pRenderTargets[1]->SetName(L"Updated RTV 1");
        m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    }
}

void Dx12RenderEngine::SetDebugData()
{
    HRESULT result = S_OK;

    // DXGI
    SetDebugName(m_pFactory.Get(), "DXGI Factory");
    SetDebugName(m_pAdapter.Get(), "DXGI Adapter");
    SetDebugName(m_pSwapChain.Get(), "DXGI Swapchain");

    // D3D12
    result = m_pDevice->SetName(m_useWarpDevice ? L"DX12 WARP Device" : L"DX12 Hardware Device");
    result = m_pCommandQueue->SetName(L"Engine Command Queue");
    result = m_pCommandAllocator->SetName(L"Engine Command Allocator");
    result = m_pCommandList->SetName(L"Engine Command List");

    // resources for building and compositing UI
    result = m_pRtvHeap->SetName(L"Engine RTV Heap");
    result = m_pSrvHeap->SetName(L"Engine ImGui textures SRV Heap");
    result = m_pRenderTargets[0]->SetName(L"Engine Render Target 0");
    result = m_pRenderTargets[1]->SetName(L"Engine Render Target 1");
    result = m_pUploadBuffer->SetName(L"Engine Generic Upload Buffer");
    result = m_pGeometryBuffer->SetName(L"Engine Geometry Buffer");

    // synchronization/timing constructs
    result = m_pFence->SetName(L"Engine Fence");

    PrintMessage(Info, "Debug names set");
}
