#pragma once

#include <map>
#include <string>
#include <vector>

#include "Dx12RenderEngine.h"
#include "Mesh.h"
#include "Util.h"
#include "Util3D.h"


enum DrawableType
{
    StaticMeshDrawable,
    UnknownDrawable
};

static const char* DrawableTypeStrings[]
{
    "Static Mesh",
    "Unknown"
};

struct alignas(256) MeshConstants
{
    XMFLOAT4X4 modelMatrix;             // transform in world-space
};
struct alignas(256) GlobalConstants
{
    XMFLOAT4X4 viewMatrix;              // camera view
    XMFLOAT4X4 projectionMatrix;        // perspective projection
};

// contains per-drawable (per-instance) data
struct Drawable
{
    bool            shouldDraw;         // should this item be drawn?
    TransformData   transformData;      // first level of model/world transformation
    DrawableType    drawableType;       // is this a static mesh? point cloud? product of a mesh shader?
    uint            drawableID;         // unique identifer among all drawables
    union
    {
        uint        meshID;             // only support static meshes for now
    };
};

// data for static meshes
struct MeshBufferViews
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;  // triangle vertices
    D3D12_VERTEX_BUFFER_VIEW colorBufferView;   // per-vertex colors
    D3D12_VERTEX_BUFFER_VIEW normalBufferView;  // per-vertex normals
    D3D12_INDEX_BUFFER_VIEW  indexBufferView;   // triangle indices
};


// TODO: UI
// TODO: support for non-static geometry
// TODO: add instanced meshes which can make use of instanced draws (as opposed to the soft instancing from Drawables)
class GeometryManager
{
public:
    GeometryManager();
    ~GeometryManager();

    void Init();
    void BuildUI();

    uint AddMesh(std::string filename, bool addDrawable=true);

    // TODO: replace vectors with maps or lists to allow removal
    std::vector<Drawable>* GetDrawables()                   {return &m_drawables;}
    Drawable GetDrawable(uint index)                        {return m_drawables[index];}
    std::vector<Mesh>* GetMeshes()                          {return &m_Meshes;}
    Mesh* GetMesh(uint index)                               {return &m_Meshes[index];}
    std::vector<MeshBufferViews>* GetMeshBufferViews()      {return &m_meshBufferViews;}
    MeshBufferViews GetMeshBufferView(uint index)           {return m_meshBufferViews[index];}
    ID3D12Resource* GetConstantBufferResource()             {return m_pConstantBuffer.Get();}
    uint64 GetConstantBufferOffset(uint index)              {return m_pConstantBuffer->GetGPUVirtualAddress() + 256*index;}

protected:
    uint AddDrawable(Drawable drawable);
    MeshBufferLayout RegisterAndUploadMesh(Mesh& newMesh);


    // identifiers
    uint                                m_drawableCounter;      // generates unique IDs for each drawable instance added
    uint                                m_meshCounter;          // same, but just for meshes

    // lists of various geometry types
    std::vector<Drawable>               m_drawables;            // per-instance geometry data
    std::vector<Mesh>                   m_Meshes;               // CPU-side mesh representations

    // constant buffer for per-mesh data
    ComPtr<ID3D12Resource>              m_pConstantBuffer;
    UINT8*                              m_pConstantBufferDataDataBegin;
    MeshConstants                       m_meshConstants[64];

    // upload buffer for shipping geometry data to GPU
    ComPtr<ID3D12Resource>              m_pUploadBuffer;        // generic CPU->GPU uploads
    uint                                m_uploadBufferOffset;   // offset to next free spot in upload buffer
    UINT8*                              m_pUploadBufferBegin;   // start of mapped region
    UINT8*                              m_pUploadBufferEnd;     // end of last added element

    // GPU memory management and views to feed to pipelines
    ComPtr<ID3D12Resource>              m_pGeometryBuffer;      // committed resource for scene geometry data
    uint                                m_geometryBufferOffset; // offset to next free spot
    std::vector<MeshBufferViews>        m_meshBufferViews;      // buffer locations and offsets for per-vertex data
};
