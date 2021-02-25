#pragma once

#include "Mesh.h"

class StaticMesh : public Mesh
{
    StaticMesh();
    StaticMesh(std::string filename);
    ~StaticMesh();
};

