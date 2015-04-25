#include "gk2_puma.h"
#include "gk2_utils.h"
#include "gk2_vertices.h"
#include "gk2_window.h"
#include <fstream>
#include <iostream>

using namespace std;
using namespace gk2;



#define RESOURCES_PATH L"resources/"
const wstring Puma::ShaderFile = RESOURCES_PATH L"shaders/Puma.hlsl";
const wstring Puma::PumaFiles[6] = {
	RESOURCES_PATH L"puma/mesh1.txt",
	RESOURCES_PATH L"puma/mesh2.txt",
	RESOURCES_PATH L"puma/mesh3.txt",
	RESOURCES_PATH L"puma/mesh4.txt",
	RESOURCES_PATH L"puma/mesh5.txt",
	RESOURCES_PATH L"puma/mesh6.txt"
};


const unsigned int Puma::VB_STRIDE = sizeof(VertexPosNormal);
const unsigned int Puma::VB_OFFSET = 0;
const unsigned int Puma::BS_MASK = 0xffffffff;


const float Puma::LAP_TIME = 10.0f;

void* Puma::operator new(size_t size)
{
	return Utils::New16Aligned(size);
}

void Puma::operator delete(void* ptr)
{
	Utils::Delete16Aligned(ptr);
}

Puma::Puma(HINSTANCE hInstance)
	: ApplicationBase(hInstance), m_camera(0.01f, 100.0f)
{

}

Puma::~Puma()
{

}

void Puma::InitializeShaders()
{
	shared_ptr<ID3DBlob> vsByteCode = m_device.CompileD3DShader(ShaderFile, "VS_Main", "vs_4_0");
	shared_ptr<ID3DBlob> psByteCode = m_device.CompileD3DShader(ShaderFile, "PS_Main", "ps_4_0");
	m_vertexShader = m_device.CreateVertexShader(vsByteCode);
	m_pixelShader = m_device.CreatePixelShader(psByteCode);
	m_inputLayout = m_device.CreateInputLayout<VertexPosNormal>(vsByteCode);
	vsByteCode = m_device.CompileD3DShader(ShaderFile, "VS_Bilboard", "vs_4_0");
	psByteCode = m_device.CompileD3DShader(ShaderFile, "PS_Bilboard", "ps_4_0");
	/*
	m_vsBilboard = m_device.CreateVertexShader(vsByteCode);
	m_psBilboard = m_device.CreatePixelShader(psByteCode);
	m_ilBilboard = m_device.CreateInputLayout<VertexPos>(vsByteCode);
	*/
}

void Puma::InitializeConstantBuffers()
{
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = sizeof(XMMATRIX);
	desc.Usage = D3D11_USAGE_DEFAULT;
	m_cbWorld = m_device.CreateBuffer(desc);
	m_cbProj = m_device.CreateBuffer(desc);
	desc.ByteWidth = sizeof(XMMATRIX) * 2;
	m_cbView = m_device.CreateBuffer(desc);
	desc.ByteWidth = sizeof(XMFLOAT4) * 3;
	m_cbLightPos = m_device.CreateBuffer(desc);
	desc.ByteWidth = sizeof(XMFLOAT4) * 5;
	m_cbLightColors = m_device.CreateBuffer(desc);
	desc.ByteWidth = sizeof(XMFLOAT4);
	m_cbSurfaceColor = m_device.CreateBuffer(desc);
}

void Puma::InitializeRenderStates()
{
	D3D11_DEPTH_STENCIL_DESC dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.StencilEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_dssWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	m_dssTest = m_device.CreateDepthStencilState(dssDesc);

	D3D11_RASTERIZER_DESC rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.FrontCounterClockwise = true;
	m_rsCounterClockwise = m_device.CreateRasterizerState(rsDesc);

	D3D11_BLEND_DESC bsDesc = m_device.DefaultBlendDesc();
	bsDesc.RenderTarget[0].BlendEnable = true;
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bsDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bsDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bsDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	m_bsAlpha = m_device.CreateBlendState(bsDesc);
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_COLOR;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	m_bsAdd = m_device.CreateBlendState(bsDesc);
}

void Puma::InitializeCamera()
{
	SIZE s = getMainWindow()->getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_context->UpdateSubresource(m_cbProj.get(), 0, 0, &m_projMtx, 0, 0);
	m_camera.Zoom(7);

	UpdateCamera(m_camera.GetViewMatrix());
}


void Puma::InitializeRoom()
{
	float size = 10.0f;
	VertexPosNormal vertices[] =
	{
		//Front face
		{ XMFLOAT3(-size, -1.0f, -size), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(size, -1.0f, -size), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(size, size, -size), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-size, size, -size), XMFLOAT3(0.0f, 0.0f, 1.0f) },

		//Left face
		{ XMFLOAT3(-size, -1.0f, -size), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-size, size, -size), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-size, size, size), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-size, -1.0f, size), XMFLOAT3(1.0f, 0.0f, 0.0f) },

		//Bottom face
		{ XMFLOAT3(-size, -1.0f, -size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-size, -1.0f, size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(size, -1.0f, size), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(size, -1.0f, -size), XMFLOAT3(0.0f, 1.0f, 0.0f) },

		//Back face
		{ XMFLOAT3(-size, -1.0f, size), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-size, size, size), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(size, size, size), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(size, -1.0f, size), XMFLOAT3(0.0f, 0.0f, -1.0f) },

		//Right face
		{ XMFLOAT3(size, -1.0f, -size), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(size, -1.0f, size), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(size, size, size), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(size, size, -size), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

		//Top face
		{ XMFLOAT3(-size, size, -size), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(size, size, -size), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(size, size, size), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-size, size, size), XMFLOAT3(0.0f, -1.0f, 0.0f) },
	};
	m_vbRoom = m_device.CreateVertexBuffer(vertices, 24);
	unsigned short indices[] =
	{
		0, 1, 2, 0, 2, 3,		//Front face
		4, 5, 6, 4, 6, 7,		//Left face
		8, 9, 10, 8, 10, 11,	//Botton face
		12, 13, 14, 12, 14, 15,	//Back face
		16, 17, 18, 16, 18, 19,	//Right face
		20, 21, 22, 20, 22, 23	//Top face
	};
	m_ibRoom = m_device.CreateIndexBuffer(indices, 36);
}

void Puma::InitializePlane()
{
	float size = 3.0f;
	float sizeZ = 2.0f;
	VertexPosNormal vertices[] =
	{
		{ XMFLOAT3(-0.9f, -1.0f, -sizeZ), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-0.9f, -1.0f, sizeZ), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-0.9f - (size / 2.0f), (size / 2.0f)* sqrt(3) - 1.0f, -sizeZ), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-0.9f - (size / 2.0f), (size / 2.0f)* sqrt(3) - 1.0f, sizeZ), XMFLOAT3(1.0f, 0.0f, 0.0f) }
	};
	m_vbPlane = m_device.CreateVertexBuffer(vertices, 8);
	unsigned short indices[] =
	{
		0, 1, 2, 1, 3, 2,
		0, 2, 1, 1, 2, 3
	};
	m_ibPlane = m_device.CreateIndexBuffer(indices, 12);
}

void Puma::InitializePuma()
{
	for (int i = 0; i < 6; i++)
	{
		ifstream file;
		file.open(PumaFiles[i]);
		int differentVertexPositionCount = 0;
		int differentVertexCount = 0;
		int trianglesCount = 0;
		vector<XMFLOAT3> vertexes;
		vector<unsigned short> indices;

		file >> differentVertexPositionCount;
		for (int j = 0; j < differentVertexPositionCount; j++)
		{
			float x, y, z;
			file >> x >> y >> z;
			vertexes.push_back(XMFLOAT3(x, y, z));
		}

		file >> differentVertexCount;
		for (int j = 0; j < differentVertexCount; j++)
		{
			int ind;
			float x, y, z;
			file >> ind >> x >> y >> z;
			VertexPosNormal v;
			v.Pos = vertexes[ind];
			v.Normal = XMFLOAT3(x, y, z);
			vertices[i].push_back(v);
		}

		file >> trianglesCount;
		for (int j = 0; j < trianglesCount; j++)
		{
			unsigned short ind1, ind2, ind3;
			file >> ind1 >> ind2 >> ind3;
			indices.push_back(ind1);
			indices.push_back(ind2);
			indices.push_back(ind3);
		}

		m_vbPuma[i] = m_device.CreateVertexBuffer(&vertices[i][0], differentVertexCount);
		m_ibPuma[i] = m_device.CreateIndexBuffer(&indices[0], 3 * trianglesCount);
		pumaIndicesCount[i] = 3 * trianglesCount;
		m_pumaMtx[i] = XMMatrixIdentity();
	}
}

void Puma::InitializeCircle()
{
	//ilosc punktow przyblizaj�cych okrag
	int verticesAmount = 360;
	circleVertices = new VertexPosNormal[verticesAmount];

	//k�t o jaki b�d� obracane punkty
	float delta = XM_2PI / (float)verticesAmount;

	//wyznaczanie punkt�w
	for (int t = 0; t < verticesAmount; t++)
	{

		XMVECTOR pos = XMLoadFloat3(
			&XMFLOAT3(
			circleRadius * cos(t * delta),
			circleRadius * sin(t * delta),
			0));

		pos = XMVector3Transform(pos, XMMatrixRotationY(XM_PIDIV2));
		pos = XMVector3Transform(pos, XMMatrixRotationZ(XM_PI / 6.0f));
		pos = XMVector3Transform(pos, XMMatrixTranslation(circleCenter.x, circleCenter.y, 0.0f));
	
		circleVertices[t].Pos = XMFLOAT3(XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos));
		circleVertices[t].Normal = XMFLOAT3(0.5f, sqrt(3) / 2.0f, 0.0f);
	}

	m_vbCircle = m_device.CreateVertexBuffer(circleVertices, verticesAmount);

	int indicesAmount = verticesAmount * 2;
	unsigned short* indices = new unsigned short[indicesAmount];
	int counter = 0;
	for (int i = 0; i < indicesAmount - 2; i += 2)
	{
		indices[i] = counter++;
		indices[i + 1] = counter;
	}
	indices[indicesAmount - 2] = counter;
	indices[indicesAmount - 1] = 0;

	m_ibCircle = m_device.CreateIndexBuffer(indices, indicesAmount);

	//delete[] vertices;
}

void Puma::SetShaders()
{
	m_context->VSSetShader(m_vertexShader.get(), 0, 0);
	m_context->PSSetShader(m_pixelShader.get(), 0, 0);
	m_context->IASetInputLayout(m_inputLayout.get());
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Puma::SetConstantBuffers()
{
	ID3D11Buffer* vsb[] = { m_cbWorld.get(), m_cbView.get(), m_cbProj.get(), m_cbLightPos.get() };
	m_context->VSSetConstantBuffers(0, 4, vsb);
	ID3D11Buffer* psb[] = { m_cbLightColors.get(), m_cbSurfaceColor.get() };
	m_context->PSSetConstantBuffers(0, 2, psb);
}

bool Puma::LoadContent()
{
	InitializeShaders();
	InitializeConstantBuffers();
	InitializeRenderStates();
	InitializeCamera();

	InitializeRoom();
	InitializePuma();
	InitializePlane();
	InitializeCircle();

	SetShaders();
	SetConstantBuffers();

	return true;
}

void Puma::UnloadContent()
{
	m_vertexShader.reset();
	m_pixelShader.reset();
	m_inputLayout.reset();



	m_vbRoom.reset();
	m_ibRoom.reset();


	m_cbWorld.reset();
	m_cbView.reset();
	m_cbProj.reset();
	m_cbLightPos.reset();
	m_cbLightColors.reset();
	m_cbSurfaceColor.reset();
}

void Puma::UpdateCamera(const XMMATRIX& view)
{
	XMMATRIX viewMtx[2];
	viewMtx[0] = view;
	XMVECTOR det;
	viewMtx[1] = XMMatrixInverse(&det, viewMtx[0]);
	m_context->UpdateSubresource(m_cbView.get(), 0, 0, viewMtx, 0, 0);
}

void Puma::SetLight0()
//Setup one positional light at the camera
{
	XMFLOAT4 positions[3];
	ZeroMemory(positions, sizeof(XMFLOAT4) * 3);
	positions[0] = m_camera.GetPosition();
	//positions[1] = XMFLOAT4(-2, -2, -2, 1);//m_camera.GetPosition();
	//positions[2] = XMFLOAT4(0, 0, -10, 1);//m_camera.GetPosition();
	m_context->UpdateSubresource(m_cbLightPos.get(), 0, 0, positions, 0, 0);

	XMFLOAT4 colors[5];
	ZeroMemory(colors, sizeof(XMFLOAT4) * 5);
	colors[0] = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f); //ambient color
	colors[1] = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f); //surface [ka, kd, ks, m]
	colors[2] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //light0 color
	//colors[3] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //light0 color
	//colors[4] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //light0 color
	m_context->UpdateSubresource(m_cbLightColors.get(), 0, 0, colors, 0, 0);
}

void Puma::DrawRoom()
{
	const XMMATRIX worldMtx = XMMatrixIdentity();
	m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);

	ID3D11Buffer* b = m_vbRoom.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibRoom.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(36, 0, 0);
}

void Puma::DrawPlane(bool val)
{
	const XMMATRIX worldMtx = XMMatrixIdentity();
	m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);
	if (val)
	{
		m_context->UpdateSubresource(m_cbSurfaceColor.get(), 0, 0, &XMFLOAT4(1, 1, 1, 0.5f), 0, 0);
	}
	ID3D11Buffer* b = m_vbPlane.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibPlane.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(12, 0, 0);
}
void Puma::DrawPuma()
{
	for (int i = 0; i < 6; i++)
	{
		const XMMATRIX worldMtx = m_pumaMtx[i];
		m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);

		ID3D11Buffer* b = m_vbPuma[i].get();
		m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
		m_context->IASetIndexBuffer(m_ibPuma[i].get(), DXGI_FORMAT_R16_UINT, 0);
		m_context->DrawIndexed(pumaIndicesCount[i], 0, 0);
	}
}
void Puma::DrawCircle()
{
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	const XMMATRIX worldMtx = XMMatrixIdentity();
	m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);

	ID3D11Buffer* b = m_vbCircle.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibCircle.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(720, 0, 0);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Puma::DrawMirroredWorld()
{
	//Setup render state for writing to the stencil buffer
	m_context->OMSetDepthStencilState(m_dssWrite.get(), 1);
	//Draw the i-th face
	m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &XMMatrixIdentity(), 0, 0);
	ID3D11Buffer* b = m_vbPlane.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibPlane.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(12, 0, 0);

	//Setup render state and view matrix for rendering the mirrored world
	m_context->OMSetDepthStencilState(m_dssTest.get(), 1);


	XMMATRIX scale = XMMatrixScaling(1, 1, -1);
	XMVECTOR det;
	XMMATRIX m_mirrorMtx;
	XMMATRIX m = XMMatrixRotationY(XM_PIDIV2) * XMMatrixRotationZ(XM_PI / 6) * XMMatrixTranslation(-1.48, 0, 0);// *XMMatrixRotationZ(-XM_PI / 6);
	for (int i = 0; i < 12; ++i)
		m_mirrorMtx = XMMatrixInverse(&det,
		m)
		* scale *
		m;

	XMMATRIX viewMtx;
	m_camera.GetViewMatrix(viewMtx);
	XMMATRIX mirrorViewMtx = XMMatrixMultiply(m_mirrorMtx, viewMtx);
	UpdateCamera(mirrorViewMtx);
	m_context->RSSetState(m_rsCounterClockwise.get());

	SetLight0();
	DrawRoom();
	//DrawPlane(true);
	DrawPuma();
	DrawCircle();
	m_context->RSSetState(NULL);

	//Restore rendering state to it's original values
	UpdateCamera(viewMtx);
	m_context->OMSetDepthStencilState(NULL, 0);
}

void Puma::inverse_kinematics(XMFLOAT3 pos, XMFLOAT3 normal, float &a1, float &a2,
	float &a3, float &a4, float &a5)
{
	float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;

	float normalizationFactor = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
	normal = XMFLOAT3(normal.x / normalizationFactor, normal.y / normalizationFactor, normal.z / normalizationFactor);
	XMFLOAT3 pos1 = XMFLOAT3(pos.x + normal.x * l3, pos.y + normal.y * l3, pos.z + normal.z * l3);
	float e = sqrtf(pos1.z*pos1.z + pos1.x*pos1.x - dz*dz);
	a1 = atan2(pos1.z, -pos1.x) + atan2(dz, e);
	XMFLOAT3 pos2(e, pos1.y - dy, .0f);
	a3 = -acosf(min(1.0f, (pos2.x*pos2.x + pos2.y*pos2.y - l1*l1 - l2*l2)
		/ (2.0f*l1*l2)));
	float k = l1 + l2 * cosf(a3), l = l2 * sinf(a3);
	a2 = -atan2(pos2.y, sqrtf(pos2.x*pos2.x + pos2.z*pos2.z)) - atan2(l, k);
	XMFLOAT3 normal1;
	XMMATRIX m1 = XMMatrixRotationY(-a1) * XMMATRIX(normal.x, 0, 0, 0,
		normal.y, 0, 0, 0,
		normal.z, 0, 0, 0,
		0.0f, 0, 0, 0);
	normal1 = XMFLOAT3(m1._11, m1._21, m1._31);
	XMMATRIX m2 = XMMatrixRotationZ(-(a2 + a3)) * XMMATRIX(normal1.x, 0, 0, 0,
		normal1.y, 0, 0, 0,
		normal1.z, 0, 0, 0,
		0.0f, 0, 0, 0);
	normal1 = XMFLOAT3(m2._11, m2._21, m2._31);
	a5 = acosf(normal1.x);
	a4 = atan2(normal1.z, normal1.y);
}


void Puma::UpdatePuma(float dt)
{
	static float counter = 0;
	static float lap = 0.0f;
	lap += dt;
	while (lap > LAP_TIME)
		lap -= LAP_TIME;
	float t = 2 * lap / LAP_TIME;
	t *= XM_2PI;

	XMVECTOR pos = XMLoadFloat3(
		&XMFLOAT3(
		circleRadius * cos(t),
		circleRadius * sin(t),
		0));
	pos = XMVector3Transform(pos, XMMatrixRotationY(XM_PIDIV2));
	pos = XMVector3Transform(pos, XMMatrixRotationZ(XM_PI / 6.0f));
	pos = XMVector3Transform(pos, XMMatrixTranslation(circleCenter.x, circleCenter.y, 0.0f));
	XMFLOAT3 norm = XMFLOAT3(sqrtf(3) / 2.0f, 0.5f, 0.0f);

	float a1, a2, a3, a4, a5;
	XMFLOAT3 p = XMFLOAT3(
		XMVectorGetX(pos),
		XMVectorGetY(pos),
		XMVectorGetZ(pos));
	p = circleVertices[((int)counter) % 360].Pos;
	counter+=0.1f;
	inverse_kinematics(p, norm, a1, a2, a3, a4, a5);
	//a1 = 0;
	vector<VertexPosNormal> newVertices[6];
	for (int i = 1; i < 6; i++)
	{
		XMMATRIX tr;
		switch (i)
		{
		case 1:
			m_pumaMtx[i] = XMMatrixRotationY(a1);
			break;
		case 2:
			m_pumaMtx[i] = XMMatrixTranslation(0, -0.27f, 0) * XMMatrixRotationZ(a2) * XMMatrixTranslation(0, 0.27f, 0) * XMMatrixRotationY(a1);
			break;
		case 3:
			m_pumaMtx[i] = XMMatrixTranslation(0, -0.27f, 0) *XMMatrixTranslation(0.91f, 0, 0) * XMMatrixRotationZ(a3) *
				XMMatrixTranslation(-0.91f, 0, 0)  * XMMatrixRotationZ(a2) *XMMatrixTranslation(0, 0.27f, 0) * XMMatrixRotationY(a1);
			break;
		case 4:
			m_pumaMtx[i] = XMMatrixTranslation(0, -0.27f, 0) *
				XMMatrixTranslation(0, 0, 0.26f) *XMMatrixRotationX(a4) * XMMatrixTranslation(0, 0, -0.26f) *XMMatrixTranslation(+0.91f, 0, 0)
				*XMMatrixRotationZ(a3) * XMMatrixTranslation(-0.91f, 0, 0) *XMMatrixRotationZ(a2) *XMMatrixTranslation(0, 0.27f, 0) *
				XMMatrixRotationY(a1);
			break;
		case 5:
			m_pumaMtx[i] = XMMatrixTranslation(0, -0.27f, 0)* XMMatrixTranslation(1.72f, 0, 0) *XMMatrixRotationZ(a5 ) * XMMatrixTranslation(-1.72f, 0, 0) *
				XMMatrixTranslation(0, 0, 0.26f) * XMMatrixRotationX(a4) *XMMatrixTranslation(0, 0, -0.26f) * XMMatrixTranslation(+0.91f, 0, 0) 
				*	XMMatrixRotationZ(a3)* XMMatrixTranslation(-0.91f, 0, 0) *XMMatrixRotationZ(a2)* 
				XMMatrixTranslation(0, 0.27f, 0) *XMMatrixRotationY(a1);
			break;
		}
	}
}

void Puma::Update(float dt)
{
	static MouseState prevState;
	MouseState currentState;
	if (!m_mouse->GetState(currentState))
		return;
	bool change = true;
	if (prevState.isButtonDown(0))
	{
		POINT d = currentState.getMousePositionChange();
		m_camera.Rotate(d.y / 300.f, d.x / 300.f);
	}
	else if (prevState.isButtonDown(1))
	{
		POINT d = currentState.getMousePositionChange();
		m_camera.Zoom(d.y / 10.0f);
	}
	else
		change = false;
	prevState = currentState;
	if (change)
		UpdateCamera(m_camera.GetViewMatrix());
	UpdatePuma(dt);
}


void Puma::Render()
{
	if (m_context == nullptr)
		return;

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	DrawMirroredWorld();
	m_context->OMSetBlendState(m_bsAlpha.get(), 0, BS_MASK);

	SetLight0();
	DrawPlane();
	m_context->OMSetBlendState(0, 0, BS_MASK);


	SetLight0();
	DrawRoom();
	DrawPuma();
	DrawCircle();

	m_swapChain->Present(0, 0);
}