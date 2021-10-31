#include "PipelineState.h"

#include <dxgi.h>

// initialize pipeline ID counter
uint PipelineState::m_pipelineIdCounter = 0;


PipelineState::PipelineState()
    :
    m_pipelineId(m_pipelineIdCounter++),
    m_viewport(0.0f, 0.0f, 800, 800),
    m_scissorRect(0, 0, 800, 800)
{
}
PipelineState::PipelineState(PipelineCreateInfo createInfo)
    :
    m_pipelineId(m_pipelineIdCounter++),
    m_viewport(0.0f, 0.0f, 800, 800),
    m_scissorRect(0, 0, 800, 800)
{
    Init(createInfo);
}
PipelineState::~PipelineState()
{
}

void PipelineState::Init(PipelineCreateInfo createInfo)
{
    Dx12RenderEngine* pEngine = Dx12RenderEngine::pCurrentEngine;
    auto* pDevice = pEngine->GetDevice();

    pEngine->CreateCommandAllocator(&m_pCommandAllocator);
    pEngine->CreateCommandList(&m_pCommandList);

    // create root signature
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        CD3DX12_ROOT_PARAMETER1 rootParameters[3];
        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);  // global
        rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);  // per-drawable
        rootParameters[2].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        pEngine->CreateRootSignature(&rootSignatureDesc, &m_pRootSignature);
    }

    // create this pipeline's heaps and committed resources
    {
        // descriptor heaps
        {
            // RTV descriptor heap
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = 1;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            CheckResult(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRtvHeap)));

            // SRV descriptor heap
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.NumDescriptors = 2;
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            CheckResult(pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvHeap)));

            // DSV descriptor heap
            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
            dsvHeapDesc.NumDescriptors = 1;
            dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            CheckResult(pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDsvHeap)));
        }

        // constant buffer for per-frame data
        {
            const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto bufferProps = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);
            CheckResult(pDevice->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferProps,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pConstantBuffer)));

            CD3DX12_RANGE readRange(0, 0);
            CheckResult(m_pConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pConstandBufferDataDataBegin)));
        }

        // render target
        {
            D3D12_RESOURCE_DESC desc = {};
            desc.Width = 800;
            desc.Height = 800;
            desc.DepthOrArraySize = 1;
            desc.SampleDesc.Count = 1;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            // create resource for render target
            const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            CheckResult(pDevice->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                nullptr,
                IID_PPV_ARGS(&m_pRenderTarget)));

            // create render target view
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
            pDevice->CreateRenderTargetView(m_pRenderTarget.Get(), nullptr, rtvHandle);
        }

        // depth stencil
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
            depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
            depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

            const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            const auto texProps = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, 800, 800, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            const auto clearProps = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0.0f, 0);
            CheckResult(pDevice->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &texProps,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearProps,
                IID_PPV_ARGS(&m_pDepthStencil)
                ));

            pDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &depthStencilDesc, m_pDsvHeap->GetCPUDescriptorHandleForHeapStart());
        }
    }

    static Shader vertexShader, pixelShader;
    vertexShader.Compile("src/shaders.hlsl", "VSMain", "vs_6_0");
    pixelShader.Compile("src/shaders.hlsl", "PSMain", "ps_6_0");

    // IA layout for vertex buffers
    static D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    m_initialized = true;

    // describe and create the PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                     = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature                  = m_pRootSignature.Get();
    psoDesc.VS                              = CD3DX12_SHADER_BYTECODE(vertexShader.GetBlob());
    psoDesc.PS                              = CD3DX12_SHADER_BYTECODE(pixelShader.GetBlob());
    psoDesc.RasterizerState                 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState                      = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState               = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);;
    psoDesc.DSVFormat                       = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask                      = UINT_MAX;
    psoDesc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                = 1;
    psoDesc.RTVFormats[0]                   = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count                = 1;

    CheckResult(pEngine->CreatePipelineState(&psoDesc, &m_pPipelineState), "compiling pipeline", true);
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
    CheckResult(pEngine->CreatePipelineState(&psoDesc, &m_pPipelineStateReverseDepth), "compiling reverse-depth pipeline", true);
    m_compiled = true;

    // set debug names
    {
        std::string commonString = "Pipeline #" + std::to_string(m_pipelineId);
        SetDebugName(m_pCommandAllocator.Get(),             commonString + " command allocator");
        SetDebugName(m_pCommandList.Get(),                  commonString + " command list");

        SetDebugName(m_pRootSignature.Get(),                commonString + " root signature");
        SetDebugName(m_pRtvHeap.Get(),                      commonString + " RTV descriptor heap");
        SetDebugName(m_pSrvHeap.Get(),                      commonString + " SRV descriptor heap");
        SetDebugName(m_pPipelineState.Get(),                commonString + " PSO");
        SetDebugName(m_pPipelineStateReverseDepth.Get(),    commonString + " reverse-depth PSO");

        SetDebugName(m_pRenderTarget.Get(),                 commonString + " render target");
        SetDebugName(m_pConstantBuffer.Get(),               commonString + " constant buffer");
        SetDebugName(m_pDepthStencil.Get(),                 commonString + " depth stencil");
    }
}

void PipelineState::Render()
{
    CheckResult(m_pCommandAllocator->Reset());
    CheckResult(m_pCommandList->Reset(m_pCommandAllocator.Get(), m_reverseDepth ? m_pPipelineStateReverseDepth.Get() : m_pPipelineState.Get()));

    // specify resource layouts and bindings
    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_pSrvHeap.Get() };
    //m_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_pConstantBuffer->GetGPUVirtualAddress());

    // set rasterizer state
    m_pCommandList->RSSetViewports(1, &m_viewport);
    m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

    // specify and prep render target(s) and affiliated resources
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDsvHeap->GetCPUDescriptorHandleForHeapStart());
    m_pCommandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    m_pCommandList->ClearRenderTargetView(rtvHandle, (m_pClearColor == nullptr) ? m_clearColor : m_pClearColor, 0, nullptr);
    m_pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, m_reverseDepth ? 0.0 : 1.0f, 0, 0, nullptr);

    // issue draws
    DrawAllGeometry();

    // close command list in preparation for later execution
    CheckResult(m_pCommandList->Close());
}

void PipelineState::DrawAllGeometry()
{
    const auto begin = m_pGeometryManager->GetDrawables()->begin();
    const auto end = m_pGeometryManager->GetDrawables()->end();

    for (auto itr = begin; itr != end; ++itr)
    {
        if (!itr->shouldDraw) continue;

        switch (itr->drawableType)
        {
        case StaticMeshDrawable:
        {
            DrawStaticMesh(*itr);
            break;
        }
        case UnknownDrawable:
        default:
        {
            assert(false);
            break;
        }
        }
    }
}

void PipelineState::DrawStaticMesh(const Drawable& drawable)
{
    MeshBufferViews meshViews = m_pGeometryManager->GetMeshBufferView(drawable.meshID);

    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pCommandList->IASetVertexBuffers(0, 1, &meshViews.vertexBufferView);
    m_pCommandList->IASetVertexBuffers(1, 1, &meshViews.colorBufferView);
    m_pCommandList->IASetIndexBuffer(&meshViews.indexBufferView);
    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_pConstantBuffer->GetGPUVirtualAddress());
    m_pCommandList->SetGraphicsRootConstantBufferView(1, m_pGeometryManager->GetConstantBufferOffset(drawable.meshID));

    const uint numTriangles = m_pGeometryManager->GetMesh(drawable.meshID)->GetNumFaces()*3;
    m_pCommandList->DrawIndexedInstanced(numTriangles, 1, 0, 0, 0);
}

// immediately update constant buffer data and retain pointer to CPU memory
void PipelineState::SetConstantBufferData(CbvData data)
{
    m_pConstantBufferData    = data.pData;
    m_constantBufferDataSize = data.size;

    UpdateConstantBufferData();
}

void PipelineState::UpdateConstantBufferData()
{
    assert(m_pConstantBufferData != nullptr);
    memcpy(m_pConstandBufferDataDataBegin, m_pConstantBufferData, m_constantBufferDataSize);
}
