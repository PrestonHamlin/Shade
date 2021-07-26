#include "Mesh.h"

using namespace std;
using namespace std::filesystem;
using namespace Assimp;
using namespace DirectX;


//Assimp::Importer Mesh::m_importer = Assimp::Importer();
Assimp::Importer Mesh::m_importer;

map<string, MeshFileFormat> Mesh::FileExtensionMap = {
    {"",        UnknownFormat},
    {".obj",    OBJ},
};


Mesh::Mesh()
    :
    m_isValidMesh(false)
{
}

Mesh::Mesh(string filename)
    :
    m_isValidMesh(false)
{
    LoadFromFile(filename);
}

Mesh::Mesh(Mesh& other)
{
    m_filename    = other.m_filename;
    m_pScene      = other.m_pScene;
    m_isValidMesh = other.m_isValidMesh;

    other.m_pScene      = nullptr;
    other.m_isValidMesh = false;
}

Mesh::~Mesh()
{
    if (m_isValidMesh)
    {
        m_importer.FreeScene();
    }

    if (m_pScene != nullptr)
    {
        delete m_pScene;
    }
}

HRESULT Mesh::LoadFromFile(string filename)
{
    HRESULT result = S_OK;

    // verify file exists
    // TODO: offset relative paths from configured resource directory
    // TODO: error propagation on bad file contents
    path filepath = path(filename);

    if (!exists(filepath))
    {
        PrintMessage(Error, "Cannot locate file \"{}\"", filepath.string());
        result = E_INVALIDARG;
    }
    else
    {
        const uint flags = aiProcess_JoinIdenticalVertices | aiProcess_Triangulate;
        //m_importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_MATERIALS); // strip materials
        m_pScene = const_cast<aiScene*>(m_importer.ReadFile(filepath.string(), flags));
        if (m_pScene == nullptr)
        {
            PrintMessage(Error, "assimp failed to load {}\n\t\t{}", filepath.filename().string(), m_importer.GetErrorString());
            result = E_FAIL;
        }
        else
        {
            // TODO: use dynamically allocated storage
            m_pScene = m_importer.GetOrphanedScene(); // detach scene from importer
            PrintMessage(Info,
                         "{} loaded:\n"
                         "\t{} meshes, {} materials, {} textures\n"
                         "\t{} cameras, {} lights, {} animations",
                         filepath.string(),
                         m_pScene->mNumMeshes, m_pScene->mNumMaterials, m_pScene->mNumTextures,
                         m_pScene->mNumCameras, m_pScene->mNumLights, m_pScene->mNumAnimations);
            for (uint i=0; i<m_pScene->mNumMeshes; ++i)
            {
                aiMesh* pMesh = m_pScene->mMeshes[i];
                PrintMessage(Info,
                             "\t\"{}\" - {} vertices, {} faces, {} bones\n"
                             "\t\t{} color channels, {} UV channels, material #{}",
                             pMesh->mName.C_Str(), pMesh->mNumVertices, pMesh->mNumFaces, pMesh->mNumBones,
                             pMesh->GetNumColorChannels(), pMesh->GetNumUVChannels(), pMesh->mMaterialIndex
                             );
            }
            for (uint i = 0; i < m_pScene->mNumMaterials; ++i)
            {
                aiMaterial* pMaterial = m_pScene->mMaterials[i];
                PrintMessage(Info,
                             "\t\"{}\" - {} properties\n",
                             pMaterial->GetName().C_Str(), pMaterial->mNumProperties
                );
                for (uint j = 0; j < pMaterial->mNumProperties; ++j)
                {
                    aiMaterialProperty* pProperty = pMaterial->mProperties[j];
                    PrintMessage(Info,
                                 "\t\t{} - {}",
                                 j, pProperty->mKey.C_Str()
                                 );
                }
            }

            m_isValidMesh = true;
            m_filename = filename;
            result = S_OK;
        }
    }

    return result;
}

void Mesh::Unload()
{
    m_importer.FreeScene();

    m_isValidMesh = false;
}

MeshBufferLayout Mesh::PopulateGeometryBuffer(void* pBuffer)
{
    HRESULT result = S_OK;
    MeshBufferLayout layout = {};
    uint* writePointer = (uint*)pBuffer;
    uint totalOffset = 0;

    // copy vertex data
    for (uint i = 0; i < m_pScene->mNumMeshes; ++i)
    {
        aiMesh* pMesh =  m_pScene->mMeshes[i];

        // vertices
        {
            layout.vertexSize = pMesh->mNumVertices * sizeof(aiVector3D);
            layout.vertexOffset = totalOffset;

            memcpy(writePointer, pMesh->mVertices, layout.vertexSize);
            writePointer += pMesh->mNumVertices * 3;
            totalOffset += layout.vertexSize;
        }

        //per-vertex colors
        if (pMesh->HasVertexColors(0))
        {
            layout.colorSize = pMesh->mNumVertices * sizeof(aiColor4D);
            layout.colorOffset = totalOffset;

            memcpy(writePointer, pMesh->mColors[i], layout.colorSize);
            writePointer += pMesh->mNumVertices * 4;
            totalOffset += layout.colorSize;
        }
        else
        {
            layout.colorSize = pMesh->mNumVertices * sizeof(aiColor4D);
            layout.colorOffset = totalOffset;

            for (uint j = 0; j < pMesh->mNumVertices; ++j)
            {
                float color[4] = { rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX), 1.0f };
                memcpy(writePointer, color, sizeof(aiColor4D));
                writePointer += 4;
            }

            totalOffset += layout.colorSize;
        }

        //if (pMesh->HasNormals())
        //{
        //    layout.normalSize = pMesh->mNumVertices * sizeof(aiVector3D);
        //    layout.normalOffset = totalOffset;
        //    //memcpy(writePointer, &test, sizeof(test));
        //    memcpy(writePointer, pMesh->mNormals, layout.normalSize);
        //    writePointer += pMesh->mNumVertices * 3;
        //    totalOffset += layout.normalSize;
        //}

        // linearize index buffer to notate faces
        if (pMesh->HasFaces())
        {
            layout.facesOffset = totalOffset;

            for (uint j = 0; j < pMesh->mNumFaces; ++j)
            {
                memcpy(writePointer, pMesh->mFaces[j].mIndices, sizeof(uint)*3);
                writePointer += 3;
                layout.facesSize += sizeof(uint)*3;
            }
            totalOffset += layout.facesSize;
        }
    }

    layout.totalSize = layout.vertexSize + layout.colorSize +
                       layout.normalSize + layout.normalOffset;
    PrintMessage("\n=== Offsets ==="
                 "\nVertices:   {}"
                 "\nColors:     {}"
                 "\nNormals:    {}"
                 "\nFaces:      {}"
                 "\n",
                 layout.vertexOffset, layout.colorOffset, layout.normalOffset, layout.facesOffset);
    PrintMessage("\n=== Sizes ==="
                 "\nVertices:   {}"
                 "\nColors:     {}"
                 "\nNormals:    {}"
                 "\nFaces:      {}"
                 "\nTOTAL:      {}"
                 "\n",
                 layout.vertexSize, layout.colorSize, layout.normalSize, layout.facesSize, layout.totalSize);

    return layout;
}
