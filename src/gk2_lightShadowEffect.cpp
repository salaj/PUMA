#include "gk2_lightShadowEffect.h"
#include "gk2_vertices.h"
#include "gk2_utils.h"

using namespace std;
using namespace gk2;

const wstring LightShadowEffect::ShaderFile = L"resources/shaders/LightShadow.hlsl";
const int LightShadowEffect::TEXTURE_SIZE = 1024;
const float LightShadowEffect::LIGHT_NEAR = 0.35f;
const float LightShadowEffect::LIGHT_FAR = 10.5f;
const float LightShadowEffect::LIGHT_ANGLE = XM_PI / 3.0f;

void* LightShadowEffect::operator new(size_t size)
{
	return Utils::New16Aligned(size);
}

void LightShadowEffect::operator delete(void* ptr)
{
	Utils::Delete16Aligned(ptr);
}

LightShadowEffect::LightShadowEffect(DeviceHelper& device, shared_ptr<ID3D11InputLayout>& layout,
	shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
	: EffectBase(context)
{
	Initialize(device, layout, ShaderFile);
	InitializeTextures(device);
}

void LightShadowEffect::InitializeTextures(DeviceHelper& device)
{
	m_lightMap = device.CreateShaderResourceView(L"resources/textures/light_cookie.png");
	m_mapMtxCB.reset(new CBMatrix(device));
	D3D11_SAMPLER_DESC sd = device.DefaultSamplerDesc();
	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	m_samplerState = device.CreateSamplerState(sd);

	D3D11_TEXTURE2D_DESC texDesc = device.DefaultTexture2DDesc();

	//TODO: setup shadow map texture's width, height, format, mipLevels and bindflags
	texDesc.Width = texDesc.Height = TEXTURE_SIZE;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.MipLevels = 1;
	m_shadowMap = device.CreateTexture2D(texDesc);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = device.DefaultDepthStencilViewDesc();

	//TODO: setup shadow map depth-stencil view's format
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	m_shadowMapDepthView = device.CreateDepthStencilView(m_shadowMap, dsvDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = device.DefaultShaderResourceDesc();

	//TODO: setup shadow map shader resource view's format and mip levels
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	m_shadowMapView = device.CreateShaderResourceView(m_shadowMap, srvDesc);
}

XMMATRIX LightShadowEffect::UpdateLight(float dt, shared_ptr<ID3D11DeviceContext> context)
{
	m_context = context;
	static float time = 0;
	time += dt;
	float swing = 0.3f * XMScalarSin(XM_2PI*time / 8);
	float rot = XM_2PI*time / 20;

/*
	XMMATRIX lamp = XMMatrixTranslation(0.0f, -0.4f, 0.0f) * XMMatrixRotationX(swing) * XMMatrixRotationY(rot) *
		XMMatrixTranslation(0.0f, 2.5f, 0.0f);
	XMFLOAT4 lp(-1.0f, 1.5f, 0.0f, 1.0f);
	XMVECTOR lightPosition = XMVector3TransformCoord(XMLoadFloat4(&lp), lamp);*/

	XMFLOAT4 lp(-3.0f, 3.5f, -2.0f, 1.0f);
	XMVECTOR lightPosition = XMVectorSet(-3.0f, 3.5f, -2.0f, 1.0f);
	XMStoreFloat4(&lp, lightPosition);
	m_lightPosCB->Update(m_context, lp);
	XMFLOAT4 lt(0, -10.0f, 0.0f, 1.0f);
	XMVECTOR lightTarget = XMVectorSet(0, 0.0f, 0.0f, 1.0f);
	XMVECTOR up;
	up.m128_f32[0] = 1.0f;
	m_lightProjMtx = XMMatrixPerspectiveFovLH(LIGHT_ANGLE, 1.0, LIGHT_NEAR, LIGHT_FAR);
	m_lightViewMtx = XMMatrixLookAtLH(lightPosition, lightTarget, up);

	//TODO: Replace with correct implementation.
	XMMATRIX texMtx = XMMatrixIdentity();
	texMtx = m_lightViewMtx * m_lightProjMtx * XMMatrixScaling(0.5f, -0.5f, 1.0f) * XMMatrixTranslation(0.5f, 0.5f, -0.00001f);

	m_mapMtxCB->Update(m_context, texMtx);
	return XMMATRIX();
}

void LightShadowEffect::SetupShadow(const shared_ptr<ID3D11DeviceContext>& context)
{
	if (context != nullptr && context != m_context)
		m_context = context;
	if (m_context == nullptr)
		return;

	m_viewCB->Update(m_context, m_lightViewMtx);
	m_projCB->Update(m_context, m_lightProjMtx);
	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(TEXTURE_SIZE);
	viewport.Height = static_cast<float>(TEXTURE_SIZE);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	m_context->RSSetViewports(1, &viewport);
	m_context->OMSetRenderTargets(0, nullptr, m_shadowMapDepthView.get());
	m_context->ClearDepthStencilView(m_shadowMapDepthView.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void LightShadowEffect::EndShadow()
{

}

void LightShadowEffect::SetLightPosBuffer(const shared_ptr<ConstantBuffer<XMFLOAT4>>& lightPos)
{
	if (lightPos != nullptr)
		m_lightPosCB = lightPos;
}

void LightShadowEffect::SetSurfaceColorBuffer(const shared_ptr<ConstantBuffer<XMFLOAT4>>& surfaceColor)
{
	if (surfaceColor != nullptr)
		m_surfaceColorCB = surfaceColor;
}

void LightShadowEffect::SetVertexShaderData()
{
	ID3D11Buffer* vsb[4] = { m_worldCB->getBufferObject().get(), m_viewCB->getBufferObject().get(),
		m_projCB->getBufferObject().get(), m_lightPosCB->getBufferObject().get() };
	m_context->VSSetConstantBuffers(0, 4, vsb);
}

void LightShadowEffect::SetPixelShaderData()
{
	ID3D11SamplerState* ss[1] = { m_samplerState.get() };
	m_context->PSSetSamplers(0, 1, ss);
	ID3D11ShaderResourceView* srv[2] = { m_lightMap.get(), m_shadowMapView.get() };
	m_context->PSSetShaderResources(0, 2, srv);
	ID3D11Buffer* psb[3] = { m_surfaceColorCB->getBufferObject().get(), m_mapMtxCB->getBufferObject().get(),
		m_lightPosCB->getBufferObject().get() };
	m_context->PSSetConstantBuffers(0, 3, psb);
}
