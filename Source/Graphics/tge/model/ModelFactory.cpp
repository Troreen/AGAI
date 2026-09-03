#include "stdafx.h"
#include "ModelFactory.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <tge/application.h>
#include <tge/settings/settings.h>
#include <tge/log/Log.h>
#include <tge/animation/animationPlayer.h>
#include <tge/graphics/DX11.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/model/Model.h>
#include <tge/model/ModelInstance.h>
#include <tge/graphics/Vertex.h>
#include <tge/math/matrix4x4.h>
#include <tge/texture/texture.h>
#include <tge/texture/TextureManager.h>
#include <tge/filewatcher/FileWatcher.h>
#include <tge/util/FixedStream.h>

//#define TGA_USE_UFBX
#ifdef TGA_USE_UFBX
#include <ufbx/ufbx.h>
#else
#include <TGAFBXImporter/source/Importer.h>
#endif

#include <DDSTextureLoader/DDSTextureLoader11.h>

using namespace Tga;
ModelFactory* ModelFactory::ourInstance = nullptr;

#define TEXTURE_SET_0 0
#define TEXTURE_SET_1 1
#define TEXTURE_SET_2 2
#define TEXTURE_SET_3 3

#define VERTEX_COLOR_SET_0 0
#define VERTEX_COLOR_SET_1 1
#define VERTEX_COLOR_SET_2 2
#define VERTEX_COLOR_SET_3 3

#define NUM_BONES_PER_VERTEX 4

using namespace Tga;

void AssignDefaultMaterials(std::string_view someFilePath, Model* aModel);

struct VertexBoneData
{
	unsigned int IDs[NUM_BONES_PER_VERTEX];
	float Weights[NUM_BONES_PER_VERTEX];

	VertexBoneData()
	{
		Reset();
	};

	void Reset()
	{
		memset(IDs, 0, sizeof(IDs));
		memset(Weights, 0, sizeof(Weights));
	}

	void AddBoneData(unsigned int BoneID, float Weight)
	{
		for (unsigned int i = 0; i < sizeof(IDs) / sizeof(IDs[0]); i++)
		{
			if (Weights[i] == 0.0)
			{
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			}
		}

		// should never get here - more bones than we have space for
		//assert(0);
	}
};

bool ModelFactory::InitUnitCube()
{
	// First we make a cube.

	Model::MeshData meshData = {};
	// Watch the winding! DX defaults to Clockwise.
	// Assume the winding as if you're viewing the face head on.
	// +Y up, +X right, +Z Forward
	meshData.vertices = 
	{
		// Front
		{
			50.0f, -50.0f, 50.0f,
			0, 0, 1,
			1, 0, 0,
			0, -1, 0,
			1, 1, 1, 1,
			0, 1
		},
		{
			50.0f, 50.0f, 50.0f,
			0, 0, 1,
			1, 0, 0,
			0, -1, 0,
			1, 1, 1, 1,
			0, 0
		},
		{
			-50.0f, 50.0f, 50.0f,
			0, 0, 1,
			1, 0, 0,
			0, -1, 0,
			1, 1, 1, 1,
			1, 0
		},
		{
			-50.0f, -50.0f, 50.0f,
			0, 0, 1,
			1, 0, 0,
			0, -1, 0,
			1, 1, 1, 1,
			1, 1
		},

		// Left
		{
			-50.0f, -50.0f, 50.0f,
			-1, 0, 0,
			0, 0, 1,
			0, -1, 0,
			1, 0, 0, 1,
			0, 1
		},
		{
			-50.0f, 50.0f, 50.0f,
			-1, 0, 0,
			0, 0, 1,
			0, -1, 0,
			1, 0, 0, 1,
			0, 0
		},
		{
			-50.0f, 50.0f, -50.0f,
			-1, 0, 0,
			0, 0, 1,
			0, -1, 0,
			1, 0, 0, 1,
			1, 0
		},
		{
			-50.0f, -50.0f, -50.0f,
			-1, 0, 0,
			0, 0, 1,
			0, -1, 0,
			1, 0, 0, 1,
			1, 1
		},

		// Back
		{
			-50.0f, -50.0f, -50.0f,
			0, 0, -1,
			1, 0, 0,
			0, -1, 0,
			0, 1, 0, 1,
			0, 1
		},
		{
			-50.0f, 50.0f, -50.0f,
			0, 0, -1,
			1, 0, 0,
			0, -1, 0,
			0, 1, 0, 1,
			0, 0
		},
		{
			50.0f, 50.0f, -50.0f,
			0, 0, -1,
			1, 0, 0,
			0, -1, 0,
			0, 1, 0, 1,
			1, 0
		},
		{
			50.0f, -50.0f, -50.0f,
			0, 0, -1,
			1, 0, 0,
			0, -1, 0,
			0, 1, 0, 1,
			1, 1
		},

		// Right
		{
			50.0f, -50.0f, -50.0f,
			1, 0, 0,
			0, 0, -1,
			0, -1, 0,
			0, 0, 1, 1,
			0, 1
		},
		{
			50.0f, 50.0f, -50.0f,
			1, 0, 0,
			0, 0, -1,
			0, -1, 0,
			0, 0, 1, 1,
			0, 0
		},
		{
			50.0f, 50.0f, 50.0f,
			1, 0, 0,
			0, 0, -1,
			0, -1, 0,
			0, 0, 1, 1
			, 1, 0
		},
		{
			50.0f, -50.0f, 50.0f,
			1, 0, 0,
			0, 0, -1,
			0, -1, 0,
			0, 0, 1, 1,
			1, 1
		},

		// Top
		{
			50.0f, 50.0f, 50.0f,
			0, 1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 1, 0, 1,
			0, 1
		},
		{
			50.0f, 50.0f, -50.0f,
			0, 1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 1, 0, 1,
			0, 0
		},
		{
			-50.0f, 50.0f, -50.0f,
			0, 1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 1, 0, 1,
			1, 0
		},
		{
			-50.0f, 50.0f, 50.0f,
			0, 1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 1, 0, 1,
			1, 1
		},

		// Bottom
		{
			-50.0f, -50.0f, 50.0f,
			0, -1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 0, 1, 1,
			0, 1
		},
		{
			-50.0f, -50.0f, -50.0f,
			0, -1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 0, 1, 1,
			0, 0
		},
		{
			50.0f, -50.0f, -50.0f,
			0, -1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 0, 1, 1,
			1, 0
		},
		{
			50.0f, -50.0f, 50.0f,
			0, -1, 0,
			1, 0, 0,
			0, 0, 1,
			1, 0, 1, 1
			, 1, 1
		},
	};

	meshData.indices =
	{
		0, 1, 2,        /* |/ */
		0, 2, 3,        /* /| */
		4, 5, 6,        /* |/ */
		4, 6, 7,        /* /| */
		8, 9, 10,       /* |/ */
		8, 10, 11,      /* /| */
		12, 13, 14,     /* |/ */
		12, 14, 15,     /* /| */
		16, 17, 18,     /* |/ */
		16, 18, 19,     /* /| */
		20, 21, 22,     /* |/ */
		20, 22, 23      /* /| */
	};

	//const Vector3f extentsCenter = 0.5f * (minExtents + maxExtents);
	//const Vector3f boxExtents = 0.5f * (maxExtents - minExtents);
	//const float myBoxSphereRadius = FMath::Max(boxExtents.X, FMath::Max(boxExtents.Y, boxExtents.Z));

	HRESULT result;

	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.ByteWidth = static_cast<UINT>(meshData.vertices.size()) * static_cast<UINT>(sizeof(Vertex));
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexSubresourceData{};
	vertexSubresourceData.pSysMem = &meshData.vertices[0];

	ID3D11Buffer* vertexBuffer;
	result = DX11::Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC indexBufferDesc{};
	indexBufferDesc.ByteWidth = static_cast<UINT>(meshData.indices.size()) * static_cast<UINT>(sizeof(unsigned int));
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexSubresourceData{};
	indexSubresourceData.pSysMem = &meshData.indices[0];

	ID3D11Buffer* indexBuffer;
	result = DX11::Device->CreateBuffer(&indexBufferDesc, &indexSubresourceData, &indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	std::shared_ptr<Model> model = std::make_shared<Model>();

	meshData.numberOfVertices = static_cast<UINT>(meshData.vertices.size());
	meshData.numberOfIndices = static_cast<UINT>(meshData.indices.size());
	meshData.stride = sizeof(Vertex);
	meshData.offset = 0;
	meshData.vertexBuffer = vertexBuffer;
	meshData.indexBuffer = indexBuffer;
	meshData.bounds = CalculateBoxSphereBounds(meshData.vertices);
	model->Init(meshData, "Cube");

	AssignDefaultMaterials("", model.get());

	myLoadedModels.insert(std::pair<StringId, std::shared_ptr<Model>>("Cube"_tgaid, model));

	return true;
}

bool ModelFactory::InitUnitPlane()
{
	Model::MeshData meshData = {};

	meshData.vertices.push_back({
		-50.0f, 0.0f, 50.0f,
		0, 1, 0,
		1, 0, 0,
		0, 0, 1,
		1, 1, 1, 1,
		0, 0
		});

	meshData.vertices.push_back({
		50.0f, 0.0f, 50.0f,
		0, 1, 0,
		1, 0, 0,
		0, 0, 1,
		1, 1, 1, 1,
		1, 0
		});

	meshData.vertices.push_back({
		50.0f, 0.0f, -50.0f,
		0, 1, 0,
		1, 0, 0,
		0, 0, 1,
		1, 1, 1, 1,
		1, 1
		});

	meshData.vertices.push_back({
		-50.0f, 0.0f, -50.0f,
		0, 1, 0,
		1, 0, 0,
		0, 0, 1,
		1, 1, 1, 1,
		0, 1
		});


	meshData.indices = { 0, 1, 2, 0, 2, 3 };

	//const Vector3f extentsCenter = 0.5f * (minExtents + maxExtents);
	//const Vector3f boxExtents = 0.5f * (maxExtents - minExtents);
	//const float myBoxSphereRadius = FMath::Max(boxExtents.X, FMath::Max(boxExtents.Y, boxExtents.Z));

	HRESULT result;

	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.ByteWidth = static_cast<UINT>(meshData.vertices.size()) * static_cast<UINT>(sizeof(Vertex));
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexSubresourceData{};
	vertexSubresourceData.pSysMem = &meshData.vertices[0];

	ID3D11Buffer* vertexBuffer;
	result = DX11::Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC indexBufferDesc{};
	indexBufferDesc.ByteWidth = static_cast<UINT>(meshData.indices.size()) * static_cast<UINT>(sizeof(unsigned int));
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexSubresourceData{};
	indexSubresourceData.pSysMem = &meshData.indices[0];

	ID3D11Buffer* indexBuffer;
	result = DX11::Device->CreateBuffer(&indexBufferDesc, &indexSubresourceData, &indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	std::shared_ptr<Model> model = std::make_shared<Model>();

	meshData.numberOfVertices = static_cast<UINT>(meshData.vertices.size());
	meshData.numberOfIndices = static_cast<UINT>(meshData.indices.size());
	meshData.stride = sizeof(Vertex);
	meshData.offset = 0;
	meshData.vertexBuffer = vertexBuffer;
	meshData.indexBuffer = indexBuffer;
	meshData.bounds = CalculateBoxSphereBounds(meshData.vertices);
	model->Init(meshData, "Plane");
	myLoadedModels.insert(std::pair<StringId, std::shared_ptr<Model>>("Plane"_tgaid, model));

	AssignDefaultMaterials("", model.get());

	return true;
}

bool ModelFactory::InitPrimitives()
{
	if (!InitUnitCube())
		return false;

	if (!InitUnitPlane())
		return false;

	return true;
}

static std::shared_ptr<const Animation> GetAnimationWrapper(std::string_view someFilePath, const std::shared_ptr<const Skeleton>& aSkeleton)
{
	return ModelFactory::GetInstance().GetAnimation(StringRegistry::RegisterOrGetString(someFilePath), aSkeleton);
}

static std::shared_ptr<const Skeleton> GetSkeletonWrapper(std::string_view someFilePath)
{
	std::shared_ptr<Model> model = ModelFactory::GetInstance().GetModel(StringRegistry::RegisterOrGetString(someFilePath));
	return model ? model->GetSkeleton() : nullptr;
}

ModelFactory::ModelFactory()
{
	RegisterGetAnimationFunction(GetAnimationWrapper);
	RegisterGetSkeletonFunction(GetSkeletonWrapper);

#ifndef TGA_USE_UFBX
	TGA::FBX::Importer::InitImporter();
#endif
	InitPrimitives();
	ourInstance = this;
}

ModelFactory::~ModelFactory()
{
	ourInstance = nullptr;
#ifndef TGA_USE_UFBX
	TGA::FBX::Importer::UninitImporter();
#endif

}

static TextureResource *AssignAlbedoTexture(std::string_view baseFileName, std::string_view materialFileName) 
{
	FixedStream<512> stream;
	stream << materialFileName << "_C.dds";
	TextureResource	*albedoTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(stream.GetData());

	if (albedoTexture == nullptr)
	{
		FixedStream<512> streamD;
		streamD << materialFileName << "_D.dds";
		albedoTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamD.GetData());
	}

	if (albedoTexture == nullptr)
	{
		FixedStream<512> streamBC;
		streamBC << baseFileName << "_C.dds";
		albedoTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamBC.GetData());
	}

	if (albedoTexture == nullptr)
	{
		FixedStream<512> streamBD;
		streamBD << baseFileName << "_D.dds";
		albedoTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamBD.GetData());
	}

	if (albedoTexture == nullptr)
	{
		albedoTexture = GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Textures/T_Default_c.dds");
	}
	return albedoTexture;
}

static TextureResource *AssignNormalTexture(std::string_view baseFileName, std::string_view materialFileName) 
{
	FixedStream<512> streamN;
	streamN << materialFileName << "_N.dds";
	TextureResource *normalTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamN.GetData(), TextureSrgbMode::ForceNoSrgbFormat);

	if (normalTexture == nullptr)
	{
		FixedStream<512> streamBN;
		streamBN << baseFileName << "_N.dds";
		normalTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamBN.GetData(), TextureSrgbMode::ForceNoSrgbFormat);
	}

	if (normalTexture == nullptr)
		normalTexture = GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Textures/T_Default_n.dds", TextureSrgbMode::ForceNoSrgbFormat);

	return normalTexture;
}

static TextureResource *AssignMaterialTexture(std::string_view baseFileName, std::string_view materialFileName) 
{
	FixedStream<512> streamM;
	streamM << materialFileName << "_M.dds";
	TextureResource *materialTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamM.GetData(), TextureSrgbMode::ForceNoSrgbFormat);

	if (materialTexture == nullptr)
	{
		FixedStream<512> streamBM;
		streamBM << baseFileName << "_M.dds";
		materialTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamBM.GetData(), TextureSrgbMode::ForceNoSrgbFormat);
	}

	if (materialTexture == nullptr)
		materialTexture = GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Textures/T_Default_m.dds", TextureSrgbMode::ForceNoSrgbFormat);

	return materialTexture;
}

static TextureResource *AssignFxTexture(std::string_view baseFileName, std::string_view materialFileName) 
{
	FixedStream<512> streamFX;
	streamFX << materialFileName << "_FX.dds";
	TextureResource *fxTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamFX.GetData(), TextureSrgbMode::ForceNoSrgbFormat);

	if (fxTexture == nullptr)
	{
		FixedStream<512> streamBFX;
		streamBFX << baseFileName << "_FX.dds";
		fxTexture = GraphicsEngine::GetInstance()->GetTextureManager().TryGetTexture(streamBFX.GetData(), TextureSrgbMode::ForceNoSrgbFormat);
	}

	if (fxTexture == nullptr)
		fxTexture = GraphicsEngine::GetInstance()->GetTextureManager().GetTexture("Textures/T_Default_fx.dds", TextureSrgbMode::ForceNoSrgbFormat);
	return fxTexture;
}

void AssignDefaultMaterials(std::string_view someFilePath, Model* aModel)
{
	size_t dotPos = someFilePath.find_last_of('.');
	std::string_view baseFileName = (dotPos != std::string_view::npos) ? someFilePath.substr(0, dotPos) : someFilePath;
	size_t slashPos = someFilePath.find_last_of("/\\");
	std::string_view path = (slashPos != std::string_view::npos) ? someFilePath.substr(0, slashPos + 1) : "";

	for (int i = 0; i < aModel->GetMeshCount(); i++)
	{
		FixedStream<512> materialFileNameStream;
		materialFileNameStream << path << aModel->GetMaterialName(i);

		TextureResource* albedoTexture = AssignAlbedoTexture(baseFileName, materialFileNameStream.GetStringView());
		aModel->SetDefaultTexture(i, 0, albedoTexture);

		TextureResource* normalTexture = AssignNormalTexture(baseFileName, materialFileNameStream.GetStringView());
		aModel->SetDefaultTexture(i, 1, normalTexture);

		TextureResource* materialTexture = AssignMaterialTexture(baseFileName, materialFileNameStream.GetStringView());
		aModel->SetDefaultTexture(i, 2, materialTexture);

		TextureResource* fxTexture = AssignFxTexture(baseFileName, materialFileNameStream.GetStringView());
		aModel->SetDefaultTexture(i, 3, fxTexture);
	}
}


ModelInstance ModelFactory::GetModelInstance(StringId someFilePath)
{
	// This needs to be moved to separate memory structures at some point.
	ModelInstance meshInstance;

	std::shared_ptr<Model> model = GetModel(someFilePath);
	if (!model)
		return meshInstance;

	meshInstance.Init(model);

	return meshInstance;
		}

AnimatedModelInstance ModelFactory::GetAnimatedModelInstance(StringId someFilePath)
{
	// This needs to be moved to separate memory structures at some point.
	AnimatedModelInstance meshInstance;

	std::shared_ptr<Model> model = GetModel(someFilePath);
	if (!model || !model->GetSkeleton()->GetRoot())
		return meshInstance;

	meshInstance.Init(model);

	return meshInstance;
}

ModelInstance ModelFactory::GetUnitCube()
{
	return GetModelInstance("Cube"_tgaid);
}

ModelInstance ModelFactory::GetUnitPlane()
{
	return GetModelInstance("Plane"_tgaid);
}

#ifdef TGA_USE_UFBX
static Matrix4x4f ConvertMatrix(const ufbx_matrix& m)
{
	Matrix4x4f mat;

	mat(1, 1) = (float)m.m00; mat(1, 2) = (float)m.m10; mat(1, 3) = (float)m.m20; mat(1, 4) = 0.f;
	mat(2, 1) = (float)m.m01; mat(2, 2) = (float)m.m11; mat(2, 3) = (float)m.m21; mat(2, 4) = 0.f;
	mat(3, 1) = (float)m.m02; mat(3, 2) = (float)m.m12; mat(3, 3) = (float)m.m22; mat(3, 4) = 0.f;
	mat(4, 1) = (float)m.m03; mat(4, 2) = (float)m.m13; mat(4, 3) = (float)m.m23; mat(4, 4) = 1.f;

	return mat;
}

static constexpr ufbx_coordinate_axes FBX_AXIS = {
	UFBX_COORDINATE_AXIS_POSITIVE_X, UFBX_COORDINATE_AXIS_POSITIVE_Y, UFBX_COORDINATE_AXIS_POSITIVE_Z,
};

static void GenerateTangents(std::vector<Vertex>& verts, const std::vector<uint32_t>& indices)
{
	// Quick implementation of tangent computation in cases they are missing
	// Verified to match imported tangent orientation on particle_chest

	for (auto& v : verts)
	{
		v.tangent = { 0,0,0 };
		v.binormal = { 0,0,0 };
	}

	// Accumulation across triangles
	// TODO: should probably have some kind of angle based weighting?
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		Vertex& v0 = verts[i0];
		Vertex& v1 = verts[i1];
		Vertex& v2 = verts[i2];

		const auto& p0 = v0.position;
		const auto& p1 = v1.position;
		const auto& p2 = v2.position;

		const auto& uv0 = v0.uvs[0];
		const auto& uv1 = v1.uvs[0];
		const auto& uv2 = v2.uvs[0];

		Tga::Vector3f dp1 = p1 - p0;
		Tga::Vector3f dp2 = p2 - p0;

		Tga::Vector2f duv1 = uv1 - uv0;
		Tga::Vector2f duv2 = uv2 - uv0;

		float denom = duv1.x * duv2.y - duv1.y * duv2.x;
		if (fabs(denom) < 1e-6f)	continue;

		float r = 1.0f / denom;

		Tga::Vector3f tangent = (dp1 * duv2.y - dp2 * duv1.y) * r;
		Tga::Vector3f bitangent = (dp2 * duv1.x - dp1 * duv2.x) * r;

		v0.tangent += tangent;
		v1.tangent += tangent;
		v2.tangent += tangent;

		v0.binormal += bitangent;
		v1.binormal += bitangent;
		v2.binormal += bitangent;
	}

	for (auto& v : verts)
	{
		Tga::Vector3f n = v.normal;
		Tga::Vector3f t = v.tangent;

		if (t.LengthSqr() < 1e-6f)
		{
			// fallback: build arbitrary tangent to avoid broken meshes
			Tga::Vector3f up = fabs(n.z) < 0.999f ? Tga::Vector3f{ 0,0,1 } : Tga::Vector3f{ 0,1,0 };
			t = n.Cross(up).GetNormalized();
		}
		else
		{
			// Gram-Schmidt
			t = (t - n * n.Dot(t)).GetNormalized();
		}

		// compute handedness:
		Tga::Vector3f b = v.binormal;
		float w = (n.Cross(t).Dot(b) < 0.0f) ? 1.0f : -1.0f;

		v.tangent = t;
		v.binormal = n.Cross(t) * w;
	}
}



std::shared_ptr<Model> ModelFactory::LoadModel(StringId someFilePath)
{
	if (someFilePath.IsEmpty())
		return nullptr;
	FilePathStream resolved_path;
	if (!Tga::Settings::ResolveAssetPath(someFilePath, resolved_path))
		return nullptr;

	ufbx_load_opts opts = 
	{
		.generate_missing_normals = true,
		.target_axes = FBX_AXIS,
	};


	ufbx_error error;
	ufbx_scene* scene = ufbx_load_file(resolved_path.GetData(), &opts, &error);

	if (!scene)
	{
		ERROR_PRINT("ufbx load failed: %s", someFilePath.GetString());
		return nullptr;
	}

	std::vector<Model::MeshData> mdlMeshData;

	Skeleton mdlSkeleton;

	std::unordered_map<const ufbx_node*, int> nodeToJoint;
	std::vector<const ufbx_node*> jointNodes;

	// Skeleton
	for (size_t mesh_i = 0; mesh_i < scene->meshes.count; mesh_i++)
	{
		ufbx_mesh* mesh = scene->meshes.data[mesh_i];

		if (mesh->skin_deformers.count == 0)
			continue;

		ufbx_skin_deformer* skin = mesh->skin_deformers.data[0];

		for (size_t c = 0; c < skin->clusters.count; c++)
		{
			ufbx_skin_cluster* cluster = skin->clusters.data[c];
			if (!cluster->bone_node) continue;

			const ufbx_node* node = cluster->bone_node;

			if (nodeToJoint.contains(node))
				continue;

			int index = (int)mdlSkeleton.joints.size();
			nodeToJoint[node] = index;

			Skeleton::Joint& joint = mdlSkeleton.joints.emplace_back();
			jointNodes.push_back(node);

			joint.name = node->name.data ? node->name.data : "";

			if (node->parent && nodeToJoint.contains(node->parent))
				joint.parent = nodeToJoint[node->parent];
			else
				joint.parent = -1;

			mdlSkeleton.jointNameToIndex[joint.name] = index;
		}
	}

	ufbx_matrix skeletonParent = ufbx_identity_matrix;

	if (jointNodes.size() > 1)
	{
		const ufbx_node* skeletonRoot = jointNodes[0];
		if (skeletonRoot && skeletonRoot->parent)
		{
			skeletonParent = skeletonRoot->parent->node_to_world;
		}

		for (size_t mesh_i = 0; mesh_i < scene->meshes.count; mesh_i++)
		{
			ufbx_mesh* mesh = scene->meshes.data[mesh_i];
			if (mesh->skin_deformers.count == 0) continue;

			ufbx_skin_deformer* skin = mesh->skin_deformers.data[0];

			for (size_t c = 0; c < skin->clusters.count; c++)
			{
				ufbx_skin_cluster* cluster = skin->clusters.data[c];
				if (!cluster->bone_node) continue;

				int jointIndex = nodeToJoint[cluster->bone_node];

				ufbx_matrix geomToBone = cluster->geometry_to_bone;
				
				// Apply transform above the skeleton root (blender tends to add this)
				ufbx_matrix fixed = ufbx_matrix_mul(&skeletonParent, &geomToBone);

				mdlSkeleton.joints[jointIndex].bindPoseInverse = ConvertMatrix(fixed);

				mdlSkeleton.modelBindPose.jointTransforms[jointIndex] =
					mdlSkeleton.joints[jointIndex].bindPoseInverse.GetInverse();
			}
		}

		assert(MAX_ANIMATION_BONES >= mdlSkeleton.joints.size() && "More joints in animation than defined in EngineDefines.h");

		mdlSkeleton.ConvertPoseToLocalSpace(mdlSkeleton.modelBindPose, mdlSkeleton.localBindPose);
	}

	// Build hierarchy
	for (size_t i = 0; i < mdlSkeleton.joints.size(); i++)
	{
		int parent = mdlSkeleton.joints[i].parent;
		if (parent >= 0)
			mdlSkeleton.joints[parent].children.push_back((unsigned)i);
	}

	// Meshes
	for (size_t mesh_i = 0; mesh_i < scene->meshes.count; mesh_i++)
	{
		ufbx_mesh* mesh = scene->meshes.data[mesh_i];

		Model::MeshData meshData;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		std::vector<VertexBoneData> boneData(mesh->num_vertices);

		// Skinning
		if (mesh->skin_deformers.count > 0)
		{
			ufbx_skin_deformer* skin = mesh->skin_deformers.data[0];

			for (size_t v = 0; v < skin->vertices.count; v++)
			{
				const ufbx_skin_vertex& sv = skin->vertices.data[v];

				for (uint32_t w = 0; w < sv.num_weights; w++)
				{
					const ufbx_skin_weight& weight =
						skin->weights.data[sv.weight_begin + w];

					ufbx_skin_cluster* cluster =
						skin->clusters.data[weight.cluster_index];

					if (!cluster->bone_node) continue;
					if (!nodeToJoint.count(cluster->bone_node)) continue;

					unsigned int boneIndex = nodeToJoint[cluster->bone_node];
					boneData[v].AddBoneData(boneIndex, (float)weight.weight);
				}
			}
		}

		// Triangulation
		std::vector<uint32_t> triBuf(mesh->max_face_triangles * 3);

		for (size_t f = 0; f < mesh->faces.count; f++)
		{
			ufbx_face face = mesh->faces.data[f];

			uint32_t numTris = ufbx_triangulate_face(
				triBuf.data(),
				triBuf.size(),
				mesh,
				face
			);

			for (uint32_t t = 0; t < numTris * 3; t++)
			{
				uint32_t ix = triBuf[t];

				Vertex vert{};

				auto pos = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
				vert.position = { (float)pos.x,(float)pos.y,(float)pos.z,1 };

				if (mesh->vertex_normal.exists)
				{
					auto n = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
					vert.normal = { (float)n.x,(float)n.y,(float)n.z };
				}

				if (mesh->vertex_tangent.exists)
				{
					auto v = ufbx_get_vertex_vec3(&mesh->vertex_tangent, ix);
					vert.tangent = { (float)v.x,(float)v.y,(float)v.z };
				}

				if (mesh->vertex_bitangent.exists)
				{
					auto b = ufbx_get_vertex_vec3(&mesh->vertex_bitangent, ix);
					vert.binormal = { (float)b.x,(float)b.y,(float)b.z };
				}

				if (mesh->vertex_uv.exists)
				{
					auto uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, ix);
					vert.uvs[0] = { (float)uv.x,1.f - (float)uv.y };
				}

				vert.vertexColors[0] = { 1,1,1,1 };

				uint32_t vIndex = mesh->vertex_indices.data[ix];

				vert.bones = {
					(float)boneData[vIndex].IDs[0],
					(float)boneData[vIndex].IDs[1],
					(float)boneData[vIndex].IDs[2],
					(float)boneData[vIndex].IDs[3]
				};

				vert.weights = {
					boneData[vIndex].Weights[0],
					boneData[vIndex].Weights[1],
					boneData[vIndex].Weights[2],
					boneData[vIndex].Weights[3]
				};

				indices.push_back((uint32_t)vertices.size());
				vertices.push_back(vert);
			}
		}

		if (!mesh->vertex_tangent.exists)
		{
			GenerateTangents(vertices, indices);
		}

		meshData.vertices = std::move(vertices);
		meshData.indices = std::move(indices);

		meshData.bounds = CalculateBoxSphereBounds(meshData.vertices);
		meshData.name = mesh->name.data ? StringRegistry::RegisterOrGetString(mesh->name.data) : "Mesh"_tgaid;

		HRESULT result;

		D3D11_BUFFER_DESC vbDesc{};
		vbDesc.ByteWidth = UINT(meshData.vertices.size() * sizeof(Vertex));
		vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vbData{};
		vbData.pSysMem = meshData.vertices.data();

		ID3D11Buffer* vb;
		result = DX11::Device->CreateBuffer(&vbDesc, &vbData, &vb);
		if (FAILED(result)) return nullptr;

		D3D11_BUFFER_DESC ibDesc{};
		ibDesc.ByteWidth = UINT(meshData.indices.size() * sizeof(uint32_t));
		ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA ibData{};
		ibData.pSysMem = meshData.indices.data();

		ID3D11Buffer* ib;
		result = DX11::Device->CreateBuffer(&ibDesc, &ibData, &ib);
		if (FAILED(result)) return nullptr;

		meshData.vertexBuffer = vb;
		meshData.indexBuffer = ib;
		meshData.numberOfVertices = (UINT)meshData.vertices.size();
		meshData.numberOfIndices = (UINT)meshData.indices.size();
		meshData.stride = sizeof(Vertex);
		meshData.offset = 0;

		mdlMeshData.push_back(std::move(meshData));
	}

	auto model = std::make_shared<Model>();
	model->Init(mdlMeshData, std::string(resolved_path.GetStringView()));

	if (!mdlSkeleton.joints.empty())
		model->mySkeleton = std::make_shared<Skeleton>(std::move(mdlSkeleton));

	AssignDefaultMaterials(someFilePath, model.get());
	myLoadedModels[someFilePath] = model;

	ufbx_free_scene(scene);
	return model;
}


std::shared_ptr<const Animation> ModelFactory::GetAnimation(
	StringId someFilePath,
	const std::shared_ptr<const Skeleton>& aSkeleton)
{
	if (someFilePath.IsEmpty())
		return nullptr;

	FilePathStream resolvedPath;
	if (!Tga::Settings::ResolveAssetPath(someFilePath, resolvedPath))
		return nullptr;

	StringId resolvedPathId = StringRegistry::RegisterOrGetString(resolvedPath.GetStringView());

	auto it = myLoadedAnimations.find(AnimationIdentifer{ resolvedPathId, aSkeleton });
	if (it != myLoadedAnimations.end())
		return it->second;

	ufbx_load_opts opts = {
		.target_axes = FBX_AXIS,
	};
	ufbx_error error;
	ufbx_scene* scene = ufbx_load_file(resolvedPath.GetData(), &opts, &error);

	if (!scene) return nullptr;
	if (scene->anim_stacks.count == 0) return nullptr;

	ufbx_anim_stack* stack = scene->anim_stacks.data[0];
	ufbx_anim* anim = stack->anim;

	std::shared_ptr<Animation> animation = std::make_shared<Animation>();

	animation->name = someFilePath.GetString();

	float fps = 30.0f;
	animation->framesPerSecond = fps;

	animation->duration = (float)(stack->time_end - stack->time_begin);
	animation->length = (unsigned)(animation->duration * fps);

	animation->frames.resize(animation->length);

	const Skeleton& skeleton = *aSkeleton;

	ufbx_matrix skeletonParent = ufbx_identity_matrix;
	ufbx_matrix skeletonParentInv = ufbx_identity_matrix;

	// resolve root once
	const ufbx_node* skeletonRoot = ufbx_find_node(scene, skeleton.joints[0].name.c_str());
	if (skeletonRoot)
	{
		if (skeletonRoot->parent)
		{
			skeletonParent = skeletonRoot->parent->node_to_world;
			skeletonParentInv = ufbx_matrix_invert(&skeletonParent);
		}
	}
	for (unsigned int f = 0; f < animation->length; f++)
	{
		double time = stack->time_begin + (double)f / fps;

		ufbx_scene* eval = ufbx_evaluate_scene(scene, anim, time, nullptr, nullptr);

		animation->frames[f].count = skeleton.joints.size();

		for (size_t j = 0; j < skeleton.joints.size(); j++)
		{
			const std::string& jointName = skeleton.joints[j].name;

			ufbx_node* sourceNode = ufbx_find_node(scene, jointName.c_str());
			if (!sourceNode)
				continue;

			ufbx_node* node = eval->nodes[sourceNode->typed_id];

			ufbx_matrix original = node->node_to_parent;

 			// Compensate for transforms in nodes above the skeleton (blender tends to create this)
			ufbx_matrix tmp = ufbx_matrix_mul(&original, &skeletonParentInv);
			ufbx_matrix fixed = ufbx_matrix_mul(&skeletonParent, &tmp);

			Matrix4x4f mat = ConvertMatrix(fixed);

			animation->frames[f].jointTransforms[j] = ScaleRotationTranslationf::CreateFromMatrix(mat);
			
		}

		ufbx_free_scene(eval);
	}

	myLoadedAnimations[AnimationIdentifer{ resolvedPathId, aSkeleton }] = animation;

	ufbx_free_scene(scene);
	return animation;
}

#else
std::shared_ptr<Model> ModelFactory::LoadModel(StringId someFilePath)
{
	if (someFilePath.IsEmpty())
		return nullptr;

	FilePathStream resolved_path;
	if (!Tga::Settings::ResolveAssetPath(someFilePath, resolved_path))
		return nullptr;

	if (myWatchedPaths.find(someFilePath) == myWatchedPaths.end())
	{
		myWatchedPaths.insert(someFilePath);
		Application::GetInstance()->GetFileWatcher()->WatchFileChange(resolved_path.GetStringView(), std::bind(&Tga::ModelFactory::OnModelChanged, this, someFilePath));
	}

	TGA::FBX::Mesh tgaModel;
	if (TGA::FBX::Importer::LoadMeshA(resolved_path.GetData(), tgaModel))
	{
		Skeleton mdlSkeleton;

		if (tgaModel.Skeleton.GetRoot())
		{
			mdlSkeleton.joints.resize(tgaModel.Skeleton.Bones.size());
			mdlSkeleton.jointNameToIndex.reserve(mdlSkeleton.joints.size());
			mdlSkeleton.jointNames.resize(mdlSkeleton.joints.size());
			for (size_t j = 0; j < tgaModel.Skeleton.Bones.size(); j++)
			{
				Skeleton::Joint& mdlJoint = mdlSkeleton.joints[j];
				TGA::FBX::Skeleton::Bone& tgaJoint = tgaModel.Skeleton.Bones[j];

				Matrix4x4f bindPoseInverseTranspose;
				memcpy(&bindPoseInverseTranspose, &tgaJoint.BindPoseInverse, sizeof(float) * 16);

				mdlJoint.bindPoseInverse = Matrix4x4f::Transpose(bindPoseInverseTranspose);

				mdlSkeleton.modelBindPose.jointTransforms[j] = mdlJoint.bindPoseInverse.GetInverse();

				mdlJoint.name = tgaJoint.Name;
				mdlJoint.parent = tgaJoint.ParentIdx;
				mdlJoint.children = tgaJoint.Children;

				mdlSkeleton.jointNameToIndex.insert({ mdlJoint.name, j });
				mdlSkeleton.jointNames[j] = mdlJoint.name;
			}
			assert(MAX_ANIMATION_BONES >= mdlSkeleton.joints.size() && "More joints in animation than defined in EngineDefines.h");

			mdlSkeleton.ConvertPoseToLocalSpace(mdlSkeleton.modelBindPose, mdlSkeleton.localBindPose);
		}

		std::vector<Model::MeshData> mdlMeshData;
		mdlMeshData.resize(tgaModel.Elements.size());

		// Convert model to our own format.
		for (size_t i = 0; i < tgaModel.Elements.size(); i++)
		{
			// The imported element data.
			TGA::FBX::Mesh::Element& element = tgaModel.Elements[i];

			// And where we'll put it in our structures.
			Model::MeshData& meshData = mdlMeshData[i];
			meshData.vertices.resize(element.Vertices.size());

			// Convert vertices to your own format
			//std::vector<Vertex> mdlVertices;
			//mdlVertices.resize(element.Vertices.size());

			for (size_t v = 0; v < element.Vertices.size(); v++)
			{
				// The most important part, the position!
				meshData.vertices[v].position = {
					element.Vertices[v].Position[0],
					element.Vertices[v].Position[1],
					element.Vertices[v].Position[2],
					element.Vertices[v].Position[3]
				};

				// All four vertex color channels I have.
				for (int vCol = 0; vCol < 4; vCol++)
				{
					meshData.vertices[v].vertexColors[vCol] = {
						element.Vertices[v].VertexColors[vCol][0],
						element.Vertices[v].VertexColors[vCol][1],
						element.Vertices[v].VertexColors[vCol][2],
						element.Vertices[v].VertexColors[vCol][3]
					};
					}

				meshData.vertices[v].normal = Vector3f(element.Vertices[v].Normal[0], element.Vertices[v].Normal[1], element.Vertices[v].Normal[2]);
				meshData.vertices[v].binormal = Vector3f(element.Vertices[v].BiNormal[0], element.Vertices[v].BiNormal[1], element.Vertices[v].BiNormal[2]);
				meshData.vertices[v].tangent = Vector3f(element.Vertices[v].Tangent[0], element.Vertices[v].Tangent[1], element.Vertices[v].Tangent[2]);

				for (unsigned int UVch = 0; UVch < 4; UVch++)
				{
					meshData.vertices[v].uvs[UVch] = {
						 element.Vertices[v].UVs[UVch][0],
						 element.Vertices[v].UVs[UVch][1]
					};
				}

				meshData.vertices[v].bones = {
					static_cast<float>(element.Vertices[v].BoneIDs[0]),
					static_cast<float>(element.Vertices[v].BoneIDs[1]),
					static_cast<float>(element.Vertices[v].BoneIDs[2]),
					static_cast<float>(element.Vertices[v].BoneIDs[3])
				};

				meshData.vertices[v].weights = {
					element.Vertices[v].BoneWeights[0],
					element.Vertices[v].BoneWeights[1],
					element.Vertices[v].BoneWeights[2],
					element.Vertices[v].BoneWeights[3]
				};
			}

			//std::vector<unsigned int>& mdlIndices = element.Indices;
			meshData.indices.resize(element.Indices.size());
			memcpy(meshData.indices.data(), element.Indices.data(), sizeof(unsigned int) * element.Indices.size());
			//meshData.Indices = element.Indices;

			HRESULT result;

			D3D11_BUFFER_DESC vertexBufferDesc{};
			vertexBufferDesc.ByteWidth = static_cast<UINT>(meshData.vertices.size()) * static_cast<UINT>(sizeof(Vertex));
			vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA vertexSubresourceData{};
			vertexSubresourceData.pSysMem = &meshData.vertices[0];

			ID3D11Buffer* vertexBuffer;
			result = DX11::Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
			if (FAILED(result))
			{
				return nullptr;
			}

			D3D11_BUFFER_DESC indexBufferDesc{};
			indexBufferDesc.ByteWidth = static_cast<UINT>(meshData.indices.size()) * static_cast<UINT>(sizeof(float)); // TODO: What :P Sizeof should be uint.
			indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA indexSubresourceData{};
			indexSubresourceData.pSysMem = &meshData.indices[0];

			ID3D11Buffer* indexBuffer;
			result = DX11::Device->CreateBuffer(&indexBufferDesc, &indexSubresourceData, &indexBuffer);
			if (FAILED(result))
			{
				return nullptr;
			}

			meshData.numberOfVertices = static_cast<UINT>(meshData.vertices.size());
			meshData.numberOfIndices = static_cast<UINT>(meshData.indices.size());
			meshData.stride = sizeof(Vertex);
			meshData.offset = 0;
			meshData.vertexBuffer = vertexBuffer;
			meshData.indexBuffer = indexBuffer;
			meshData.name = StringRegistry::RegisterOrGetString(element.MeshName);
			if (tgaModel.Materials.size() > element.MaterialIndex)
			{
				meshData.materialName = StringRegistry::RegisterOrGetString(tgaModel.Materials[element.MaterialIndex].MaterialName);
			}
			else
			{
				meshData.materialName = ""_tgaid;
			}
			meshData.bounds = CalculateBoxSphereBounds(meshData.vertices);
		}

		std::shared_ptr<Model> model = std::make_shared<Model>();

		model->Init(mdlMeshData, std::string(resolved_path.GetStringView()));
		if (mdlSkeleton.joints.size() > 0)
		{
			model->mySkeleton = std::make_shared<Skeleton>(std::move(mdlSkeleton));
		}
		AssignDefaultMaterials(someFilePath, model.get());
		myLoadedModels.insert(std::pair<StringId, std::shared_ptr<Model>>(someFilePath, model));

		return model;
	}

	return nullptr;
}

std::shared_ptr<const Animation> ModelFactory::GetAnimation(StringId someFilePath, const std::shared_ptr<const Skeleton>& aSkeleton)
{
	if (someFilePath.IsEmpty())
		return nullptr;
	FilePathStream resolvedPath;
	if (!Tga::Settings::ResolveAssetPath(someFilePath, resolvedPath))
		return nullptr;
	StringId resolvedPathId = StringRegistry::RegisterOrGetString(resolvedPath.GetStringView());

	// The FBX SDK doesn't like widechar :(.
	TGA::FBX::Animation fbxAnimation;

	auto it = myLoadedAnimations.find(AnimationIdentifer{ resolvedPathId , aSkeleton });
	if (it != myLoadedAnimations.end())
		return it->second;

	if (TGA::FBX::Importer::LoadAnimationA(resolvedPath.GetData(), fbxAnimation))
	{
		std::shared_ptr<Animation> animation = std::make_shared<Animation>();
		animation->name = fbxAnimation.Name;
		animation->length = fbxAnimation.Length;
		animation->framesPerSecond = fbxAnimation.FramesPerSecond;
		animation->frames.resize(fbxAnimation.Frames.size());
		animation->duration = static_cast<float>(fbxAnimation.Duration);

		const Tga::Skeleton& skeleton = *aSkeleton;
		
		for (size_t f = 0; f < animation->frames.size(); f++)
		{
			animation->frames[f].count = skeleton.joints.size();
			for (const auto& [boneName, boneTransform] : fbxAnimation.Frames[f].LocalTransforms)
			{
				Matrix4x4f localMatrix;
				memcpy_s(&localMatrix, sizeof(Matrix4x4f), boneTransform.Data, sizeof(float) * 16);

				auto jointIt = skeleton.jointNameToIndex.find(boneName);

				if (jointIt != skeleton.jointNameToIndex.end())
					animation->frames[f].jointTransforms[jointIt->second] = ScaleRotationTranslationf::CreateFromMatrix(localMatrix);
			}
		}

		myLoadedAnimations[AnimationIdentifer{ resolvedPathId , aSkeleton }] = animation;

		return animation;
	}

	return nullptr;
}
#endif

Tga::BoxSphereBounds Tga::ModelFactory::CalculateBoxSphereBounds(std::vector<Tga::Vertex> somePositions)
{
	Vector3f minExtents = FLT_MAX;
	Vector3f maxExtents = -FLT_MAX;

	for (unsigned int v = 0; v < somePositions.size(); v++)
	{
		if (somePositions[v].position.x > maxExtents.x)
			maxExtents.x = somePositions[v].position.x;
		if (somePositions[v].position.y > maxExtents.y)
			maxExtents.y = somePositions[v].position.y;
		if (somePositions[v].position.z > maxExtents.z)
			maxExtents.z = somePositions[v].position.z;

		if (somePositions[v].position.x < minExtents.x)
			minExtents.x = somePositions[v].position.x;
		if (somePositions[v].position.y < minExtents.y)
			minExtents.y = somePositions[v].position.y;
		if (somePositions[v].position.z < minExtents.z)
			minExtents.z = somePositions[v].position.z;
	}

	const Vector3f extentsCenter = 0.5f * (minExtents + maxExtents);
	const Vector3f boxExtents = 0.5f * (maxExtents - minExtents);
	const float radius = boxExtents.Length();
	return { radius, boxExtents, extentsCenter };
}


AnimationPlayer ModelFactory::GetAnimationPlayer(StringId someFilePath, const std::shared_ptr<const Skeleton>& aSkeleton)
{
	AnimationPlayer instance;
	std::shared_ptr<const Animation> animation = GetAnimation(someFilePath, aSkeleton);
	if (animation)
		instance.Init(animation);

	return instance;
}

std::shared_ptr<Model> ModelFactory::GetModel(StringId someFilePath)
{
	auto it = myLoadedModels.find(someFilePath);
	if (it != myLoadedModels.end())
		return it->second;

	return LoadModel(someFilePath);
}

void ModelFactory::OnModelChanged(StringId aUnresolvedPath)
{
	myLoadedModels.erase(aUnresolvedPath);
}
std::shared_ptr<Model> ModelFactory::GetModel(std::string_view aFilePath) { return GetModel(StringRegistry::RegisterOrGetString(aFilePath)); }

AnimatedModelInstance ModelFactory::GetAnimatedModelInstance(std::string_view aFilePath) { return GetAnimatedModelInstance(StringRegistry::RegisterOrGetString(aFilePath)); }

ModelInstance ModelFactory::GetModelInstance(std::string_view aFilePath) { return GetModelInstance(StringRegistry::RegisterOrGetString(aFilePath)); }

std::shared_ptr<const Animation> ModelFactory::GetAnimation(std::string_view aFilePath, const std::shared_ptr<const Skeleton>& aSkeleton) { return GetAnimation(StringRegistry::RegisterOrGetString(aFilePath), aSkeleton); }

AnimationPlayer ModelFactory::GetAnimationPlayer(std::string_view aFilePath, const std::shared_ptr<const Skeleton>& aSkeleton) { return GetAnimationPlayer(StringRegistry::RegisterOrGetString(aFilePath), aSkeleton); }
