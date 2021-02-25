#include "Dx12RenderEngine.h"

#include <dwmapi.h>

#include "Mesh.h"
#include "ShaderToyScene.h"

#include "Widgets.h"


static ImFont* testFont;

Dx12RenderEngine::Dx12RenderEngine(UINT width, UINT height, std::wstring name) :
    RenderEngine(width, height, name),
    m_frameIndex(0),
    m_pCbvDataBegin(nullptr),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_rtvDescriptorSize(0),
    m_constantBufferData({})
{
}

void Dx12RenderEngine::OnInit(const HWND window)
{
    m_window = window;

    GetWindowRect(m_window, &m_windowPosition);

    LoadPipeline();
    LoadAssets();
    SetDebugData();

    m_ImGuiContext = ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui_ImplDX12_Init(m_device.Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, m_srvHeap.Get(),
                        m_srvHeap->GetCPUDescriptorHandleForHeapStart(), m_srvHeap->GetGPUDescriptorHandleForHeapStart());
    ImGui_ImplWin32_Init(window);

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
        config.GlyphOffset = {1, 1}; // open-iconic and others tend to float up and left, so this re-centers them
        io.Fonts->AddFontFromFileTTF("./assets/fonts/open-iconic.ttf", 13.0f, &config, rangesPrivateUse);

        PrintMessage(Info, "Fonts set");
    }
}

void Dx12RenderEngine::SetDebugData()
{
    HRESULT result = S_OK;

    result = m_device->SetName(m_useWarpDevice ? L"DX12 WARP Device" : L"DX12 Hardware Device");

    // command creation/submission constructs
    result = m_commandAllocator->SetName(L"Command Allocator");
    result = m_commandList->SetName(L"Command List");
    result = m_commandQueue->SetName(L"Command Queue");

    // resources
    result = m_rtvHeap->SetName(L"RTV Heap");
    result = m_srvHeap->SetName(L"ImGui textures SRV Heap");
    result = m_vertexBuffer->SetName(L"Vertex Buffer");
    result = m_indexBuffer->SetName(L"Index Buffer");
    result = m_constantBuffer->SetName(L"Constant Buffer");
    result = m_renderTargetTexture->SetName(L"RTV copy texture");

    // pipeline state
    result = m_pipelineState->SetName(L"Pipeline State");
    result = m_rootSignature->SetName(L"Root Signature");
    result = m_renderTargets[0]->SetName(L"Render Target 0");
    result = m_renderTargets[1]->SetName(L"Render Target 1");

    // synchronization/timing constructs
    result = m_fence->SetName(L"Fence");

    // TODO: use all wide or narrow strings?
    // DXGI objects lack the SetName() wrapper around SetPrivateData()
    constexpr char factoryDebugName[]     = "DX12 Device Factory";
    constexpr char adapterDebugName[]     = "DX12 Hardware Adapter";
    constexpr char adapterDebugNameWarp[] = "DX12 WARP Adapter";
    constexpr char swapchainDebugName[]   = "Swapchain";

    m_factory->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(factoryDebugName), factoryDebugName);
    if (m_useWarpDevice) m_adapter->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(adapterDebugNameWarp), adapterDebugNameWarp);
    else                 m_adapter->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(adapterDebugName), adapterDebugName);
    m_swapChain->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(swapchainDebugName), swapchainDebugName);

    PrintMessage(Info, "Debug names set");
}

void Dx12RenderEngine::OnResize(RECT* pRect)
{
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

// Load the rendering pipeline dependencies.
void Dx12RenderEngine::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    // enable experimental shader models prior to device creation
    D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModels, nullptr, 0);

    // select adapter and create device
    CheckResult(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));
    if (m_useWarpDevice)
    {
        CheckResult(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&m_adapter)));
    }
    else
    {
        GetHardwareAdapter(m_factory.Get(), &m_adapter);
    }
    CheckResult(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    CheckResult(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    CheckResult(m_factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
        m_window,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
        ));

    // This sample does not support fullscreen transitions.
    //CheckResult(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
    CheckResult(m_factory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    //CheckResult(m_factory->MakeWindowAssociation(m_window, 0));

    CheckResult(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        CheckResult(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // SRV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 2;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        CheckResult(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            CheckResult(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    CheckResult(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// Load the sample assets.
void Dx12RenderEngine::LoadAssets()
{
    // create a root signature consisting of an inline CBV and an SRV table
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];
        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            //D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            //D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            //D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        CheckResult(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        CheckResult(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        ComPtr<ID3DBlob> errorBlob;
        //HRESULT vsresult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_6_0", compileFlags, 0, &vertexShader, &errorBlob);
        HRESULT vsresult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob);
        if (errorBlob) OutputDebugStringA((LPCSTR)errorBlob->GetBufferPointer());
        CheckResult(vsresult);
        //HRESULT psresult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_6_0", compileFlags, 0, &pixelShader, &errorBlob);
        HRESULT psresult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob);
        if (errorBlob) OutputDebugStringA((LPCSTR)errorBlob->GetBufferPointer());
        CheckResult(psresult);

        // vertex input layout
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            //{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // describe and create the PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        CheckResult(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create the command list.
    CheckResult(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    CheckResult(m_commandList->Close());

    //Mesh cubeMesh("media/cube.obj");
    //Mesh badMesh("shaders.hlsl");
    //Mesh otherBadMesh("foo.bar");
    //Mesh colladaMesh("media/colored_cube_trimmed2.dae");
    //PrintMessage(Info, "this is an info message");
    //PrintMessage(Info, "this is an info message {} {}", "WITH", "ARGS");
    //PrintMessage(Info, "this is an info message {} {} {} {}", "WITH", "EVEN", "MORE", "ARGS");
    //PrintMessage(Info, "this is an info message {} {} {} {} {} {} {}", "WITH", 0, "EVEN", 1, "MORE", 2, "ARGS");
    //PrintMessage(Info, "this is an info message {} {} {} {:g} {:*^10} {:#x} {}", "WITH", 0, "EVEN", 0.1 + 0.2, "MORE", 3735928559, "ARGS");


    // create and populate vertex buffer
    {
        // Define the geometry for a triangle.
        //Vertex triangleVertices[] =
        //{
        //	{ { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        //	{ { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        //	{ { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        //};
        // cube
        Vertex triangleVertices[8] = {
            { {-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f} }, // 0
            { {-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f} }, // 1
            { { 1.0f,  1.0f, -1.0f}, {1.0f, 1.0f, 0.0f} }, // 2
            { { 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f} }, // 3
            { {-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f} }, // 4
            { {-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f} }, // 5
            { { 1.0f,  1.0f,  1.0f}, {1.0f, 1.0f, 1.0f} }, // 6
            { { 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f} }  // 7
        };
        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not
        // recommended. Every time the GPU needs it, the upload heap will be marshalled
        // over. Please read up on Default Heap usage. An upload heap is used here for
        // code simplicity and because there are very few verts to actually transfer.
        CheckResult(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        CheckResult(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // create and populate index buffer
    {
        // Define the geometry for a triangle.
        //short triangleIndices[] =
        //{
        //    0, 1, 2,
        //    0, 3, 1,
        //    0, 2, 4,
        //};
        short triangleIndices[] =
        {
            0, 1, 2, 0, 2, 3,
            4, 6, 5, 4, 7, 6,
            4, 5, 1, 4, 1, 0,
            3, 2, 6, 3, 6, 7,
            1, 5, 6, 1, 6, 2,
            4, 0, 3, 4, 3, 7
        };
        const UINT indexBufferSize = sizeof(triangleIndices);

        // Note: using upload heaps to transfer static data like vert buffers is not
        // recommended. Every time the GPU needs it, the upload heap will be marshalled
        // over. Please read up on Default Heap usage. An upload heap is used here for
        // code simplicity and because there are very few verts to actually transfer.
        CheckResult(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)));

        // Copy the triangle data to the index buffer.
        UINT8* pIndexDataBegin;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        CheckResult(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
        memcpy(pIndexDataBegin, triangleIndices, sizeof(triangleIndices));
        m_indexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;
    }

    // create constant buffer resource for inlined per-frame data
    {
        CheckResult(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer)));

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        CheckResult(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
        memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
    }

    // texture resource for RTV copy
    {
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width = m_width;
        textureDesc.Height = m_height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        CheckResult(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_renderTargetTexture)));


        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        UINT offset = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 0, offset);
        srvHandle.Offset(offset);
        //m_device->CreateShaderResourceView(m_renderTargetTexture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart() + offset);
        m_device->CreateShaderResourceView(m_renderTargetTexture.Get(), &srvDesc, srvHandle);
    }

    // create fences and perform initial upload sync
    {
        CheckResult(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            CheckResult(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command
        // list in our main loop but for now, we just want to wait for setup to
        // complete before continuing.
        WaitForPreviousFrame();
    }
}

// Update frame-based values.
void Dx12RenderEngine::OnUpdate()
{
    //const float translationSpeed = 0.005f;
    //const float offsetBounds = 1.25f;

    // UI interactions may be requests for change in rendering, or otherwise change state
    ProcessUiEvents();

    //m_constantBufferData.angles.x += translationSpeed;
    m_constantBufferData.angles.x += 0.005f;
    m_constantBufferData.angles.y += 0.005f;
    m_constantBufferData.angles.z += 0.005f;

    static uint64_t frameCount = 0;
    frameCount++;

    // Update the model matrix.
    float angle = static_cast<float>(frameCount);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    // Update the view matrix.
    const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // Update the projection matrix.
    m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fieldOfViewAngle), m_aspectRatio, 0.1f, 100.0f);

    // Update the MVP matrix
    XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
    m_constantBufferData.MVP = mvpMatrix;

    memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
}

// Render the scene.
void Dx12RenderEngine::OnRender()
{
    // Inform ImGui backends and core that we are starting a new frame, and construct the immediate-mode UI. The UI
    //  will be drawn later via insertions into command list.
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    BuildUi();

    // after UI is described, update the floating windows and re-draw their viewports
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(NULL, (void*)m_commandList.Get());

    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    CheckResult(m_swapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void Dx12RenderEngine::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(m_ImGuiContext);
    m_ImGuiContext = nullptr;
}

// Use immediate-mode ImGui constructs and state data to construct UI. Some state will be updated via references.
void Dx12RenderEngine::BuildUi()
{
    ImGuiIO& io = ImGui::GetIO();
    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();

    // Create dockspace before anything else and fill main viewport. This should also supplant the "debug" viewport.
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    // main window menu bar containing drop-down menu dialogues and window controls
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::Text("This is some menu text");
            ImGui::MenuItem("New");
            ImGui::MenuItem("Blah");
            if (ImGui::MenuItem("Quit"))
            {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::Text("This is some menu text");
            ImGui::Separator();
            ImGui::Checkbox("Show Debug Console", &m_showDebugConsole);
            ImGui::Checkbox("Show ImGui Metrics/Debug", &m_showImGuiMetrics);
            ImGui::Checkbox("Show ImGui Style Editor", &m_showImGuiStyleEditor);
            ImGui::Separator();
            ImGui::MenuItem("Foo");
            ImGui::MenuItem("Bar");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::Text("This is some menu text");
            ImGui::Separator();
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

        // Title and status text
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Shade");

        // custom nav controls for minimize, maximize, drag to move, etc...
        if (ImGui::SmallButton("Drag")) // when button is released, save current window position
        {
            GetWindowRect(m_window, &m_windowPosition);
        }
        if (ImGui::IsItemActive()) // while button is held, redraw with drag offset from original position
        {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);
            ImGui::Text("Drag delta: %f, %f", dragDelta.x, dragDelta.y);

            SetWindowPos(m_window, nullptr, m_windowPosition.left + dragDelta.x, m_windowPosition.top + dragDelta.y, m_width, m_height, 0);
        }

        // invisible button for drag controls rather than hook title bar events?
        static std::string msg = "hello";
        if (ImGui::InvisibleButton("invis", { 10,10 }))
        {
            msg = "world";
        }
        ImGui::Text(msg.c_str());

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

    // ImGui simple box
    {
        ImGui::Begin("test");
        ImGui::Text("this is a test");
        ImGui::End();
    }

    // clear-color picker
    {
        ImVec4 pickerColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);

        ImGui::Begin("color picker");
        if (ImGui::ColorButton("button", pickerColor))
        {
            ImGui::OpenPopup("colorPickerPopup");
        }

        // describe color-picker popup
        if (ImGui::BeginPopup("colorPickerPopup"))
        {
            const uint flags = ImGuiColorEditFlags_PickerHueWheel | // triangle inside wheel
                               ImGuiColorEditFlags_NoSmallPreview | // no (duplicate) mini previews next to text fields
                               ImGuiColorEditFlags_NoSidePreview  | // no preview beside picker
                               ImGuiColorEditFlags_NoLabel;         // no element name as text
            ImGui::ColorPicker4("fancyColorPicker", m_clearColor, flags);
            ImGui::Text("Custom text in the custom popup");
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    // frame stats
    {
        DXGI_FRAME_STATISTICS frameStats;
        m_swapChain->GetFrameStatistics(&frameStats);

        ImGui::Begin("Frame Stats");
        ImGui::Text("Present #%u\n", frameStats.PresentRefreshCount);
        ImGui::End();
    }

    // display mode info
    {
        ImGui::Begin("Display Info");

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
        ImGui::Text(monitorInfo.szDevice);
        ImGui::Text(displayDevice.DeviceString);
        ImGui::Text("Resolution: %ux%u", width, height);

        ImGui::Separator();
        ImGui::Text("ImGui main viewport");
        ImGui::Text("Window: %.1f,%.1f, %.1fx%.1f", mainViewport->Pos.x, mainViewport->Pos.y, mainViewport->Size.x, mainViewport->Size.y);
        ImGui::Text("Client: %.1f,%.1f, %.1fx%.1f", mainViewport->WorkPos.x, mainViewport->WorkPos.y, mainViewport->WorkSize.x, mainViewport->WorkSize.y);
        ImGui::Text("Window: %u,%u, %ux%u", m_left, m_top, m_width, m_height);
        ImGui::Text("Rect:   %i,%i,%i,%i %ix%i", m_windowPosition.left, m_windowPosition.top, m_windowPosition.right, m_windowPosition.bottom,
                                                 m_windowPosition.right - m_windowPosition.left, m_windowPosition.bottom - m_windowPosition.top);
        ImGui::Separator();
        ImGui::End();
    }

    // file browser widget
    {
        ImGui::Begin("File Browser");

        // static field to be updated by browser popup
        static char filePath[1024] = "file path";
        FilePickerWidget(filePath, IM_ARRAYSIZE(filePath));

        ImGui::End(); // file browser
    }

    // swapchain copy and some simple controls
    {
        ImGui::Begin("Swapchain");
        D3D12_GPU_DESCRIPTOR_HANDLE handle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        ImGui::Image((ImTextureID)handle.ptr, {((200.0f * m_width)/m_height), 200});
        ImGui::SliderFloat("FOV", &m_fieldOfViewAngle, 0.01, 360);
        ImGui::SliderFloat("Aspect Ratio", &m_aspectRatio, 0.01, 4);

        ImGui::SliderInt("scissor right", (int*)&m_scissorRect.right, 1, 2000);
        ImGui::SliderInt("scissor bottom", (int*)&m_scissorRect.bottom, 1, 2000);
        ImGui::End();
    }

    // menus opened via main menu bar buttons
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

    //ImGui::EndFrame();
    ImGui::Render();
}

// Update UI/engine state based on UI events.
void Dx12RenderEngine::ProcessUiEvents()
{
}

// Fill the command list with all the render commands and dependent state.
void Dx12RenderEngine::PopulateCommandList()
{
    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };

    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress.
    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    CheckResult(m_commandAllocator->Reset());
    CheckResult(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);
    m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

    // copy rendered frame into texture for UI usage
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
    m_commandList->CopyResource(m_renderTargetTexture.Get(), m_renderTargets[m_frameIndex].Get());
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

    //m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT));

    // ImGui
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

    // Indicate that the back buffer will now be used to present.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    CheckResult(m_commandList->Close());
}

void Dx12RenderEngine::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    CheckResult(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        CheckResult(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Dx12RenderEngine::ResizeSwapchain(uint width, uint height)
{
    // update swapchain and RTV state
    {
        // wait for render targets to not be in use, then remove references to back buffers
        WaitForPreviousFrame();
        m_renderTargets[0].Reset();
        m_renderTargets[1].Reset();

        // resize the swapchain buffers
        DXGI_MODE_DESC newMode = {0};
        newMode.Width = width;
        newMode.Height = height;
        newMode.RefreshRate.Numerator = 60;
        newMode.RefreshRate.Denominator = 1;
        m_swapChain->ResizeTarget(&newMode);
        m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0); // resize to client area

        // re-bind swapchain back buffers to render target resources, then update RTVs
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        CheckResult(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_renderTargets[0])));
        m_device->CreateRenderTargetView(m_renderTargets[0].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);
        CheckResult(m_swapChain->GetBuffer(1, IID_PPV_ARGS(&m_renderTargets[1])));
        m_device->CreateRenderTargetView(m_renderTargets[1].Get(), nullptr, rtvHandle);

        // re-configure render state
        m_renderTargets[0]->SetName(L"Updated RTV 0");
        m_renderTargets[1]->SetName(L"Updated RTV 1");
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        m_viewport.Width = width;
        m_viewport.Height = height;
        m_scissorRect.right = width;
        m_scissorRect.bottom = height;
    }

    // copy texture is dependent on RTV, so re-create it
    {
        m_renderTargetTexture.Reset();

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        CheckResult(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_renderTargetTexture)));

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        UINT offset = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 0, offset);
        srvHandle.Offset(offset);
        m_device->CreateShaderResourceView(m_renderTargetTexture.Get(), &srvDesc, srvHandle);

        m_renderTargetTexture->SetName(L"updated RTV copy texture");
    }
}

void Dx12RenderEngine::ToggleFullscreen()
{
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
