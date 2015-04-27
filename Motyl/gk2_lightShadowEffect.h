#ifndef __GK2_LIGHT_SHADOW_EFFECT_H_
#define __GK2_LIGHT_SHADOW_EFFECT_H_

#include "gk2_effectBase.h"

namespace gk2
{
	class LightShadowEffect : public EffectBase
	{
	public:
		LightShadowEffect(gk2::DeviceHelper& device, std::shared_ptr<ID3D11InputLayout>& layout,
					std::shared_ptr<ID3D11DeviceContext> context = nullptr);

		static void* operator new(size_t size);
		static void operator delete(void* ptr);

		void SetLightPosBuffer(const std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>>& lightPos);
		void SetSurfaceColorBuffer(const std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>>& surfaceColor);

		XMMATRIX UpdateLight(float dt, std::shared_ptr<ID3D11DeviceContext> context);
		void SetupShadow(const std::shared_ptr<ID3D11DeviceContext>& context);
		void EndShadow();

	protected:
		virtual void SetVertexShaderData();
		virtual void SetPixelShaderData();

	private:
		static const std::wstring ShaderFile;

		static const float LIGHT_NEAR;
		static const float LIGHT_FAR;
		static const float LIGHT_ANGLE;
		static const int TEXTURE_SIZE;

		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_lightPosCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_surfaceColorCB;
		std::shared_ptr<gk2::CBMatrix> m_mapMtxCB;
		std::shared_ptr<ID3D11SamplerState> m_samplerState;

		std::shared_ptr<ID3D11ShaderResourceView> m_lightMap;
		std::shared_ptr<ID3D11Texture2D> m_shadowMap;
		std::shared_ptr<ID3D11DepthStencilView> m_shadowMapDepthView;
		std::shared_ptr<ID3D11ShaderResourceView> m_shadowMapView;

		XMMATRIX m_lightProjMtx;
		XMMATRIX m_lightViewMtx;

		void InitializeTextures(gk2::DeviceHelper& device);
	};
}

#endif __GK2_LIGHT_SHADOW_EFFECT_H_
