#pragma once

#include <DirectXMath.h>

#include "Util.h"

using namespace DirectX;


class Camera
{
public:
    Camera::Camera();
    Camera::~Camera();

    void Update();

    // translations
    void Move(XMFLOAT3 displacement);
    void Move(float x, float y, float z);

    // rotations
    void Rotate(XMFLOAT3 angles);
    void Rotate(float alpha, float beta, float gamma);

    void YawLeft(uint ticks=1);
    void YawRight(uint ticks=1);
    void PitchUp(uint ticks=1);
    void PitchDown(uint ticks=1);

    void IncreaseMinZ() {m_minZ += 0.01;}
    void DecreaseMinZ() {m_minZ -= 0.01;}
    void IncreaseMaxZ() {m_maxZ += 0.01;}
    void DecreaseMaxZ() {m_maxZ -= 0.01;}

    XMMATRIX GetViewMatrix();
    XMMATRIX GetProjectionMatrix();

    void SetPosition(XMFLOAT3 pos)      {m_position = pos;}
    void SetDirection(XMFLOAT3 dir)     {m_direction = dir;}    // note this is different from "look at"
    XMFLOAT3 GetPosition()              {return m_position;}
    XMFLOAT3 GetDirection()             {return m_direction;}
    XMFLOAT3 GetUpVector()              {return m_up;}
    float GetMinZ()                     {return m_minZ;}
    float GetMaxZ()                     {return m_maxZ;}

private:
    XMFLOAT3                            m_position;
    XMFLOAT3                            m_direction;
    XMFLOAT3                            m_up;
    float                               m_pitch;                // angle relative to XZ plane
    float                               m_yaw;                  // angle about +Z axis
    float                               m_moveSpeed;
    float                               m_turnSpeed;
    float                               m_FieldOfView;
    float                               m_aspectRatio;
    float                               m_minZ;
    float                               m_maxZ;
};
