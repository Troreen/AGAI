#include "stdafx.h"

#include <tge/drawers/SpriteDrawer.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/DX11.h>
#include <tge/sprite/sprite.h>
#include <tge/texture/TextureManager.h>
#include <tge/shaders/SpriteShader.h>

#include <tge/math/Matrix2x2.h>
#include <tge/application.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/log/Log.h>


using namespace Tga;

constexpr size_t BATCH_SIZE = 1024;

SpriteBatchScope::~SpriteBatchScope()
{
	if (mySpriteDrawer != nullptr)
	{
		UnMapAndRender();
		mySpriteDrawer->EndBatch();
		mySpriteDrawer = nullptr;
	}
}

void SpriteBatchScope::Draw(const Sprite2DInstanceData& aInstance)
{
	Draw(&aInstance, 1);
};

void SpriteBatchScope::Draw(const Sprite2DInstanceData* aInstances, size_t aInstanceCount)
{
	aInstances;
	aInstanceCount;

	GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();

	assert(myInstanceCount < BATCH_SIZE);

	Vector2ui resolution = DX11::GetResolution();

	for (int i = 0; i < aInstanceCount; i++)
	{
		const Sprite2DInstanceData& instance = aInstances[i];

		if (instance.isHidden)
			continue;

		SpriteShaderInstanceData& shaderInstance = myInstanceData[myInstanceCount];

		Vector2f pivot = Vector2f(-instance.pivot.x, instance.pivot.y);
		Matrix2x2f scalingMatrix = Matrix2x2f::CreateFromScale(Vector2f((instance.size.x) * instance.sizeMultiplier.x, (instance.size.y) * instance.sizeMultiplier.y));
		Matrix2x2f rotationMatrix = Matrix2x2f::CreateFromRotation(instance.rotation);
		
		Matrix2x2f m = scalingMatrix * rotationMatrix;
		Vector2f p = pivot * m + instance.position;

		shaderInstance.transform = Matrix4x4f
		{
			m(1,1),m(1,2),0,0,
			m(2,1),m(2,2),0,0,
			0     ,0     ,1,0,
			p.x   ,p.y   ,0,1,
		} * graphicsStateStack.GetTransform();

		shaderInstance.uvRect.x = instance.textureRect.startX;
		shaderInstance.uvRect.y = instance.textureRect.endY;
		shaderInstance.uvRect.z = instance.textureRect.endX;
		shaderInstance.uvRect.w = instance.textureRect.startY;

		shaderInstance.uv.x = instance.uv.x;
		shaderInstance.uv.y = instance.uv.y;
		shaderInstance.uv.z = instance.uvScale.x;
		shaderInstance.uv.w = instance.uvScale.y;

		shaderInstance.color = instance.color.AsLinearVec4();

		myInstanceCount++;
		if (myInstanceCount >= BATCH_SIZE)
		{
			UnMapAndRender();
			Map();
		}
	}
};

void SpriteBatchScope::Draw(const Sprite3DInstanceData& aInstance)
{
	Draw(&aInstance, 1);
};

void SpriteBatchScope::Draw(const Sprite3DInstanceData* aInstances, size_t aInstanceCount)
{
	aInstances;
	aInstanceCount;

	assert(myInstanceCount < BATCH_SIZE);

	GraphicsStateStack& graphicsStateStack = Tga::GraphicsEngine::GetInstance()->GetGraphicsStateStack();

	for (int i = 0; i < aInstanceCount; i++)
	{
		const Sprite3DInstanceData& instance = aInstances[i];

		if (instance.isHidden)
			continue;

		SpriteShaderInstanceData& shaderInstance = myInstanceData[myInstanceCount];

		shaderInstance.transform = instance.transform * graphicsStateStack.GetTransform();

		shaderInstance.uvRect.x = instance.textureRect.startX;
		shaderInstance.uvRect.y = instance.textureRect.endY;
		shaderInstance.uvRect.z = instance.textureRect.endX;
		shaderInstance.uvRect.w = instance.textureRect.startY;

		shaderInstance.uv.x = instance.uv.x;
		shaderInstance.uv.y = instance.uv.y;
		shaderInstance.uv.z = instance.uvScale.x;
		shaderInstance.uv.w = instance.uvScale.y;

		shaderInstance.color = instance.color.AsLinearVec4();

		myInstanceCount++;
		if (myInstanceCount >= BATCH_SIZE)
		{
			UnMapAndRender();
			Map();
		}
	}
}

void SpriteBatchScope::UnMapAndRender()
{
	assert(mySpriteDrawer);

	ID3D11DeviceContext* context = DX11::Context;

	context->Unmap(mySpriteDrawer->myInstanceBuffer.Get(), 0);
	DX11::LogDrawCall();
	context->DrawInstanced(6, (UINT)myInstanceCount, 0, 0);

	myInstanceData = nullptr;
	myInstanceCount = 0;
}

void SpriteBatchScope::Map()
{
	assert(mySpriteDrawer);
	assert(myInstanceCount == 0);

	D3D11_MAPPED_SUBRESOURCE mappedObjectResource;
	HRESULT result = DX11::Context->Map(mySpriteDrawer->myInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedObjectResource);
	if (FAILED(result))
	{
		INFO_PRINT("Error in rendering!");
		return;
	}

	myInstanceData = static_cast<SpriteShaderInstanceData*>(mappedObjectResource.pData);
}

SpriteDrawer::SpriteDrawer()
{
}

SpriteDrawer::~SpriteDrawer()
{
}

void SpriteDrawer::Init()
{
	D3D11_BUFFER_DESC objectBufferDesc;
	objectBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	objectBufferDesc.ByteWidth = sizeof(SpriteShaderInstanceData) * BATCH_SIZE;
	objectBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	objectBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	objectBufferDesc.MiscFlags = 0;
	objectBufferDesc.StructureByteStride = 0;

	HRESULT result;
	result = DX11::Device->CreateBuffer(&objectBufferDesc, nullptr, myInstanceBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		ERROR_PRINT("%s", "Object Buffer error");
		return;
	}
	myIsLoaded = InitShaders() && CreateBuffer();
}

bool SpriteDrawer::InitShaders()
{
	myDefaultShader = std::make_unique<SpriteShader>();
	if (!myDefaultShader->Init())
	{
		return false;
	}

	return true;
}

bool SpriteDrawer::CreateBuffer()
{
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	float startSize = 1.0f;
	float theZ = 0.0f;
	myVertices[0].x = -0;
	myVertices[0].y = -startSize;
	myVertices[0].z = theZ;
	myVertices[0].w = 1.0f;
	myVertices[0].vertexIndex = 0;

	myVertices[1].x = -0;
	myVertices[1].y = -0;
	myVertices[1].z = theZ;
	myVertices[1].w = 1.0f;
	myVertices[1].vertexIndex = 1;

	myVertices[2].x = startSize;
	myVertices[2].y = -startSize;
	myVertices[2].z = theZ;
	myVertices[2].w = 1.0f;
	myVertices[2].vertexIndex = 2;

	myVertices[3].x = startSize;
	myVertices[3].y = -0;
	myVertices[3].z = theZ;
	myVertices[3].w = 1.0f;
	myVertices[3].vertexIndex = 3;

	myVertices[4].x = startSize;
	myVertices[4].y = -startSize;
	myVertices[4].z = theZ;
	myVertices[4].w = 1.0f;
	myVertices[4].vertexIndex = 4;

	myVertices[5].x = -0;
	myVertices[5].y = -0;
	myVertices[5].z = theZ;
	myVertices[5].w = 1.0f;
	myVertices[5].vertexIndex = 5;

	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexInstanced) * 6;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = myVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT hr = DX11::Device->CreateBuffer(&vertexBufferDesc, &vertexData, myVertexBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		ERROR_PRINT("%s", "Buffer error");
		return false;
	}

	return true;
}

SpriteBatchScope SpriteDrawer::BeginBatch(const SpriteSharedData& aSharedData)
{
	assert(myIsLoaded);
	assert(!myIsInBatch);
	myIsInBatch = true;

	if (aSharedData.customShader)
	{
		aSharedData.customShader->PrepareRender(aSharedData);
	}
	else
	{
		myDefaultShader->PrepareRender(aSharedData);
	}

	unsigned int strides[2];
	unsigned int offsets[2];
	ID3D11Buffer* bufferPointers[2];

	strides[0] = sizeof(VertexInstanced);
	strides[1] = sizeof(SpriteShaderInstanceData);

	offsets[0] = 0;
	offsets[1] = 0;

	bufferPointers[0] = myVertexBuffer.Get();
	bufferPointers[1] = myInstanceBuffer.Get();

	DX11::Context->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);

	SpriteBatchScope scope(*this);
	scope.Map();

	return scope;
}

void SpriteDrawer::EndBatch()
{
	assert(myIsInBatch);
	myIsInBatch = false;
}