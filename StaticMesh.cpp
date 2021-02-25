#include "StaticMesh.h"

StaticMesh::StaticMesh()
{
    Mesh::Mesh();
}

StaticMesh::StaticMesh(std::string filename)
{
    Mesh::Mesh(filename);
}

StaticMesh::~StaticMesh()
{
    Mesh::~Mesh();
}
