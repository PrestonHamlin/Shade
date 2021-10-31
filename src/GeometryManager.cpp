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

    // constant buffer for per-mesh per-frame data
    {
        const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto bufferProps = CD3DX12_RESOURCE_DESC::Buffer(256 * 64);
        CheckResult(pDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferProps,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_pConstantBuffer)));

        CD3DX12_RANGE readRange(0, 0);
        CheckResult(m_pConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pConstantBufferDataDataBegin)));
    }

    // upload heap (committed resource) for generic usage
    {
        constexpr uint uploadBufferSize = 32*1024*1024;
        const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto bufferProps = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
        CheckResult(pDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferProps,
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
        const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto bufferProps = CD3DX12_RESOURCE_DESC::Buffer(geometryBufferSize);
        CheckResult(pDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferProps,
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

void GeometryManager::BuildUI()
{
    ImGui::Begin("Geometry Manager");
    ImGui::Text("Meshes: %d", m_Meshes.size());

    // direct access is useful for ImGui, and we don't need accessor functions if the geometry manager draws itself
    for (Drawable& drawable : m_drawables) // TODO: re-batch
    {
        bool dirty = false;
        ImGui::PushID(drawable.meshID);
        ImGui::Separator();
        if (ImGui::DragFloat3("Scale", &drawable.transformData.scale.x, 1.0, -100, 100, "%.3f", ImGuiSliderFlags_Logarithmic))
        {
            dirty = true;
        }
        if (ImGui::DragFloat3("Rotation", &drawable.transformData.rotation.x, 1.0, -1000, 1000, "%.3f", ImGuiSliderFlags_Logarithmic))
        {
            dirty = true;
        }
        if (ImGui::DragFloat3("Translation", &drawable.transformData.translation.x, 1.0, -1000, 1000, "%.3f", ImGuiSliderFlags_Logarithmic))
        {
            dirty = true;
        }
        if (dirty)
        {
            drawable.transformData.matrixDirty = true;
            drawable.transformData.StoreTransformMatrixT(PointerByteIncrement(m_pConstantBufferDataDataBegin, 256*drawable.meshID));
        }
        ImGui::PopID();
    }
    ImGui::End();
}


uint GeometryManager::AddMesh(string filename, bool addDrawable)
{
    Mesh* pMesh = new Mesh(filename);
    m_Meshes.push_back(pMesh);
    RegisterAndUploadMesh(pMesh);

    // we should only upload each mesh once unless there is an explicit AddMesh call for the same file
    uint meshCount = m_Meshes.size();
    uint meshUploadCount = m_meshBufferViews.size();
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

        drawable.transformData.StoreTransformMatrix(&m_meshConstants[meshCount].modelMatrix);
        drawable.transformData.StoreTransformMatrixT(PointerByteIncrement(m_pConstantBufferDataDataBegin, 256*drawable.meshID));
    }

    return meshCount;
}

uint GeometryManager::AddDrawable(Drawable drawable)
{
    m_drawables.push_back(drawable);
    return drawable.drawableID;
}

MeshBufferLayout GeometryManager::RegisterAndUploadMesh(Mesh* pMesh)
{
    MeshBufferLayout layout = pMesh->PopulateGeometryBuffer((void*)m_pUploadBufferEnd);

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
    newMeshViews.colorBufferView.StrideInBytes  = sizeof(aiColor4D);
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
    m_meshBufferViews.push_back(newMeshViews);

    return layout;
}
