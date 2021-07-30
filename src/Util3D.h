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
    void UpdateTransformMatrix();
    DirectX::XMMATRIX GetTransformMatrix();
    DirectX::XMMATRIX GetTransformMatrixT();
    void GetTransformMatrix(void* pDest);
    void GetTransformMatrixT(void* pDest);
    void StoreTransformMatrix(void* pDest);
    void StoreTransformMatrixT(void* pDest);

};
