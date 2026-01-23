#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Dx11Renderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Foundation/Mesh/Transform.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Core/SwapChain.hpp"
#include "Render/DX11/Foundation/Core/DepthStencilBuffer.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX11/Foundation/Resource/MaterialData.hpp"
#include "Render/DX11/Foundation/ConstantBuffer.h"
#include "Render/DX11/Foundation/RenderItem.hpp"
#include "Render/DX11/Shading/Util/ShadingObjectManager.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Render/DX11/Shading/GBuffer.hpp"

using namespace Render::DX11;
using namespace DirectX;

extern "C" RendererAPI Common::Render::Renderer* Render::CreateRenderer() {
	return new Dx11Renderer();
}

extern "C" RendererAPI void Render::DestroyRenderer(Common::Render::Renderer* const renderer) {
	delete renderer;
}

Dx11Renderer::Dx11Renderer() {
	mShadingObjectManager = std::make_unique<Shading::Util::ShadingObjectManager>();
	mShaderManager = std::make_unique<Shading::Util::ShaderManager>();

	mFrameResource = std::make_unique<Foundation::Resource::FrameResource>();

	mShadingObjectManager->Add<Shading::GBuffer::GBufferClass>();
}

Dx11Renderer::~Dx11Renderer() { CleanUp(); }

BOOL Dx11Renderer::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Common::Foundation::Core::WindowsManager* const pWndManager,
		Common::ImGuiManager::ImGuiManager* const pImGuiManager,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		UINT width, UINT height) {
	CheckReturn(pLogFile, Dx11LowRenderer::Initialize(
		pLogFile, pWndManager, pImGuiManager, pArgSet, width, height));

	CheckReturn(mpLogFile, mShadingObjectManager->Initialize(mpLogFile));
	CheckReturn(mpLogFile, mShaderManager->Initialize(
		mpLogFile, L".\\..\\..\\..\\assets\\Shaders\\HLSL5\\"));

	CheckReturn(mpLogFile, mFrameResource->Initalize(mpLogFile, mDevice.get()));

	// GBuffer
	{
		auto initData = Shading::GBuffer::MakeInitData();
		initData->Width = mClientWidth;
		initData->Height = mClientHeight;
		initData->Device = mDevice.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}

	CheckReturn(mpLogFile, mShadingObjectManager->CompileShaders());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineStates());

	return TRUE;
}

void Dx11Renderer::CleanUp() {
	if (mbCleanedUp) return;

	mDevice->Flush();

	mRenderItems.clear();

	for (auto& meshGeo : mMeshGeometries) {
		meshGeo.second->CleanUp();
	}

	if (mFrameResource) {
		mFrameResource->CleanUp();
		mFrameResource.reset();
	}
	if (mShaderManager) {
		mShaderManager->CleanUp();
		mShaderManager.reset();
	}
	if (mShadingObjectManager) {
		mShadingObjectManager->CleanUp();
		mShadingObjectManager.reset();
	}

	Dx11LowRenderer::CleanUp();
}

BOOL Dx11Renderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, Dx11LowRenderer::OnResize(width, height));

	CheckReturn(mpLogFile, mShadingObjectManager->OnResize(width, height));

#ifdef _DEBUG
	std::cout << std::format("Dx11Renderer resized (Width: {}, Height: {}", width, height) << std::endl;
#endif

	return TRUE;
}

BOOL Dx11Renderer::Update(FLOAT deltaTime) {
	CheckReturn(mpLogFile, mShadingObjectManager->Update());

	CheckReturn(mpLogFile, UpdateCB());

	return TRUE;
}

BOOL Dx11Renderer::Draw() {
	auto ritems = mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Opaque];

	const auto gbuffer = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();
	CheckReturn(mpLogFile, gbuffer->DrawGBuffer(
		mFrameResource.get(),
		mSwapChain->ScreenViewport(),
		mDepthStencilBuffer->DepthStencilView(),
		ritems.data(), static_cast<UINT>(ritems.size())));

	CheckReturn(mpLogFile, mSwapChain->Present());

	return TRUE;
}

BOOL Dx11Renderer::AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Mesh::Transform* const pTransform, Common::Foundation::Hash& hash) {
	auto meshGeo = std::make_unique<Foundation::Resource::MeshGeometry>();
	CheckReturn(mpLogFile, meshGeo->Initialize(
		mpLogFile, mDevice.get(),
		pMesh->Vertices(), pMesh->VerticesByteSize(),
		pMesh->Indices(), pMesh->IndicesByteSize(), pMesh->IndexCount()));

	std::vector<Common::Foundation::Mesh::Mesh::SubsetPair> subsets;
	pMesh->Subsets(subsets);

	for (const auto& subset : subsets) {
		Foundation::Resource::SubmeshGeometry submesh;
		submesh.StartIndexLocation = subset.second.StartIndexLocation;
		submesh.BaseVertexLocation = 0;
		submesh.IndexCount = subset.second.Size;
		meshGeo->Subsets[subset.first] = submesh;
	}

	mbMeshGeometryAdded = TRUE;

	CheckReturn(mpLogFile, BuildRenderItem(pMesh, pTransform, hash, meshGeo.get()));	

	mMeshGeometries[hash] = std::move(meshGeo);

	return TRUE;
}

BOOL Dx11Renderer::UpdateMeshTransform(Common::Foundation::Hash hash, Common::Foundation::Mesh::Transform* const pTransform) {
	if (mRenderItemRefs.find(hash) == mRenderItemRefs.end()) return TRUE;

	const auto ritem = mRenderItemRefs[hash];

	ritem->PrevWorld = ritem->World;
	XMStoreFloat4x4(
		&ritem->World,
		XMMatrixAffineTransformation(
			pTransform->Scale,
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			pTransform->Rotation,
			pTransform->Position
		)
	);
	ritem->FrameDirty = TRUE;

	return TRUE;
}

void Dx11Renderer::RemoveMesh(Common::Foundation::Hash hash) {}

BOOL Dx11Renderer::UpdateCB() {
	CheckReturn(mpLogFile, UpdatePassCB());
	CheckReturn(mpLogFile, UpdateObjectCB());
	CheckReturn(mpLogFile, UpdateMaterialCB());

	return TRUE;
}

BOOL Dx11Renderer::UpdatePassCB() {
	static UINT counter = 0;

	PassCB passCB{};
	passCB.FrameCount = counter++;

	const XMMATRIX view = XMLoadFloat4x4(&mpCamera->View());
	const XMMATRIX proj = XMLoadFloat4x4(&mpCamera->Proj());
	const XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMStoreFloat4x4(&passCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&passCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&passCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat3(&passCB.EyePosW, mpCamera->Position());

	//const auto OffsetIndex = static_cast<UINT>(counter % mTAA->HaltonSequenceSize());
	//passCB.JitteredOffset = mTAA->HaltonSequence(OffsetIndex);

	CheckReturn(mpLogFile, mFrameResource->PassCB.BeginFrame());
	mFrameResource->PassCB.CopyData(passCB);
	mFrameResource->PassCB.EndFrame();

	return TRUE;
}

BOOL Dx11Renderer::UpdateObjectCB() {
	CheckReturn(mpLogFile, mFrameResource->ObjectCB.BeginFrame());

	for (auto& ritem : mRenderItems) {
		if (ritem->FrameDirty) {
			const XMMATRIX World = XMLoadFloat4x4(&ritem->World);

			ObjectCB objCB{};
			XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(World));

			mFrameResource->ObjectCB.CopyData(objCB, ritem->ObjectCBIndex);

			ritem->FrameDirty = FALSE;
		}
	}

	mFrameResource->ObjectCB.EndFrame();

	return TRUE;
}

BOOL Dx11Renderer::UpdateMaterialCB() {
	CheckReturn(mpLogFile, mFrameResource->MaterialCB.BeginFrame());

	for (auto& material : mMaterials) {
		if (material->FrameDirty) {
			MaterialCB matCB{};
			matCB.Albedo = material->Albedo;
			matCB.Roughness = material->Roughness;
			matCB.Metalness = material->Metalness;
			matCB.Specular = material->Specular;

			mFrameResource->MaterialCB.CopyData(matCB, material->MaterialCBIndex);

			material->FrameDirty = FALSE;
		}
	}

	mFrameResource->MaterialCB.EndFrame();

	return TRUE;
}

BOOL Dx11Renderer::BuildRenderItem(
		Common::Foundation::Mesh::Mesh* const pMesh,
		Common::Foundation::Mesh::Transform* const pTransform,
		Common::Foundation::Hash& hash,
		Foundation::Resource::MeshGeometry* pMeshGeo) {
	for (UINT count = 0; const auto& subset : pMeshGeo->Subsets) {
		auto ritem = std::make_unique<Foundation::RenderItem>();
		hash = Foundation::RenderItem::Hash(ritem.get());

		ritem->ObjectCBIndex = static_cast<INT>(mRenderItems.size());
		ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ritem->Geometry = pMeshGeo;
		ritem->IndexCount = pMeshGeo->Subsets[subset.first].IndexCount;
		ritem->StartIndexLocation = pMeshGeo->Subsets[subset.first].StartIndexLocation;
		ritem->BaseVertexLocation = pMeshGeo->Subsets[subset.first].BaseVertexLocation;
		XMStoreFloat4x4(
			&ritem->World,
			XMMatrixAffineTransformation(
				pTransform->Scale,
				XMVectorSet(0.f, 0.f, 0.f, 1.f),
				pTransform->Rotation,
				pTransform->Position
			)
		);

		auto material = pMesh->GetMaterial(count++);

		CheckReturn(mpLogFile, BuildMeshMaterial(&material, ritem->Material));
		
		mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Opaque].push_back(ritem.get());
		mRenderItemRefs[hash] = ritem.get();
		mRenderItems.push_back(std::move(ritem));
	}

	return TRUE;
}

BOOL Dx11Renderer::BuildMeshMaterial(
		Common::Foundation::Mesh::Material* const pMaterial,
		Foundation::Resource::MaterialData*& pMatData) {
	auto matData = std::make_unique<Foundation::Resource::MaterialData>();

	CheckReturn(mpLogFile, BuildMeshTextures(pMaterial, matData.get()));

	matData->MaterialCBIndex = static_cast<INT>(mMaterials.size());
	matData->Albedo = pMaterial->Albedo;
	matData->Specular = pMaterial->Specular;
	matData->Roughness = pMaterial->Roughness;
	matData->Metalness = pMaterial->Metalness;

	pMatData = matData.get();

	mMaterials.push_back(std::move(matData));

	return TRUE;
}

BOOL Dx11Renderer::BuildMeshTextures(
		Common::Foundation::Mesh::Material* const pMaterial,
		Foundation::Resource::MaterialData* const pMatData) {
	if (!pMaterial->AlbedoMap.empty()) {

	}
	if (!pMaterial->NormalMap.empty()) {

	}
	if (!pMaterial->AlphaMap.empty()) {

	}
	if (!pMaterial->RoughnessMap.empty()) {

	}
	if (!pMaterial->MetalnessMap.empty()) {

	}
	if (!pMaterial->SpecularMap.empty()) {

	}

	return TRUE;
}