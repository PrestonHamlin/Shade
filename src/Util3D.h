// Util3D - Collection of utility functions and objects for 3D math.
#pragma once

#include <DirectXMath.h>

#include "Util.h"


// per-object scale/rotate/translation data
struct TransformData
{
    DirectX::XMFLOAT3 scale;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT3 translation;
    DirectX::XMMATRIX transformMatrix;
    bool              matrixDirty;

    TransformData();
    DirectX::XMMATRIX GetTransformMatrix();
};
