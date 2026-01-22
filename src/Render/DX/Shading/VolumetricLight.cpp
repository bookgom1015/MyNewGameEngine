#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/VolumetricLight.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/MathUtil.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_CalculateScatteringAndDensity = L"CalculateScatteringAndDensity.hlsl";
	const WCHAR* const HLSL_AccumulateSacttering = L"AccumulateSacttering.hlsl";
	const WCHAR* const HLSL_BlendScattering = L"BlendScattering.hlsl";
	const WCHAR* const HLSL_ApplyFog = L"ApplyFog.hlsl";
}

VolumetricLight::InitDataPtr VolumetricLight::MakeInitData() {
	return std::unique_ptr<VolumetricLightClass::InitData>(new VolumetricLightClass::InitData());
}

VolumetricLight::VolumetricLightClass::VolumetricLightClass() {
	for (UINT i = 0; i < 2; ++i) 
		mFrustumVolumeMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT VolumetricLight::VolumetricLightClass::CbvSrvUavDescCount() const { return 0 +
	+ Descriptor::FrustumVolumeMap::Count // For previous volume maps 
	+ Descriptor::FrustumVolumeMap::Count // For current volume maps
	; 
}

UINT VolumetricLight::VolumetricLightClass::RtvDescCount() const { return 0; }

UINT VolumetricLight::VolumetricLightClass::DsvDescCount() const { return 0; }

BOOL VolumetricLight::VolumetricLightClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

void VolumetricLight::VolumetricLightClass::CleanUp() {
	for (UINT i = 0; i < 2; ++i) {
		auto& resource = mFrustumVolumeMaps[i];
		if (resource) resource.reset();
	}

	for (UINT i = 0; i < PipelineState::Count; ++i)
		mPipelineStates[i].Reset();

	for (UINT i = 0; i < RootSignature::Count; ++i)
		mRootSignatures[i].Reset();
}

BOOL VolumetricLight::VolumetricLightClass::CompileShaders() {
	// CalculateScatteringAndDensity
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_CalculateScatteringAndDensity, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_CalculateScatteringAndDensity]));
	}
	// AccumulateScattering
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_AccumulateSacttering, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_AccumulateScattering]));
	}
	// BlendScattering
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_BlendScattering, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_BlendScattering]));
	}
	// ApplyFog
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyFog, L"VS", L"vs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ApplyFog]));

		// Default
		{
			const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyFog, L"PS", L"ps_6_5");
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ApplyFog]));
		}
		// Tricubic Sampling
		{
			DxcDefine defines[] = {
				{ L"TriCubicSampling", L"1" },
			};
			const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyFog, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ApplyFog_Tricubic]));
		}
	}

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// CalculateScatteringAndDensity
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[3]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MaxLights, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MaxLights, 0, 1);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::CalculateScatteringAndDensity::Count]{};
		slotRootParameter[RootSignature::CalculateScatteringAndDensity::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::CalculateScatteringAndDensity::CB_Light].InitAsConstantBufferView(1);
		slotRootParameter[RootSignature::CalculateScatteringAndDensity::RC_Consts].InitAsConstants(
			ShadingConvention::VolumetricLight::RootConstant::CalculateScatteringAndDensity::Count, 2);
		slotRootParameter[RootSignature::CalculateScatteringAndDensity::SI_ZDepthMaps].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::CalculateScatteringAndDensity::SI_ZDepthCubeMaps].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::CalculateScatteringAndDensity::UO_FrustumVolumeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_CalculateScatteringAndDensity]),
			L"VolumetricLight_GR_CalculateScatteringAndDensity"));
	}
	// AccumulateScattering
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::AccumulateScattering::Count]{};
		slotRootParameter[RootSignature::AccumulateScattering::RC_Consts].InitAsConstants(ShadingConvention::VolumetricLight::RootConstant::AccumulateScattering::Count, 0);
		slotRootParameter[RootSignature::AccumulateScattering::UIO_FrustumVolumeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_AccumulateScattering]),
			L"VolumetricLight_GR_AccumulateScattering"));
	}
	// BlendScattering
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::BlendScattering::Count]{};
		slotRootParameter[RootSignature::BlendScattering::SI_PreviousScattering].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::BlendScattering::UIO_CurrentScattering].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_BlendScattering]),
			L"VolumetricLight_GR_BlendScattering"));
	}
	// ApplyFog
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ApplyFog::Count]{};
		slotRootParameter[RootSignature::ApplyFog::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::ApplyFog::RC_Consts].InitAsConstants(ShadingConvention::VolumetricLight::RootConstant::ApplyFog::Count, 1);
		slotRootParameter[RootSignature::ApplyFog::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ApplyFog::SI_FrustumVolumeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ApplyFog]),
			L"VolumetricLight_GR_ApplyFog"));
	}

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::BuildPipelineStates() {
	// CalculateScatteringAndDensity
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_CalculateScatteringAndDensity].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_CalculateScatteringAndDensity]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_CalculateScatteringAndDensity]),
			L"VolumetricLight_CP_CalculateScatteringAndDensity"));
	}
	// AccumulateScattering
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_AccumulateScattering].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_AccumulateScattering]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}
		
		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_AccumulateScattering]),
			L"VolumetricLight_CP_AccumulateScattering"));
	}
	// BlendScattering
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_BlendScattering].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_BlendScattering]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_BlendScattering]),
			L"VolumetricLight_CP_BlendScattering"));
	}
	// ApplyFog
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ApplyFog].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ApplyFog]);
			NullCheck(mpLogFile, VS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = HDR_FORMAT;
		//psoDesc.RTVFormats[1] = DebugMapFormat;

		D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
		blendDesc.BlendEnable = TRUE;
		blendDesc.LogicOpEnable = FALSE;
		blendDesc.SrcBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.DestBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.SrcBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[0] = blendDesc;
		//psoDesc.BlendState.RenderTarget[1] = blendDesc;

		// Default
		{
			{
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ApplyFog]);
				NullCheck(mpLogFile, PS);
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ApplyFog]),
				L"VolumetricLight_GP_ApplyFog"));
		}
		// Tricubic Sampling
		{
			{
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ApplyFog_Tricubic]);
				NullCheck(mpLogFile, PS);
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ApplyFog_Tricubic]),
				L"VolumetricLight_GP_ApplyFog_Tricubic"));
		}
	}

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	for (UINT i = 0; i < 2; ++i) {
		mhFrustumVolumeMapCpus[Descriptor::E_Srv][i] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhFrustumVolumeMapGpus[Descriptor::E_Srv][i] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhFrustumVolumeMapCpus[Descriptor::E_Uav][i] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhFrustumVolumeMapGpus[Descriptor::E_Uav][i] = pDescHeap->CbvSrvUavGpuOffset(1);
	}

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::BuildFog(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* ppDepthMaps[],
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMaps,
		FLOAT nearZ, FLOAT farZ, FLOAT depth_exp,
		FLOAT uniformDensity, FLOAT densityScale, 
		FLOAT anisotropicCoeff,
		UINT numLights) {
	mCurrentFrame = ++mFrameCount % 2 != 0;
	mPreviousFrame = (mCurrentFrame + 1) % 2;

	CheckReturn(mpLogFile, CalculateScatteringAndDensity(
		pFrameResource, ppDepthMaps, si_depthMaps, 
		nearZ, farZ, depth_exp, uniformDensity, anisotropicCoeff, numLights));
	CheckReturn(mpLogFile, AccumulateScattering(
		pFrameResource, nearZ, farZ, depth_exp, densityScale));
	CheckReturn(mpLogFile, BlendScattering(pFrameResource));

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::ApplyFog(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
		FLOAT nearZ, FLOAT farZ, FLOAT depth_exp,
		BOOL tricubicSamplingEnabled) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[tricubicSamplingEnabled ? 
		PipelineState::GP_ApplyFog_Tricubic : PipelineState::GP_ApplyFog].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ApplyFog].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		CmdList->SetGraphicsRootConstantBufferView(
			RootSignature::ApplyFog::CB_Pass, pFrameResource->MainPassCB.CBAddress());
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::ApplyFog::SI_PositionMap, si_positionMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::ApplyFog::SI_FrustumVolumeMap, mhFrustumVolumeMapGpus[Descriptor::E_Srv][mCurrentFrame]);

		ShadingConvention::VolumetricLight::RootConstant::ApplyFog::Struct rc;
		rc.gNearZ = nearZ;
		rc.gFarZ = farZ;
		rc.gDepthExponent = depth_exp;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::VolumetricLight::RootConstant::ApplyFog::Struct>(
			RootSignature::ApplyFog::RC_Consts,
			ShadingConvention::VolumetricLight::RootConstant::ApplyFog::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(6, 1, 0, 0);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	texDesc.Format = ShadingConvention::VolumetricLight::FrustumVolumeMapFormat;
	texDesc.Width = mInitData.TextureWidth;
	texDesc.Height = mInitData.TextureHeight;
	texDesc.DepthOrArraySize = mInitData.TextureDepth;
	texDesc.Alignment = 0;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	for (size_t i = 0; i < 2; ++i) {
		std::wstring name(L"VolumetricLight_FrustumVolumeMap_");
		name.append(std::to_wstring(i));

		CheckReturn(mpLogFile, mFrustumVolumeMaps[i]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			name.c_str()));
	}

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Format = ShadingConvention::VolumetricLight::FrustumVolumeMapFormat;
	srvDesc.Texture3D.MostDetailedMip = 0;
	srvDesc.Texture3D.MipLevels = 1;
	srvDesc.Texture3D.ResourceMinLODClamp = 0.f;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Format = ShadingConvention::VolumetricLight::FrustumVolumeMapFormat;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.WSize = mInitData.TextureDepth;
	uavDesc.Texture3D.FirstWSlice = 0;

	for (UINT i = 0; i < 2; ++i) {
		const auto& resource = mFrustumVolumeMaps[i]->Resource();
		mInitData.Device->CreateShaderResourceView(resource, &srvDesc, mhFrustumVolumeMapCpus[Descriptor::E_Srv][i]);
		mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, mhFrustumVolumeMapCpus[Descriptor::E_Uav][i]);
	}

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::CalculateScatteringAndDensity(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* ppDepthMaps[],
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMaps,
		FLOAT nearZ, FLOAT farZ, FLOAT depth_exp,
		FLOAT uniformDensity, FLOAT anisotropicCoeff,
		UINT numLights) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_CalculateScatteringAndDensity].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);
	
	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::GR_CalculateScatteringAndDensity].Get());
	
		for (UINT i = 0; i < numLights; ++i) 
			ppDepthMaps[i]->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		mFrustumVolumeMaps[mPreviousFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		mFrustumVolumeMaps[mCurrentFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mFrustumVolumeMaps[mCurrentFrame].get());
	
		CmdList->SetComputeRootConstantBufferView(
			RootSignature::CalculateScatteringAndDensity::CB_Pass, 
			pFrameResource->MainPassCB.CBAddress());
		CmdList->SetComputeRootConstantBufferView(
			RootSignature::CalculateScatteringAndDensity::CB_Light, 
			pFrameResource->LightCB.CBAddress());
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::CalculateScatteringAndDensity::SI_ZDepthMaps, si_depthMaps);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::CalculateScatteringAndDensity::SI_ZDepthCubeMaps, si_depthMaps);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::CalculateScatteringAndDensity::UO_FrustumVolumeMap, mhFrustumVolumeMapGpus[Descriptor::E_Uav][mCurrentFrame]);
		
		ShadingConvention::VolumetricLight::RootConstant::CalculateScatteringAndDensity::Struct rc;
		rc.gNearZ = nearZ;
		rc.gFarZ = farZ;
		rc.gDepthExponent = depth_exp;
		rc.gAnisotropicCoefficient = anisotropicCoeff;
		rc.gUniformDensity = uniformDensity;
		rc.gFrameCount = mFrameCount;
		
		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::VolumetricLight::RootConstant::CalculateScatteringAndDensity::Struct>(
			RootSignature::CalculateScatteringAndDensity::RC_Consts,
			ShadingConvention::VolumetricLight::RootConstant::CalculateScatteringAndDensity::Count,
			&rc,
			0,
			CmdList,
			TRUE);
	
		CmdList->Dispatch(
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(
				mInitData.TextureWidth, 
				ShadingConvention::VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Width),
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(
				mInitData.TextureHeight, 
				ShadingConvention::VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Height),
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(
				mInitData.TextureDepth, 
				ShadingConvention::VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Depth));
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::AccumulateScattering(
		Foundation::Resource::FrameResource* const pFrameResource,
		FLOAT nearZ, FLOAT farZ, FLOAT depth_exp, FLOAT densityScale) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_AccumulateScattering].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);
	
	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::GR_AccumulateScattering].Get());

		mFrustumVolumeMaps[mCurrentFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mFrustumVolumeMaps[mCurrentFrame].get());

		ShadingConvention::VolumetricLight::RootConstant::AccumulateScattering::Struct rc;
		rc.gNearZ = nearZ;
		rc.gFarZ = farZ;
		rc.gDepthExponent = depth_exp;
		rc.gDensityScale = densityScale;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::VolumetricLight::RootConstant::AccumulateScattering::Struct>(
			RootSignature::AccumulateScattering::RC_Consts,
			ShadingConvention::VolumetricLight::RootConstant::AccumulateScattering::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		CmdList->SetComputeRootDescriptorTable(RootSignature::AccumulateScattering::UIO_FrustumVolumeMap, mhFrustumVolumeMapGpus[Descriptor::E_Uav][mCurrentFrame]);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				static_cast<UINT>(mInitData.TextureWidth), 
				ShadingConvention::VolumetricLight::ThreadGroup::AccumulateScattering::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				static_cast<UINT>(mInitData.TextureHeight), 
				ShadingConvention::VolumetricLight::ThreadGroup::AccumulateScattering::Height),
			ShadingConvention::VolumetricLight::ThreadGroup::AccumulateScattering::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL VolumetricLight::VolumetricLightClass::BlendScattering(Foundation::Resource::FrameResource* const pFrameResource) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_BlendScattering].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::GR_BlendScattering].Get());

		mFrustumVolumeMaps[mPreviousFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		mFrustumVolumeMaps[mCurrentFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mFrustumVolumeMaps[mCurrentFrame].get());

		CmdList->SetComputeRootDescriptorTable(
			RootSignature::BlendScattering::SI_PreviousScattering, 
			mhFrustumVolumeMapGpus[Descriptor::E_Srv][mPreviousFrame]);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::BlendScattering::UIO_CurrentScattering, 
			mhFrustumVolumeMapGpus[Descriptor::E_Uav][mCurrentFrame]);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(
				mInitData.TextureWidth,
				ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Width),
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(
				mInitData.TextureHeight,
				ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Height),
			Foundation::Util::D3D12Util::D3D12Util::CeilDivide(
				mInitData.TextureDepth,
				ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Depth));
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}