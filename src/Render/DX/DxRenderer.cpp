#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Foundation/Mesh/Transform.hpp"
#include "Common/Foundation/Light.h"
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
#include "Render/DX/Shading/Util/TextureScaler.hpp"
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
#include "Render/DX/Shading/VolumetricLight.hpp"
#include "Render/DX/Shading/SSCS.hpp"
#include "Render/DX/Shading/MotionBlur.hpp"
#include "Render/DX/Shading/Bloom.hpp"
#include "Render/DX/Shading/DOF.hpp"
#include "Render/DX/Shading/EyeAdaption.hpp"
#include "Render/DX/Shading/RaytracedShadow.hpp"
#include "Render/DX/Shading/ChromaticAberration.hpp"
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
	
	mShadingObjectManager->Add<Shading::Util::MipmapGenerator::MipmapGeneratorClass>();
	mShadingObjectManager->Add<Shading::Util::EquirectangularConverter::EquirectangularConverterClass>();
	mShadingObjectManager->Add<Shading::Util::TextureScaler::TextureScalerClass>();
	mShadingObjectManager->Add<Shading::EnvironmentMap::EnvironmentMapClass>();
	mShadingObjectManager->Add<Shading::GammaCorrection::GammaCorrectionClass>();
	mShadingObjectManager->Add<Shading::ToneMapping::ToneMappingClass>();
	mShadingObjectManager->Add<Shading::GBuffer::GBufferClass>();
	mShadingObjectManager->Add<Shading::BRDF::BRDFClass>();
	mShadingObjectManager->Add<Shading::Shadow::ShadowClass>();
	mShadingObjectManager->Add<Shading::TAA::TAAClass>();
	mShadingObjectManager->Add<Shading::SSAO::SSAOClass>();
	mShadingObjectManager->Add<Shading::RTAO::RTAOClass>();
	mShadingObjectManager->Add<Shading::RayGen::RayGenClass>();
	mShadingObjectManager->Add<Shading::RaySorting::RaySortingClass>();
	mShadingObjectManager->Add<Shading::SVGF::SVGFClass>();
	mShadingObjectManager->Add<Shading::BlurFilter::BlurFilterClass>();
	mShadingObjectManager->Add<Shading::VolumetricLight::VolumetricLightClass>();
	mShadingObjectManager->Add<Shading::SSCS::SSCSClass>();
	mShadingObjectManager->Add<Shading::MotionBlur::MotionBlurClass>();
	mShadingObjectManager->Add<Shading::Bloom::BloomClass>();
	mShadingObjectManager->Add<Shading::DOF::DOFClass>();
	mShadingObjectManager->Add<Shading::EyeAdaption::EyeAdaptionClass>();
	mShadingObjectManager->Add<Shading::RaytracedShadow::RaytracedShadowClass>();
	mShadingObjectManager->Add<Shading::ChromaticAberration::ChromaticAberrationClass>();

	// Accleration structure manager
	mAccelerationStructureManager = std::make_unique<Shading::Util::AccelerationStructureManager>();
}

DxRenderer::~DxRenderer() { CleanUp(); }

BOOL DxRenderer::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		Common::Foundation::Core::WindowsManager* const pWndManager, 
		Common::ImGuiManager::ImGuiManager* const pImGuiManager,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::Initialize(pLogFile, pWndManager, pImGuiManager, pArgSet, width, height));

	CheckReturn(mpLogFile, InitShadingObjects());
	CheckReturn(mpLogFile, BuildFrameResources());

	CheckReturn(mpLogFile, mpImGuiManager->InitializeD3D12(mDevice.get(), mDescriptorHeap.get()));
	mpImGuiManager->HookMsgCallback(mpWindowsManager);

	CheckReturn(mpLogFile, mAccelerationStructureManager->Initialize(mpLogFile, mDevice.get(), mCommandObject.get()));

	mSceneBounds.Center = XMFLOAT3(0.f, 0.f, 0.f);
	const FLOAT WidthSquared = 128.f * 128.f;
	mSceneBounds.Radius = sqrtf(WidthSquared + WidthSquared);

	CheckReturn(mpLogFile, BuildSkySphere());
	CheckReturn(mpLogFile, BuildLights());

	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	return TRUE;
}

void DxRenderer::CleanUp() {
	if (mbCleanedUp) return;

	mCommandObject->FlushCommandQueue();

	if (mAccelerationStructureManager) {
		mAccelerationStructureManager->CleanUp();
		mAccelerationStructureManager.reset();
	}

	if (mSkySphere) mSkySphere.reset();
	mRenderItemRefs.clear();
	mRenderItems.clear();

	mMeshGeometries.clear();
	mMaterials.clear();

	for (UINT i = 0; i < Foundation::Resource::FrameResource::Count; ++i) {
		auto& resource = mFrameResources[i];
		if (resource) {
			resource->CleanUp();
			resource.reset();
		}
	}

	if (mShaderManager) {
		mShaderManager->CleanUp();
		mShaderManager.reset();
	}
	if (mShadingObjectManager) {
		mShadingObjectManager->CleanUp();
		mShadingObjectManager.reset();
	}

	DxLowRenderer::CleanUp();
}

BOOL DxRenderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::OnResize(width, height));

	CheckReturn(mpLogFile, mShadingObjectManager->OnResize(width, height));

#ifdef _DEBUG
	std::cout << std::format("DxRenderer resized (Width: {}, Height: {}", width, height) << std::endl;
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

	mDeltaTime = deltaTime;

	return TRUE;
}

BOOL DxRenderer::Draw() {
	static bool OnlyOnce{};
	if (!OnlyOnce) {
		OnlyOnce = true;
		CheckReturn(mpLogFile, BuildScene());
	}

	const auto& opaques = mRenderItemGroups[Common::Foundation::Mesh::RenderType::E_Opaque];

	const auto gbuffer = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();
	const auto tone = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();
	CheckReturn(mpLogFile, gbuffer->DrawGBuffer(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(), 
		mSwapChain->ScissorRect(),
		tone->InterMediateMapResource(), 
		tone->InterMediateMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(), 
		mDepthStencilBuffer->DepthStencilBufferDsv(),
		mRendableItems[Common::Foundation::Mesh::RenderType::E_Opaque],
		0.4f, 0.1f));

	CheckReturn(mpLogFile, DrawShadow());

	if (mpShadingArgumentSet->SSCS.Enabled) 
		CheckReturn(mpLogFile, ApplyContactShadow());

	const auto svgf = mShadingObjectManager->Get<Shading::SVGF::SVGFClass>();
	CheckReturn(mpLogFile, svgf->CalculateDepthParticalDerivative(
		mpCurrentFrameResource,
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->DepthStencilBufferSrv()));

	if (mpShadingArgumentSet->AOEnabled)
		CheckReturn(mpLogFile, DrawAO());

	const auto brdf = mShadingObjectManager->Get<Shading::BRDF::BRDFClass>();
	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	const auto rayShadow = mShadingObjectManager->Get<Shading::RaytracedShadow::RaytracedShadowClass>();
	CheckReturn(mpLogFile, brdf->ComputeBRDF(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		tone->InterMediateMapResource(),
		tone->InterMediateMapRtv(),
		gbuffer->AlbedoMap(),
		gbuffer->AlbedoMapSrv(),
		gbuffer->NormalMap(),
		gbuffer->NormalMapSrv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->DepthStencilBufferSrv(),
		gbuffer->SpecularMap(),
		gbuffer->SpecularMapSrv(),
		gbuffer->RoughnessMetalnessMap(),
		gbuffer->RoughnessMetalnessMapSrv(),
		gbuffer->PositionMap(),
		gbuffer->PositionMapSrv(),
		mpShadingArgumentSet->RaytracingEnabled ?
			rayShadow->ShadowMap() : shadow->ShadowMap(),
		mpShadingArgumentSet->RaytracingEnabled ?
			rayShadow->ShadowMapSrv() : shadow->ShadowMapSrv(),
		mpShadingArgumentSet->ShadowEnabled));

	CheckReturn(mpLogFile, IntegrateIrradiance());

	const auto env = mShadingObjectManager->Get<Shading::EnvironmentMap::EnvironmentMapClass>();
	CheckReturn(mpLogFile, env->DrawSkySphere(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		tone->InterMediateMapResource(),
		tone->InterMediateMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(), 
		mDepthStencilBuffer->DepthStencilBufferDsv(),
		mSkySphere.get()));

	CheckReturn(mpLogFile, ApplyVolumetricLight());

	if (mpShadingArgumentSet->Bloom.Enabled)
		CheckReturn(mpLogFile, ApplyBloom());

	CheckReturn(mpLogFile, ApplyEyeAdaption());

	if (mpShadingArgumentSet->ChromaticAberration.Enabled) {
		const auto chromatic = mShadingObjectManager->Get<Shading::ChromaticAberration::ChromaticAberrationClass>();
		CheckReturn(mpLogFile, chromatic->ApplyChromaticAberration(
			mpCurrentFrameResource,
			mSwapChain->ScreenViewport(),
			mSwapChain->ScissorRect(),
			tone->InterMediateMapResource(),
			tone->InterMediateMapRtv(),
			tone->InterMediateCopyMapResource(),
			tone->InterMediateCopyMapSrv(),
			mpShadingArgumentSet->ChromaticAberration.Strength,
			mpShadingArgumentSet->ChromaticAberration.Threshold,
			mpShadingArgumentSet->ChromaticAberration.Feather,
			mpShadingArgumentSet->ChromaticAberration.ShiftPx,
			mpShadingArgumentSet->ChromaticAberration.Exponent));
	}

	if (mpShadingArgumentSet->DOF.Enabled)
		CheckReturn(mpLogFile, ApplyDOF());

	if (mpShadingArgumentSet->TAA.Enabled) {
		const auto taa = mShadingObjectManager->Get<Shading::TAA::TAAClass>();
		CheckReturn(mpLogFile, taa->ApplyTAA(
			mpCurrentFrameResource,
			mSwapChain->ScreenViewport(),
			mSwapChain->ScissorRect(),
			tone->InterMediateMapResource(),
			tone->InterMediateMapRtv(),
			tone->InterMediateCopyMapResource(),
			tone->InterMediateCopyMapSrv(),
			gbuffer->VelocityMap(),
			gbuffer->VelocityMapSrv(),
			mpShadingArgumentSet->TAA.ModulationFactor));
	}

	const auto eye = mShadingObjectManager->Get<Shading::EyeAdaption::EyeAdaptionClass>();
	CheckReturn(mpLogFile, tone->Resolve(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mSwapChain->BackBuffer(),
		mSwapChain->BackBufferRtv(),
		eye->Luminance(),
		mpShadingArgumentSet->ToneMapping.Exposure,
		mpShadingArgumentSet->ToneMapping.MiddleGrayKey,
		mpShadingArgumentSet->ToneMapping.TonemapperType));
	
	if (mpShadingArgumentSet->GammaCorrection.Enabled) {
		const auto gamma = mShadingObjectManager->Get<Shading::GammaCorrection::GammaCorrectionClass>();
		CheckReturn(mpLogFile, gamma->ApplyCorrection(
			mpCurrentFrameResource,
			mSwapChain->ScreenViewport(),
			mSwapChain->ScissorRect(),
			mSwapChain->BackBuffer(),
			mSwapChain->BackBufferRtv(),
			mSwapChain->BackBufferCopy(),
			mSwapChain->BackBufferCopySrv(),
			mpShadingArgumentSet->GammaCorrection.Gamma));
	}

	if (mpShadingArgumentSet->MotionBlur.Enabled) {
		const auto motion = mShadingObjectManager->Get<Shading::MotionBlur::MotionBlurClass>();
		motion->ApplyMotionBlur(
			mpCurrentFrameResource,
			mSwapChain->ScreenViewport(),
			mSwapChain->ScissorRect(),
			mSwapChain->BackBuffer(),
			mSwapChain->BackBufferRtv(),
			mSwapChain->BackBufferCopy(),
			mSwapChain->BackBufferCopySrv(),
			mDepthStencilBuffer->GetDepthStencilBuffer(),
			mDepthStencilBuffer->DepthStencilBufferSrv(),
			gbuffer->VelocityMap(),
			gbuffer->VelocityMapSrv(),
			mpShadingArgumentSet->MotionBlur.Intensity,
			mpShadingArgumentSet->MotionBlur.Limit,
			mpShadingArgumentSet->MotionBlur.DepthBias,
			mpShadingArgumentSet->MotionBlur.SampleCount);
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
	CheckReturn(mpLogFile, UpdateContactShadowCB());

	return TRUE;
}

BOOL DxRenderer::UpdateMainPassCB() {
	static ConstantBuffers::PassCB passCB{
		.ViewProj = Common::Util::MathUtil::Identity4x4()
	};

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
	
	passCB.PrevViewProj = passCB.ViewProj;
	XMStoreFloat4x4(&passCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&passCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&passCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&passCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&passCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&passCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&passCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
	XMStoreFloat3(&passCB.EyePosW, mpCamera->Position());

	const auto taa = mShadingObjectManager->Get<Shading::TAA::TAAClass>();

	if (mpShadingArgumentSet->TAA.Enabled) {
		const auto OffsetIndex = static_cast<UINT>(
			mCommandObject->CurrentFence() % taa->HaltonSequenceSize());
		passCB.JitteredOffset = taa->HaltonSequence(OffsetIndex);
	}
	else {
		passCB.JitteredOffset = { 0.f, 0.f };
	}
	
	mpCurrentFrameResource->MainPassCB.CopyCB(passCB);

	return TRUE;
}

BOOL DxRenderer::UpdateLightCB() {
	static ConstantBuffers::LightCB ligthCB{};

	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	const auto LightCount = shadow->LightCount();

	ligthCB.LightCount = LightCount;

	const XMMATRIX T(
		0.5f,  0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f,  0.f,  1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	for (UINT i = 0; i < LightCount; ++i) {
		const auto light = shadow->Light(i);

		if (light->Type == Common::Foundation::LightType::E_Directional) {
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
		else if (light->Type == Common::Foundation::LightType::E_Point || light->Type == Common::Foundation::LightType::E_Tube) {
			const auto proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, 50.f);

			XMVECTOR pos;
			if (light->Type == Common::Foundation::LightType::E_Tube) {
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
		else if (light->Type == Common::Foundation::LightType::E_Spot) {
			const auto Proj = XMMatrixPerspectiveFovLH(Common::Util::MathUtil::DegreesToRadians(light->OuterConeAngle) * 2.f, 1.f, 0.1f, light->AttenuationRadius);
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
		else if (light->Type == Common::Foundation::LightType::E_Rect) {
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

		ligthCB.Lights[i] = *light;
		mpCurrentFrameResource->LightCB.CopyCB(ligthCB);
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

			mpCurrentFrameResource->ObjectCB.CopyCB(objCB, ritem->ObjectCBIndex);

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

			mpCurrentFrameResource->MaterialCB.CopyCB(matCB, material->MaterialCBIndex);

			--material->NumFramesDirty;
		}
	}

	return TRUE;
}

BOOL DxRenderer::UpdateProjectToCubeCB() {
	ConstantBuffers::ProjectToCubeCB  projToCubeCB{};

	XMStoreFloat4x4(
		&projToCubeCB.Proj, 
		XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, 10.f)));

	// Positive +X
	XMStoreFloat4x4(
		&projToCubeCB.Views[0],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(1.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive -X
	XMStoreFloat4x4(
		&projToCubeCB.Views[1],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(-1.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive +Y
	XMStoreFloat4x4(
		&projToCubeCB.Views[2],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, -1.f, 0.f)
		))
	);
	// Positive -Y
	XMStoreFloat4x4(
		&projToCubeCB.Views[3],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, -1.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, 1.f, 0.f)
		))
	);
	// Positive +Z
	XMStoreFloat4x4(
		&projToCubeCB.Views[4],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, 1.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive -Z
	XMStoreFloat4x4(
		&projToCubeCB.Views[5],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, -1.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);

	mpCurrentFrameResource->ProjectToCubeCB.CopyCB(projToCubeCB);
	mCommandObject->FlushCommandQueue();

	return TRUE;
}

BOOL DxRenderer::UpdateAmbientOcclusionCB() {
	const auto ssao = mShadingObjectManager->Get<Shading::SSAO::SSAOClass>();

	const XMMATRIX view = XMLoadFloat4x4(&mpCamera->View());
	const XMMATRIX proj = XMLoadFloat4x4(&mpCamera->Proj());

	const XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);

	ConstantBuffers::AmbientOcclusionCB aoCB{};
	XMStoreFloat4x4(&aoCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&aoCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&aoCB.InvProj, XMMatrixTranspose(invProj));

	const XMMATRIX P = XMLoadFloat4x4(&mpCamera->Proj());
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	const XMMATRIX T(
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);
	XMStoreFloat4x4(&aoCB.ProjTex, XMMatrixTranspose(P * T));

	ssao->GetOffsetVectors(aoCB.OffsetVectors);

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
		aoCB.TextureDim = { mClientWidth, mClientHeight };

		aoCB.OcclusionRadius = mpShadingArgumentSet->SSAO.OcclusionRadius;
		aoCB.OcclusionFadeStart = mpShadingArgumentSet->SSAO.OcclusionFadeStart;
		aoCB.OcclusionFadeEnd = mpShadingArgumentSet->SSAO.OcclusionFadeEnd;
		aoCB.OcclusionStrength = mpShadingArgumentSet->SSAO.OcclusionStrength;
		aoCB.SurfaceEpsilon = mpShadingArgumentSet->SSAO.SurfaceEpsilon;
		aoCB.SampleCount = mpShadingArgumentSet->SSAO.SampleCount;
		aoCB.CheckerboardRayGenEnabled = FALSE;
		aoCB.EvenPixelsActivated = FALSE;
	}

	aoCB.FrameCount = static_cast<UINT>(mpCurrentFrameResource->mFence);

	mpCurrentFrameResource->AmbientOcclusionCB.CopyCB(aoCB);

	return TRUE;
}

BOOL DxRenderer::UpdateRayGenCB() {
	const auto raygen = mShadingObjectManager->Get<Shading::RayGen::RayGenClass>();

	ConstantBuffers::RayGenCB rayGenCB;

	const BOOL CheckboardRayGeneration = mpShadingArgumentSet->RTAO.CheckboardRayGeneration;
	const UINT PixelStepX = CheckboardRayGeneration ? 2 : 1;

	rayGenCB.TextureDim = { Foundation::Util::D3D12Util::CeilDivide(mClientWidth, PixelStepX), mClientHeight };
	rayGenCB.NumSamplesPerSet = raygen->NumSamples();
	rayGenCB.NumSampleSets = raygen->NumSampleSets();
	rayGenCB.NumPixelsPerDimPerSet = mpShadingArgumentSet->RTAO.SampleSetSize;
	rayGenCB.CheckerboardRayGenEnabled = CheckboardRayGeneration;
	rayGenCB.CheckerboardGenerateRaysForEvenPixels = mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;
	rayGenCB.Seed = mpShadingArgumentSet->RTAO.RandomFrameSeed ? raygen->Seed() : 1879;

	mpCurrentFrameResource->RayGenCB.CopyCB(rayGenCB);

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

	mpCurrentFrameResource->RaySortingCB.CopyCB(raySortingCB);

	return TRUE;
}

BOOL DxRenderer::UpdateCalcLocalMeanVarianceCB() {
	ConstantBuffers::SVGF::CalcLocalMeanVarianceCB localMeanCB;

	const BOOL CheckboardRayGeneration = mpShadingArgumentSet->RaytracingEnabled ?
		mpShadingArgumentSet->RTAO.CheckboardRayGeneration : FALSE;
	const UINT PixelStepY = CheckboardRayGeneration ? 2 : 1;

	localMeanCB.TextureDim = { mClientWidth, mClientHeight };
	localMeanCB.KernelWidth = 9;
	localMeanCB.KernelRadius = 9 >> 1;
	localMeanCB.CheckerboardSamplingEnabled = CheckboardRayGeneration;
	localMeanCB.EvenPixelActivated = mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;
	localMeanCB.PixelStepY = PixelStepY;

	mpCurrentFrameResource->CalcLocalMeanVarianceCB.CopyCB(localMeanCB);

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
	blendFrameCB.CheckerboardEnabled = mpShadingArgumentSet->RaytracingEnabled ? 
		mpShadingArgumentSet->RTAO.CheckboardRayGeneration : FALSE;
	blendFrameCB.CheckerboardEvenPixelActivated = mpShadingArgumentSet->RaytracingEnabled ?
		mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels : FALSE;

	mpCurrentFrameResource->BlendWithCurrentFrameCB.CopyCB(blendFrameCB);

	return TRUE;
}

BOOL DxRenderer::UpdateCrossBilateralFilterCB() {
	ConstantBuffers::SVGF::CrossBilateralFilterCB filterCB;

	filterCB.DepthNumMantissaBits = Shading::SVGF::NumMantissaBitsInFloatFormat(16);
	filterCB.DepthSigma = 1.f;

	mpCurrentFrameResource->CrossBilateralFilterCB.CopyCB(filterCB);

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

	mpCurrentFrameResource->AtrousWaveletTransformFilterCB.CopyCB(filterCB);

	return TRUE;
}

BOOL DxRenderer::UpdateContactShadowCB() {
	static UINT frame = 0;

	ConstantBuffers::ContactShadowCB csCB{};

	csCB.BiasBase = mpShadingArgumentSet->SSCS.BiasBase;
	csCB.BiasSlope = mpShadingArgumentSet->SSCS.BiasSlope;
	csCB.DepthEpsilonBase = mpShadingArgumentSet->SSCS.DepthEpsilonBase;
	csCB.DepthEpsilonScale = mpShadingArgumentSet->SSCS.DepthEpsilonScale;

	csCB.StepScaleFar = mpShadingArgumentSet->SSCS.StepScaleFar;
	csCB.StepScaleFarDist = mpShadingArgumentSet->SSCS.StepScaleFarDist;
	csCB.RayMaxDistance = mpShadingArgumentSet->SSCS.RayMaxDistance;

	csCB.Thickness = mpShadingArgumentSet->SSCS.Thcikness;
	csCB.ThicknessFarScale = mpShadingArgumentSet->SSCS.ThicknessFarScale;
	csCB.StepScaleFarDist = mpShadingArgumentSet->SSCS.ThicknessFarDist;

	csCB.TextureDimX = mClientWidth;
	csCB.MaxSteps = mpShadingArgumentSet->SSCS.Steps;
	csCB.FrameCount = frame++;

	mpCurrentFrameResource->ContactShadowCB.CopyCB(csCB);

	return TRUE;
}

BOOL DxRenderer::ResolvePendingLights() {
	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();

	for (UINT i = 0, end = static_cast<UINT>(mPendingLights.size()); i < end; ++i) {
		const auto& light = mPendingLights.front();
		shadow->AddLight(light);

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
		const auto obj = mShadingObjectManager->Get<Shading::Util::MipmapGenerator::MipmapGeneratorClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// EquirectangularConverter
	{
		auto initData = Shading::Util::EquirectangularConverter::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::Util::EquirectangularConverter::EquirectangularConverterClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// TextureScaler
	{
		auto initData = Shading::Util::TextureScaler::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::Util::TextureScaler::TextureScalerClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// EnvironmentMap
	{
		auto initData = Shading::EnvironmentMap::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::EnvironmentMap::EnvironmentMapClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::GammaCorrection::GammaCorrectionClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::BRDF::BRDFClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::TAA::TAAClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::SSAO::SSAOClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// RTAO
	{
		auto initData = Shading::RTAO::MakeInitData();
		initData->RaytracingSupported = mbRaytracingSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		const auto obj = mShadingObjectManager->Get<Shading::RTAO::RTAOClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		initData->SampleSetDistributedAcrossPixels = &mpShadingArgumentSet->RTAO.SampleSetSize;
		initData->MaxSampleSetDistributedAcrossPixels = mpShadingArgumentSet->RTAO.MaxSampleSetSize;
		initData->CurrentFrameIndex = &mCurrentFrameResourceIndex;
		const auto obj = mShadingObjectManager->Get<Shading::RayGen::RayGenClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::RaySorting::RaySortingClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
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
		const auto obj = mShadingObjectManager->Get<Shading::SVGF::SVGFClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// BlurFilter
	{
		auto initData = Shading::BlurFilter::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::BlurFilter::BlurFilterClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// VolumetricLight
	{
		auto initData = Shading::VolumetricLight::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->TextureWidth = 160;
		initData->TextureHeight = 90;
		initData->TextureDepth = 128;
		const auto obj = mShadingObjectManager->Get<Shading::VolumetricLight::VolumetricLightClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// SSCS
	{
		auto initData = Shading::SSCS::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		const auto obj = mShadingObjectManager->Get<Shading::SSCS::SSCSClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// MotionBlur
	{
		auto initData = Shading::MotionBlur::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		const auto obj = mShadingObjectManager->Get<Shading::MotionBlur::MotionBlurClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// Bloom
	{
		auto initData = Shading::Bloom::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		const auto obj = mShadingObjectManager->Get<Shading::Bloom::BloomClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// DOF
	{
		auto initData = Shading::DOF::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		const auto obj = mShadingObjectManager->Get<Shading::DOF::DOFClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// EyeAdaption
	{
		auto initData = Shading::EyeAdaption::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		const auto obj = mShadingObjectManager->Get<Shading::EyeAdaption::EyeAdaptionClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// RaytracedShadow
	{
		auto initData = Shading::RaytracedShadow::MakeInitData();
		initData->RaytracingSupported = mbRaytracingSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		const auto obj = mShadingObjectManager->Get<Shading::RaytracedShadow::RaytracedShadowClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// GammaCorrection
	{
		auto initData = Shading::ChromaticAberration::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		const auto obj = mShadingObjectManager->Get<Shading::ChromaticAberration::ChromaticAberrationClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}

	CheckReturn(mpLogFile, mShadingObjectManager->CompileShaders(mShaderManager.get(), L".\\..\\..\\..\\assets\\Shaders\\HLSL\\"));
	CheckReturn(mpLogFile, mShadingObjectManager->BuildRootSignatures());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineStates());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildDescriptors(mDescriptorHeap.get()));

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
	GeometryGenerator geoGen{};
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

	mSkySphere = std::make_unique<Foundation::RenderItem>(Foundation::Resource::FrameResource::Count);

	CheckReturn(mpLogFile, mCommandObject->ResetDirectCommandList());

	const auto CmdList = mCommandObject->DirectCommandList();
	CheckReturn(mpLogFile, BuildMeshGeometry(CmdList, &sphereSubmesh, vertices, indices, "SkySphere", mSkySphere->Geometry));

	CheckReturn(mpLogFile, mCommandObject->ExecuteDirectCommandList());

	mSkySphere->ObjectCBIndex = static_cast<INT>(mRenderItems.size());
	mSkySphere->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mSkySphere->IndexCount = mSkySphere->Geometry->Subsets["SkySphere"].IndexCount;
	mSkySphere->StartIndexLocation = mSkySphere->Geometry->Subsets["SkySphere"].StartIndexLocation;
	mSkySphere->BaseVertexLocation = mSkySphere->Geometry->Subsets["SkySphere"].BaseVertexLocation;
	XMStoreFloat4x4(&mSkySphere->World, XMMatrixScaling(1000.f, 1000.f, 1000.f));

	return TRUE;
}

BOOL DxRenderer::BuildLights() {
	auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	// Directional light 1
	{
		std::shared_ptr<Common::Foundation::Light> light = std::make_shared<Common::Foundation::Light>();
		light->Type = Common::Foundation::LightType::E_Directional;
		light->Direction = { 0.577f, -0.577f, 0.577f };
		light->Color = { 240.f / 255.f, 235.f / 255.f, 223.f / 255.f };
		light->Intensity = 1.802f;
		shadow->AddLight(light);
	}
	// Directional light 2
	{
		std::shared_ptr<Common::Foundation::Light> light = std::make_shared<Common::Foundation::Light>();
		light->Type = Common::Foundation::LightType::E_Directional;
		light->Direction = { 0.067f, -0.701f, -0.836f };
		light->Color = { 149.f / 255.f, 142.f / 255.f, 100.f / 255.f };
		light->Intensity = 1.534f;
		shadow->AddLight(light);
	}

	return TRUE;
}

BOOL DxRenderer::BuildScene() {
	auto env = mShadingObjectManager->Get<Shading::EnvironmentMap::EnvironmentMapClass>();
	auto mipmap = mShadingObjectManager->Get<Shading::Util::MipmapGenerator::MipmapGeneratorClass>();
	auto converter = mShadingObjectManager->Get<Shading::Util::EquirectangularConverter::EquirectangularConverterClass>();
	CheckReturn(mpLogFile, env->SetEnvironmentMap(
		mpCurrentFrameResource,
		mipmap,
		converter,
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

	auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();

	std::vector<Common::Foundation::Light*> lights{};
	shadow->Lights(lights);

	CheckReturn(mpLogFile, mpImGuiManager->DrawImGui(
		CmdList, 
		mpShadingArgumentSet, 
		lights.data(),
		shadow->LightCount(),
		mPendingLights,
		mClientWidth, 
		mClientHeight, 
		mbRaytracingSupported));

	CheckReturn(mpLogFile, mCommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL DxRenderer::DrawShadow() {
	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	const auto rayShadow = mShadingObjectManager->Get<Shading::RaytracedShadow::RaytracedShadowClass>();
	const auto gbuffer = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();

	if (mpShadingArgumentSet->RaytracingEnabled) {
		CheckReturn(mpLogFile, rayShadow->CalcShadow(
			mpCurrentFrameResource,
			mAccelerationStructureManager->AccelerationStructure(),
			gbuffer->PositionMap(),
			gbuffer->PositionMapSrv(),
			gbuffer->NormalMap(),
			gbuffer->NormalMapSrv(),
			mDepthStencilBuffer->GetDepthStencilBuffer(),
			mDepthStencilBuffer->DepthStencilBufferSrv()));
	}
	else {
		CheckReturn(mpLogFile, shadow->Run(
			mpCurrentFrameResource,
			gbuffer->PositionMap(),
			gbuffer->PositionMapSrv(),
			mRendableItems[Common::Foundation::Mesh::RenderType::E_Opaque]));
	}

	return TRUE;
}

BOOL DxRenderer::ApplyContactShadow() {
	const auto sscs = mShadingObjectManager->Get<Shading::SSCS::SSCSClass>();
	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	const auto gbuffer = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();

	CheckReturn(mpLogFile, sscs->ComputeContactShadow(
		mpCurrentFrameResource,
		gbuffer->PositionMap(),
		gbuffer->PositionMapSrv(),
		gbuffer->NormalMap(),
		gbuffer->NormalMapSrv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->DepthStencilBufferSrv()));

	CheckReturn(mpLogFile, sscs->ApplyContactShadow(
		mpCurrentFrameResource,
		shadow->ShadowMap(),
		shadow->ShadowMapUav()));

	return TRUE;
}

BOOL DxRenderer::DrawAO() {
	const auto raygen = mShadingObjectManager->Get<Shading::RayGen::RayGenClass>();
	const auto raysorting = mShadingObjectManager->Get<Shading::RaySorting::RaySortingClass>();
	const auto rtao = mShadingObjectManager->Get<Shading::RTAO::RTAOClass>();
	const auto svgf = mShadingObjectManager->Get<Shading::SVGF::SVGFClass>();
	const auto ssao = mShadingObjectManager->Get<Shading::SSAO::SSAOClass>();
	const auto gbuffer = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();

	if (mpShadingArgumentSet->RaytracingEnabled) {		
		if (mpShadingArgumentSet->RTAO.RaySortingEnabled) {
			CheckReturn(mpLogFile, raygen->GenerateRays(
				mpCurrentFrameResource,
				gbuffer->NormalDepthMap(),
				gbuffer->NormalDepthMapSrv(),
				gbuffer->PositionMap(),
				gbuffer->PositionMapSrv(),
				mpShadingArgumentSet->RTAO.CheckboardRayGeneration));

			CheckReturn(mpLogFile, raysorting->CalcRayIndexOffset(
				mpCurrentFrameResource,
				gbuffer->NormalDepthMap(),
				gbuffer->NormalDepthMapSrv()));
		}

		CheckReturn(mpLogFile, rtao->DrawAO(
			mpCurrentFrameResource,
			mAccelerationStructureManager->AccelerationStructure(),
			gbuffer->PositionMap(),
			gbuffer->PositionMapSrv(),
			gbuffer->NormalDepthMap(),
			gbuffer->NormalDepthMapSrv(),
			raygen->RayDirectionOriginDepthMap(),
			raygen->RayDirectionOriginDepthMapSrv(),
			raysorting->RayIndexOffsetMap(),
			raysorting->RayIndexOffsetMapSrv(),
			mpShadingArgumentSet->RTAO.RaySortingEnabled,
			mpShadingArgumentSet->RTAO.CheckboardRayGeneration));

		// Denosing(Spatio - Temporal Variance Guided Filtering)
		{
			// Temporal supersampling 
			{
				// Stage 1: Reverse reprojection
				{
					const auto PrevTemporalCacheFrameIndex = rtao->CurrentTemporalCacheFrameIndex();
					const auto CurrTemporalCacheFrameIndex = rtao->MoveToNextTemporalCacheFrame();

					const auto PrevTemporalAOFrameIndex = rtao->CurrentTemporalAOFrameIndex();
					const auto CurrTemporalAOFrameIndex = rtao->MoveToNextTemporalAOFrame();

					// Retrieves values from previous frame via reverse reprojection.
					CheckReturn(mpLogFile, svgf->ReverseReprojectPreviousFrame(
						mpCurrentFrameResource,
						gbuffer->NormalDepthMap(),
						gbuffer->NormalDepthMapSrv(),
						gbuffer->ReprojectedNormalDepthMap(),
						gbuffer->ReprojectedNormalDepthMapSrv(),
						gbuffer->CachedNormalDepthMap(),
						gbuffer->CachedNormalDepthMapSrv(),
						gbuffer->VelocityMap(),
						gbuffer->VelocityMapSrv(),
						rtao->TemporalAOCoefficientResource(PrevTemporalAOFrameIndex),
						rtao->TemporalAOCoefficientSrv(PrevTemporalAOFrameIndex),
						rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_AOCoefficientSquaredMean, PrevTemporalCacheFrameIndex),
						rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_AOCoefficientSquaredMean, PrevTemporalCacheFrameIndex),
						rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_RayHitDistance, PrevTemporalCacheFrameIndex),
						rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_RayHitDistance, PrevTemporalCacheFrameIndex),
						rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_TSPP, PrevTemporalCacheFrameIndex),
						rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_TSPP, PrevTemporalCacheFrameIndex),
						rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
						rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::EU_TSPP, CurrTemporalCacheFrameIndex),
						Shading::SVGF::Value::E_Contrast));
				}
				// Stage 2: Blending current frame value with the reprojected cached value.
				{
					// Calculate local mean and variance for clamping during the blending operation.
					CheckReturn(mpLogFile, svgf->CalculateLocalMeanVariance(
						mpCurrentFrameResource,
						rtao->AOCoefficientResource(Shading::RTAO::Resource::AO::E_AOCoefficient),
						rtao->AOCoefficientDescriptor(Shading::RTAO::Descriptor::AO::ES_AOCoefficient),
						Shading::SVGF::Value::E_Contrast,
						mpShadingArgumentSet->RTAO.CheckboardRayGeneration));
					// Interpolate the variance for the inactive cells from the valid checkerboard cells.
					if (mpShadingArgumentSet->RTAO.CheckboardRayGeneration) {
						CheckReturn(mpLogFile, svgf->FillInCheckerboard(
							mpCurrentFrameResource,
							mpShadingArgumentSet->RTAO.CheckboardRayGeneration));
					}
					// Blends reprojected values with current frame values.
					// Inactive pixels are filtered from active neighbors on checkerboard sampling before the blending operation.
					{
						const auto CurrTemporalCacheFrameIndex = rtao->MoveToNextTemporalCacheFrame();
						const auto CurrAOResourceFrameIndex = rtao->MoveToNextTemporalAOFrame();

						CheckReturn(mpLogFile, svgf->BlendWithCurrentFrame(
							mpCurrentFrameResource,
							rtao->AOCoefficientResource(Shading::RTAO::Resource::AO::E_AOCoefficient),
							rtao->AOCoefficientDescriptor(Shading::RTAO::Descriptor::AO::ES_AOCoefficient),
							rtao->AOCoefficientResource(Shading::RTAO::Resource::AO::E_RayHitDistance),
							rtao->AOCoefficientDescriptor(Shading::RTAO::Descriptor::AO::ES_RayHitDistance),
							rtao->TemporalAOCoefficientResource(CurrAOResourceFrameIndex),
							rtao->TemporalAOCoefficientUav(CurrAOResourceFrameIndex),
							rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_AOCoefficientSquaredMean, CurrTemporalCacheFrameIndex),
							rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::EU_AOCoefficientSquaredMean, CurrTemporalCacheFrameIndex),
							rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_RayHitDistance, CurrTemporalCacheFrameIndex),
							rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::EU_RayHitDistance, CurrTemporalCacheFrameIndex),
							rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
							rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::EU_TSPP, CurrTemporalCacheFrameIndex),
							Shading::SVGF::Value::E_Contrast));
					}
				}
			}
			// Filtering
			{
				// Stage 1: Applies a single pass of a Atrous wavelet transform filter.
				if (mpShadingArgumentSet->RTAO.Denoiser.FullscreenBlurEnabaled) {
					const auto CurrTemporalCacheFrameIndex = rtao->CurrentTemporalCacheFrameIndex();
					const auto InputAOResourceFrameIndex = rtao->CurrentTemporalAOFrameIndex();
					const auto OutputAOResourceFrameIndex = rtao->MoveToNextTemporalAOFrame();
				
					const FLOAT RayHitDistToKernelWidthScale = 22 / mpShadingArgumentSet->RTAO.OcclusionRadius *
						mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.AdaptiveKernelSizeRayHitDistanceScaleFactor;
					const FLOAT RayHitDistToKernelSizeScaleExp = Foundation::Util::D3D12Util::Lerp(
						1,
						mpShadingArgumentSet->RTAO.AtrousWaveletTransformFilter.AdaptiveKernelSizeRayHitDistanceScaleExponent,
						Foundation::Util::D3D12Util::RelativeCoef(mpShadingArgumentSet->RTAO.OcclusionRadius, 4, 22));

					CheckReturn(mpLogFile, svgf->ApplyAtrousWaveletTransformFilter(
						mpCurrentFrameResource,
						gbuffer->NormalDepthMap(),
						gbuffer->NormalDepthMapSrv(),
						rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_RayHitDistance, CurrTemporalCacheFrameIndex),
						rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_RayHitDistance, CurrTemporalCacheFrameIndex),
						rtao->TemporalCacheResource(Shading::RTAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
						rtao->TemporalCacheDescriptor(Shading::RTAO::Descriptor::TemporalCache::ES_TSPP, CurrTemporalCacheFrameIndex),
						rtao->TemporalAOCoefficientResource(InputAOResourceFrameIndex),
						rtao->TemporalAOCoefficientSrv(InputAOResourceFrameIndex),
						rtao->TemporalAOCoefficientResource(OutputAOResourceFrameIndex),
						rtao->TemporalAOCoefficientUav(OutputAOResourceFrameIndex),
						Shading::SVGF::Value::E_Contrast,
						RayHitDistToKernelWidthScale,
						RayHitDistToKernelSizeScaleExp));
				}
				// Stage 2: 3x3 multi-pass disocclusion blur (with more relaxed depth-aware constraints for such pixels).
				if (mpShadingArgumentSet->RTAO.Denoiser.DisocclusionBlurEnabled) {
					const auto CurrAOResourceFrameIndex = rtao->CurrentTemporalAOFrameIndex();

					CheckReturn(mpLogFile, svgf->BlurDisocclusion(
						mpCurrentFrameResource,
						mDepthStencilBuffer->GetDepthStencilBuffer(),
						mDepthStencilBuffer->DepthStencilBufferSrv(),
						gbuffer->RoughnessMetalnessMap(),
						gbuffer->RoughnessMetalnessMapSrv(),
						rtao->TemporalAOCoefficientResource(CurrAOResourceFrameIndex),
						rtao->TemporalAOCoefficientUav(CurrAOResourceFrameIndex),
						Shading::SVGF::Value::E_Contrast,
						mpShadingArgumentSet->RTAO.Denoiser.LowTsppBlurPassCount));
				}
			}
		}
	}
	else {
		CheckReturn(mpLogFile, ssao->DrawAO(
			mpCurrentFrameResource,
			gbuffer->NormalDepthMap(),
			gbuffer->NormalDepthMapSrv(),
			gbuffer->PositionMap(),
			gbuffer->PositionMapSrv()));

		// Denosing(Spatio - Temporal Variance Guided Filtering)
		{
			// Temporal supersampling 
			{
				// Stage 1: Reverse reprojection
				{
					const auto PrevTemporalCacheFrameIndex = ssao->CurrentTemporalCacheFrameIndex();
					const auto CurrTemporalCacheFrameIndex = ssao->MoveToNextTemporalCacheFrame();
		
					const auto PrevTemporalAOFrameIndex = ssao->CurrentTemporalAOFrameIndex();
					const auto CurrTemporalAOFrameIndex = ssao->MoveToNextTemporalAOFrame();
		
					// Retrieves values from previous frame via reverse reprojection.
					CheckReturn(mpLogFile, svgf->ReverseReprojectPreviousFrame(
						mpCurrentFrameResource,
						gbuffer->NormalDepthMap(),
						gbuffer->NormalDepthMapSrv(),
						gbuffer->ReprojectedNormalDepthMap(),
						gbuffer->ReprojectedNormalDepthMapSrv(),
						gbuffer->CachedNormalDepthMap(),
						gbuffer->CachedNormalDepthMapSrv(),
						gbuffer->VelocityMap(),
						gbuffer->VelocityMapSrv(),
						ssao->TemporalAOCoefficientResource(PrevTemporalAOFrameIndex),
						ssao->TemporalAOCoefficientSrv(PrevTemporalAOFrameIndex),
						ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_AOCoefficientSquaredMean, PrevTemporalCacheFrameIndex),
						ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::ES_AOCoefficientSquaredMean, PrevTemporalCacheFrameIndex),
						ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_RayHitDistance, PrevTemporalCacheFrameIndex),
						ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::ES_RayHitDistance, PrevTemporalCacheFrameIndex),
						ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_TSPP, PrevTemporalCacheFrameIndex),
						ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::ES_TSPP, PrevTemporalCacheFrameIndex),
						ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
						ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::EU_TSPP, CurrTemporalCacheFrameIndex),
						Shading::SVGF::Value::E_Contrast));
				}
				// Stage 2: Blending current frame value with the reprojected cached value.
				{
					// Calculate local mean and variance for clamping during the blending operation.
					CheckReturn(mpLogFile, svgf->CalculateLocalMeanVariance(
						mpCurrentFrameResource,
						ssao->AOCoefficientResource(Shading::SSAO::Resource::AO::E_AOCoefficient),
						ssao->AOCoefficientDescriptor(Shading::SSAO::Descriptor::AO::ES_AOCoefficient),
						Shading::SVGF::Value::E_Contrast,
						FALSE));
					
					// Blends reprojected values with current frame values.
					// Inactive pixels are filtered from active neighbors on checkerboard sampling before the blending operation.
					{
						const auto CurrTemporalCacheFrameIndex = ssao->MoveToNextTemporalCacheFrame();
						const auto CurrAOResourceFrameIndex = ssao->MoveToNextTemporalAOFrame();
		
						CheckReturn(mpLogFile, svgf->BlendWithCurrentFrame(
							mpCurrentFrameResource,
							ssao->AOCoefficientResource(Shading::SSAO::Resource::AO::E_AOCoefficient),
							ssao->AOCoefficientDescriptor(Shading::SSAO::Descriptor::AO::ES_AOCoefficient),
							ssao->AOCoefficientResource(Shading::SSAO::Resource::AO::E_RayHitDistance),
							ssao->AOCoefficientDescriptor(Shading::SSAO::Descriptor::AO::ES_RayHitDistance),
							ssao->TemporalAOCoefficientResource(CurrAOResourceFrameIndex),
							ssao->TemporalAOCoefficientUav(CurrAOResourceFrameIndex),
							ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_AOCoefficientSquaredMean, CurrTemporalCacheFrameIndex),
							ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::EU_AOCoefficientSquaredMean, CurrTemporalCacheFrameIndex),
							ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_RayHitDistance, CurrTemporalCacheFrameIndex),
							ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::EU_RayHitDistance, CurrTemporalCacheFrameIndex),
							ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
							ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::EU_TSPP, CurrTemporalCacheFrameIndex),
							Shading::SVGF::Value::E_Contrast));
					}
				}
			}
			// Filtering
			{
				// Stage 1: Applies a single pass of a Atrous wavelet transform filter.
				if (mpShadingArgumentSet->SSAO.Denoiser.FullscreenBlurEnabaled) {
					const auto CurrTemporalCacheFrameIndex = ssao->CurrentTemporalCacheFrameIndex();
					const auto InputAOResourceFrameIndex = ssao->CurrentTemporalAOFrameIndex();
					const auto OutputAOResourceFrameIndex = ssao->MoveToNextTemporalAOFrame();
			
					const FLOAT RayHitDistToKernelWidthScale = 22 / mpShadingArgumentSet->SSAO.OcclusionRadius *
						mpShadingArgumentSet->SSAO.AtrousWaveletTransformFilter.AdaptiveKernelSizeRayHitDistanceScaleFactor;
					const FLOAT RayHitDistToKernelSizeScaleExp = Foundation::Util::D3D12Util::Lerp(
						1,
						mpShadingArgumentSet->SSAO.AtrousWaveletTransformFilter.AdaptiveKernelSizeRayHitDistanceScaleExponent,
						Foundation::Util::D3D12Util::RelativeCoef(mpShadingArgumentSet->SSAO.OcclusionRadius, 4, 22));
			
					CheckReturn(mpLogFile, svgf->ApplyAtrousWaveletTransformFilter(
						mpCurrentFrameResource,
						gbuffer->NormalDepthMap(),
						gbuffer->NormalDepthMapSrv(),
						ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_RayHitDistance, CurrTemporalCacheFrameIndex),
						ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::ES_RayHitDistance, CurrTemporalCacheFrameIndex),
						ssao->TemporalCacheResource(Shading::SSAO::Resource::TemporalCache::E_TSPP, CurrTemporalCacheFrameIndex),
						ssao->TemporalCacheDescriptor(Shading::SSAO::Descriptor::TemporalCache::ES_TSPP, CurrTemporalCacheFrameIndex),
						ssao->TemporalAOCoefficientResource(InputAOResourceFrameIndex),
						ssao->TemporalAOCoefficientSrv(InputAOResourceFrameIndex),
						ssao->TemporalAOCoefficientResource(OutputAOResourceFrameIndex),
						ssao->TemporalAOCoefficientUav(OutputAOResourceFrameIndex),
						Shading::SVGF::Value::E_Contrast,
						RayHitDistToKernelWidthScale,
						RayHitDistToKernelSizeScaleExp));
				}
				// Stage 2: 3x3 multi-pass disocclusion blur (with more relaxed depth-aware constraints for such pixels).
				if (mpShadingArgumentSet->SSAO.Denoiser.DisocclusionBlurEnabled) {
					const auto CurrAOResourceFrameIndex = ssao->CurrentTemporalAOFrameIndex();
			
					CheckReturn(mpLogFile, svgf->BlurDisocclusion(
						mpCurrentFrameResource,
						mDepthStencilBuffer->GetDepthStencilBuffer(),
						mDepthStencilBuffer->DepthStencilBufferSrv(),
						gbuffer->RoughnessMetalnessMap(),
						gbuffer->RoughnessMetalnessMapSrv(),
						ssao->TemporalAOCoefficientResource(CurrAOResourceFrameIndex),
						ssao->TemporalAOCoefficientUav(CurrAOResourceFrameIndex),
						Shading::SVGF::Value::E_Contrast,
						mpShadingArgumentSet->SSAO.Denoiser.LowTsppBlurPassCount));
				}
			}
		}
	}

	return TRUE;
}

BOOL DxRenderer::IntegrateIrradiance() {
	const auto rtao = mShadingObjectManager->Get<Shading::RTAO::RTAOClass>();
	const auto ssao = mShadingObjectManager->Get<Shading::SSAO::SSAOClass>();
	const auto brdf = mShadingObjectManager->Get<Shading::BRDF::BRDFClass>();
	const auto tone = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();	
	const auto gbuffer = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();
	const auto env = mShadingObjectManager->Get<Shading::EnvironmentMap::EnvironmentMapClass>();
	
	const auto CurrTemporalAOFrameIndex = rtao->CurrentTemporalAOFrameIndex();
	const auto AOMap = mpShadingArgumentSet->RaytracingEnabled 
		? rtao->TemporalAOCoefficientResource(rtao->CurrentTemporalAOFrameIndex()) 
		: ssao->TemporalAOCoefficientResource(ssao->CurrentTemporalAOFrameIndex());
	const auto AOSrv = mpShadingArgumentSet->RaytracingEnabled 
		? rtao->TemporalAOCoefficientSrv(rtao->CurrentTemporalAOFrameIndex()) 
		: ssao->TemporalAOCoefficientSrv(ssao->CurrentTemporalAOFrameIndex());

	CheckReturn(mpLogFile, brdf->IntegrateIrradiance(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		tone->InterMediateMapResource(),
		tone->InterMediateMapRtv(),
		tone->InterMediateCopyMapResource(),
		tone->InterMediateCopyMapSrv(),
		gbuffer->AlbedoMap(),
		gbuffer->AlbedoMapSrv(),
		gbuffer->NormalMap(),
		gbuffer->NormalMapSrv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->DepthStencilBufferSrv(),
		gbuffer->SpecularMap(),
		gbuffer->SpecularMapSrv(),
		gbuffer->RoughnessMetalnessMap(),
		gbuffer->RoughnessMetalnessMapSrv(),
		gbuffer->PositionMap(),
		gbuffer->PositionMapSrv(),
		AOMap,
		AOSrv,
		env->DiffuseIrradianceCubeMap(),
		env->DiffuseIrradianceCubeMapSrv(),
		env->BrdfLutMap(),
		env->BrdfLutMapSrv(),
		env->PrefilteredEnvironmentCubeMap(),
		env->PrefilteredEnvironmentCubeMapSrv(),
		mpShadingArgumentSet->AOEnabled));

	return TRUE;
}

BOOL DxRenderer::ApplyVolumetricLight() {
	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	const auto volume = mShadingObjectManager->Get<Shading::VolumetricLight::VolumetricLightClass>();
	const auto tone = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();
	const auto gbuffer = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();

	std::vector<Common::Foundation::Light*> lights;
	std::vector<Foundation::Resource::GpuResource*> depthMaps;

	shadow->Lights(lights);
	shadow->ZDepthMaps(depthMaps);

	CheckReturn(mpLogFile, volume->BuildFog(
		mpCurrentFrameResource,
		depthMaps.data(),
		shadow->ZDepthMapSrv(),
		mpCamera->NearZ(),
		mpCamera->FarZ(),
		mpShadingArgumentSet->VolumetricLight.DepthExponent,
		mpShadingArgumentSet->VolumetricLight.UniformDensity,
		mpShadingArgumentSet->VolumetricLight.DensityScale,
		mpShadingArgumentSet->VolumetricLight.AnisotropicCoefficient,
		shadow->LightCount()));

	CheckReturn(mpLogFile, volume->ApplyFog(
		mpCurrentFrameResource,
		tone->InterMediateMapResource(),
		tone->InterMediateMapRtv(),
		gbuffer->PositionMap(),
		gbuffer->PositionMapSrv(),
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mpCamera->NearZ(), mpCamera->FarZ(), mpShadingArgumentSet->VolumetricLight.DepthExponent,
		mpShadingArgumentSet->VolumetricLight.TricubicSamplingEnabled));

	return TRUE;
}

BOOL DxRenderer::ApplyEyeAdaption() {
	const auto eye = mShadingObjectManager->Get<Shading::EyeAdaption::EyeAdaptionClass>();
	const auto tone = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();

	CheckReturn(mpLogFile, eye->ClearHistogram(
		mpCurrentFrameResource));

	CheckReturn(mpLogFile, eye->BuildLuminanceHistogram(
		mpCurrentFrameResource,
		tone->InterMediateMapResource(),
		tone->InterMediateMapSrv()));

	CheckReturn(mpLogFile, eye->PercentileExtract(
		mpCurrentFrameResource));

	CheckReturn(mpLogFile, eye->TemporalSmoothing(
		mpCurrentFrameResource,
		mDeltaTime));

	return TRUE;
}

BOOL DxRenderer::ApplyDOF() {
	const auto dof = mShadingObjectManager->Get<Shading::DOF::DOFClass>();
	const auto gbuffer = mShadingObjectManager->Get<Shading::GBuffer::GBufferClass>();
	const auto tone = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();

	CheckReturn(mpLogFile, dof->CalcFocalDistance(
		mpCurrentFrameResource,
		gbuffer->PositionMap(),
		gbuffer->PositionMapSrv()));

	CheckReturn(mpLogFile, dof->CircleOfConfusion(
		mpCurrentFrameResource,
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->DepthStencilBufferSrv(),
		mpShadingArgumentSet->DOF.FocusRange));

	CheckReturn(mpLogFile, dof->Bokeh(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		tone->InterMediateMapResource(),
		tone->InterMediateMapRtv(),
		tone->InterMediateCopyMapResource(),
		tone->InterMediateCopyMapSrv(),
		mpShadingArgumentSet->DOF.BokehSampleCount,
		mpShadingArgumentSet->DOF.BokehRadius,
		mpShadingArgumentSet->DOF.BokehThreshold,
		mpShadingArgumentSet->DOF.HighlightPower));

	CheckReturn(mpLogFile, dof->BokehBlur(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		tone->InterMediateMapResource(),
		tone->InterMediateMapRtv(),
		tone->InterMediateCopyMapResource(),
		tone->InterMediateCopyMapSrv()));

	return TRUE;
}

BOOL DxRenderer::ApplyBloom() {
	const auto downSampleFunc = [&](
			Foundation::Resource::GpuResource* const pInputMap,
			D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
			Foundation::Resource::GpuResource* const pOutputMap,
			D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
			UINT srcTexDimX, UINT srcTexDimY, UINT dstTexDimX, UINT dstTexDimY,
			UINT kernelRadius) -> BOOL {
		const auto scaler = mShadingObjectManager->Get<Shading::Util::TextureScaler::TextureScalerClass>();
		CheckReturn(mpLogFile, scaler->DownSample2Nx2N(
			mpCurrentFrameResource,
			pInputMap,
			si_inputMap,
			pOutputMap,
			uo_outputMap,
			srcTexDimX, srcTexDimY, dstTexDimX, dstTexDimY,
			kernelRadius));

		return TRUE;
	};

	const auto blurFunc = [&](
			Foundation::Resource::GpuResource* const pInputMap,
			D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
			Foundation::Resource::GpuResource* const pOutputMap,
			D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
			UINT width, UINT height) -> BOOL {
		const auto blurFilter = mShadingObjectManager->Get<Shading::BlurFilter::BlurFilterClass>();
		CheckReturn(mpLogFile, blurFilter->GaussianBlur(
			mpCurrentFrameResource,
			Shading::BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN9x9,
			pInputMap,
			si_inputMap,
			pOutputMap,
			uo_outputMap,
			width, height));

		return TRUE;
	};

	const auto bloom = mShadingObjectManager->Get<Shading::Bloom::BloomClass>();
	const auto tone = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();

	CheckReturn(mpLogFile, bloom->ExtractHighlights(
		mpCurrentFrameResource,
		tone->InterMediateMapResource(),
		tone->InterMediateMapSrv(),
		mpShadingArgumentSet->Bloom.Threshold,
		mpShadingArgumentSet->Bloom.SoftKnee,
		downSampleFunc));

	CheckReturn(mpLogFile, bloom->BlurHighlights(
		mpCurrentFrameResource, downSampleFunc, blurFunc));

	CheckReturn(mpLogFile, bloom->ApplyBloom(
		mpCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		tone->InterMediateMapResource(),
		tone->InterMediateMapRtv(),
		tone->InterMediateCopyMapResource(),
		tone->InterMediateCopyMapSrv()));

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