#pragma once

#include "Util.h"

#include <map>

//#undef min
//#undef max

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


enum MeshFileFormat
{
    UnknownFormat,
    OBJ,
};


struct ObjVertex
{
    float x;
    float y;
    float z;
};

class Mesh
{
public:

    Mesh();
    Mesh(std::string filename);
    ~Mesh();

    HRESULT LoadFromFile(std::string filename);
    //HRESULT LoadFromFile(std::string filename, MeshFileFormat format = UnknownFormat);
    //HRESULT LoadFromFileObj(std::filesystem::path filepath);
    void Unload();

    //MeshFileFormat DetermineMeshFileFormat(std::experimental::filesystem::path filepath);
    //MeshFileFormat DetermineMeshFileFormat(std::filesystem::path filepath);

    bool IsValidMesh() const { return m_isValidMesh; }

private:
    static std::map<std::string, MeshFileFormat> FileExtensionMap;
    Assimp::Importer m_importer;

    bool m_isValidMesh;
    std::string m_filename;
};

