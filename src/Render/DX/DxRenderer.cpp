#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
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
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Resource/MaterialData.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShadingObjectManager.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Render/DX/Shading/Util/EquirectangularConverter.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"
#include "Render/DX/Shading/Util/AccelerationStructure.hpp"
#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "Render/DX/Shading/GammaCorrection.hpp"
#include "Render/DX/Shading/ToneMapping.hpp"
#include "Render/DX/Shading/GBuffer.hpp"
#include "Render/DX/Shading/BRDF.hpp"
#include "Render/DX/Shading/Shadow.hpp"
#include "Render/DX/Shading/TAA.hpp"
#include "Render/DX/Shading/SSAO.hpp"
#include "Render/DX/Shading/RTAO.hpp"
#include "Render/DX/Shading/RayGen.hpp"
#include "Render/DX/Shading/RaySorting.hpp"
#include "Render/DX/Shading/SVGF.hpp"
#include "Render/DX/Shading/BlurFilter.hpp"
#include "ImGuiManager/DX/DxImGuiManager.hpp"
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
	mTAA = std::make_unique<Shading::TAA::TAAClass>();
	mSSAO= std::make_unique<Shading::SSAO::SSAOClass>();
	mRTAO = std::make_unique<Shading::RTAO::RTAOClass>();
	mRayGen = std::make_unique<Shading::RayGen::RayGenClass>();
	mRaySorting = std::make_unique<Shading::RaySorting::RaySortingClass>();
	mSVGF = std::make_unique<Shading::SVGF::SVGFClass>();
	mBlurFilter = std::make_unique<Shading::BlurFilter::BlurFilterClass>();

	mShadingObjectManager->AddShadingObject(mMipmapGenerator.get());
	mShadingObjectManager->AddShadingObject(mEquirectangularConverter.get());
	mShadingObjectManager->AddShadingObject(mEnvironmentMap.get());
	mShadingObjectManager->AddShadingObject(mGammaCorrection.get());
	mShadingObjectManager->AddShadingObject(mToneMapping.get());
	mShadingObjectManager->AddShadingObject(mGBuffer.get());
	mShadingObjectManager->AddShadingObject(mBRDF.get());
	mShadingObjectManager->AddShadingObject(mShadow.get());
	mShadingObjectManager->AddShadingObject(mTAA.get());
	mShadingObjectManager->AddShadingObject(mSSAO.get());
	mShadingObjectManager->AddShadingObject(mRTAO.get());
	mShadingObjectManager->AddShadingObject(mRayGen.get());
	mShadingObjectManager->AddShadingObject(mRaySorting.get());
	mShadingObjectManager->AddShadingObject(mSVGF.get());
	mShadingObjectManager->AddShadingObject(mBlurFilter.get());

	// Constant buffers
	mMainPassCB = std::make_unique<ConstantBuffers::PassCB>();
	mLightCB = std::make_unique<ConstantBuffers::LightCB>();
	mProjectToCubeCB = std::make_unique<ConstantBuffers::ProjectToCubeCB>();

	// Accleration structure manager
	mAccelerationStructureManager = std::make_unique<Shading::Util::AccelerationStructureManager>();
}

DxRenderer::~DxRenderer() {}

BOOL DxRenderer::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		Common::Foundation::Core::WindowsManager* const pWndManager, 
		Common::ImGuiManager::ImGuiManager* const pImGuiManager,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		UINT width, UINT height) {

	CheckReturn(mpLogFile, DxLowRenderer::Initialize(pLogFile, pWndManager, pImGuiManager, pArgSet, width, height));
	mbInitialized = TRUE;

	CheckReturn(mpLogFile, InitShadingObjects());
	CheckReturn(mpLogFile, BuildFrameResources());

	CheckReturn(mpLogFile, mpImGuiManager->InitializeD3D12(mDevice.get(), mDescriptorHeap.get()));
	mpImGuiManager->HookMsgCallback(mpWindowsManager);

	CheckReturn(mpLogFile, mAccelerationStructureManager->Initialize(mpLogFile, mDevice.get(), mCommandObject.get()));

	CheckReturn(mpLogFile, mShadingObjectManager->CompileShaders(mShaderManager.get(), L".\\..\\..\\..\\assets\\Shaders\\HLSL\\"));
	CheckReturn(mpLogFile, mShadingObjectManager->BuildRootSignatures());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineStates());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildDescriptors(mDescriptorHeap.get()));

	CheckReturn(mpLogFile, BuildScene());

	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	return TRUE;
}

void DxRenderer::CleanUp() {
	if (mbInitialized)
		mCommandObject->FlushCommandQueue();

	mpImGuiManager->CleanUpD3D12();

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
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
	CheckReturn(mpLogFile, mCommandObject->WaitCompletion(mpCurrentFrameResource->mFence));	
	CheckReturn(mpLogFile, mpCurrentFrameResource->ResetCommandListAllocators());

	CheckReturn(mpLogFile, UpdateConstantBuffers());
	CheckReturn(mpLogFile, ResolvePendingLights());

	CheckReturn(mpLogFile, PopulateRendableItems());

	if (mbRaytracingSupported) {
		const auto& rendableOpaques = mRendableItems[Common::Foundation::Mesh::RenderType::E_Opaque];

		const UINT NumRitems = static_cast<UINT>(rendableOpaques.size());
		if (NumRitems > 0) {
			CheckReturn(mpLogFile, mAccelerationStructureManager->Update(mpCurrentFrameResource, rendableOpaques.data(), NumRitems));

			if (mbMeshGeometryAdded) {
				CheckReturn(mpLogFile, mShadingObjectManager->BuildShaderTables(NumRitems));

				mbMeshGeometryAdded = FALSE;
			}
		}

		mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels = !mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;
	}

	CheckReturn(mpLogFile, mShadingObjectManager->Update());

	return TRUE;
}

BOOL DxRenderer::Draw() {
	const auto skySphere = mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Sky].front();
	const auto& opaques = mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Opaque];

	const auto& rendableOpaques = mRendableItems[Common::Foundation::Mesh::RenderType::E_Opaque];

	CheckReturn(mpLogFile, mGBuffer->DrawGBuffer(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(), 
		mSwapChain->ScissorRect(),
		mToneMapping->InterMediateMapResource(), 
		mToneMapping->InterMediateMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(), 
		mDepthStencilBuffer->DepthStencilBufferDsv(),
		rendableOpaques));

	CheckReturn(mpLogFile, mShadow->Run(
		mpCurrentFrameResource,
		mGBuffer->PositionMap(),
		mGBuffer->PositionMapSrv(),
		rendableOpaques));

	CheckReturn(mpLogFile, mSVGF->CalculateDepthParticalDerivative(
		mpCurrentFrameResource,
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->DepthStencilBufferSrv()));

	if (mpShadingArgumentSet->AOEnabled)
		CheckReturn(mpLogFile, DrawAO());

	CheckReturn(mpLogFile, mBRDF->ComputeBRDF(
		mpCurrentFrameResource,
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
		mShadow->ShadowMap(),
		mShadow->ShadowMapSrv(),
		mpShadingArgumentSet->ShadowEnabled));

	CheckReturn(mpLogFile, IntegrateIrradiance());

	CheckReturn(mpLogFile, mEnvironmentMap->DrawSkySphere(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mToneMapping->InterMediateMapResource(),
		mToneMapping->InterMediateMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(), 
		mDepthStencilBuffer->DepthStencilBufferDsv(),
		skySphere));

	if (mpShadingArgumentSet->TAA.Enabled) {
		CheckReturn(mpLogFile, mTAA->ApplyTAA(
			mpCurrentFrameResource,
			mSwapChain->ScreenViewport(),
			mSwapChain->ScissorRect(),
			mToneMapping->InterMediateMapResource(),
			mToneMapping->InterMediateMapRtv(),
			mToneMapping->InterMediateCopyMapResource(),
			mToneMapping->InterMediateCopyMapSrv(),
			mGBuffer->VelocityMap(),
			mGBuffer->VelocityMapSrv(),
			mpShadingArgumentSet->TAA.ModulationFactor));
	}

	CheckReturn(mpLogFile, mToneMapping->Resolve(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mSwapChain->BackBuffer(),
		mSwapChain->BackBufferRtv(),
		mpShadingArgumentSet->ToneMapping.Exposure));

	if (mpShadingArgumentSet->GammaCorrection.Enabled) {
		CheckReturn(mpLogFile, mGammaCorrection->ApplyCorrection(
			mpCurrentFrameResource,
			mSwapChain->ScreenViewport(),
			mSwapChain->ScissorRect(),
			mSwapChain->BackBuffer(),
			mSwapChain->BackBufferRtv(),
			mSwapChain->BackBufferCopy(),
			mSwapChain->BackBufferCopySrv(),
			mpShadingArgumentSet->GammaCorrection.Gamma));
	}

	CheckReturn(mpLogFile, DrawImGui());		
	CheckReturn(mpLogFile, PresentAndSignal());

	return TRUE;
}

BOOL DxRenderer::AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Mesh::Transform* const pTransform, Common::Foundation::Hash& hash) {
	CheckReturn(mpLogFile, mCommandObject->ResetCommandList(
		mpCurrentFrameResource->CommandAllocator(0),
		0));

	const auto CmdList = mCommandObject->CommandList(0);

	{
		Foundation::Resource::MeshGeometry* meshGeo;
		CheckReturn(mpLogFile, BuildMeshGeometry(CmdList, pMesh, meshGeo));

		if (mbRaytracingSupported)
			CheckReturn(mpLogFile, mAccelerationStructureManager->BuildBLAS(CmdList, meshGeo));

		mbMeshGeometryAdded = TRUE;

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
	ritem->NumFramesDirty = Foundation::Resource::FrameResource::Count;
	ritem->RebuildAccerationStructure = TRUE;

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
	if (mpCamera == nullptr) return TRUE;

	CheckReturn(mpLogFile, UpdateMainPassCB());
	CheckReturn(mpLogFile, UpdateLightCB());
	CheckReturn(mpLogFile, UpdateObjectCB());
	CheckReturn(mpLogFile, UpdateMaterialCB());
	CheckReturn(mpLogFile, UpdateProjectToCubeCB());
	CheckReturn(mpLogFile, UpdateAmbientOcclusionCB());
	CheckReturn(mpLogFile, UpdateRayGenCB());
	CheckReturn(mpLogFile, UpdateRaySortingCB());
	CheckReturn(mpLogFile, UpdateCalcLocalMeanVarianceCB());
	CheckReturn(mpLogFile, UpdateBlendWithCurrentFrameCB());
	CheckReturn(mpLogFile, UpdateCrossBilateralFilterCB());
	CheckReturn(mpLogFile, UpdateAtrousWaveletTransformFilterCB());

	return TRUE;
}

BOOL DxRenderer::UpdateMainPassCB() {
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

	if (mpShadingArgumentSet->TAA.Enabled) {
		const auto OffsetIndex = static_cast<UINT>(mCommandObject->CurrentFence() % mTAA->HaltonSequenceSize());
		mMainPassCB->JitteredOffset = mTAA->HaltonSequence(OffsetIndex);
	}
	else {
		mMainPassCB->JitteredOffset = { 0.f, 0.f };
	}
	
	mpCurrentFrameResource->CopyMainPassCB(0, *mMainPassCB.get());

	return TRUE;
}

BOOL DxRenderer::UpdateLightCB() {
	const auto LightCount = mShadow->LightCount();

	mLightCB->LightCount = LightCount;

	const XMMATRIX T(
		0.5f,  0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f,  0.f,  1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	for (UINT i = 0; i < LightCount; ++i) {
		const auto light = mShadow->Light(i);

		if (light->Type == Common::Render::LightType::E_Directional) {
			const XMVECTOR lightDir = XMLoadFloat3(&light->Direction);
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
			XMStoreFloat4x4(&light->Mat0, XMMatrixTranspose(viewProj));

			const XMMATRIX S = lightView * lightProj * T;
			XMStoreFloat4x4(&light->Mat1, XMMatrixTranspose(S));

			XMStoreFloat3(&light->Position, lightPos);
		}
		else if (light->Type == Common::Render::LightType::E_Point || light->Type == Common::Render::LightType::E_Tube) {
			const auto proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, 50.f);

			XMVECTOR pos;
			if (light->Type == Common::Render::LightType::E_Tube) {
				const auto Pos0 = XMLoadFloat3(&light->Position);
				const auto Pos1 = XMLoadFloat3(&light->Position1);

				pos = (Pos0 + Pos1) * 0.5f;
			}
			else {
				pos = XMLoadFloat3(&light->Position);
			}

			// Positive +X
			{
				const auto target = pos + XMVectorSet(1.f, 0.f, 0.f, 0.f);
				const auto view_px = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_px = view_px * proj;
				XMStoreFloat4x4(&light->Mat0, XMMatrixTranspose(vp_px));
			}
			// Positive -X
			{
				const auto target = pos + XMVectorSet(-1.f, 0.f, 0.f, 0.f);
				const auto view_nx = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nx = view_nx * proj;
				XMStoreFloat4x4(&light->Mat1, XMMatrixTranspose(vp_nx));
			}
			// Positive +Y
			{
				const auto target = pos + XMVectorSet(0.f, 1.f, 0.f, 0.f);
				const auto view_py = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, -1.f, 0.f));
				const auto vp_py = view_py * proj;
				XMStoreFloat4x4(&light->Mat2, XMMatrixTranspose(vp_py));
			}
			// Positive -Y
			{
				const auto target = pos + XMVectorSet(0.f, -1.f, 0.f, 0.f);
				const auto view_ny = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, 1.f, 0.f));
				const auto vp_ny = view_ny * proj;
				XMStoreFloat4x4(&light->Mat3, XMMatrixTranspose(vp_ny));
			}
			// Positive +Z
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, 1.f, 0.f);
				const auto view_pz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_pz = view_pz * proj;
				XMStoreFloat4x4(&light->Mat4, XMMatrixTranspose(vp_pz));
			}
			// Positive -Z
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, -1.f, 0.f);
				const auto view_nz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nz = view_nz * proj;
				XMStoreFloat4x4(&light->Mat5, XMMatrixTranspose(vp_nz));
			}
		}
		else if (light->Type == Common::Render::LightType::E_Spot) {
			const auto Proj = XMMatrixPerspectiveFovLH(Common::Util::MathUtil::DegreesToRadians(light->OuterConeAngle), 1.f, 0.1f, light->AttenuationRadius);
			const auto Pos = XMLoadFloat3(&light->Position);

			const XMVECTOR Direction = XMLoadFloat3(&light->Direction);

			const XMVECTOR UpVector = Common::Util::MathUtil::CalcUpVector(Direction);

			const auto Target = Pos + Direction;
			const auto View = XMMatrixLookAtLH(Pos, Target, UpVector);
			const auto ViewProj = View * Proj;
			XMStoreFloat4x4(&light->Mat0, XMMatrixTranspose(ViewProj));

			const XMMATRIX S = View * Proj * T;
			XMStoreFloat4x4(&light->Mat1, XMMatrixTranspose(S));
		}
		else if (light->Type == Common::Render::LightType::E_Rect) {
			const XMVECTOR lightDir = XMLoadFloat3(&light->Direction);
			const XMVECTOR lightUp = Common::Util::MathUtil::CalcUpVector(light->Direction);
			const XMVECTOR lightRight = XMVector3Cross(lightUp, lightDir);
			XMStoreFloat3(&light->Up, lightUp);
			XMStoreFloat3(&light->Right, lightRight);

			const XMVECTOR LightCenter = XMLoadFloat3(&light->Center);
			const FLOAT HalfSizeX = light->Size.x * 0.5f;
			const FLOAT HalfSizeY = light->Size.y * 0.5f;
			const XMVECTOR LightPos0 = LightCenter + lightUp * HalfSizeY + lightRight * HalfSizeX;
			const XMVECTOR LightPos1 = LightCenter + lightUp * HalfSizeY - lightRight * HalfSizeX;
			const XMVECTOR LightPos2 = LightCenter - lightUp * HalfSizeY - lightRight * HalfSizeX;
			const XMVECTOR LightPos3 = LightCenter - lightUp * HalfSizeY + lightRight * HalfSizeX;
			XMStoreFloat3(&light->Position, LightPos0);
			XMStoreFloat3(&light->Position1, LightPos1);
			XMStoreFloat3(&light->Position2, LightPos2);
			XMStoreFloat3(&light->Position3, LightPos3);
		}

		mLightCB->Lights[i] = *light;
		mpCurrentFrameResource->CopyLightCB(0, *mLightCB.get());
	}

	return TRUE;
}

BOOL DxRenderer::UpdateObjectCB() {
	for (auto& ritem : mRenderItems) {
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (ritem->NumFramesDirty > 0) {
			const XMMATRIX PrevWorld = XMLoadFloat4x4(&ritem->PrevWorld);
			const XMMATRIX World = XMLoadFloat4x4(&ritem->World);
			const XMMATRIX TexTransform = XMLoadFloat4x4(&ritem->TexTransform);

			ConstantBuffers::ObjectCB objCB;

			XMStoreFloat4x4(&objCB.PrevWorld, XMMatrixTranspose(PrevWorld));
			XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(World));
			XMStoreFloat4x4(&objCB.TexTransform, XMMatrixTranspose(TexTransform));

			mpCurrentFrameResource->CopyObjectCB(ritem->ObjectCBIndex, objCB);

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

			mpCurrentFrameResource->CopyMaterialCB(material->MaterialCBIndex, matCB);

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

	mpCurrentFrameResource->CopyProjectToCubeCB(*mProjectToCubeCB.get());

	return TRUE;
}

BOOL DxRenderer::UpdateAmbientOcclusionCB() {
	ConstantBuffers::AmbientOcclusionCB aoCB;
	aoCB.View = mMainPassCB->View;
	aoCB.Proj = mMainPassCB->Proj;
	aoCB.InvProj = mMainPassCB->InvProj;

	const XMMATRIX P = XMLoadFloat4x4(&mpCamera->Proj());
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	const XMMATRIX T(
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);
	XMStoreFloat4x4(&aoCB.ProjTex, XMMatrixTranspose(P * T));

	mSSAO->GetOffsetVectors(aoCB.OffsetVectors);

	if (mpShadingArgumentSet->RaytracingEnabled) {
		const BOOL CheckboardRayGeneration = mpShadingArgumentSet->RTAO.CheckboardRayGeneration;
		const UINT PixelStepX = CheckboardRayGeneration ? 2 : 1;

		aoCB.TextureDim = { Foundation::Util::D3D12Util::CeilDivide(mClientWidth, PixelStepX), mClientHeight };

		aoCB.OcclusionRadius = mpShadingArgumentSet->RTAO.OcclusionRadius;
		aoCB.OcclusionFadeStart = mpShadingArgumentSet->RTAO.OcclusionFadeStart;
		aoCB.OcclusionFadeEnd = mpShadingArgumentSet->RTAO.OcclusionFadeEnd;
		aoCB.SurfaceEpsilon = mpShadingArgumentSet->RTAO.SurfaceEpsilon;
		aoCB.SampleCount = mpShadingArgumentSet->RTAO.SampleCount;
		aoCB.CheckerboardRayGenEnabled = CheckboardRayGeneration;
		aoCB.EvenPixelsActivated = mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;
	}
	else {
		aoCB.OcclusionRadius = mpShadingArgumentSet->SSAO.OcclusionRadius;
		aoCB.OcclusionFadeStart = mpShadingArgumentSet->SSAO.OcclusionFadeStart;
		aoCB.OcclusionFadeEnd = mpShadingArgumentSet->SSAO.OcclusionFadeEnd;
		aoCB.OcclusionStrength = mpShadingArgumentSet->SSAO.OcclusionStrength;
		aoCB.SurfaceEpsilon = mpShadingArgumentSet->SSAO.SurfaceEpsilon;
		aoCB.SampleCount = mpShadingArgumentSet->SSAO.SampleCount;
	}

	aoCB.FrameCount = static_cast<UINT>(mpCurrentFrameResource->mFence);

	mpCurrentFrameResource->CopyAmbientOcclusionCB(aoCB);

	return TRUE;
}

BOOL DxRenderer::UpdateRayGenCB() {
	ConstantBuffers::RayGenCB rayGenCB;

	const BOOL CheckboardRayGeneration = mpShadingArgumentSet->RTAO.CheckboardRayGeneration;
	const UINT PixelStepX = CheckboardRayGeneration ? 2 : 1;

	rayGenCB.TextureDim = { Foundation::Util::D3D12Util::CeilDivide(mClientWidth, PixelStepX), mClientHeight };
	rayGenCB.NumSamplesPerSet = mRayGen->NumSamples();
	rayGenCB.NumSampleSets = mRayGen->NumSampleSets();
	rayGenCB.NumPixelsPerDimPerSet = mpShadingArgumentSet->RTAO.SampleSetSize;
	rayGenCB.CheckerboardRayGenEnabled = CheckboardRayGeneration;
	rayGenCB.CheckerboardGenerateRaysForEvenPixels = mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;
	rayGenCB.Seed = mpShadingArgumentSet->RTAO.RandomFrameSeed ? mRayGen->Seed() : 1879;

	mpCurrentFrameResource->CopyRayGenCB(rayGenCB);

	return TRUE;
}

BOOL DxRenderer::UpdateRaySortingCB() {
	ConstantBuffers::RaySortingCB raySortingCB;

	const BOOL CheckboardRayGeneration = mpShadingArgumentSet->RTAO.CheckboardRayGeneration;
	const UINT PixelStepX = CheckboardRayGeneration ? 2 : 1;

	raySortingCB.TextureDim = { Foundation::Util::D3D12Util::CeilDivide(mClientWidth, PixelStepX), mClientHeight };
	raySortingCB.BinDepthSize = mpShadingArgumentSet->RTAO.OcclusionRadius * mpShadingArgumentSet->RaySorting.DepthBinSizeMultiplier;
	raySortingCB.UseOctahedralRayDirectionQuantization = TRUE;
	raySortingCB.CheckerboardRayGenEnabled = CheckboardRayGeneration;
	raySortingCB.CheckerboardGenerateRaysForEvenPixels = mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;

	mpCurrentFrameResource->CopyRaySortingCB(raySortingCB);

	return TRUE;
}

BOOL DxRenderer::UpdateCalcLocalMeanVarianceCB() {
	ConstantBuffers::SVGF::CalcLocalMeanVarianceCB localMeanCB;

	const BOOL CheckboardRayGeneration = mpShadingArgumentSet->RTAO.CheckboardRayGeneration;
	const UINT PixelStepY = CheckboardRayGeneration ? 2 : 1;

	localMeanCB.TextureDim = { mClientWidth, mClientHeight };
	localMeanCB.KernelWidth = 9;
	localMeanCB.KernelRadius = 9 >> 1;
	localMeanCB.CheckerboardSamplingEnabled = CheckboardRayGeneration;
	localMeanCB.EvenPixelActivated = mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;
	localMeanCB.PixelStepY = PixelStepY;

	mpCurrentFrameResource->CopyCalcLocalMeanVarianceCB(localMeanCB);

	return TRUE;
}

BOOL DxRenderer::UpdateBlendWithCurrentFrameCB() {
	ConstantBuffers::SVGF::BlendWithCurrentFrameCB blendFrameCB;

	blendFrameCB.StdDevGamma = mpShadingArgumentSet->RTAO.BlendWithCurrentFrame.StdDevGamma;
	blendFrameCB.ClampCachedValues = mpShadingArgumentSet->RTAO.BlendWithCurrentFrame.UseClamping;
	blendFrameCB.ClampingMinStdDevTolerance = mpShadingArgumentSet->RTAO.BlendWithCurrentFrame.MinStdDevTolerance;

	blendFrameCB.ClampDifferenceToTsppScale = mpShadingArgumentSet->RTAO.BlendWithCurrentFrame.ClampDifferenceToTSPPScale;
	blendFrameCB.ForceUseMinSmoothingFactor = FALSE;
	blendFrameCB.MinSmoothingFactor = 1.f / mpShadingArgumentSet->RTAO.MaxTSPP;
	blendFrameCB.MinTsppToUseTemporalVariance = mpShadingArgumentSet->RTAO.BlendWithCurrentFrame.MinTSPPToUseTemporalVariance;

	blendFrameCB.BlurStrengthMaxTspp = mpShadingArgumentSet->RTAO.BlendWithCurrentFrame.LowTSPPMaxTSPP;
	blendFrameCB.BlurDecayStrength = mpShadingArgumentSet->RTAO.BlendWithCurrentFrame.LowTSPPDecayConstant;
	blendFrameCB.CheckerboardEnabled = mpShadingArgumentSet->RTAO.CheckboardRayGeneration;
	blendFrameCB.CheckerboardEvenPixelActivated = mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;

	mpCurrentFrameResource->CopyBlendWithCurrentFrameCB(blendFrameCB);

	return TRUE;
}

BOOL DxRenderer::UpdateCrossBilateralFilterCB() {
	ConstantBuffers::SVGF::CrossBilateralFilterCB filterCB;

	filterCB.DepthNumMantissaBits = Shading::SVGF::NumMantissaBitsInFloatFormat(16);
	filterCB.DepthSigma = 1.f;

	mpCurrentFrameResource->CopyCrossBilateralFilterCB(filterCB);

	return TRUE;
}

BOOL DxRenderer::UpdateAtrousWaveletTransformFilterCB() {
	ConstantBuffers::SVGF::AtrousWaveletTransformFilterCB filterCB;

	filterCB.TextureDim = { mClientWidth, mClientHeight };
	filterCB.DepthWeightCutoff = mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.DepthWeightCutoff;
	filterCB.UsingBilateralDownsamplingBuffers = FALSE;

	// Adaptive kernel radius rotation.
	FLOAT kernelRadiusLerfCoef = 0;
	if (mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.KernelRadiusRotateKernelEnabled) {
		static UINT frameID = 0;
		UINT i = frameID++ % mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.KernelRadiusRotateKernelNumCycles;
		kernelRadiusLerfCoef = i / static_cast<FLOAT>(mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.KernelRadiusRotateKernelNumCycles);
	}

	filterCB.UseAdaptiveKernelSize = mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.UseAdaptiveKernelSize;
	filterCB.KernelRadiusLerfCoef = kernelRadiusLerfCoef;
	filterCB.MinKernelWidth = mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.FilterMinKernelWidth;
	filterCB.MaxKernelWidth = static_cast<UINT>((mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.FilterMaxKernelWidthPercentage / 100.f) * mClientWidth);

	filterCB.PerspectiveCorrectDepthInterpolation = mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.PerspectiveCorrectDepthInterpolation;
	filterCB.MinVarianceToDenoise = mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.MinVarianceToDenoise;

	filterCB.ValueSigma = mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.ValueSigma;
	filterCB.DepthSigma = mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.DepthSigma;
	filterCB.NormalSigma = mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.NormalSigma;
	filterCB.FovY = mpCamera->FovY();

	filterCB.DepthNumMantissaBits = Shading::SVGF::NumMantissaBitsInFloatFormat(16);

	mpCurrentFrameResource->CopyAtrousWaveletTransformFilterCB(filterCB);

	return TRUE;
}

BOOL DxRenderer::ResolvePendingLights() {
	for (UINT i = 0, end = static_cast<UINT>(mPendingLights.size()); i < end; ++i) {
		const auto& light = mPendingLights.front();
		mShadow->AddLight(light);

		mPendingLights.pop();
	}

	return TRUE;
}

BOOL DxRenderer::PopulateRendableItems() {
	const auto& opaques = mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Opaque];

	auto& rendableOpaques = mRendableItems[Common::Foundation::Mesh::RenderType::E_Opaque];
	rendableOpaques.clear();

	for (const auto opaque : opaques) {
		if (mpCurrentFrameResource->mFence < opaque->Geometry->Fence) continue;

		rendableOpaques.push_back(opaque);
	}

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
	geo->IndexByteStride = sizeof(std::uint16_t);
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
	mpCurrentFrameResource->mFence = Fence;

	geo->VertexByteStride = static_cast<UINT>(sizeof(Common::Foundation::Mesh::Vertex));
	geo->VertexBufferByteSize = VerticesByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = IndicesByteSize;
	geo->IndexByteStride = sizeof(std::uint32_t);
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
	// TAA
	{
		auto initData = Shading::TAA::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mTAA->Initialize(mpLogFile, initData.get()));
	}
	// SSAO
	{
		auto initData = Shading::SSAO::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mSSAO->Initialize(mpLogFile, initData.get()));
	}
	// RTAO
	{
		auto initData = Shading::RTAO::MakeInitData();
		initData->ShadingArgumentSet = mpShadingArgumentSet;
		initData->RaytracingSupported = mbRaytracingSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mRTAO->Initialize(mpLogFile, initData.get()));
	}
	// RayGen
	{
		auto initData = Shading::RayGen::MakeInitData();
		initData->ShadingArgumentSet = mpShadingArgumentSet;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		initData->SamplesPerPixel = &mpShadingArgumentSet->RTAO.SampleCount;
		initData->MaxSamplesPerPixel = mpShadingArgumentSet->RTAO.MaxSampleCount;
		initData->SampleSetDistributedAcrossPixels= &mpShadingArgumentSet->RTAO.SampleSetSize;
		initData->MaxSampleSetDistributedAcrossPixels = mpShadingArgumentSet->RTAO.MaxSampleSetSize;
		initData->CurrentFrameIndex = &mCurrentFrameResourceIndex;
		CheckReturn(mpLogFile, mRayGen->Initialize(mpLogFile, initData.get()));
	}
	// RaySorting
	{
		auto initData = Shading::RaySorting::MakeInitData();
		initData->ShadingArgumentSet = mpShadingArgumentSet;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mRaySorting->Initialize(mpLogFile, initData.get()));
	}
	// SVGF
	{
		auto initData = Shading::SVGF::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mSVGF->Initialize(mpLogFile, initData.get()));
	}
	// BlurFilter
	{
		auto initData = Shading::BlurFilter::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mBlurFilter->Initialize(mpLogFile, initData.get()));
	}

	return TRUE;
}

BOOL DxRenderer::BuildFrameResources() {
	for (UINT i = 0; i < Foundation::Resource::FrameResource::Count; i++) {
		mFrameResources.push_back(std::make_unique<Foundation::Resource::FrameResource>());

		CheckReturn(mpLogFile, mFrameResources.back()->Initialize(mpLogFile, mDevice.get(), static_cast<UINT>(mProcessor->Logical), 2, 32, 32));
	}

	mCurrentFrameResourceIndex = 0;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

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
		std::shared_ptr<Foundation::Light> light = std::make_shared<Foundation::Light>();
		light->Type = Common::Render::LightType::E_Directional;
		light->Direction = { 0.577f, -0.577f, 0.577f };
		light->Color = { 240.f / 255.f, 235.f / 255.f, 223.f / 255.f };
		light->Intensity = 1.802f;
		mShadow->AddLight(light);
	}
	// Directional light 2
	{
		std::shared_ptr<Foundation::Light> light = std::make_shared<Foundation::Light>();
		light->Type = Common::Render::LightType::E_Directional;
		light->Direction = { 0.067f, -0.701f, -0.836f };
		light->Color = { 149.f / 255.f, 142.f / 255.f, 100.f / 255.f };
		light->Intensity = 1.534f;
		mShadow->AddLight(light);
	}

	return TRUE;
}

BOOL DxRenderer::BuildScene() {
	mSceneBounds.Center = XMFLOAT3(0.f, 0.f, 0.f);
	const FLOAT WidthSquared = 64.f * 64.f;
	mSceneBounds.Radius = sqrtf(WidthSquared + WidthSquared);

	// Some of shading objects require particular constant buffers
	CheckReturn(mpLogFile, UpdateConstantBuffers());	

	CheckReturn(mpLogFile, BuildSkySphere());
	CheckReturn(mpLogFile, BuildLights());

	CheckReturn(mpLogFile, mSSAO->BuildRandomVectorTexture());

	CheckReturn(mpLogFile, mEnvironmentMap->SetEnvironmentMap(
		mpCurrentFrameResource,
		mMipmapGenerator.get(), 
		mEquirectangularConverter.get(), 
		L"forest_hdr", L"./../../../assets/textures/"));

	return TRUE;
}

BOOL DxRenderer::DrawImGui() {
	CheckReturn(mpLogFile, mCommandObject->ResetCommandList(
		mpCurrentFrameResource->CommandAllocator(0),
		0,
		nullptr));

	const auto CmdList = mCommandObject->CommandList(0);
	mDescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->RSSetViewports(1, &mSwapChain->ScreenViewport());
	CmdList->RSSetScissorRects(1, &mSwapChain->ScissorRect());

	mSwapChain->BackBuffer()->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	CmdList->OMSetRenderTargets(1, &mSwapChain->BackBufferRtv(), TRUE, nullptr);

	CheckReturn(mpLogFile, mpImGuiManager->DrawImGui(
		CmdList, 
		mpShadingArgumentSet, 
		mShadow->Lights(),
		mShadow->LightCount(),
		mPendingLights,
		mClientWidth, 
		mClientHeight, 
		mbRaytracingSupported));

	CheckReturn(mpLogFile, mCommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL DxRenderer::DrawAO() {
	if (mpShadingArgumentSet->RaytracingEnabled) {		
		if (mpShadingArgumentSet->RTAO.RaySortingEnabled) {
			CheckReturn(mpLogFile, mRayGen->GenerateRays(
				mpCurrentFrameResource,
				mGBuffer->NormalDepthMap(),
				mGBuffer->NormalDepthMapSrv(),
				mGBuffer->PositionMap(),
				mGBuffer->PositionMapSrv()));

			CheckReturn(mpLogFile, mRaySorting->CalcRayIndexOffset(
				mpCurrentFrameResource,
				mGBuffer->NormalDepthMap(),
				mGBuffer->NormalDepthMapSrv()));
		}

		CheckReturn(mpLogFile, mRTAO->DrawAO(
			mpCurrentFrameResource,
			mAccelerationStructureManager->AccelerationStructure(),
			mGBuffer->PositionMap(),
			mGBuffer->PositionMapSrv(),
			mGBuffer->NormalDepthMap(),
			mGBuffer->NormalDepthMapSrv(),
			mRayGen->RayDirectionOriginDepthMap(),
			mRayGen->RayDirectionOriginDepthMapSrv(),
			mRaySorting->RayIndexOffsetMap(),
			mRaySorting->RayIndexOffsetMapSrv(),
			mpShadingArgumentSet->RTAO.RaySortingEnabled));

		// Denosing(Spatio - Temporal Variance Guided Filtering)
		{
			// Temporal supersampling 
			{
				// Stage 1: Reverse reprojection
				{
					const auto PrevTemporalCacheFrameIndex = mRTAO->CurrentTemporalCacheFrameIndex();
					const auto CurrTemporalCacheFrameIndex = mRTAO->MoveToNextTemporalCacheFrame();

					const auto PrevTemporalAOFrameIndex = mRTAO->CurrentTemporalAOFrameIndex();
					const auto CurrTemporalAOFrameIndex = mRTAO->MoveToNextTemporalAOFrame();

					// Retrieves values from previous frame via reverse reprojection.
					CheckReturn(mpLogFile, mSVGF->ReverseReprojectPreviousFrame(
						mpCurrentFrameResource,
						mGBuffer->NormalDepthMap(),
						mGBuffer->NormalDepthMapSrv(),
						mGBuffer->ReprojectedNormalDepthMap(),
						mGBuffer->ReprojectedNormalDepthMapSrv(),
						mGBuffer->CachedNormalDepthMap(),
						mGBuffer->CachedNormalDepthMapSrv(),
						mGBuffer->VelocityMap(),
						mGBuffer->VelocityMapSrv(),
						mRTAO->TemporalAOCoefficientResource(PrevTemporalAOFrameIndex),
						mRTAO->TemporalAOCoefficientSrv(PrevTemporalAOFrameIndex),
						mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_AOCoefficientSquaredMean, PrevTemporalCacheFrameIndex),
						mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_AOCoefficientSquaredMean, PrevTemporalCacheFrameIndex),
						mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_RayHitDistance, PrevTemporalCacheFrameIndex),
						mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_RayHitDistance, PrevTemporalCacheFrameIndex),
						mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_TSPP, PrevTemporalCacheFrameIndex),
						mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_TSPP, PrevTemporalCacheFrameIndex),
						mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
						mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::EU_TSPP, CurrTemporalCacheFrameIndex),
						Shading::SVGF::Value::E_Contrast));
				}
				// Stage 2: Blending current frame value with the reprojected cached value.
				{
					// Calculate local mean and variance for clamping during the blending operation.
					CheckReturn(mpLogFile, mSVGF->CalculateLocalMeanVariance(
						mpCurrentFrameResource,
						mRTAO->AOCoefficientResource(Shading::RTAO::Resource::AO::E_AOCoefficient),
						mRTAO->AOCoefficientDescriptor(Shading::RTAO::Descriptor::AO::ES_AOCoefficient),
						Shading::SVGF::Value::E_Contrast,
						mpShadingArgumentSet->RTAO.CheckboardRayGeneration));
					// Interpolate the variance for the inactive cells from the valid checkerboard cells.
					if (mpShadingArgumentSet->RTAO.CheckboardRayGeneration) {
						CheckReturn(mpLogFile, mSVGF->FillInCheckerboard(
							mpCurrentFrameResource,
							mpShadingArgumentSet->RTAO.CheckboardRayGeneration));
					}
					// Blends reprojected values with current frame values.
					// Inactive pixels are filtered from active neighbors on checkerboard sampling before the blending operation.
					{
						const auto CurrTemporalCacheFrameIndex = mRTAO->MoveToNextTemporalCacheFrame();
						const auto CurrAOResourceFrameIndex = mRTAO->MoveToNextTemporalAOFrame();

						CheckReturn(mpLogFile, mSVGF->BlendWithCurrentFrame(
							mpCurrentFrameResource,
							mRTAO->AOCoefficientResource(Shading::RTAO::Resource::AO::E_AOCoefficient),
							mRTAO->AOCoefficientDescriptor(Shading::RTAO::Descriptor::AO::ES_AOCoefficient),
							mRTAO->AOCoefficientResource(Shading::RTAO::Resource::AO::E_RayHitDistance),
							mRTAO->AOCoefficientDescriptor(Shading::RTAO::Descriptor::AO::ES_RayHitDistance),
							mRTAO->TemporalAOCoefficientResource(CurrAOResourceFrameIndex),
							mRTAO->TemporalAOCoefficientUav(CurrAOResourceFrameIndex),
							mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_AOCoefficientSquaredMean, CurrTemporalCacheFrameIndex),
							mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::EU_AOCoefficientSquaredMean, CurrTemporalCacheFrameIndex),
							mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_RayHitDistance, CurrTemporalCacheFrameIndex),
							mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::EU_RayHitDistance, CurrTemporalCacheFrameIndex),
							mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
							mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::EU_TSPP, CurrTemporalCacheFrameIndex),
							Shading::SVGF::Value::E_Contrast));
					}
				}
			}
			// Filtering
			{
				// Stage 1: Applies a single pass of a Atrous wavelet transform filter.
				if (mpShadingArgumentSet->RTAO.Denoiser.FullscreenBlurEnabaled) {
					const auto CurrTemporalCacheFrameIndex = mRTAO->CurrentTemporalCacheFrameIndex();
					const auto InputAOResourceFrameIndex = mRTAO->CurrentTemporalAOFrameIndex();
					const auto OutputAOResourceFrameIndex = mRTAO->MoveToNextTemporalAOFrame();
				
					const FLOAT RayHitDistToKernelWidthScale = 22 / mpShadingArgumentSet->RTAO.OcclusionRadius *
						mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.AdaptiveKernelSizeRayHitDistanceScaleFactor;
					const FLOAT RayHitDistToKernelSizeScaleExp = Foundation::Util::D3D12Util::Lerp(
						1,
						mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.AdaptiveKernelSizeRayHitDistanceScaleExponent,
						Foundation::Util::D3D12Util::RelativeCoef(mpShadingArgumentSet->RTAO.OcclusionRadius, 4, 22));

					CheckReturn(mpLogFile, mSVGF->ApplyAtrousWaveletTransformFilter(
						mpCurrentFrameResource,
						mGBuffer->NormalDepthMap(),
						mGBuffer->NormalDepthMapSrv(),
						mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_RayHitDistance, CurrTemporalCacheFrameIndex),
						mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_RayHitDistance, CurrTemporalCacheFrameIndex),
						mRTAO->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
						mRTAO->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_TSPP, CurrTemporalCacheFrameIndex),
						mRTAO->TemporalAOCoefficientResource(InputAOResourceFrameIndex),
						mRTAO->TemporalAOCoefficientSrv(InputAOResourceFrameIndex),
						mRTAO->TemporalAOCoefficientResource(OutputAOResourceFrameIndex),
						mRTAO->TemporalAOCoefficientSrv(OutputAOResourceFrameIndex),
						Shading::SVGF::Value::E_Contrast,
						RayHitDistToKernelWidthScale,
						RayHitDistToKernelSizeScaleExp));
				}
				// Stage 2: 3x3 multi-pass disocclusion blur (with more relaxed depth-aware constraints for such pixels).
				if (mpShadingArgumentSet->RTAO.Denoiser.DisocclusionBlurEnabled) {
					const auto CurrAOResourceFrameIndex = mRTAO->CurrentTemporalAOFrameIndex();

					CheckReturn(mpLogFile, mSVGF->BlurDisocclusion(
						mpCurrentFrameResource,
						mDepthStencilBuffer->GetDepthStencilBuffer(),
						mDepthStencilBuffer->DepthStencilBufferSrv(),
						mGBuffer->RoughnessMetalnessMap(),
						mGBuffer->RoughnessMetalnessMapSrv(),
						mRTAO->TemporalAOCoefficientResource(CurrAOResourceFrameIndex),
						mRTAO->TemporalAOCoefficientUav(CurrAOResourceFrameIndex),
						Shading::SVGF::Value::E_Contrast,
						mpShadingArgumentSet->RTAO.Denoiser.LowTsppBlurPassCount));
				}
			}
		}
	}
	else {
		CheckReturn(mpLogFile, mSSAO->DrawAO(
			mpCurrentFrameResource,
			mGBuffer->NormalMap(),
			mGBuffer->NormalMapSrv(),
			mGBuffer->PositionMap(),
			mGBuffer->PositionMapSrv()));

		for (UINT i = 0, end = mpShadingArgumentSet->SSAO.BlurCount; i < end; ++i) {
			CheckReturn(mpLogFile, mBlurFilter->GaussianBlur(
				mpCurrentFrameResource,
				Shading::BlurFilter::PipelineState::CP_GaussianBlurFilter3x3,
				mSSAO->AOMap(0),
				mSSAO->AOMapSrv(0),
				mSSAO->AOMap(1),
				mSSAO->AOMapUav(1),
				mSSAO->TexWidth(), mSSAO->TexHeight()));

			CheckReturn(mpLogFile, mBlurFilter->GaussianBlur(
				mpCurrentFrameResource,
				Shading::BlurFilter::PipelineState::CP_GaussianBlurFilter3x3,
				mSSAO->AOMap(1),
				mSSAO->AOMapSrv(1),
				mSSAO->AOMap(0),
				mSSAO->AOMapUav(0),
				mSSAO->TexWidth(), mSSAO->TexHeight()));
		}
	}

	return TRUE;
}

BOOL DxRenderer::IntegrateIrradiance() {
	const auto CurrTemporalAOFrameIndex = mRTAO->CurrentTemporalAOFrameIndex();
	const auto AOMap = mpShadingArgumentSet->RaytracingEnabled ? mRTAO->TemporalAOCoefficientResource(CurrTemporalAOFrameIndex) : mSSAO->AOMap(0);
	const auto AOSrv = mpShadingArgumentSet->RaytracingEnabled ? mRTAO->TemporalAOCoefficientSrv(CurrTemporalAOFrameIndex) : mSSAO->AOMapSrv(0);

	CheckReturn(mpLogFile, mBRDF->IntegrateIrradiance(
		mpCurrentFrameResource,
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
		AOMap,
		AOSrv,
		mEnvironmentMap->DiffuseIrradianceCubeMap(),
		mEnvironmentMap->DiffuseIrradianceCubeMapSrv(),
		mEnvironmentMap->BrdfLutMap(),
		mEnvironmentMap->BrdfLutMapSrv(),
		mEnvironmentMap->PrefilteredEnvironmentCubeMap(),
		mEnvironmentMap->PrefilteredEnvironmentCubeMapSrv(),
		mpShadingArgumentSet->AOEnabled));

	return TRUE;
}

BOOL DxRenderer::PresentAndSignal() {
	CheckReturn(mpLogFile, mSwapChain->ReadyToPresent(mpCurrentFrameResource));
	CheckReturn(mpLogFile, mSwapChain->Present(mFactory->AllowTearing()));
	mSwapChain->NextBackBuffer();
	
	mpCurrentFrameResource->mFence = mCommandObject->IncreaseFence();

	CheckReturn(mpLogFile, mCommandObject->Signal());

	return TRUE;
}