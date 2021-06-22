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

//void PipelineState::Init(Dx12RenderEngine* pEngine)
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
            //D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            //D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            //D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        pEngine->CreateRootSignature(&rootSignatureDesc, &m_pRootSignature);
        //ComPtr<ID3DBlob> signature;
        //ComPtr<ID3DBlob> error;
        //CheckResult(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        //CheckResult(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }


    // create this pipeline's heaps
    {
        //// descriptor heaps
        //{
            // RTV descriptor heap
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = 1;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            CheckResult(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRtvHeap)));

            //m_rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            // SRV descriptor heap
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.NumDescriptors = 2;
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            CheckResult(pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvHeap)));
        //}
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

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        CheckResult(m_pConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pConstandBufferDataDataBegin)));
        //memcpy(m_pConstandBufferDataDataBegin, m_pConstantBufferData, sizeof(m_constantBufferDataSize));
    }


    // upload heap (committed resource) for generic usage
    {
        constexpr uint uploadBufferSize = 8*1024*1024;
        CheckResult(pDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_pUploadBuffer)));

        // keep this buffer persistently mapped
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

    // resources
    {
        // render target
        {
            D3D12_RESOURCE_DESC desc = {};
            desc.Width = 800;
            desc.Height = 800;
            desc.DepthOrArraySize = 1;
            desc.SampleDesc.Count = 1;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            //desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            // create resource for render target
            CheckResult(pDevice->CreateCommittedResource(
                        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
                        //&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, m_width, m_height),
                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                        nullptr,
                        IID_PPV_ARGS(&m_pRenderTarget)));

            // create render target view
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
            pDevice->CreateRenderTargetView(m_pRenderTarget.Get(), nullptr, rtvHandle);
        }
    }



    // copy the vertex data to GPU. TODO: this should use mesh data, and also be mutable
    {
        // Define the geometry for a triangle.
        struct Vertex
        {
            XMFLOAT3 position;
            //XMFLOAT4 color;
            XMFLOAT3 color;
        };
        //Vertex triangleVertices[] =
        //{
        //    { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, } },
        //    { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, } },
        //    { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, } }
        //};
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
        short triangleIndices[] =
        {
            0, 1, 2, 0, 2, 3,
            4, 6, 5, 4, 7, 6,
            4, 5, 1, 4, 1, 0,
            3, 2, 6, 3, 6, 7,
            1, 5, 6, 1, 6, 2,
            4, 0, 3, 4, 3, 7
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);
        const UINT indexBufferSize = sizeof(triangleIndices);

        memcpy(m_pUploadBufferBegin, triangleVertices, vertexBufferSize);
        m_vertexBufferView.BufferLocation = m_pUploadBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        m_pUploadBufferEnd = m_pUploadBufferBegin + vertexBufferSize;
        m_uploadBufferOffset = vertexBufferSize;

        memcpy(m_pUploadBufferEnd, triangleIndices, indexBufferSize);
        m_indexBufferView.BufferLocation = m_pUploadBuffer->GetGPUVirtualAddress() + vertexBufferSize;
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;
    }


    static Shader vertexShader, pixelShader;
    vertexShader.Compile("shaders.hlsl", "VSMain", "vs_6_0");
    pixelShader.Compile("shaders.hlsl", "PSMain", "ps_6_0");

    // vertex input layout
    static D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        //{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
    //psoDesc.RTVFormats[0]                   = DXGI_FORMAT_R32G32B32A32_FLOAT;
    psoDesc.SampleDesc.Count                = 1;

    CheckResult(pEngine->CreatePipelineState(&psoDesc, &m_pPipelineState));




    m_mesh.LoadFromFile("media/cube.obj");





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
    }
}



void PipelineState::SetShader(std::string file, ShaderKind type, std::string entry)
{

}




void PipelineState::Render()
{
    CheckResult(m_pCommandAllocator->Reset());
    CheckResult(m_pCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState.Get()));

    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_pSrvHeap.Get() };
    //m_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_pCommandList->SetGraphicsRootConstantBufferView(0, m_pConstantBuffer->GetGPUVirtualAddress());
    m_pCommandList->RSSetViewports(1, &m_viewport);
    m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

    // specify render target(s)
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
    m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    if (m_pClearColor == nullptr) m_pCommandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
    else                          m_pCommandList->ClearRenderTargetView(rtvHandle, m_pClearColor, 0, nullptr);
    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_pCommandList->IASetIndexBuffer(&m_indexBufferView);
    m_pCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

    CheckResult(m_pCommandList->Close());
}




void PipelineState::SetConstantBufferData(void* pData, uint dataSize)
{
    m_pConstantBufferData = pData;
    m_constantBufferDataSize = dataSize;

    memcpy(m_pConstandBufferDataDataBegin, m_pConstantBufferData, m_constantBufferDataSize);
}
