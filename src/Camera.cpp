#include "Camera.h"


Camera::Camera() :
    m_position({0.f, 0.f, 0.f}),
    m_direction({1.f, 0.f, 0.f}),
    m_up({0.f, 1.f, 0.f}),
    m_moveSpeed(0.1f),
    m_turnSpeed(0.1f),
    m_yaw(0.f),
    m_pitch(0.f),
    m_fieldOfView(45.0f),
    m_aspectRatio(1.0f),
    m_nearZ(0.01f),
    m_farZ(1000.0f)
{

}

Camera::~Camera()
{

}

void Camera::Update()
{
    // ensure valid values
    if (m_nearZ <= 0) m_nearZ = 0.001;
    if (m_farZ <= m_nearZ) m_farZ = m_nearZ + 0.01;

    // update look direction
    float r = cos(m_pitch);
    m_direction.x = r * sin(m_yaw);
    m_direction.y =     sin(m_pitch);
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
}

XMMATRIX Camera::GetProjectionMatrix()
{
    return m_reverseZ ? XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fieldOfView), m_aspectRatio, m_farZ, m_nearZ)
                      : XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fieldOfView), m_aspectRatio, m_nearZ, m_farZ);
}
