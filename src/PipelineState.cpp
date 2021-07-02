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
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];
        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

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
        }

        // constant buffer for per-frame data
        {
            CheckResult(pDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pConstantBuffer)));

            CD3DX12_RANGE readRange(0, 0);
            CheckResult(m_pConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pConstandBufferDataDataBegin)));
        }
        // upload heap (committed resource) for generic usage
        {
            constexpr uint uploadBufferSize = 32*1024*1024;
            CheckResult(pDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pUploadBuffer)));

            CD3DX12_RANGE readRange(0, 0);
            CheckResult(m_pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pUploadBufferBegin)));
            m_pUploadBufferEnd = m_pUploadBufferBegin;
            m_uploadBufferOffset = 0;
        }
        // default heap (committed resource) for geometry data
        {
            constexpr uint geometryBufferSize = 8*1024*1024;
            CheckResult(pDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(geometryBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pGeometryBuffer)));
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
            CheckResult(pDevice->CreateCommittedResource(
                        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                        nullptr,
                        IID_PPV_ARGS(&m_pRenderTarget)));

            // create render target view
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
            pDevice->CreateRenderTargetView(m_pRenderTarget.Get(), nullptr, rtvHandle);
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
    psoDesc.DepthStencilState.DepthEnable   = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask                      = UINT_MAX;
    psoDesc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                = 1;
    psoDesc.RTVFormats[0]                   = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count                = 1;

    CheckResult(pEngine->CreatePipelineState(&psoDesc, &m_pPipelineState), "compiling pipeline", true);
    m_compiled = true;

    // set debug names
    {
        std::string commonString = "Pipeline #" + std::to_string(m_pipelineId);
        SetDebugName(m_pCommandAllocator.Get(), commonString + " command allocator");
        SetDebugName(m_pCommandList.Get(),      commonString + " command list");

        SetDebugName(m_pRootSignature.Get(),    commonString + " root signature");
        SetDebugName(m_pRtvHeap.Get(),          commonString + " RTV descriptor heap");
        SetDebugName(m_pSrvHeap.Get(),          commonString + " SRV descriptor heap");
        SetDebugName(m_pPipelineState.Get(),    commonString + " PSO");

        SetDebugName(m_pRenderTarget.Get(),     commonString + " render target");
        SetDebugName(m_pUploadBuffer.Get(),     commonString + " upload buffer");
        SetDebugName(m_pGeometryBuffer.Get(),   commonString + " geometry buffer");
        SetDebugName(m_pConstantBuffer.Get(),   commonString + " constant buffer");
    }
}

// TODO: Maintain vector of meshes? Or just some layout metadata for draw instructions?
void PipelineState::AddMesh(Mesh* pMesh)
{
    m_meshes.push_back(pMesh);

    // TODO: programattically control residency of meshes
    MeshBufferLayout layout = pMesh->PopulateGeometryBuffer(m_pUploadBufferBegin);
    m_pUploadBufferEnd += layout.totalSize;

    // this should be done per draw
    m_vertexBufferView.BufferLocation = m_pUploadBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(aiVector3D);
    m_vertexBufferView.SizeInBytes = layout.vertexSize;

    m_colorBufferView.BufferLocation = m_pUploadBuffer->GetGPUVirtualAddress() + layout.colorOffset;
    m_colorBufferView.StrideInBytes = sizeof(aiColor4D);
    m_colorBufferView.SizeInBytes = layout.colorSize;

    m_normalBufferView.BufferLocation = m_pUploadBuffer->GetGPUVirtualAddress() + layout.normalOffset;
    m_normalBufferView.StrideInBytes = sizeof(aiVector3D);
    m_normalBufferView.SizeInBytes = layout.normalSize;

    m_indexBufferView.BufferLocation = m_pUploadBuffer->GetGPUVirtualAddress() + layout.facesOffset;
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_indexBufferView.SizeInBytes = layout.facesSize;

    PrintMessage("\nVertex Buffer: {}B @ {}"
                 "\nColor Buffer:  {}B @ {}"
                 "\nNormal Buffer: {}B @ {}"
                 "\nIndex Buffer:  {}B @ {}"
                 "\n",
                 m_vertexBufferView.SizeInBytes, m_vertexBufferView.BufferLocation,
                 m_colorBufferView.SizeInBytes, m_colorBufferView.BufferLocation,
                 m_normalBufferView.SizeInBytes, m_normalBufferView.BufferLocation,
                 m_indexBufferView.SizeInBytes, m_indexBufferView.BufferLocation
                 );
}

void PipelineState::Render()
{
    CheckResult(m_pCommandAllocator->Reset());
    CheckResult(m_pCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState.Get()));

    // specify resource layouts and bindings
    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_pSrvHeap.Get() };
    //m_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_pConstantBuffer->GetGPUVirtualAddress());

    // set rasterizer state
    m_pCommandList->RSSetViewports(1, &m_viewport);
    m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

    // specify and prep render target(s)
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
    m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    if (m_pClearColor == nullptr) m_pCommandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
    else                          m_pCommandList->ClearRenderTargetView(rtvHandle, m_pClearColor, 0, nullptr);

    // configure Input Assembler
    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_pCommandList->IASetVertexBuffers(1, 1, &m_colorBufferView);
    m_pCommandList->IASetIndexBuffer(&m_indexBufferView);

    // issue draw
    m_pCommandList->DrawIndexedInstanced(m_meshes[0]->GetNumFaces()*3, 1, 0, 0, 0);

    // close command list in preparation for later execution
    CheckResult(m_pCommandList->Close());
}

// immediately update constant buffer data and retain pointer to CPU memory
void PipelineState::SetConstantBufferData(CbvData data)
{
    m_pConstantBufferData    = data.pData;
    m_constantBufferDataSize = data.size;

    memcpy(m_pConstandBufferDataDataBegin, m_pConstantBufferData, m_constantBufferDataSize);
}
