#include "gk2_camera.h"

using namespace gk2;

Camera::Camera(float minDistance, float maxDistance, float distance)
	: m_angleX(0.0f), m_angleY(0.0f), m_distance(distance)
{
	SetRange(minDistance, maxDistance);
	camPosition = XMVectorSet(0.0f, 2.0f, 2.0f, 0.0f);
	camTarget = XMVectorSet(0.0f, 1.0f, 1.0f, 0.0f);
	camUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	lastCamTarget = camTarget;
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
	RotateHorizontally(dy);
	RotateVertically(-dx);
	calculateTargetVector();
}

void Camera::RotateHorizontally(float dy)
{
	float val = m_angleY + dy;
	float clamped;
	if (val < -XM_PIDIV2)
		clamped = -XM_PIDIV2;
	else if (val > XM_PIDIV2)
		clamped = XM_PIDIV2;
	else
		clamped = val;
	m_angleY = clamped;

}

void Camera::calculateTargetVector()
{
	matrixRot = XMMatrixRotationAxis(XMVector3Cross(lastCamTarget, camUp), m_angleY)*
		XMMatrixRotationAxis(camUp, m_angleX);
	camTarget = XMVector3TransformCoord(lastCamTarget, matrixRot);
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

void Camera::BuildView()
{
	camView = XMMatrixLookToLH(camPosition, camTarget, camUp);
}


XMMATRIX Camera::GetViewMatrix()
{
	XMMATRIX viewMtx;
	GetViewMatrix(viewMtx);
	return viewMtx;
}

void Camera::GetViewMatrix(XMMATRIX& viewMtx)
{
	BuildView();
	viewMtx = camView;
}

void Camera::UpdatePosition(XMVECTOR& offset)
{
	float factor = 0.01f;
	offset *= factor;
	camPosition += offset;
	lastCamTarget = camTarget;
	m_angleX = 0;
	m_angleY = 0;
}

XMFLOAT4 Camera::GetPosition()
{
	return XMFLOAT4(
		XMVectorGetX(camPosition),
		XMVectorGetY(camPosition), 
		XMVectorGetZ(camPosition), 
		1.0f);
}