#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Foundation/Mesh/Transform.hpp"
#include "Common/Render/ShadingArgument.hpp"
#include "Common/Util/MathUtil.hpp"
#include "Render/DX/Foundation/ConstantBuffer.h"
#include "Render/DX/Foundation/RenderItem.hpp"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Core/DepthStencilBuffer.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Resource/MaterialData.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShadingObjectManager.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Render/DX/Shading/Util/EquirectangularConverter.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"
#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "Render/DX/Shading/GammaCorrection.hpp"
#include "Render/DX/Shading/ToneMapping.hpp"
#include "Render/DX/Shading/GBuffer.hpp"
#include "Render/DX/Shading/BRDF.hpp"
#include "Render/DX/Shading/Shadow.hpp"
#include "FrankLuna/GeometryGenerator.h"

using namespace Render::DX;
using namespace DirectX;

extern "C" RendererAPI Common::Render::Renderer* Render::CreateRenderer() {
	return new DxRenderer();
}

extern "C" RendererAPI void Render::DestroyRenderer(Common::Render::Renderer* const renderer) {
	delete renderer;
}

DxRenderer::DxRenderer() {
	// Shading objets
	mShadingObjectManager = std::make_unique<Shading::Util::ShadingObjectManager>();
	mShaderManager = std::make_unique<Shading::Util::ShaderManager>();

	mMipmapGenerator = std::make_unique<Shading::Util::MipmapGenerator::MipmapGeneratorClass>();
	mEquirectangularConverter = std::make_unique<Shading::Util::EquirectangularConverter::EquirectangularConverterClass>();

	mEnvironmentMap = std::make_unique<Shading::EnvironmentMap::EnvironmentMapClass>();
	mGammaCorrection = std::make_unique<Shading::GammaCorrection::GammaCorrectionClass>();
	mToneMapping = std::make_unique<Shading::ToneMapping::ToneMappingClass>();
	mGBuffer = std::make_unique<Shading::GBuffer::GBufferClass>();
	mBRDF = std::make_unique<Shading::BRDF::BRDFClass>();
	mShadow = std::make_unique<Shading::Shadow::ShadowClass>();

	mShadingObjectManager->AddShadingObject(mMipmapGenerator.get());
	mShadingObjectManager->AddShadingObject(mEquirectangularConverter.get());
	mShadingObjectManager->AddShadingObject(mEnvironmentMap.get());
	mShadingObjectManager->AddShadingObject(mGammaCorrection.get());
	mShadingObjectManager->AddShadingObject(mToneMapping.get());
	mShadingObjectManager->AddShadingObject(mGBuffer.get());
	mShadingObjectManager->AddShadingObject(mBRDF.get());
	mShadingObjectManager->AddShadingObject(mShadow.get());

	// Constant buffers
	mMainPassCB = std::make_unique<ConstantBuffers::PassCB>();
	mProjectToCubeCB = std::make_unique<ConstantBuffers::ProjectToCubeCB>();
}

DxRenderer::~DxRenderer() {
	CleanUp();
}

BOOL DxRenderer::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		Common::Foundation::Core::WindowsManager* const pWndManager, 
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::Initialize(pLogFile, pWndManager, pArgSet, width, height));

	CheckReturn(mpLogFile, InitShadingObjects());
	CheckReturn(mpLogFile, BuildFrameResources());

	CheckReturn(mpLogFile, mShadingObjectManager->CompileShaders(mShaderManager.get(), L".\\..\\..\\..\\..\\assets\\Shaders\\HLSL\\"));
	CheckReturn(mpLogFile, mShadingObjectManager->BuildRootSignatures());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineStates());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildDescriptors(mDescriptorHeap.get()));

	CheckReturn(mpLogFile, BuildScene());

	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	return TRUE;
}

void DxRenderer::CleanUp() {
	DxLowRenderer::CleanUp();
}

BOOL DxRenderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::OnResize(width, height));

	CheckReturn(mpLogFile, mShadingObjectManager->OnResize(width, height));

#ifdef _DEBUG
	std::cout << "DxRenderer resized (Width: " << width << " Height: " << height << ")" << std::endl;
#endif

	return TRUE;
}

BOOL DxRenderer::Update(FLOAT deltaTime) {
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % Foundation::Resource::FrameResource::Count;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
	CheckReturn(mpLogFile, mCommandObject->WaitCompletion(mCurrentFrameResource->mFence));
	
	CheckReturn(mpLogFile, UpdateConstantBuffers());

	return TRUE;
}

BOOL DxRenderer::Draw() {
	CheckReturn(mpLogFile, mCurrentFrameResource->ResetCommandListAllocators());

	const auto skySphere = mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Sky].front();
	const auto& opaques = mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Opaque];

	std::vector<Foundation::RenderItem*> opaquesReadyToDraw;
	for (const auto opaque : opaques) {
		if (mCurrentFrameResource->mFence < opaque->Geometry->Fence) continue;

		opaquesReadyToDraw.push_back(opaque);
	}
	
	CheckReturn(mpLogFile, mShadow->Run(
		mCurrentFrameResource,
		opaquesReadyToDraw));

	CheckReturn(mpLogFile, mGBuffer->DrawGBuffer(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(), 
		mSwapChain->ScissorRect(),
		mToneMapping->InterMediateMapResource(), 
		mToneMapping->InterMediateMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(), 
		mDepthStencilBuffer->DepthStencilBufferDsv(),
		opaquesReadyToDraw));

	CheckReturn(mpLogFile, mBRDF->IntegrateDiffuse(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mToneMapping->InterMediateMapResource(),
		mToneMapping->InterMediateMapRtv(),
		mGBuffer->AlbedoMap(),
		mGBuffer->AlbedoMapSrv(),
		mGBuffer->NormalMap(), 
		mGBuffer->NormalMapSrv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(), 
		mDepthStencilBuffer->DepthStencilBufferSrv(),
		mGBuffer->SpecularMap(),
		mGBuffer->SpecularMapSrv(),
		mGBuffer->RoughnessMetalnessMap(),
		mGBuffer->RoughnessMetalnessMapSrv(),
		mGBuffer->PositionMap(),
		mGBuffer->PositionMapSrv(),
		mEnvironmentMap->DiffuseIrradianceCubeMap(), 
		mEnvironmentMap->DiffuseIrradianceCubeMapSrv()));

	CheckReturn(mpLogFile, mBRDF->IntegrateSpecular(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(), 
		mSwapChain->ScissorRect(),
		mToneMapping->InterMediateMapResource(),
		mToneMapping->InterMediateMapRtv(),
		mToneMapping->InterMediateCopyMapResource(), 
		mToneMapping->InterMediateCopyMapSrv(),
		mGBuffer->AlbedoMap(), 
		mGBuffer->AlbedoMapSrv(),
		mGBuffer->NormalMap(),
		mGBuffer->NormalMapSrv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->DepthStencilBufferSrv(),
		mGBuffer->SpecularMap(), 
		mGBuffer->SpecularMapSrv(),
		mGBuffer->RoughnessMetalnessMap(), 
		mGBuffer->RoughnessMetalnessMapSrv(),
		mGBuffer->PositionMap(),
		mGBuffer->PositionMapSrv(),
		mEnvironmentMap->BrdfLutMap(),
		mEnvironmentMap->BrdfLutMapSrv(),
		mEnvironmentMap->PrefilteredEnvironmentCubeMap(),
		mEnvironmentMap->PrefilteredEnvironmentCubeMapSrv()));

	CheckReturn(mpLogFile, mEnvironmentMap->DrawSkySphere(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mToneMapping->InterMediateMapResource(),
		mToneMapping->InterMediateMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(), 
		mDepthStencilBuffer->DepthStencilBufferDsv(),
		skySphere));

	CheckReturn(mpLogFile, mToneMapping->Resolve(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mSwapChain->BackBuffer(),
		mSwapChain->BackBufferRtv(),
		mpArgumentSet->ToneMapping.Exposure));

	CheckReturn(mpLogFile, mGammaCorrection->ApplyCorrection(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mSwapChain->BackBuffer(),
		mSwapChain->BackBufferRtv(),
		mSwapChain->BackBufferCopy(),
		mSwapChain->BackBufferCopySrv(),
		mpArgumentSet->GammaCorrection.Gamma));
	
	CheckReturn(mpLogFile, PresentAndSignal());

	return TRUE;
}

BOOL DxRenderer::AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Hash& hash) {
	CheckReturn(mpLogFile, mCommandObject->ResetCommandList(
		mCurrentFrameResource->CommandAllocator(0),
		0));
	const auto CmdList = mCommandObject->CommandList(0);

	{
		Foundation::Resource::MeshGeometry* meshGeo;
		CheckReturn(mpLogFile, BuildMeshGeometry(CmdList, pMesh, meshGeo));

		UINT count = 0;

		for (const auto& subset : meshGeo->Subsets) {
			auto ritem = std::make_unique<Foundation::RenderItem>(Foundation::Resource::FrameResource::Count);
			hash = Foundation::RenderItem::Hash(ritem.get());

			ritem->ObjectCBIndex = static_cast<INT>(mRenderItems.size());
			ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			ritem->Geometry = meshGeo;
			ritem->IndexCount = meshGeo->Subsets[subset.first].IndexCount;
			ritem->StartIndexLocation = meshGeo->Subsets[subset.first].StartIndexLocation;
			ritem->BaseVertexLocation = meshGeo->Subsets[subset.first].BaseVertexLocation;
			
			auto material = pMesh->GetMaterial(count++);

			CheckReturn(mpLogFile, BuildMeshMaterial(CmdList, &material, ritem->Material));

			mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Opaque].push_back(ritem.get());
			mRenderItemRefs[hash] = ritem.get();
			mRenderItems.push_back(std::move(ritem));
		}
	}

	CheckReturn(mpLogFile, mCommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL DxRenderer::UpdateMeshTransform(Common::Foundation::Hash hash, Common::Foundation::Mesh::Transform* const pTransform) {
	if (mRenderItemRefs.find(hash) == mRenderItemRefs.end()) return TRUE;

	const auto ritem = mRenderItemRefs[hash];
	XMStoreFloat4x4(
		&ritem->World,
		XMMatrixAffineTransformation(
			pTransform->Scale,
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			pTransform->Rotation,
			pTransform->Position
		)
	);

	return TRUE;
}

void DxRenderer::RemoveMesh(Common::Foundation::Hash hash) {
	if (mRenderItemRefs.find(hash) == mRenderItemRefs.end()) return;
}

BOOL DxRenderer::CreateDescriptorHeaps() {
	UINT cbvSrvUavCount = mShadingObjectManager->CbvSrvUavDescCount();
	UINT rtvCount = mShadingObjectManager->RtvDescCount();
	UINT dsvCount = mShadingObjectManager->DsvDescCount();

	CheckReturn(mpLogFile, mDescriptorHeap->CreateDescriptorHeaps(cbvSrvUavCount, rtvCount, dsvCount));

	return TRUE;
}

BOOL DxRenderer::UpdateConstantBuffers() {
	CheckReturn(mpLogFile, UpdateLightCB());
	CheckReturn(mpLogFile, UpdateMainPassCB());
	CheckReturn(mpLogFile, UpdateObjectCB());
	CheckReturn(mpLogFile, UpdateMaterialCB());
	CheckReturn(mpLogFile, UpdateProjectToCubeCB());

	return TRUE;
}

BOOL DxRenderer::UpdateMainPassCB() {
	if (mpCamera == nullptr) return TRUE;

	// Transform NDC space [-1 , +1]^2 to texture space [0, 1]^2
	const XMMATRIX T(
		0.5f, 0.f,  0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f,  0.f,  1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	const XMMATRIX view = XMLoadFloat4x4(&mpCamera->View());
	const XMMATRIX proj = XMLoadFloat4x4(&mpCamera->Proj());
	const XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	
	const XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	const XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	const XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
	
	const XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);
	
	mMainPassCB->PrevViewProj = mMainPassCB->ViewProj;
	XMStoreFloat4x4(&mMainPassCB->View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB->InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB->Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB->InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB->ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB->InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mMainPassCB->ViewProjTex, XMMatrixTranspose(viewProjTex));
	XMStoreFloat3(&mMainPassCB->EyePosW, mpCamera->Position());

	//const size_t offsetIndex = static_cast<size_t>(mCommandObject->CurrentFence()) % mFittedToBakcBufferHaltonSequence.size();
	//mMainPassCB->JitteredOffset = bTaaEnabled ? mFittedToBakcBufferHaltonSequence[offsetIndex] : XMFLOAT2(0.f, 0.f);
	
	mCurrentFrameResource->CopyMainPassCB(0, *mMainPassCB.get());

	return TRUE;
}

BOOL DxRenderer::UpdateLightCB() {
	for (UINT i = 0; i < mShadow->LightCount(); ++i) {
		auto& light = mShadow->Light(i);

		if (light.Type == Common::Render::LightType::E_Directional) {
			const XMMATRIX T(
				0.5f, 0.f, 0.f, 0.f,
				0.f, -0.5f, 0.f, 0.f,
				0.f, 0.f, 1.f, 0.f,
				0.5f, 0.5f, 0.f, 1.f
			);

			const XMVECTOR lightDir = XMLoadFloat3(&light.Direction);
			const XMVECTOR lightPos = -2.f * mSceneBounds.Radius * lightDir;
			const XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
			const XMVECTOR lightUp = UnitVector::UpVector;
			const XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

			// Transform bounding sphere to light space.
			XMFLOAT3 sphereCenterLS;
			XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

			// Ortho frustum in light space encloses scene.
			const FLOAT l = sphereCenterLS.x - mSceneBounds.Radius;
			const FLOAT b = sphereCenterLS.y - mSceneBounds.Radius;
			const FLOAT n = sphereCenterLS.z - mSceneBounds.Radius;
			const FLOAT r = sphereCenterLS.x + mSceneBounds.Radius;
			const FLOAT t = sphereCenterLS.y + mSceneBounds.Radius;
			const FLOAT f = sphereCenterLS.z + mSceneBounds.Radius;

			const XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

			const XMMATRIX viewProj = XMMatrixMultiply(lightView, lightProj);
			XMStoreFloat4x4(&light.Mat0, XMMatrixTranspose(viewProj));

			const XMMATRIX S = lightView * lightProj * T;
			XMStoreFloat4x4(&light.Mat1, XMMatrixTranspose(S));

			XMStoreFloat3(&light.Position, lightPos);
		}
		else if (light.Type == Common::Render::LightType::E_Point || light.Type == Common::Render::LightType::E_Tube) {
			const auto proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 1.f, 50.f);
			const auto pos = XMLoadFloat3(&light.Position);

			// Positive +X
			{
				const auto target = pos + XMVectorSet(1.f, 0.f, 0.f, 0.f);
				const auto view_px = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_px = view_px * proj;
				XMStoreFloat4x4(&light.Mat0, XMMatrixTranspose(vp_px));
			}
			// Positive -X
			{
				const auto target = pos + XMVectorSet(-1.f, 0.f, 0.f, 0.f);
				const auto view_nx = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nx = view_nx * proj;
				XMStoreFloat4x4(&light.Mat1, XMMatrixTranspose(vp_nx));
			}
			// Positive +Y
			{
				const auto target = pos + XMVectorSet(0.f, 1.f, 0.f, 0.f);
				const auto view_py = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, -1.f, 0.f));
				const auto vp_py = view_py * proj;
				XMStoreFloat4x4(&light.Mat2, XMMatrixTranspose(vp_py));
			}
			// Positive -Y
			{
				const auto target = pos + XMVectorSet(0.f, -1.f, 0.f, 0.f);
				const auto view_ny = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, 1.f, 0.f));
				const auto vp_ny = view_ny * proj;
				XMStoreFloat4x4(&light.Mat3, XMMatrixTranspose(vp_ny));
			}
			// Positive +Z
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, 1.f, 0.f);
				const auto view_pz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_pz = view_pz * proj;
				XMStoreFloat4x4(&light.Mat4, XMMatrixTranspose(vp_pz));
			}
			// Positive -Z
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, -1.f, 0.f);
				const auto view_nz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nz = view_nz * proj;
				XMStoreFloat4x4(&light.Mat5, XMMatrixTranspose(vp_nz));
			}
		}
		else if (light.Type == Common::Render::LightType::E_Rect) {
			const XMVECTOR lightDir = XMLoadFloat3(&light.Direction);
			const XMVECTOR lightUp = Common::Util::MathUtil::CalcUpVector(light.Direction);
			const XMVECTOR lightRight = XMVector3Cross(lightUp, lightDir);
			XMStoreFloat3(&light.Up, lightUp);
			XMStoreFloat3(&light.Right, lightRight);

			const XMVECTOR lightCenter = XMLoadFloat3(&light.Center);
			const FLOAT halfSizeX = light.Size.x * 0.5f;
			const FLOAT halfSizeY = light.Size.y * 0.5f;
			const XMVECTOR lightPos0 = lightCenter + lightUp * halfSizeY + lightRight * halfSizeX;
			const XMVECTOR lightPos1 = lightCenter + lightUp * halfSizeY - lightRight * halfSizeX;
			const XMVECTOR lightPos2 = lightCenter - lightUp * halfSizeY - lightRight * halfSizeX;
			const XMVECTOR lightPos3 = lightCenter - lightUp * halfSizeY + lightRight * halfSizeX;
			XMStoreFloat3(&light.Position, lightPos0);
			XMStoreFloat3(&light.Position1, lightPos1);
			XMStoreFloat3(&light.Position2, lightPos2);
			XMStoreFloat3(&light.Position3, lightPos3);

			const XMVECTOR targetPos = lightCenter + lightDir;
			const XMMATRIX lightView = XMMatrixLookAtLH(lightCenter, targetPos, lightUp);
		}

		mMainPassCB->Lights[i] = light;
	}

	return TRUE;
}

BOOL DxRenderer::UpdateObjectCB() {
	for (auto& ritem : mRenderItems) {
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (ritem->NumFramesDirty > 0) {
			const XMMATRIX world = XMLoadFloat4x4(&ritem->World);
			const XMMATRIX texTransform = XMLoadFloat4x4(&ritem->TexTransform);

			ConstantBuffers::ObjectCB objCB;

			objCB.PrevWorld = ritem->PrevWolrd;
			XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objCB.TexTransform, XMMatrixTranspose(texTransform));

			mCurrentFrameResource->CopyObjectCB(ritem->ObjectCBIndex, objCB);

			ritem->PrevWolrd = objCB.World;
			// Next FrameResource need to be updated too.
			--ritem->NumFramesDirty;
		}
	}

	return TRUE;
}

BOOL DxRenderer::UpdateMaterialCB() {
	for (auto& material : mMaterials) {
		if (material->NumFramesDirty > 0) {
			ConstantBuffers::MaterialCB matCB;

			matCB.Albedo = material->Albedo;
			matCB.Roughness = material->Roughness;
			matCB.Metalness = material->Metalness;
			matCB.Specular = material->Specular;
			matCB.MatTransform = material->MatTransform;

			matCB.AlbedoMapIndex = material->AlbedoMapIndex;
			matCB.NormalMapIndex = material->NormalMapIndex;
			matCB.AlphaMapIndex = material->AlphaMapIndex;
			matCB.RoughnessMapIndex = material->RoughnessMapIndex;
			matCB.MetalnessMapIndex = material->MetalnessMapIndex;
			matCB.SpecularMapIndex = material->SpecularMapIndex;

			mCurrentFrameResource->CopyMaterialCB(material->MaterialCBIndex, matCB);

			--material->NumFramesDirty;
		}
	}

	return TRUE;
}

BOOL DxRenderer::UpdateProjectToCubeCB() {
	XMStoreFloat4x4(&mProjectToCubeCB->Proj, XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, 10.f)));

	// Positive +X
	XMStoreFloat4x4(
		&mProjectToCubeCB->View[0],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(1.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive -X
	XMStoreFloat4x4(
		&mProjectToCubeCB->View[1],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(-1.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive +Y
	XMStoreFloat4x4(
		&mProjectToCubeCB->View[2],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, -1.f, 0.f)
		))
	);
	// Positive -Y
	XMStoreFloat4x4(
		&mProjectToCubeCB->View[3],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, -1.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, 1.f, 0.f)
		))
	);
	// Positive +Z
	XMStoreFloat4x4(
		&mProjectToCubeCB->View[4],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, 1.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive -Z
	XMStoreFloat4x4(
		&mProjectToCubeCB->View[5],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, -1.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);

	mCurrentFrameResource->CopyProjectToCubeCB(0, *mProjectToCubeCB.get());

	return TRUE;
}

BOOL DxRenderer::BuildMeshGeometry(
		ID3D12GraphicsCommandList6* const pCmdList,
		Foundation::Resource::SubmeshGeometry* const pSubmesh,
		const std::vector<Common::Foundation::Mesh::Vertex>& vertices,
		const std::vector<std::uint16_t>& indices,
		const std::string& name,
		Foundation::Resource::MeshGeometry*& pMeshGeo) {
	auto geo = std::make_unique<Foundation::Resource::MeshGeometry>();
	const auto Hash = Foundation::Resource::MeshGeometry::Hash(geo.get());

	const UINT VerticesByteSize = static_cast<UINT>(vertices.size() * sizeof(Common::Foundation::Mesh::Vertex));
	const UINT IndicesByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

	CheckHRESULT(mpLogFile, D3DCreateBlob(VerticesByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), VerticesByteSize);

	CheckHRESULT(mpLogFile, D3DCreateBlob(IndicesByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), IndicesByteSize);

	const auto device = mDevice.get();

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		device,
		pCmdList,
		vertices.data(),
		VerticesByteSize,
		geo->VertexBufferUploader,
		geo->VertexBufferGPU)
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		device,
		pCmdList,
		indices.data(),
		IndicesByteSize,
		geo->IndexBufferUploader,
		geo->IndexBufferGPU)
	);
		
	geo->VertexByteStride = static_cast<UINT>(sizeof(Common::Foundation::Mesh::Vertex));
	geo->VertexBufferByteSize = VerticesByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = IndicesByteSize;
	geo->Subsets[name] = *pSubmesh;
	
	pMeshGeo = geo.get();
	mMeshGeometries[Hash] = std::move(geo);

	return TRUE;
}

BOOL DxRenderer::BuildMeshGeometry(
		ID3D12GraphicsCommandList6* const pCmdList, 
		Common::Foundation::Mesh::Mesh* const pMesh,
		Foundation::Resource::MeshGeometry*& pMeshGeo) {
	auto geo = std::make_unique<Foundation::Resource::MeshGeometry>();
	const auto Hash = Foundation::Resource::MeshGeometry::Hash(geo.get());

	const UINT VerticesByteSize = pMesh->VerticesByteSize();
	const UINT IndicesByteSize = pMesh->IndicesByteSize();

	const auto Vertices = pMesh->Vertices();
	const auto Indices = pMesh->Indices();

	CheckHRESULT(mpLogFile, D3DCreateBlob(VerticesByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), Vertices, VerticesByteSize);

	CheckHRESULT(mpLogFile, D3DCreateBlob(IndicesByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), Indices, IndicesByteSize);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		mDevice.get(),
		pCmdList,
		Vertices,
		VerticesByteSize,
		geo->VertexBufferUploader,
		geo->VertexBufferGPU)
	);
	
	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		mDevice.get(),
		pCmdList,
		Indices,
		IndicesByteSize,
		geo->IndexBufferUploader,
		geo->IndexBufferGPU)
	);

	const auto Fence = mCommandObject->IncreaseFence();
	mCurrentFrameResource->mFence = Fence;

	geo->VertexByteStride = static_cast<UINT>(sizeof(Common::Foundation::Mesh::Vertex));
	geo->VertexBufferByteSize = VerticesByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = IndicesByteSize;
	geo->Fence = Fence;

	std::vector<Common::Foundation::Mesh::Mesh::SubsetPair> subsets;
	pMesh->Subsets(subsets);

	for (const auto& subset : subsets) {
		Foundation::Resource::SubmeshGeometry submesh;
		submesh.StartIndexLocation = subset.second.StartIndexLocation;
		submesh.BaseVertexLocation = 0;
		submesh.IndexCount = subset.second.Size;
		geo->Subsets[subset.first] = submesh;
	}

	pMeshGeo = geo.get();
	mMeshGeometries[Hash] = std::move(geo);

	return TRUE;
}

BOOL DxRenderer::BuildMeshMaterial(
		ID3D12GraphicsCommandList6* const pCmdList,
		Common::Foundation::Mesh::Material* const pMaterial,
		Foundation::Resource::MaterialData*& pMatData) {
	auto matData = std::make_unique<Foundation::Resource::MaterialData>(Foundation::Resource::FrameResource::Count);

	CheckReturn(mpLogFile, BuildMeshTextures(
		pCmdList,
		pMaterial,
		matData.get()));

	matData->MaterialCBIndex = static_cast<INT>(mMaterials.size());	
	matData->Albedo = pMaterial->Albedo;
	matData->Specular = pMaterial->Specular;
	matData->Roughness = pMaterial->Roughness;
	matData->Metalness = pMaterial->Metalness;

	pMatData = matData.get();

	mMaterials.push_back(std::move(matData));

	return TRUE;
}

BOOL DxRenderer::BuildMeshTextures(
		ID3D12GraphicsCommandList6* const pCmdList,
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

BOOL DxRenderer::InitShadingObjects() {
	CheckReturn(mpLogFile, mShadingObjectManager->Initialize(mpLogFile));
	CheckReturn(mpLogFile, mShaderManager->Initialize(mpLogFile, static_cast<UINT>(mProcessor->Logical)));

	// MipmapGenerator
	{
		auto initData = Shading::Util::MipmapGenerator::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mMipmapGenerator->Initialize(mpLogFile, initData.get()));
	}
	// EquirectangularConverter
	{
		auto initData = Shading::Util::EquirectangularConverter::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mEquirectangularConverter->Initialize(mpLogFile, initData.get()));
	}
	// EnvironmentMap
	{
		auto initData = Shading::EnvironmentMap::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mEnvironmentMap->Initialize(mpLogFile, initData.get()));
	}
	// GammaCorrection
	{
		auto initData = Shading::GammaCorrection::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mGammaCorrection->Initialize(mpLogFile, initData.get()));
	}
	// ToneMapping
	{
		auto initData = Shading::ToneMapping::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mToneMapping->Initialize(mpLogFile, initData.get()));
	}
	// GBuffer
	{
		auto initData = Shading::GBuffer::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mGBuffer->Initialize(mpLogFile, initData.get()));
	}
	// BRDF
	{
		auto initData = Shading::BRDF::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mBRDF->Initialize(mpLogFile, initData.get()));
	}
	// Shadow
	{
		auto initData = Shading::Shadow::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		initData->TexWidth = 2048;
		initData->TexHeight = 2048;
		CheckReturn(mpLogFile, mShadow->Initialize(mpLogFile, initData.get()));
	}

	return TRUE;
}

BOOL DxRenderer::BuildFrameResources() {
	for (UINT i = 0; i < Foundation::Resource::FrameResource::Count; i++) {
		mFrameResources.push_back(std::make_unique<Foundation::Resource::FrameResource>());

		CheckReturn(mpLogFile, mFrameResources.back()->Initialize(mpLogFile, mDevice.get(), static_cast<UINT>(mProcessor->Logical), 2, 32, 32));
	}

	mCurrentFrameResourceIndex = 0;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	return TRUE;
}

BOOL DxRenderer::BuildSkySphere() {
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.f, 32, 32);

	Foundation::Resource::SubmeshGeometry sphereSubmesh;
	sphereSubmesh.StartIndexLocation = 0;
	sphereSubmesh.BaseVertexLocation = 0;

	const auto indexCount = static_cast<UINT>(sphere.GetIndices16().size());
	const auto vertexCount = static_cast<UINT>(sphere.Vertices.size());

	sphereSubmesh.IndexCount = indexCount;

	std::vector<Common::Foundation::Mesh::Vertex> vertices(vertexCount);
	for (UINT i = 0, end = static_cast<UINT>(sphere.Vertices.size()); i < end; ++i) {
		const auto index = i + sphereSubmesh.BaseVertexLocation;
		vertices[index].Position = sphere.Vertices[i].Position;
		vertices[index].Normal = sphere.Vertices[i].Normal;
		vertices[index].TexCoord = sphere.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices(indexCount);
	for (UINT i = 0, end = static_cast<UINT>(sphere.GetIndices16().size()); i < end; ++i) {
		const auto index = i + sphereSubmesh.StartIndexLocation;
		indices[index] = sphere.GetIndices16()[i];
	}

	auto ritem = std::make_unique<Foundation::RenderItem>(Foundation::Resource::FrameResource::Count);

	CheckReturn(mpLogFile, mCommandObject->ResetDirectCommandList());

	const auto CmdList = mCommandObject->DirectCommandList();
	CheckReturn(mpLogFile, BuildMeshGeometry(CmdList, &sphereSubmesh, vertices, indices, "SkySphere", ritem->Geometry));

	CheckReturn(mpLogFile, mCommandObject->ExecuteDirectCommandList());

	ritem->ObjectCBIndex = static_cast<INT>(mRenderItems.size());
	ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ritem->IndexCount = ritem->Geometry->Subsets["SkySphere"].IndexCount;
	ritem->StartIndexLocation = ritem->Geometry->Subsets["SkySphere"].StartIndexLocation;
	ritem->BaseVertexLocation = ritem->Geometry->Subsets["SkySphere"].BaseVertexLocation;
	XMStoreFloat4x4(&ritem->World, XMMatrixScaling(1000.f, 1000.f, 1000.f));

	mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Sky].push_back(ritem.get());
	mRenderItems.push_back(std::move(ritem));

	return TRUE;
}

BOOL DxRenderer::BuildLights() {
	// Directional light 1
	{
		Foundation::Light light;
		light.Type = Common::Render::LightType::E_Directional;
		light.Direction = { 0.577f, -0.577f, 0.577f };
		light.Color = { 240.f / 255.f, 235.f / 255.f, 223.f / 255.f };
		light.Intensity = 1.802f;
		mShadow->AddLight(light);
	}
	// Directional light 2
	{
		Foundation::Light light;
		light.Type = Common::Render::LightType::E_Directional;
		light.Direction = { 0.067f, -0.701f, -0.836f };
		light.Color = { 149.f / 255.f, 142.f / 255.f, 100.f / 255.f };
		light.Intensity = 1.534f;
		mShadow->AddLight(light);
	}

	return TRUE;
}

BOOL DxRenderer::BuildScene() {
	mSceneBounds.Center = XMFLOAT3(0.f, 0.f, 0.f);
	const FLOAT WidthSquared = 32.f * 32.f;
	mSceneBounds.Radius = sqrtf(WidthSquared + WidthSquared);

	// Some of shading objects require particular constant buffers
	CheckReturn(mpLogFile, UpdateConstantBuffers());	

	CheckReturn(mpLogFile, BuildSkySphere());
	CheckReturn(mpLogFile, BuildLights());

	CheckReturn(mpLogFile, mEnvironmentMap->SetEnvironmentMap(
		mCurrentFrameResource,
		mMipmapGenerator.get(), 
		mEquirectangularConverter.get(), 
		L"forest_hdr", L"./../../../../assets/textures/"));

	return TRUE;
}

BOOL DxRenderer::PresentAndSignal() {
	CheckReturn(mpLogFile, mSwapChain->ReadyToPresent(mCurrentFrameResource));
	CheckReturn(mpLogFile, mSwapChain->Present(mFactory->AllowTearing()));
	mSwapChain->NextBackBuffer();
	
	mCurrentFrameResource->mFence = mCommandObject->IncreaseFence();

	CheckReturn(mpLogFile, mCommandObject->Signal());

	return TRUE;
}