#include "Camera.h"


Camera::Camera() :
    m_position({0.f, 0.f, 0.f}),
    m_direction({1.f, 0.f, 0.f}),
    m_up({0.f, 1.f, 0.f}),
    m_moveSpeed(0.1f),
    m_turnSpeed(0.1f),
    m_yaw(0.f),
    m_pitch(0.f),
    m_FieldOfView(45.0f),
    m_aspectRatio(1.0f),
    m_minZ(0.0f),
    m_maxZ(1.0f)
{

}

Camera::~Camera()
{

}

void Camera::Update()
{
    // update look direction
    float r = cos(m_pitch);
    m_direction.x = r * sin(m_yaw);
    m_direction.y =     sin(m_pitch);
    m_direction.z = r * cos(m_yaw);
}

void Camera::Move(XMFLOAT3 displacement)
{
    m_position.x += displacement.x;
    m_position.y += displacement.y;
    m_position.z += displacement.z;
}
void Camera::Move(float x, float y, float z)
{
    m_position.x += x;
    m_position.y += y;
    m_position.z += z;
}

void Camera::Rotate(XMFLOAT3 angles)
{
    XMMATRIX rotations = XMMatrixRotationX(angles.x) *
                         XMMatrixRotationY(angles.y) *
                         XMMatrixRotationZ(angles.z);

}
void Camera::Rotate(float alpha, float beta, float gamma)
{
    XMMATRIX rotations = {};
    if (alpha != 0.0f) rotations *= XMMatrixRotationX(alpha);
    if (beta != 0.0f)  rotations *= XMMatrixRotationX(beta);
    if (gamma != 0.0f) rotations *= XMMatrixRotationX(gamma);
}

void Camera::YawRight(uint ticks)
{
    m_yaw += ticks*m_turnSpeed;
}
void Camera::YawLeft(uint ticks)
{
    m_yaw -= ticks*m_turnSpeed;
}
void Camera::PitchUp(uint ticks)
{
    m_pitch += ticks*m_turnSpeed;
}
void Camera::PitchDown(uint ticks)
{
    m_pitch -= ticks*m_turnSpeed;
}

XMMATRIX Camera::GetViewMatrix()
{
    return XMMatrixLookToLH(XMLoadFloat3(&m_position), XMLoadFloat3(&m_direction), XMLoadFloat3(&m_up));
    //return XMMatrixLookToRH(XMLoadFloat3(&m_position), XMLoadFloat3(&m_direction), XMLoadFloat3(&m_up));
}
XMMATRIX Camera::GetProjectionMatrix()
{
    //return XMMatrixPerspectiveFovLH(45.0f, 1.0f, 0.1f, 100.0f);
    return XMMatrixPerspectiveFovLH(m_FieldOfView, m_aspectRatio, m_minZ, m_maxZ);
}
