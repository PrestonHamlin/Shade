#pragma once

#include "Util.h"

#include <map>

//#undef min
//#undef max

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <DirectXMath.h>


enum MeshFileFormat
{
    UnknownFormat,
    OBJ,
};

struct MeshBufferLayout
{
    uint vertexOffset;
    uint vertexSize;
    uint colorOffset;
    uint colorSize;
    uint normalOffset;
    uint normalSize;
    uint facesOffset;
    uint facesSize;
    uint totalSize;
};


class Mesh
{
public:
    Mesh();
    Mesh(std::string filename);
    Mesh(Mesh& other);
    ~Mesh();

    // main functionality
    HRESULT LoadFromFile(std::string filename);
    void Unload();
    MeshBufferLayout PopulateGeometryBuffer(void* pBuffer);

    // setters/getters/queries
    const aiScene* GetScene() const {return m_pScene;}
    const uint GetNumVertices(uint index=0) const {return m_pScene->mMeshes[index]->mNumVertices;}
    const uint GetNumFaces(uint index=0) const {return m_pScene->mMeshes[index]->mNumFaces;}
    bool IsValidMesh() const { return m_isValidMesh; }

private:
    static std::map<std::string, MeshFileFormat> FileExtensionMap;
    static Assimp::Importer m_importer;
    aiScene* m_pScene;

    bool m_isValidMesh;
    std::string m_filename;
};
