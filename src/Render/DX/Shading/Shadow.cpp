#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/Shadow.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/RenderItem.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Resource/MaterialData.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

// Cascase Shadow
// https://scahp.tistory.com/39

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_DrawZDepth = L"DrawZDepth.hlsl";
	const WCHAR* const HLSL_DrawShadow = L"DrawShadow.hlsl";

	ShadingConvention::Shadow::ShadowMap::Type ResolveShadowMapType(
			const Render::DX::Foundation::Light* const light) {
		if (light->Type == Common::Render::LightType::E_Point
			|| light->Type == Common::Render::LightType::E_Tube)
			return ShadingConvention::Shadow::ShadowMap::E_CubeMap;
		//else if (light->Type == Common::Render::LightType::E_Directional)
		//	return Shadow::ShadowMap::E_Texture2DArray;
		else
			return ShadingConvention::Shadow::ShadowMap::E_Texture2D;

	}
}

Shadow::InitDataPtr Shadow::MakeInitData() {
	return std::unique_ptr<ShadowClass::InitData>(new ShadowClass::InitData());
}

Shadow::ShadowClass::ShadowClass() {
	for (UINT i = 0; i < MaxLights; ++i) 
		mZDepthMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
	mShadowMap = std::make_unique<Foundation::Resource::GpuResource>();
}

Shadow::ShadowClass::~ShadowClass() { CleanUp(); }

void Shadow::ShadowClass::Lights(std::vector<Render::DX::Foundation::Light*>& lights) {
	for (UINT i = 0; i < mLightCount; ++i)
		lights.push_back(mLights[i].get());
}

void Shadow::ShadowClass::ZDepthMaps(std::vector<Render::DX::Foundation::Resource::GpuResource*>& maps) {
	for (UINT i = 0; i < mLightCount; ++i)
		maps.push_back(mZDepthMaps[i].get());
}

UINT Shadow::ShadowClass::CbvSrvUavDescCount() const { return 0
	+ MaxLights // ZDepthMapSrvs
	+ MaxLights // ZDepthCubeMapSrvs
	+ 1			// ShadowMapSrv
	+ 1			// ShadowMapUav
	; 
}

UINT Shadow::ShadowClass::RtvDescCount() const { return 0; }

UINT Shadow::ShadowClass::DsvDescCount() const { return 0
	+ MaxLights // ZDepthMaps
	+ MaxLights // ZDepthCubeMaps
	; 
}

BOOL Shadow::ShadowClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	mViewport = { 0.0f, 0.0f, static_cast<FLOAT>(mInitData.TexWidth), static_cast<FLOAT>(mInitData.TexHeight), 0.0f, 1.0f };
	mScissorRect = { 0, 0, static_cast<INT>(mInitData.TexWidth), static_cast<INT>(mInitData.TexHeight) };

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

void Shadow::ShadowClass::CleanUp() {
	if (mbCleanedUp) return;

	if (mShadowMap) mShadowMap.reset();

	for (UINT i = 0; i < MaxLights; ++i) {
		auto& resource = mZDepthMaps[i];
		if (resource) resource.reset();
	}

	for (UINT i = 0; i < PipelineState::Count; ++i)
		mPipelineStates[i].Reset();

	for (UINT i = 0; i < RootSignature::Count; ++i)
		mRootSignatures[i].Reset();

	mbCleanedUp = TRUE;
}

BOOL Shadow::ShadowClass::CompileShaders() {
	// DrawZDepth
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawZDepth, L"VS", L"vs_6_5");
		const auto GS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawZDepth, L"GS", L"gs_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawZDepth, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_DrawZDepth]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(GS, mShaderHashes[Shader::GS_DrawZDepth]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_DrawZDepth]));
	}
	// DrawShadow
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawShadow, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_DrawShadow]));
	}

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// DrawZDepth
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, ShadingConvention::GBuffer::MaxNumTextures, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::DrawZDepth::Count]{};
		slotRootParameter[RootSignature::DrawZDepth::CB_Light].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::DrawZDepth::CB_Object].InitAsConstantBufferView(1);
		slotRootParameter[RootSignature::DrawZDepth::CB_Material].InitAsConstantBufferView(2);
		slotRootParameter[RootSignature::DrawZDepth::RC_Consts].InitAsConstants(ShadingConvention::Shadow::RootConstant::DrawZDepth::Count, 3);
		slotRootParameter[RootSignature::DrawZDepth::SI_Textures].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_DrawZDepth]),
			L"Shadow_GR_DrawZDepth"));
	}
	// DrawShadow
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[4]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::DrawShadow::Count]{};
		slotRootParameter[RootSignature::DrawShadow::CB_Light].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::DrawShadow::RC_Consts].InitAsConstants(ShadingConvention::Shadow::RootConstant::DrawShadow::Count, 1);
		slotRootParameter[RootSignature::DrawShadow::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::DrawShadow::SI_ZDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::DrawShadow::SI_ZDepthCubeMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::DrawShadow::UIO_ShadowMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_DrawShadow]),
			L"Shadow_GR_DrawShadow"));
	}

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildPipelineStates() {
	// DrawZDepth
	{
		const auto inputLayout = Foundation::Util::D3D12Util::InputLayoutDesc();
		auto psoDesc = Foundation::Util::D3D12Util::DefaultPsoDesc(inputLayout, ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_DrawZDepth].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_DrawZDepth]);
			NullCheck(mpLogFile, VS);
			const auto GS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::GS_DrawZDepth]);
			NullCheck(mpLogFile, GS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_DrawZDepth]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.GS = { reinterpret_cast<BYTE*>(GS->GetBufferPointer()), GS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 0;
		psoDesc.DSVFormat = ShadingConvention::Shadow::ZDepthMapFormat;
		psoDesc.RasterizerState.DepthBias = 10000;
		psoDesc.RasterizerState.SlopeScaledDepthBias = 1.f;
		psoDesc.RasterizerState.DepthBiasClamp = 0.f;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_DrawZDepth]),
			L"Shadow_GP_DrawZDepth"));
	}
	// DrawShadow
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_DrawShadow].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_DrawShadow]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_DrawShadow]),
			L"Shadow_CP_DrawShadow"));
	}

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	for (UINT i = 0; i < MaxLights; ++i) {
		mhZDepthMapCpuSrvs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhZDepthMapGpuSrvs[i] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhZDepthMapCpuDsvs[i] = pDescHeap->DsvCpuOffset(1);
	}
	
	mhShadowMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhShadowMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhShadowMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhShadowMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL Shadow::ShadowClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL Shadow::ShadowClass::Run(
		Foundation::Resource::FrameResource* const pFrameResource, 
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		const std::vector<Render::DX::Foundation::RenderItem*>& ritems) {
	for (UINT i = 0; i < mLightCount; ++i) {
		CheckReturn(mpLogFile, DrawZDepth(pFrameResource, ritems, i));
		CheckReturn(mpLogFile, DrawShadow(pFrameResource, pPositionMap, si_positionMap, ritems, i));
	}

	return TRUE;
}

BOOL Shadow::ShadowClass::AddLight(const std::shared_ptr<Foundation::Light>& light) {
	if (mLightCount >= MaxLights) ReturnFalse(mpLogFile, L"Can not add light due to the light count limit");

	const auto ShadowMapType = ResolveShadowMapType(light.get());

	BuildResource(ShadowMapType);
	BuildDescriptor(ShadowMapType);

	mLights[mLightCount++] = light;

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Width = mInitData.ClientWidth;
	rscDesc.Height = mInitData.ClientHeight;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_CLEAR_VALUE zdepthOptClear;
	zdepthOptClear.Format = ShadingConvention::Shadow::ZDepthMapFormat;
	zdepthOptClear.DepthStencil.Depth = 1.f;
	zdepthOptClear.DepthStencil.Stencil = 0;

	// ShadowMap
	{
		rscDesc.Format = ShadingConvention::Shadow::ShadowMapFormat;

		CheckReturn(mpLogFile, mShadowMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"Shadow_ShadowMap"));
	}
	
	return TRUE;
}

BOOL Shadow::ShadowClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	// ShadowMap
	{
		srvDesc.Format = ShadingConvention::Shadow::ShadowMapFormat;
		uavDesc.Format = ShadingConvention::Shadow::ShadowMapFormat;

		const auto shadowMap = mShadowMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, shadowMap, &srvDesc, mhShadowMapCpuSrv);
		Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, shadowMap, nullptr, &uavDesc, mhShadowMapCpuUav);
	}
	
	return TRUE;
}

BOOL Shadow::ShadowClass::BuildResource(ShadingConvention::Shadow::ShadowMap::Type type) {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Format = ShadingConvention::Shadow::ZDepthMapFormat;
	rscDesc.Width = mInitData.TexWidth;
	rscDesc.Height = mInitData.TexHeight;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE zdepthOptClear;
	zdepthOptClear.Format = ShadingConvention::Shadow::ZDepthMapFormat;
	zdepthOptClear.DepthStencil.Depth = 1.f;
	zdepthOptClear.DepthStencil.Stencil = 0;

	std::wstring name;
	if (type == ShadingConvention::Shadow::ShadowMap::E_CubeMap) {
		rscDesc.DepthOrArraySize = 6;
		name = std::format(L"Shadow_ZDepthCubeMap_{}", mLightCount);
	}
	else if (type == ShadingConvention::Shadow::ShadowMap::E_Texture2DArray) {
		rscDesc.DepthOrArraySize = ShadingConvention::Shadow::CascadeCount;
		name = std::format(L"Shadow_ZDepthArray_{}", mLightCount);
	}
	else {
		rscDesc.DepthOrArraySize = 1;		
		name = std::format(L"Shadow_ZDepthMap_{}", mLightCount);
	}

	CheckReturn(mpLogFile, mZDepthMaps[mLightCount]->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_DEPTH_READ,
		&zdepthOptClear,
		name.c_str()));

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildDescriptor(ShadingConvention::Shadow::ShadowMap::Type type) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = ShadingConvention::Shadow::ZDepthMapFormat;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	if (type == ShadingConvention::Shadow::ShadowMap::E_CubeMap) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.ArraySize = 6;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.f;
		
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.ArraySize = 6;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.MipSlice = 0;		
	}
	else if (type == ShadingConvention::Shadow::ShadowMap::E_Texture2DArray) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.ArraySize = ShadingConvention::Shadow::CascadeCount;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.f;

		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.ArraySize = ShadingConvention::Shadow::CascadeCount;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.MipSlice = 0;
	}
	else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		srvDesc.Texture2D.PlaneSlice = 0;

		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}

	const auto depthMap = mZDepthMaps[mLightCount]->Resource();
	Foundation::Util::D3D12Util::CreateShaderResourceView(
		mInitData.Device, depthMap, &srvDesc, mhZDepthMapCpuSrvs[mLightCount]);
	Foundation::Util::D3D12Util::CreateDepthStencilView(
		mInitData.Device, depthMap, &dsvDesc, mhZDepthMapCpuDsvs[mLightCount]);

	return TRUE;
}

BOOL Shadow::ShadowClass::DrawZDepth(
		Foundation::Resource::FrameResource* const pFrameResource,
		const std::vector<Render::DX::Foundation::RenderItem*>& ritems,
		UINT lightIndex) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::GP_DrawZDepth].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetPipelineState(mPipelineStates[PipelineState::GP_DrawZDepth].Get());
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_DrawZDepth].Get());

		CmdList->RSSetViewports(1, &mViewport);
		CmdList->RSSetScissorRects(1, &mScissorRect);
		
		mZDepthMaps[lightIndex]->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		const auto dsv = mhZDepthMapCpuDsvs[lightIndex];

		CmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		CmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

		CmdList->SetGraphicsRootConstantBufferView(
			RootSignature::DrawZDepth::CB_Light, pFrameResource->LightCB.CBAddress());

		ShadingConvention::Shadow::RootConstant::DrawZDepth::Struct rc;
		rc.gLightIndex = lightIndex;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::Shadow::RootConstant::DrawZDepth::Struct>(
			RootSignature::DrawZDepth::RC_Consts,
			ShadingConvention::Shadow::RootConstant::DrawZDepth::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		//CmdList->SetGraphicsRootDescriptorTable(RootSignature::DrawZDepth::SI_Textures, si_textures);

		CheckReturn(mpLogFile, DrawRenderItems(pFrameResource, CmdList, ritems));
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL Shadow::ShadowClass::DrawShadow(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		const std::vector<Render::DX::Foundation::RenderItem*>& ritems,
		UINT lightIndex) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_DrawShadow].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::GR_DrawShadow].Get());

		mShadowMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mShadowMap.get());

		mZDepthMaps[lightIndex]->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_READ);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->SetComputeRootConstantBufferView(
			RootSignature::DrawShadow::CB_Light, pFrameResource->LightCB.CBAddress());

		ShadingConvention::Shadow::RootConstant::DrawShadow::Struct rc;
		rc.gLightIndex = lightIndex;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::Shadow::RootConstant::DrawShadow::Struct>(
			RootSignature::DrawShadow::RC_Consts,
			ShadingConvention::Shadow::RootConstant::DrawShadow::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		CmdList->SetComputeRootDescriptorTable(
			RootSignature::DrawShadow::SI_PositionMap, si_positionMap);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::DrawShadow::UIO_ShadowMap, mhShadowMapGpuUav);

		const auto ShadowMapType = ResolveShadowMapType(mLights[lightIndex].get());

		if (ShadowMapType == ShadingConvention::Shadow::ShadowMap::E_CubeMap) 
			CmdList->SetComputeRootDescriptorTable(
				RootSignature::DrawShadow::SI_ZDepthCubeMap, 
				mhZDepthMapGpuSrvs[lightIndex]);
		else if (ShadowMapType == ShadingConvention::Shadow::ShadowMap::E_Texture2DArray)
			;
		else CmdList->SetComputeRootDescriptorTable(
			RootSignature::DrawShadow::SI_ZDepthMap, 
			mhZDepthMapGpuSrvs[lightIndex]);


		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				static_cast<UINT>(mInitData.ClientWidth), 
				ShadingConvention::Shadow::ThreadGroup::DrawShadow::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				static_cast<UINT>(mInitData.ClientHeight), 
				ShadingConvention::Shadow::ThreadGroup::DrawShadow::Height), 
			ShadingConvention::Shadow::ThreadGroup::DrawShadow::Depth);

		Foundation::Util::D3D12Util::UavBarrier(CmdList, mShadowMap.get());
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL Shadow::ShadowClass::DrawRenderItems(
		Foundation::Resource::FrameResource* const pFrameResource,
		ID3D12GraphicsCommandList6* const pCmdList,
		const std::vector<Render::DX::Foundation::RenderItem*>& ritems) {
	for (UINT i = 0; i < ritems.size(); ++i) {
		auto& ri = ritems[i];

		pCmdList->IASetVertexBuffers(0, 1, &ri->Geometry->VertexBufferView());
		pCmdList->IASetIndexBuffer(&ri->Geometry->IndexBufferView());
		pCmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		pCmdList->SetGraphicsRootConstantBufferView(
			RootSignature::DrawZDepth::CB_Object, 
			pFrameResource->ObjectCB.CBAddress(ri->ObjectCBIndex));

		pCmdList->SetGraphicsRootConstantBufferView(
			RootSignature::DrawZDepth::CB_Material, 
			pFrameResource->MaterialCB.CBAddress(ri->Material->MaterialCBIndex));

		pCmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	return TRUE;
}