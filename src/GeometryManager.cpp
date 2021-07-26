#include "GeometryManager.h"

using namespace std;


GeometryManager::GeometryManager() :
    m_drawableCounter(0),
    m_meshCounter(0),
    m_pUploadBuffer(nullptr),
    m_uploadBufferOffset(0),
    m_pUploadBufferBegin(nullptr),
    m_pUploadBufferEnd(nullptr),
    m_pGeometryBuffer(nullptr),
    m_geometryBufferOffset(0)
{
}
GeometryManager::~GeometryManager()
{
}

void GeometryManager::Init()
{
    Dx12RenderEngine* pEngine = Dx12RenderEngine::pCurrentEngine;
    auto* pDevice = pEngine->GetDevice();

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

    // set debug names
    {
        string commonString = "Geometry Manager";
        SetDebugName(m_pUploadBuffer.Get(),     commonString + " upload buffer");
        SetDebugName(m_pGeometryBuffer.Get(),   commonString + " geometry buffer");
    }
}

uint GeometryManager::AddMesh(string filename, bool addDrawable)
{
    Mesh& newMesh = m_Meshes.emplace_back(filename);
    RegisterAndUploadMesh(newMesh);

    // we should only upload each mesh once unless there is an explicit AddMesh call for the same file
    uint meshCount = m_Meshes.size();
    uint meshUploadCount = m_MeshBufferViews.size();
    assert(meshCount == meshUploadCount);

    if (addDrawable)
    {
        Drawable drawable = {};
        drawable.shouldDraw     = true;
        drawable.drawableType   = StaticMeshDrawable;
        drawable.drawableID     = m_drawableCounter++;
        drawable.meshID         = m_meshCounter++;
        drawable.transformData  = TransformData();
        AddDrawable(drawable);
    }

    return meshCount;
}

uint GeometryManager::AddDrawable(Drawable drawable)
{
    m_drawables.push_back(drawable);
    return drawable.drawableID;
}

MeshBufferLayout GeometryManager::RegisterAndUploadMesh(Mesh& newMesh)
{
    MeshBufferLayout layout = newMesh.PopulateGeometryBuffer((void*)m_pUploadBufferBegin);

    MeshBufferViews newMeshViews = {};
    auto baseOffset = m_pUploadBuffer->GetGPUVirtualAddress() + m_uploadBufferOffset;
    newMeshViews.vertexBufferView.BufferLocation = baseOffset + layout.vertexOffset;
    newMeshViews.colorBufferView.BufferLocation  = baseOffset + layout.colorOffset;
    newMeshViews.normalBufferView.BufferLocation = baseOffset + layout.normalOffset;
    newMeshViews.indexBufferView.BufferLocation  = baseOffset + layout.facesOffset;

    newMeshViews.vertexBufferView.SizeInBytes = layout.vertexSize;
    newMeshViews.colorBufferView.SizeInBytes  = layout.colorSize;
    newMeshViews.normalBufferView.SizeInBytes = layout.normalSize;
    newMeshViews.indexBufferView.SizeInBytes  = layout.facesSize;

    newMeshViews.vertexBufferView.StrideInBytes = sizeof(aiVector3D);
    newMeshViews.colorBufferView.StrideInBytes  =  sizeof(aiColor4D);
    newMeshViews.normalBufferView.StrideInBytes = sizeof(aiVector3D);
    newMeshViews.indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

    PrintMessage("\nVertex Buffer: {}B @ {}"
                 "\nColor Buffer:  {}B @ {}"
                 "\nNormal Buffer: {}B @ {}"
                 "\nIndex Buffer:  {}B @ {}"
                 "\n",
                 newMeshViews.vertexBufferView.SizeInBytes, newMeshViews.vertexBufferView.BufferLocation,
                 newMeshViews.colorBufferView.SizeInBytes, newMeshViews.colorBufferView.BufferLocation,
                 newMeshViews.normalBufferView.SizeInBytes, newMeshViews.normalBufferView.BufferLocation,
                 newMeshViews.indexBufferView.SizeInBytes, newMeshViews.indexBufferView.BufferLocation
                 );

    m_pUploadBufferEnd += layout.totalSize;
    m_uploadBufferOffset += layout.totalSize;
    m_MeshBufferViews.push_back(newMeshViews);

    return layout;
}
