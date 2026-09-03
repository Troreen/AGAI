#include "stdafx.h"
#include <tge/drawers/LineDrawer.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/graphics/DX11.h>
#include <tge/render/RenderObject.h>
#include <tge/shaders/shader.h>
#include <tge/application.h>
#include <tge/log/Log.h>
#include <tge/primitives/LinePrimitive.h>

using namespace Tga;
LineDrawer::LineDrawer()
	: Shader()
{}

LineDrawer::~LineDrawer() {}

bool LineDrawer::Init()
{
	Shader::Init();
	InitShaders();
	CreateBuffer();
	return true;
}

void LineDrawer::CreateBuffer()
{
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	float theZ = 0.0f;
	myVertices[0].x = -0.5f;
	myVertices[0].y = 0.5f;
	myVertices[0].z = theZ;

	myVertices[1].x = -0.5f;
	myVertices[1].y = -0.5f;
	myVertices[1].z = theZ;

	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;       
	vertexBufferDesc.ByteWidth = sizeof(SimpleVertex) * 2000;        
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = myVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	HRESULT hr = DX11::Device->CreateBuffer(&vertexBufferDesc, nullptr, myVertexBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		ERROR_PRINT("%s", "Buffer error");
		return;
	}

}

void Tga::LineDrawer::Draw(const LineMultiPrimitive& aObject)
{
	assert(aObject.count <= 1000 && "A single multi Primivive can only draw 1000 lines"); //1000 is a magic number I dont know why it limits there. Maybe buffer size to graphics card.

	if (!myVertexBuffer)
	{
		return;
	}
	PrepareRender();
	DX11::Context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINELIST);

	D3D11_MAPPED_SUBRESOURCE mappedVertexResource;
	HRESULT result = DX11::Context->Map(myVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexResource);
	if (FAILED(result))
	{
		INFO_PRINT("Error in rendering!");
		return;
	}

	SimpleVertex* dataVertexPtr;
	GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();
	Matrix4x4f transform = graphicsStateStack.GetTransform();

	dataVertexPtr = (SimpleVertex*)mappedVertexResource.pData;
	int theCount = 0;
	for (unsigned int i = 0; i < aObject.count; i++)
	{
		Vector3f fromPos = Vector4f(aObject.fromPositions[i], 1.f) * transform;
		Vector3f toPos = Vector4f(aObject.toPositions[i], 1.f) * transform;

		dataVertexPtr[theCount].x = fromPos.x;
		dataVertexPtr[theCount].y = fromPos.y;
		dataVertexPtr[theCount].z = fromPos.z;

		dataVertexPtr[theCount + 1].x = toPos.x;
		dataVertexPtr[theCount + 1].y = toPos.y;
		dataVertexPtr[theCount + 1].z = toPos.z;

		dataVertexPtr[theCount].colorA = aObject.colors[i].myA;
		dataVertexPtr[theCount].colorR = aObject.colors[i].myR;
		dataVertexPtr[theCount].colorG = aObject.colors[i].myG;
		dataVertexPtr[theCount].colorB = aObject.colors[i].myB;

		dataVertexPtr[theCount+1].colorA = aObject.colors[i].myA;
		dataVertexPtr[theCount+1].colorR = aObject.colors[i].myR;
		dataVertexPtr[theCount+1].colorG = aObject.colors[i].myG;
		dataVertexPtr[theCount+1].colorB = aObject.colors[i].myB;
		theCount += 2;
	}

	DX11::Context->Unmap(myVertexBuffer.Get(), 0);


	unsigned int strides = sizeof(SimpleVertex);
	unsigned int offsets = 0;
	DX11::Context->IASetVertexBuffers(0, 1, myVertexBuffer.GetAddressOf(), &strides, &offsets);

	DX11::LogDrawCall();
	DX11::Context->Draw(aObject.count * 2, 0);
}

bool Tga::LineDrawer::InitShaders()
{
	CreateShaders("shaders/lineshader_VS", "shaders/lineshader_PS");

	return true;
}

bool LineDrawer::CreateInputLayout(const std::string& aVS)
{
	// LAYOUT
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];

	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;


	unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	HRESULT result = DX11::Device->CreateInputLayout(polygonLayout, numElements, aVS.data(), aVS.size(), myLayout.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		ERROR_PRINT("%s", "Layout error");
		return false;
	}
	return true;
}

void LineDrawer::SetShaderParameters(const LinePrimitive& aObject)
{
	UpdateVertexes(aObject);

	unsigned int strides = sizeof(SimpleVertex);
	unsigned int offsets = 0;
	DX11::Context->IASetVertexBuffers(0, 1, myVertexBuffer.GetAddressOf(), &strides, &offsets);
}

void LineDrawer::UpdateVertexes(const LinePrimitive& aObject)
{
	aObject;
	D3D11_MAPPED_SUBRESOURCE mappedVertexResource;
	HRESULT result = DX11::Context->Map(myVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexResource);
	if(FAILED(result))
	{
		INFO_PRINT("Error in rendering!");
		return;
	}

	SimpleVertex* dataVertexPtr;
	GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();
	Matrix4x4f transform = graphicsStateStack.GetTransform();

	Vector3f fromPos = Vector4f(aObject.fromPosition, 1.f) * transform;
	Vector3f toPos = Vector4f(aObject.toPosition, 1.f) * transform;

	dataVertexPtr = (SimpleVertex*)mappedVertexResource.pData;

	dataVertexPtr[0].x = fromPos.x;
	dataVertexPtr[0].y = fromPos.y;
	dataVertexPtr[0].z = fromPos.z;

	dataVertexPtr[1].x = toPos.x;
	dataVertexPtr[1].y = toPos.y;
	dataVertexPtr[1].z = toPos.z;

	dataVertexPtr[0].colorA = aObject.color.w;
	dataVertexPtr[0].colorR = aObject.color.x;
	dataVertexPtr[0].colorG = aObject.color.y;
	dataVertexPtr[0].colorB = aObject.color.z;

	dataVertexPtr[1].colorA = aObject.color.w;
	dataVertexPtr[1].colorR = aObject.color.x;
	dataVertexPtr[1].colorG = aObject.color.y;
	dataVertexPtr[1].colorB = aObject.color.z;

	DX11::Context->Unmap(myVertexBuffer.Get(), 0);
}


void LineDrawer::Draw(const LinePrimitive& aObject)
{
	PrepareRender();
	DX11::Context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINELIST);

	SetShaderParameters(aObject);
	DX11::LogDrawCall();
	DX11::Context->Draw(2, 0);
}