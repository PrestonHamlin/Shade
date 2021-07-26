#include "Util3D.h"

using namespace DirectX;


XMMATRIX TransformData::GetTransformMatrix()
{
    if (matrixDirty)
    {
        XMMATRIX scaleMatrix       = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX rotationMatrix    = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        XMMATRIX translationMatrix = XMMatrixTranslation(translation.x, translation.y, translation.z);
        transformMatrix            = scaleMatrix * rotationMatrix * translationMatrix;
        matrixDirty                = false;
    }

    return transformMatrix;
}

TransformData::TransformData()
{
    scale           = XMFLOAT3(1, 1, 1);
    rotation        = XMFLOAT3(0, 0, 0);
    translation     = XMFLOAT3(0, 0, 0);
    transformMatrix = {};
    matrixDirty     = true;
}
