#include "Util3D.h"

using namespace DirectX;


TransformData::TransformData()
{
    scale           = XMFLOAT3(1, 1, 1);
    rotation        = XMFLOAT3(0, 0, 0);
    translation     = XMFLOAT3(0, 0, 0);
    transformMatrix = {};
    matrixDirty     = true;
}

void TransformData::UpdateTransformMatrix()
{
    XMMATRIX scaleMatrix       = XMMatrixScaling(scale.x, scale.y, scale.z);
    XMMATRIX rotationMatrix    = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
    XMMATRIX translationMatrix = XMMatrixTranslation(translation.x, translation.y, translation.z);
    transformMatrix            = scaleMatrix * rotationMatrix * translationMatrix;
    matrixDirty                = false;
}

XMMATRIX TransformData::GetTransformMatrix()
{
    if (matrixDirty) UpdateTransformMatrix();

    return transformMatrix;
}

XMMATRIX TransformData::GetTransformMatrixT()
{
    if (matrixDirty) UpdateTransformMatrix();

    return XMMatrixTranspose(transformMatrix);
}

void TransformData::GetTransformMatrix(void* pDest)
{
    if (matrixDirty) UpdateTransformMatrix();

    memcpy(pDest, &transformMatrix, sizeof(transformMatrix));
}

void TransformData::GetTransformMatrixT(void* pDest)
{
    if (matrixDirty) UpdateTransformMatrix();

    XMMATRIX transposed = XMMatrixTranspose(transformMatrix);
    memcpy(pDest, &transposed, sizeof(transposed));
}

void TransformData::StoreTransformMatrix(void* pDest)
{
    if (matrixDirty) UpdateTransformMatrix();

    XMStoreFloat4x4((XMFLOAT4X4*)pDest, transformMatrix);
}

void TransformData::StoreTransformMatrixT(void* pDest)
{
    if (matrixDirty) UpdateTransformMatrix();

    XMStoreFloat4x4((XMFLOAT4X4*)pDest, XMMatrixTranspose(transformMatrix));
}
