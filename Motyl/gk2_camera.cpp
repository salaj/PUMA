#include "gk2_camera.h"

using namespace gk2;

Camera::Camera(float minDistance, float maxDistance, float distance)
	: m_angleX(0.0f), m_angleY(0.0f), m_distance(distance)
{
	SetRange(minDistance, maxDistance);
	m_position = XMFLOAT3(0, 0, 10);
}

void Camera::ClampDistance()
{
	if (m_distance < m_minDistance)
		m_distance = m_minDistance;
	if (m_distance > m_maxDistance)
		m_distance = m_maxDistance;
}

void Camera::SetRange(float minDistance, float maxDistance)
{
	if (minDistance < 0)
		minDistance = 0;
	if (maxDistance < minDistance)
		maxDistance = minDistance;
	m_minDistance = minDistance;
	m_maxDistance = maxDistance;
	ClampDistance();
}

void Camera::Zoom(float d)
{
	m_distance += d;
	ClampDistance();
}

void Camera::Rotate(float dx, float dy)
{
	m_angleX = XMScalarModAngle(m_angleX + dx);
	m_angleY = XMScalarModAngle(m_angleY + dy);
}

void Camera::RotateHorizontally(float dy)
{
	float val = m_angleY + dy;
	float clamped;
	if (val < -XM_PI)
		clamped = -XM_PI;
	else if (val > XM_PI)
		clamped = XM_PI;
	else
		clamped = val;
	m_angleY = clamped;
}

void Camera::RotateVertically(float dx)
{
	float val = m_angleX + dx;
	float clamped;
	if (val < -XM_PIDIV2)
		clamped = -XM_PIDIV2;
	else if (val > XM_PIDIV2)
		clamped = XM_PIDIV2;
	else
		clamped = val;
	m_angleX = clamped;
}

XMMATRIX Camera::GetViewMatrix()
{
	XMMATRIX viewMtx;
	GetViewMatrix(viewMtx);
	return viewMtx;
}

void Camera::GetViewMatrix(XMMATRIX& viewMtx)
{
	viewMtx = XMMatrixRotationX(m_angleX) * XMMatrixRotationY(-m_angleY) *
		XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
}

void Camera::UpdatePosition(XMFLOAT3& offset)
{
	m_position.x += offset.x;
	m_position.y += offset.y;
	m_position.z += offset.z;
}

XMFLOAT4 Camera::GetPosition()
{
	XMMATRIX viewMtx;
	GetViewMatrix(viewMtx);
	XMVECTOR det;
	viewMtx = XMMatrixInverse(&det, viewMtx);
	XMMATRIX alt = XMMatrixTranslation(0.0f, 0.0f, -m_distance) * XMMatrixRotationY(m_angleY) * XMMatrixRotationX(-m_angleX);
	XMFLOAT3 res(0.0f, 0.0f, 0.0f);
	XMVECTOR transl = XMVector3TransformCoord(XMLoadFloat3(&res), viewMtx);
	XMStoreFloat3(&res, transl);
	return XMFLOAT4(res.x, res.y, res.z, 1.0f);
}