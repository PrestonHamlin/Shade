#include "Mesh.h"

using namespace std;
using namespace std::filesystem;
using namespace Assimp;
//using namespace std::experimental::filesystem::v1;


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
    m_filename(filename),
    m_isValidMesh(false)
{
    LoadFromFile(filename);
}

Mesh::~Mesh()
{
    m_importer.FreeScene();
}





HRESULT Mesh::LoadFromFile(string filename)
{
    HRESULT result = S_OK;

    // verify file exists
    // TODO: offset relative paths from configured resource directory
    path filepath = path(filename);

    if (!exists(filepath))
    {
        PrintMessage(Error, "Cannot locate file \"{}\"", filepath.string());
        result = E_INVALIDARG;
    }
    else
    {
        //const aiScene* pScene = m_importer.ReadFile(filepath.string(), 0);
        //const aiScene* pScene = m_importer.ReadFile(filepath.string(), aiProcess_MakeLeftHanded);
        const uint flags = aiProcess_MakeLeftHanded | aiProcess_JoinIdenticalVertices;
        //m_importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_MATERIALS);
        m_pScene = const_cast<aiScene*>(m_importer.ReadFile(filepath.string(), flags));
        if (m_pScene == nullptr)
        {
            PrintMessage(Error, "assimp failed to load {}\n\t\t{}", filepath.filename().string(), m_importer.GetErrorString());
            result = E_FAIL;
        }
        else
        {
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


            //pScene->num
            m_isValidMesh = true;
            result = S_OK;
        }


        // TODO: actually load file contents...
        // TODO: error propagation on bad file contents
    }

    return result;
}


const void* Mesh::GetVertexData()
{
    HRESULT result = S_OK;

    return static_cast<void*>(m_pScene->mMeshes[0]->mVertices);
}


void Mesh::Unload()
{
    m_importer.FreeScene();

    m_isValidMesh = false;
}
