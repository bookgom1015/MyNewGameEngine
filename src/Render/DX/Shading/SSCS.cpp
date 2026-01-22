#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/SSCS.hpp"
#include "Common/Debug/Logger.hpp"
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
	const WCHAR* const HLSL_ComputeContactShadow = L"ComputeContactShadow.hlsl";
	const WCHAR* const HLSL_ApplyContactShadow = L"ApplyContactShadow.hlsl";
}

SSCS::InitDataPtr SSCS::MakeInitData() {
	return std::unique_ptr<SSCSClass::InitData>(new SSCSClass::InitData());
}

SSCS::SSCSClass::SSCSClass() {
	mDebugMap = std::make_unique<Foundation::Resource::GpuResource>();
	mContactShadowMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT SSCS::SSCSClass::CbvSrvUavDescCount() const { return 3; }

UINT SSCS::SSCSClass::RtvDescCount() const { return 0; }

UINT SSCS::SSCSClass::DsvDescCount() const { return 0; }

BOOL SSCS::SSCSClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

void SSCS::SSCSClass::CleanUp() {
	if (mContactShadowMap) mContactShadowMap.reset();
	if (mDebugMap) mDebugMap.reset();

	for (UINT i = 0; i < PipelineState::Count; ++i)
		mPipelineStates[i].Reset();

	for (UINT i = 0; i < RootSignature::Count; ++i)
		mRootSignatures[i].Reset();
}

BOOL SSCS::SSCSClass::CompileShaders() {
	// ComputeContactShadow
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ComputeContactShadow, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_ComputeContactShadow]));
	}
	// ApplyContactShadow
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyContactShadow, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_ApplyContactShadow]));
	}

	return TRUE;
}

BOOL SSCS::SSCSClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// ComputeContactShadow
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[5]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ComputeContactShadow::Count]{};
		slotRootParameter[RootSignature::ComputeContactShadow::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::ComputeContactShadow::CB_Light]
			.InitAsConstantBufferView(1);
		slotRootParameter[RootSignature::ComputeContactShadow::CB_SSCS]
			.InitAsConstantBufferView(2);
		slotRootParameter[RootSignature::ComputeContactShadow::SI_PositionMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeContactShadow::SI_NormalMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeContactShadow::SI_DepthMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeContactShadow::UO_ContactShadowMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeContactShadow::UO_DebugMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ComputeContactShadow]),
			L"SSCS_GR_ComputeContactShadow"));
	}
	// ApplyContactShadow
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ApplyContactShadow::Count]{};
		slotRootParameter[RootSignature::ApplyContactShadow::UI_ContactShadowMap].InitAsDescriptorTable(
			1, &texTables[index++]);
		slotRootParameter[RootSignature::ApplyContactShadow::UIO_ShadowMap].InitAsDescriptorTable(
			1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ApplyContactShadow]),
			L"SSCS_GR_ApplyContactShadow"));
	}

	return TRUE;
}

BOOL SSCS::SSCSClass::BuildPipelineStates() {
	// ComputeContactShadow
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ComputeContactShadow].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_ComputeContactShadow]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_ComputeContactShadow]),
			L"SSCS_CP_ComputeContactShadow"));
	}
	// ApplyContactShadow
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ApplyContactShadow].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_ApplyContactShadow]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_ApplyContactShadow]),
			L"SSCS_CP_ApplyContactShadow"));
	}

	return TRUE;
}

BOOL SSCS::SSCSClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhDebugMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhDebugMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	mhContactShadowMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhContactShadowMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);

	mhContactShadowMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhContactShadowMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL SSCS::SSCSClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL SSCS::SSCSClass::ComputeContactShadow(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		Foundation::Resource::GpuResource* const pNormalMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		Foundation::Resource::GpuResource* const pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_ComputeContactShadow].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[RootSignature::GR_ComputeContactShadow].Get());

		CmdList->SetComputeRootConstantBufferView(
			RootSignature::ComputeContactShadow::CB_Pass, 
			pFrameResource->MainPassCB.CBAddress());
		CmdList->SetComputeRootConstantBufferView(
			RootSignature::ComputeContactShadow::CB_Light, 
			pFrameResource->LightCB.CBAddress());
		CmdList->SetComputeRootConstantBufferView(
			RootSignature::ComputeContactShadow::CB_SSCS,
			pFrameResource->ContactShadowCB.CBAddress());

		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_READ);

		mContactShadowMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mContactShadowMap.get());

		mDebugMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mDebugMap.get());

		CmdList->SetComputeRootDescriptorTable(
			RootSignature::ComputeContactShadow::SI_PositionMap, si_positionMap);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::ComputeContactShadow::SI_NormalMap, si_normalMap);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::ComputeContactShadow::SI_DepthMap, si_depthMap);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::ComputeContactShadow::UO_ContactShadowMap, mhContactShadowMapGpuUav);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::ComputeContactShadow::UO_DebugMap, mhDebugMapGpuUav);
		
		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientWidth, ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientHeight, ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Height),
			ShadingConvention::SSCS::ThreadGroup::ComputeContactShadow::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SSCS::SSCSClass::ApplyContactShadow(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pShadowMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uio_shadowMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_ApplyContactShadow].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::GR_ApplyContactShadow].Get());

		pShadowMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pShadowMap);

		mContactShadowMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mContactShadowMap.get());

		CmdList->SetComputeRootDescriptorTable(RootSignature::ApplyContactShadow::UI_ContactShadowMap, mhContactShadowMapGpuUav);
		CmdList->SetComputeRootDescriptorTable(RootSignature::ApplyContactShadow::UIO_ShadowMap, uio_shadowMap);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientWidth, ShadingConvention::SSCS::ThreadGroup::ApplyContactShadow::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientHeight, ShadingConvention::SSCS::ThreadGroup::ApplyContactShadow::Height),
			ShadingConvention::SSCS::ThreadGroup::ApplyContactShadow::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SSCS::SSCSClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = mInitData.ClientWidth;
	texDesc.Height = mInitData.ClientHeight;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// DebugMap
	{
		texDesc.Format = ShadingConvention::SSCS::DebugMapFormat;

		CheckReturn(mpLogFile, mDebugMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SSCS_DebugMap"));
	}
	// ContactShadowMap
	{
		texDesc.Format = ShadingConvention::SSCS::ContactShadowMapFormat;

		CheckReturn(mpLogFile, mContactShadowMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SSCS_ContactShadowMap"));
	}

	return TRUE;
}

BOOL SSCS::SSCSClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;	

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;	

	// DebugMap
	{
		uavDesc.Format = ShadingConvention::SSCS::DebugMapFormat;

		const auto resource = mDebugMap->Resource();

		Foundation::Util::D3D12Util::CreateUnorderedAccessView(
			mInitData.Device, resource, nullptr, &uavDesc, mhDebugMapCpuUav);
	}
	// ContactShadowMap
	{
		srvDesc.Format = ShadingConvention::SSCS::ContactShadowMapFormat;
		uavDesc.Format = ShadingConvention::SSCS::ContactShadowMapFormat;

		const auto resource = mContactShadowMap->Resource();

		Foundation::Util::D3D12Util::CreateShaderResourceView(
			mInitData.Device, resource, &srvDesc, mhContactShadowMapCpuSrv);
		Foundation::Util::D3D12Util::CreateUnorderedAccessView(
			mInitData.Device, resource, nullptr, &uavDesc, mhContactShadowMapCpuUav);
	}

	return TRUE;
}