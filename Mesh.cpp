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
: m_isValidMesh(false)
{
}

Mesh::Mesh(string filename)
: m_isValidMesh(false)
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
        const aiScene* pScene = m_importer.ReadFile(filepath.string(), aiProcess_MakeLeftHanded);
        if (pScene == nullptr)
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
                         pScene->mNumMeshes, pScene->mNumMaterials, pScene->mNumTextures,
                         pScene->mNumCameras, pScene->mNumLights, pScene->mNumAnimations);
            for (uint i=0; i<pScene->mNumMeshes; ++i)
            {
                aiMesh* pMesh = pScene->mMeshes[i];
                PrintMessage(Info,
                             "\t\"{}\" - {} vertices, {} faces, {} bones\n"
                             "\t\t{} color channels, {} UV channels, material #{}",
                             pMesh->mName.C_Str(), pMesh->mNumVertices, pMesh->mNumFaces, pMesh->mNumBones,
                             pMesh->GetNumColorChannels(), pMesh->GetNumUVChannels(), pMesh->mMaterialIndex
                             );
            }
            for (uint i = 0; i < pScene->mNumMaterials; ++i)
            {
                aiMaterial* pMaterial = pScene->mMaterials[i];
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


void Mesh::Unload()
{
    m_importer.FreeScene();

    m_isValidMesh = false;
}









//HRESULT Mesh::LoadFromFile(string filename, MeshFileFormat format)
//{
//    HRESULT result = S_OK;
//
//    // verify file exists
//    // TODO: offset relative paths from configured resource directory
//    path filepath = path(filename);
//
//    if (!exists(filepath))
//    {
//        PrintMessage(Error, "Cannot locate file \"{}\"", filepath.string());
//        result = E_INVALIDARG;
//    }
//    else
//    {
//        if (format == UnknownFormat)
//        {
//            format = DetermineMeshFileFormat(filepath);
//        }
//
//        switch (format)
//        {
//        case OBJ:
//            PrintMessage("Loading .obj file... ");
//            result = LoadFromFileObj(filepath);
//            PrintMessage("done!\n");
//            break;
//        case UnknownFormat:
//        default:
//            PrintMessage(Error, "Unhandled mesh file extension: {}", filepath.filename().string());
//            result = E_INVALIDARG;
//            break;
//        }
//
//        // TODO: actually load file contents...
//        // TODO: error propagation on bad file contents
//    }
//
//    return result;
//}


//HRESULT Mesh::LoadFromFileObj(std::filesystem::path filepath)
//{
//    HRESULT result = S_OK;
//    ifstream ifile(filepath);
//
//    if (!ifile)
//    {
//        char msg[80];
//        strerror_s(msg, errno);
//        //PrintMessage(Error, "Cannot open file {} - {%s}", filepath, strerror(errno));
//        PrintMessage(Error, "Cannot open file {} - {}", filepath.string(), msg);
//        result = E_FAIL;
//    }
//    else
//    {
//        string line;
//
//        getline(ifile, line);
//    }
//
//    return result;
//}


//MeshFileFormat Mesh::DetermineMeshFileFormat(path filepath)
//{
//    string extension = filepath.extension().string();
//
//    // TODO: exceptions
//    return FileExtensionMap[extension];
//
//    // TODO: Attempt to determine format from contents? Probably not.
//}


