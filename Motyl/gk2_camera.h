#ifndef __GK2_CAMERA_H_
#define __GK2_CAMERA_H_

#include <d3d11.h>
#include <xnamath.h>

namespace gk2
{
	class Camera
	{
	public:
		Camera(float minDistance, float maxDistance, float distance =  0.0f);

		void SetRange(float minDistance, float maxDistance);
		void Zoom(float d);
		void Rotate(float dx, float dy);
		void RotateHorizontally(float dx);
		void RotateVertically(float dy);
		XMMATRIX GetViewMatrix();
		void GetViewMatrix(XMMATRIX& viewMatrix);
		XMFLOAT4 GetPosition();
		void UpdatePosition(XMVECTOR& offset);
		void BuildView();

		XMVECTOR camPosition;
		XMVECTOR camTarget;
		XMVECTOR camUp;

	private:
		float m_angleX;
		float m_angleY;
		float m_distance;
		float m_minDistance;
		float m_maxDistance;
		XMVECTOR lastCamTarget;
		XMMATRIX camView;
		XMMATRIX matrixRot;

		void ClampDistance();
		void calculateTargetVector();
	};
}

#endif __GK2_CAMERA_H_
