#ifndef __GK2_PUMA_H_
#define __GK2_PUMA_H_

#include "gk2_applicationBase.h"
#include "gk2_camera.h"
#include <xnamath.h>
#include <vector>
#include "gk2_vertices.h"
using namespace std;
namespace gk2
{
	class Puma : public gk2::ApplicationBase
	{
	public:
		Puma(HINSTANCE hInstance);
		virtual ~Puma();
		static void* operator new(size_t size);
		static void operator delete(void* ptr);
	protected:
		virtual bool LoadContent();
		virtual void UnloadContent();

		virtual void Update(float dt);
		virtual void Render();
	private:

		static const unsigned int VB_STRIDE;
		static const unsigned int VB_OFFSET;
		static const unsigned int BS_MASK;

		static const float LAP_TIME;

		gk2::Camera m_camera;

		XMMATRIX m_projMtx;
		XMMATRIX m_pumaMtx[6];

		std::shared_ptr<ID3D11VertexShader> m_vertexShader;
		std::shared_ptr<ID3D11PixelShader> m_pixelShader;
		std::shared_ptr<ID3D11InputLayout> m_inputLayout;


		std::shared_ptr<ID3D11Buffer> m_vbRoom;
		std::shared_ptr<ID3D11Buffer> m_ibRoom;


		std::shared_ptr<ID3D11Buffer> m_vbPlane;
		std::shared_ptr<ID3D11Buffer> m_ibPlane;

		std::shared_ptr<ID3D11Buffer> m_vbPuma[6];
		std::shared_ptr<ID3D11Buffer> m_ibPuma[6];
		vector<VertexPosNormal> vertices[6];
		int pumaIndicesCount[6];

		std::shared_ptr<ID3D11DepthStencilState> m_dssWrite;
		std::shared_ptr<ID3D11DepthStencilState> m_dssTest;
		std::shared_ptr<ID3D11RasterizerState> m_rsCounterClockwise;
		std::shared_ptr<ID3D11BlendState> m_bsAlpha;
		std::shared_ptr<ID3D11BlendState> m_bsAdd;

		//dane okrêgu
		XMFLOAT2 circleCenter = XMFLOAT2(-0.9f - 1.3f / 2.0f, -1.0f + 1.3f / 2.0f* sqrt(3));
		float circleRadius = 0.5f;

		std::shared_ptr<ID3D11Buffer> m_vbCircle;
		std::shared_ptr<ID3D11Buffer> m_ibCircle;
		VertexPosNormal *circleVertices;

		void inverse_kinematics(XMFLOAT3 pos, XMFLOAT3 normal, float &a1, float &a2,
			float &a3, float &a4, float &a5);

		std::shared_ptr<ID3D11Buffer> m_cbWorld;
		std::shared_ptr<ID3D11Buffer> m_cbView;
		std::shared_ptr<ID3D11Buffer> m_cbProj;
		std::shared_ptr<ID3D11Buffer> m_cbLightPos;
		std::shared_ptr<ID3D11Buffer> m_cbLightColors;
		std::shared_ptr<ID3D11Buffer> m_cbSurfaceColor;


		static const std::wstring ShaderFile;
		static const std::wstring PumaFiles[6];

		void InitializeShaders();
		void InitializeConstantBuffers();
		void InitializeRenderStates();
		void InitializeCamera();

		void InitializeRoom();
		void InitializePlane();
		void InitializePuma();
		void InitializeCircle();


		void UpdateCamera(const XMMATRIX& view);
		void UpdatePuma(float dt);
		void UpdateInput();

		void SetShaders();
		void SetConstantBuffers();

		void SetLight0();


		void DrawRoom();
		void DrawPlane(bool val = false);
		void DrawPuma();
		void DrawCircle();
		void DrawMirroredWorld();
	};
}


#endif __GK2_BUTTERFLY_H_