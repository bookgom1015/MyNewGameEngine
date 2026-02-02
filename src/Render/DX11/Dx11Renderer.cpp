#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Dx11Renderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Foundation/Mesh/Transform.hpp"
#include "Common/Render/ShadingArgument.hpp"
#include "Common/Util/MathUtil.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Core/SwapChain.hpp"
#include "Render/DX11/Foundation/Core/DepthStencilBuffer.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX11/Foundation/Resource/MaterialData.hpp"
#include "Render/DX11/Foundation/ConstantBuffer.h"
#include "Render/DX11/Foundation/RenderItem.hpp"
#include "Render/DX11/Foundation/Util/D3D11Util.hpp"
#include "Render/DX11/Shading/Util/ShadingObjectManager.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Render/DX11/Shading/Util/SamplerUtil.hpp"
#include "Render/DX11/Shading/GBuffer.hpp"
#include "Render/DX11/Shading/BRDF.hpp"
#include "Render/DX11/Shading/Shadow.hpp"
#include "Render/DX11/Shading/GammaCorrection.hpp"
#include "Render/DX11/Shading/ToneMapping.hpp"
#include "Render/DX11/Shading/TAA.hpp"
#include "Render/DX11/Shading/EnvironmentMap.hpp"
#include "ImGuiManager/DX11/Dx11ImGuiManager.hpp"
#include "FrankLuna/GeometryGenerator.h"

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
	mShadingObjectManager->Add<Shading::BRDF::BRDFClass>();
	mShadingObjectManager->Add<Shading::Shadow::ShadowClass>();
	mShadingObjectManager->Add<Shading::GammaCorrection::GammaCorrectionClass>();
	mShadingObjectManager->Add<Shading::ToneMapping::ToneMappingClass>();
	mShadingObjectManager->Add<Shading::TAA::TAAClass>();
	mShadingObjectManager->Add<Shading::EnvironmentMap::EnvironmentMapClass>();
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

	CheckReturn(mpLogFile, Foundation::Util::D3D11Util::Initialize(mpLogFile));
	CheckReturn(mpLogFile, Shading::Util::SamplerUtil::Initialize(mpLogFile, mDevice.get()));

	CheckReturn(mpLogFile, mpImGuiManager->InitializeD3D11(mDevice.get()));
	mpImGuiManager->HookMsgCallback(mpWindowsManager);

	CheckReturn(mpLogFile, mShadingObjectManager->Initialize(mpLogFile));
	CheckReturn(mpLogFile, mShaderManager->Initialize(
		mpLogFile, L".\\..\\..\\..\\assets\\Shaders\\HLSL5\\"));

	CheckReturn(mpLogFile, mFrameResource->Initalize(mpLogFile, mDevice.get()));

	CheckReturn(mpLogFile, InitShadingObjects());

	mSceneBounds.Center = XMFLOAT3(0.f, 0.f, 0.f);
	const FLOAT WidthSquared = 128.f * 128.f;
	mSceneBounds.Radius = sqrtf(WidthSquared + WidthSquared);

	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	{
		std::shared_ptr<Common::Foundation::Light> light = std::make_shared<Common::Foundation::Light>();
		light->Type = Common::Foundation::LightType::E_Directional;
		light->Direction = { 0.577f, -0.577f, 0.577f };
		light->Color = { 240.f / 255.f, 235.f / 255.f, 223.f / 255.f };
		light->Intensity = 1.802f;
		shadow->AddLight(light);
	}

	CheckReturn(mpLogFile, mShadingObjectManager->CompileShaders());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineStates());

	CheckReturn(mpLogFile, BuildSkySphere());

	return TRUE;
}

void Dx11Renderer::CleanUp() {
	if (mbCleanedUp) return;

	mDevice->Flush();

	Shading::Util::SamplerUtil::CleanUp();

	if (mSkySphere) mSkySphere.reset();
	mRenderItems.clear();

	for (auto& meshGeo : mMeshGeometries) 
		meshGeo.second->CleanUp();
	mMeshGeometries.clear();

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
	CheckReturn(mpLogFile, ResolvePendingLights());

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

	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	CheckReturn(mpLogFile, shadow->Run(
		mFrameResource.get(),
		gbuffer->PositionMapSrv(),
		ritems.data(), static_cast<UINT>(ritems.size())));

	const auto tone = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();
	const auto brdf = mShadingObjectManager->Get<Shading::BRDF::BRDFClass>();
	const auto env = mShadingObjectManager->Get<Shading::EnvironmentMap::EnvironmentMapClass>();
	CheckReturn(mpLogFile, brdf->ComputeBRDF(
		mFrameResource.get(),
		mSwapChain->ScreenViewport(),
		tone->InterMediateMapRtv(),
		gbuffer->AlbedoMapSrv(),
		gbuffer->NormalMapSrv(),
		gbuffer->PositionMapSrv(),
		gbuffer->RoughnessMetalnessMapSrv(),
		shadow->ShadowMapSrv(),
		shadow->LightCount(),
		env->DiffuseIrradianceMapSrv(),
		env->PrefilteredEnvironmentCubeMapSrv(),
		env->BrdfLutMapSrv()));

	CheckReturn(mpLogFile, env->DrawSkySphere(
		mFrameResource.get(),
		mSwapChain->ScreenViewport(),
		tone->InterMediateMapRtv(),
		mDepthStencilBuffer->DepthStencilView(),
		mSkySphere.get()));

	CheckReturn(mpLogFile, tone->Apply(
		mFrameResource.get(),
		mSwapChain->ScreenViewport(),
		mSwapChain->SwapChainBuffer(),
		mSwapChain->SwapChainBufferRtv()));

	if (mpShadingArgumentSet->GammaCorrection.Enabled) {
		const auto gamma = mShadingObjectManager->Get<Shading::GammaCorrection::GammaCorrectionClass>();
		CheckReturn(mpLogFile, gamma->Apply(
			mFrameResource.get(),
			mSwapChain->ScreenViewport(),
			mSwapChain->SwapChainBuffer(),
			mSwapChain->SwapChainBufferRtv(),
			mSwapChain->SwapChainBufferCopy(),
			mSwapChain->SwapChainBufferCopySrv()));
	}

	if (mpShadingArgumentSet->TAA.Enabled) {
		const auto taa = mShadingObjectManager->Get<Shading::TAA::TAAClass>();
		CheckReturn(mpLogFile, taa->Apply(
			mFrameResource.get(),
			mSwapChain->ScreenViewport(),
			mSwapChain->SwapChainBuffer(),
			mSwapChain->SwapChainBufferRtv(),
			mSwapChain->SwapChainBufferCopy(),
			mSwapChain->SwapChainBufferCopySrv(),
			gbuffer->VelocityMapSrv()));
	}

	std::vector<Common::Foundation::Light*> lights{};
	shadow->Lights(lights);

	CheckReturn(mpLogFile, mpImGuiManager->DrawImGui(
		mpShadingArgumentSet, 
		lights.data(),
		shadow->LightCount(),
		mPendingLights, 
		mClientWidth, mClientHeight));

	CheckReturn(mpLogFile, mSwapChain->Present());

	return TRUE;
}

BOOL Dx11Renderer::AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Mesh::Transform* const pTransform, Common::Foundation::Hash& hash) {
	auto meshGeo = std::make_unique<Foundation::Resource::MeshGeometry>();

	CheckReturn(mpLogFile, BuildMeshGeometry(pMesh, meshGeo.get()));
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
	ritem->FrameDirty = 2;

	return TRUE;
}

void Dx11Renderer::RemoveMesh(Common::Foundation::Hash hash) {}

BOOL Dx11Renderer::UpdateCB() {
	CheckReturn(mpLogFile, UpdatePassCB());
	CheckReturn(mpLogFile, UpdateObjectCB());
	CheckReturn(mpLogFile, UpdateMaterialCB());
	CheckReturn(mpLogFile, UpdateLightCB());
	CheckReturn(mpLogFile, UpdateGBufferCB());

	return TRUE;
}

BOOL Dx11Renderer::UpdatePassCB() {
	static UINT counter = 0;

	static PassCB passCB{ .ViewProj = Common::Util::MathUtil::Identity4x4() };
	passCB.FrameCount = counter++;

	passCB.PrevViewProj = passCB.ViewProj;
	const XMMATRIX view = XMLoadFloat4x4(&mpCamera->View());
	const XMMATRIX proj = XMLoadFloat4x4(&mpCamera->Proj());
	const XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	const XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	const XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);

	XMStoreFloat4x4(&passCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&passCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&passCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&passCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&passCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat3(&passCB.EyePosW, mpCamera->Position());

	const auto taa = mShadingObjectManager->Get<Shading::TAA::TAAClass>();
	if (mpShadingArgumentSet->TAA.Enabled) {
		const auto OffsetIndex = static_cast<UINT>(counter % taa->HaltonSequenceSize());
		passCB.JitteredOffset = taa->HaltonSequence(OffsetIndex);
	}
	else {
		passCB.JitteredOffset = { 0.f, 0.f };
	}

	CheckReturn(mpLogFile, mFrameResource->PassCB.SetData(passCB));

	return TRUE;
}

BOOL Dx11Renderer::UpdateObjectCB() {
	CheckReturn(mpLogFile, mFrameResource->ObjectCB.BeginFrame());

	for (auto& ritem : mRenderItems) {
		if (ritem->FrameDirty > 0) {
			const XMMATRIX PrevWorld = XMLoadFloat4x4(&ritem->PrevWorld);
			const XMMATRIX World = XMLoadFloat4x4(&ritem->World);

			ObjectCB objCB{};

			XMStoreFloat4x4(&objCB.PrevWorld, XMMatrixTranspose(PrevWorld));
			XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(World));

			mFrameResource->ObjectCB.CopyData(objCB, ritem->ObjectCBIndex);

			ritem->FrameDirty = --(ritem->FrameDirty);
		}
	}

	mFrameResource->ObjectCB.EndFrame();

	return TRUE;
}

BOOL Dx11Renderer::UpdateMaterialCB() {
	CheckReturn(mpLogFile, mFrameResource->MaterialCB.BeginFrame());

	for (auto& material : mMaterials) {
		if (material->FrameDirty > 0) {
			MaterialCB matCB{};
			matCB.Albedo = material->Albedo;
			matCB.Roughness = material->Roughness;
			matCB.Metalness = material->Metalness;
			matCB.Specular = material->Specular;

			mFrameResource->MaterialCB.CopyData(matCB, material->MaterialCBIndex);

			material->FrameDirty = --material->FrameDirty;
		}
	}

	mFrameResource->MaterialCB.EndFrame();

	return TRUE;
}

BOOL Dx11Renderer::UpdateLightCB() {
	static LightCB lightCB{};

	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
	const auto LightCount = shadow->LightCount();
	lightCB.LightCount = LightCount;

	static const XMMATRIX T(
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
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

		lightCB.Lights[i] = *light;
	}

	CheckReturn(mpLogFile, mFrameResource->LightCB.BeginFrame());
	mFrameResource->LightCB.CopyData(lightCB);
	mFrameResource->LightCB.EndFrame();

	return TRUE;
}

BOOL Dx11Renderer::UpdateGBufferCB() {
	GBufferCB gbufferCB{};

	gbufferCB.TexDim = { 
		static_cast<FLOAT>(mClientWidth), static_cast<FLOAT>(mClientHeight) };
	gbufferCB.DitheringMaxDist = 0.4f;
	gbufferCB.DitheringMinDist = 0.1f;

	CheckReturn(mpLogFile, mFrameResource->GBufferCB.BeginFrame());
	mFrameResource->GBufferCB.CopyData(gbufferCB);
	mFrameResource->GBufferCB.EndFrame();

	return TRUE;
}

BOOL Dx11Renderer::ResolvePendingLights() {
	const auto shadow = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();

	for (UINT i = 0, end = static_cast<UINT>(mPendingLights.size()); i < end; ++i) {
		const auto& light = mPendingLights.front();
		shadow->AddLight(light);

		mPendingLights.pop();
	}

	return TRUE;
}

BOOL Dx11Renderer::InitShadingObjects() {
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
	// BRDF
	{
		auto initData = Shading::BRDF::MakeInitData();
		initData->Width = mClientWidth;
		initData->Height = mClientHeight;
		initData->Device = mDevice.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::BRDF::BRDFClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// Shadow
	{
		auto initData = Shading::Shadow::MakeInitData();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		initData->TextureWidth = 4096;
		initData->TextureHeight = 4096;
		initData->Device = mDevice.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::Shadow::ShadowClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// GammaCorrection
	{
		auto initData = Shading::GammaCorrection::MakeInitData();
		initData->Width = mClientWidth;
		initData->Height = mClientHeight;
		initData->Device = mDevice.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::GammaCorrection::GammaCorrectionClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// ToneMapping
	{
		auto initData = Shading::ToneMapping::MakeInitData();
		initData->Width = mClientWidth;
		initData->Height = mClientHeight;
		initData->Device = mDevice.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::ToneMapping::ToneMappingClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// TAA
	{
		auto initData = Shading::TAA::MakeInitData();
		initData->Width = mClientWidth;
		initData->Height = mClientHeight;
		initData->Device = mDevice.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::TAA::TAAClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}
	// EnvironmentMap
	{
		auto initData = Shading::EnvironmentMap::MakeInitData();
		initData->Width = mClientWidth;
		initData->Height = mClientHeight;
		initData->Device = mDevice.get();
		initData->ShaderManager = mShaderManager.get();
		const auto obj = mShadingObjectManager->Get<Shading::EnvironmentMap::EnvironmentMapClass>();
		CheckReturn(mpLogFile, obj->Initialize(mpLogFile, initData.get()));
	}

	return TRUE;
}

BOOL Dx11Renderer::BuildMeshGeometry(
		Common::Foundation::Mesh::Mesh* const pMesh,
		Foundation::Resource::MeshGeometry* const pMeshGeo) {
	CheckReturn(mpLogFile, pMeshGeo->Initialize(
		mpLogFile, mDevice.get(),
		pMesh->Vertices(), pMesh->VerticesByteSize(),
		pMesh->Indices(), pMesh->IndicesByteSize(), pMesh->IndexCount(), DXGI_FORMAT_R32_UINT));

	std::vector<Common::Foundation::Mesh::Mesh::SubsetPair> subsets;
	pMesh->Subsets(subsets);

	for (const auto& subset : subsets) {
		Foundation::Resource::SubmeshGeometry submesh;
		submesh.StartIndexLocation = subset.second.StartIndexLocation;
		submesh.BaseVertexLocation = 0;
		submesh.IndexCount = subset.second.Size;
		pMeshGeo->Subsets[subset.first] = submesh;
	}

	mbMeshGeometryAdded = TRUE;

	return TRUE;
}

BOOL Dx11Renderer::BuildMeshGeometry(
		Common::Foundation::Mesh::Vertex* const pVertices,
		UINT verticesByteSize,
		UINT* const pIndices,
		UINT indicesByteSize,
		UINT numIndices,
		Foundation::Resource::MeshGeometry* const pMeshGeo,
		const std::string& name) {
	CheckReturn(mpLogFile, pMeshGeo->Initialize(
		mpLogFile, mDevice.get(),
		pVertices, verticesByteSize,
		pIndices, indicesByteSize, numIndices, DXGI_FORMAT_R32_UINT));

	Foundation::Resource::SubmeshGeometry submesh;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	submesh.IndexCount = numIndices;

	pMeshGeo->Subsets[name] = submesh;

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

BOOL Dx11Renderer::BuildRenderItem(
		Common::Foundation::Hash& hash,
		Foundation::Resource::MeshGeometry* pMeshGeo,
		const std::string& name,
		const DirectX::XMMATRIX& world) {
	auto ritem = std::make_unique<Foundation::RenderItem>();
	hash = Foundation::RenderItem::Hash(ritem.get());

	ritem->ObjectCBIndex = static_cast<INT>(mRenderItems.size());
	ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ritem->Geometry = pMeshGeo;
	ritem->IndexCount = pMeshGeo->Subsets[name].IndexCount;
	ritem->StartIndexLocation = pMeshGeo->Subsets[name].StartIndexLocation;
	ritem->BaseVertexLocation = pMeshGeo->Subsets[name].BaseVertexLocation;
	XMStoreFloat4x4(&ritem->World, world);

	mSkySphere = std::move(ritem);

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

BOOL Dx11Renderer::BuildSkySphere() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.f, 32, 32);

	Foundation::Resource::SubmeshGeometry sphereSubmesh;
	sphereSubmesh.StartIndexLocation = 0;
	sphereSubmesh.BaseVertexLocation = 0;

	const auto indexCount = static_cast<UINT>(sphere.Indices32.size());
	const auto vertexCount = static_cast<UINT>(sphere.Vertices.size());

	sphereSubmesh.IndexCount = indexCount;

	std::vector<Common::Foundation::Mesh::Vertex> vertices(vertexCount);
	for (UINT i = 0, end = static_cast<UINT>(sphere.Vertices.size()); i < end; ++i) {
		const auto index = i + sphereSubmesh.BaseVertexLocation;
		vertices[index].Position = sphere.Vertices[i].Position;
		vertices[index].Normal = sphere.Vertices[i].Normal;
		vertices[index].TexCoord = sphere.Vertices[i].TexC;
	}

	std::vector<UINT> indices(indexCount);
	for (UINT i = 0, end = static_cast<UINT>(sphere.Indices32.size()); i < end; ++i) {
		const auto index = i + sphereSubmesh.StartIndexLocation;
		indices[index] = sphere.Indices32[i];
	}

	auto vertexByteSize = static_cast<UINT>(
		sphere.Vertices.size() * sizeof(Common::Foundation::Mesh::Vertex));
	auto indexByteSize = static_cast<UINT>(
		sphere.Indices32.size() * sizeof(UINT));

	auto meshGeo = std::make_unique<Foundation::Resource::MeshGeometry>();
	CheckReturn(mpLogFile, BuildMeshGeometry(
		vertices.data(), vertexByteSize,
		indices.data(), indexByteSize, indexCount,
		meshGeo.get(), "SkySphere"));

	Common::Foundation::Hash hash{};
	CheckReturn(mpLogFile, BuildRenderItem(
		hash, meshGeo.get(), "SkySphere", XMMatrixScaling(1000.f, 1000.f, 1000.f)));

	mMeshGeometries[hash] = std::move(meshGeo);

	return TRUE;
}
