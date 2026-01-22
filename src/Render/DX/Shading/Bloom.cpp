#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/Bloom.hpp"
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
	const WCHAR* const HLSL_ExtractHighlights = L"ExtractHighlights.hlsl";
	const WCHAR* const HLSL_BlendBloomWithDownSampled = L"BlendBloomWithDownSampled.hlsl";
	const WCHAR* const HLSL_ApplyBloom = L"ApplyBloom.hlsl";
}

Bloom::InitDataPtr Bloom::MakeInitData() {
	return std::unique_ptr<BloomClass::InitData>(new BloomClass::InitData());
}

Bloom::BloomClass::BloomClass() {
	for (UINT i = 0; i < Resource::Count; ++i) {
		mHighlightMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
		mBloomMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
	}
}

UINT Bloom::BloomClass::CbvSrvUavDescCount() const { return 0 +
	Resource::Count +	// HightlightMap Srvs
	Resource::Count +	// HightlightMap Uavs
	Resource::Count +	// BloomMap Srvs
	Resource::Count;	// BloomMap Uavs
}

UINT Bloom::BloomClass::RtvDescCount() const { return 0; }

UINT Bloom::BloomClass::DsvDescCount() const { return 0; }

BOOL Bloom::BloomClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

void Bloom::BloomClass::CleanUp() {
	for (auto& resource : mBloomMaps) 
		if (resource) resource.reset();

	for (auto& resource : mHighlightMaps)
		if (resource) resource.reset();

	for (UINT i = 0; i < PipelineState::Count; ++i)
		mPipelineStates[i].Reset();

	for (UINT i = 0; i < RootSignature::Count; ++i)
		mRootSignatures[i].Reset();
}

BOOL Bloom::BloomClass::CompileShaders() {
	// ExtractHighlights
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ExtractHighlights, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_ExtractHighlights]));
	}
	// BlendBloomWithDownSampled
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_BlendBloomWithDownSampled, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_BlendBloomWithDownSampled]));
	}
	// ApplyBloom
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"VS", L"vs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ApplyBloom]));
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"MS", L"ms_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_ApplyBloom]));
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ApplyBloom]));
	}

	return TRUE;
}

BOOL Bloom::BloomClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();
	// ExtractHighlights
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ExtractHighlights::Count]{};
		slotRootParameter[RootSignature::ExtractHighlights::RC_Consts].InitAsConstants(
			ShadingConvention::Bloom::RootConstant::ExtractHighlights::Count, 0);
		slotRootParameter[RootSignature::ExtractHighlights::UIO_HighlightMap].InitAsDescriptorTable(1, &texTables[index++]);
		
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ExtractHighlights]),
			L"Bloom_GR_ExtractHighlights"));
	}
	// BlendBloomWithDownSampled
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::BlendBloomWithDownSampled::Count]{};
		slotRootParameter[RootSignature::BlendBloomWithDownSampled::RC_Consts].InitAsConstants(
			ShadingConvention::Bloom::RootConstant::BlendBloomWithDownSampled::Count, 0);
		slotRootParameter[RootSignature::BlendBloomWithDownSampled::SI_LowerScaleMap].InitAsDescriptorTable(
			1, &texTables[index++]);
		slotRootParameter[RootSignature::BlendBloomWithDownSampled::UIO_HigherScaleMap].InitAsDescriptorTable(
			1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_BlendBloomWithDownSampled]),
			L"Bloom_GR_BlendBloomWithDownSampled");

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_BlendBloomWithDownSampled]),
			L"Bloom_GR_BlendBloomWithDownSampled"));
	}
	// ApplyBloom
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ApplyBloom::Count]{};
		slotRootParameter[RootSignature::ApplyBloom::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ApplyBloom::SI_BloomMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ApplyBloom]),
			L"Bloom_GR_ApplyBloom"));
	}

	return TRUE;
}

BOOL Bloom::BloomClass::BuildPipelineStates() {
	// ExtractHighlights
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ExtractHighlights].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_ExtractHighlights]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_ExtractHighlights]),
			L"Bloom_CP_ExtractHighlights"));
	}
	// BlendBloomWithDownSampled
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_BlendBloomWithDownSampled].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_BlendBloomWithDownSampled]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_BlendBloomWithDownSampled]),
			L"Bloom_CP_BlendBloomWithDownSampled"));
	}
	// ApplyBloom
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ApplyBloom].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_ApplyBloom]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ApplyBloom]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_ApplyBloom]),
				L"Bloom_MP_ApplyBloom"));
		}
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ApplyBloom].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ApplyBloom]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ApplyBloom]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ApplyBloom]),
				L"Bloom_GP_ApplyBloom"));
		}
	}

	return TRUE;
}

BOOL Bloom::BloomClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	for (UINT i = 0; i < Resource::Count; ++i) {
		// HighlightMap
		{
			mhHighlightMapCpuSrvs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
			mhHighlightMapGpuSrvs[i] = pDescHeap->CbvSrvUavGpuOffset(1);

			mhHighlightMapCpuUavs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
			mhHighlightMapGpuUavs[i] = pDescHeap->CbvSrvUavGpuOffset(1);
		}
		// BloomMap
		{
			mhBloomMapCpuSrvs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
			mhBloomMapGpuSrvs[i] = pDescHeap->CbvSrvUavGpuOffset(1);

			mhBloomMapCpuUavs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
			mhBloomMapGpuUavs[i] = pDescHeap->CbvSrvUavGpuOffset(1);
		}
	}

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL Bloom::BloomClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL Bloom::BloomClass::ExtractHighlights(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer,
		FLOAT threshold, FLOAT softknee,
		DownSampleFunc downSampleFunc) {
	const auto QuarterWidth = mInitData.ClientWidth >> 1;
	const auto QuarterHeight = mInitData.ClientHeight >> 1;

	CheckReturn(mpLogFile, downSampleFunc(
		pBackBuffer,
		si_backBuffer,
		mHighlightMaps[Resource::E_4thRes].get(),
		mhHighlightMapGpuUavs[Resource::E_4thRes],
		mInitData.ClientWidth,
		mInitData.ClientHeight,
		QuarterWidth,
		QuarterHeight,
		2));

	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_ExtractHighlights].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::GR_ExtractHighlights].Get());

		const auto QuaerterMap = mHighlightMaps[Resource::E_4thRes].get();

		QuaerterMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, QuaerterMap);

		ShadingConvention::Bloom::RootConstant::ExtractHighlights::Struct rc;
		rc.gThreshold = threshold;
		rc.gSoftKnee = softknee;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::Bloom::RootConstant::ExtractHighlights::Struct>(
			RootSignature::ExtractHighlights::RC_Consts,
			ShadingConvention::Bloom::RootConstant::ExtractHighlights::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		CmdList->SetComputeRootDescriptorTable(
			RootSignature::ExtractHighlights::UIO_HighlightMap, mhHighlightMapGpuUavs[Resource::E_4thRes]);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				QuarterWidth, ShadingConvention::Bloom::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				QuarterHeight, ShadingConvention::Bloom::ThreadGroup::Default::Height),
			ShadingConvention::Bloom::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL Bloom::BloomClass::BlurHighlights(
		Foundation::Resource::FrameResource* const pFrameResource, 
		DownSampleFunc downSampleFunc, 
		BlurFunc blurFunc) {
	CheckReturn(mpLogFile, DownSampling(downSampleFunc));
	CheckReturn(mpLogFile, UpSamplingWithBlur(pFrameResource, blurFunc))

	return TRUE;
}

BOOL Bloom::BloomClass::ApplyBloom(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ?
		PipelineState::MP_ApplyBloom : PipelineState::GP_ApplyBloom].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ApplyBloom].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		const auto BloomMap = mBloomMaps[0].get();
		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		BloomMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ApplyBloom::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ApplyBloom::SI_BloomMap, mhBloomMapGpuSrvs[0]);

		if (mInitData.MeshShaderSupported) {
			CmdList->DispatchMesh(1, 1, 1);
		}
		else {
			CmdList->IASetVertexBuffers(0, 0, nullptr);
			CmdList->IASetIndexBuffer(nullptr);
			CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CmdList->DrawInstanced(6, 1, 0, 0);
		}
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL Bloom::BloomClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	{
		texDesc.Format = ShadingConvention::Bloom::HighlightMapFormat;

		UINT texW = mInitData.ClientWidth >> 1;
		UINT texH = mInitData.ClientHeight >> 1;

		for (UINT i = 0; i < Resource::Count; ++i) {
			texDesc.Width = texW;
			texDesc.Height = texH;

			// HighlightMap
			{
				std::wstringstream name;

				name << L"Bloom_HighlightMap_" << i;

				CheckReturn(mpLogFile, mHighlightMaps[i]->Initialize(
					mInitData.Device,
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					name.str().c_str()));
			}
			// BloomMap
			{
				std::wstringstream name;

				name << L"Bloom_BloomMap_" << i;

				CheckReturn(mpLogFile, mBloomMaps[i]->Initialize(
					mInitData.Device,
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					name.str().c_str()));
			}

			texW = texW >> 1;
			texH = texH >> 1;
		}
	}

	return TRUE;
}

BOOL Bloom::BloomClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	{
		srvDesc.Format = ShadingConvention::Bloom::HighlightMapFormat;
		uavDesc.Format = ShadingConvention::Bloom::HighlightMapFormat;

		for (UINT i = 0; i < Resource::Count; ++i) {
			// HighlightMap
			{
				const auto resource = mHighlightMaps[i]->Resource();

				Foundation::Util::D3D12Util::CreateShaderResourceView(
					mInitData.Device, resource, &srvDesc, mhHighlightMapCpuSrvs[i]);
				Foundation::Util::D3D12Util::CreateUnorderedAccessView(
					mInitData.Device, resource, nullptr, &uavDesc, mhHighlightMapCpuUavs[i]);
			}
			// BloomMap
			{
				const auto resource = mBloomMaps[i]->Resource();

				Foundation::Util::D3D12Util::CreateShaderResourceView(
					mInitData.Device, resource, &srvDesc, mhBloomMapCpuSrvs[i]);
				Foundation::Util::D3D12Util::CreateUnorderedAccessView(
					mInitData.Device, resource, nullptr, &uavDesc, mhBloomMapCpuUavs[i]);
			}
		}
	}

	return TRUE;
}

BOOL Bloom::BloomClass::DownSampling(DownSampleFunc downSampleFunc) {
	auto srcIndex = Resource::E_4thRes;
	auto dstIndex = Resource::E_16thRes;

	UINT srcTexDimX = mInitData.ClientWidth;
	UINT srcTexDimY = mInitData.ClientHeight;
	UINT dstTexDimX = mInitData.ClientWidth >> 1;
	UINT dstTexDimY = mInitData.ClientHeight >> 1;

	for (UINT i = 1; i < Resource::Count; ++i) {
		CheckReturn(mpLogFile, downSampleFunc(
			mHighlightMaps[srcIndex].get(),
			mhHighlightMapGpuSrvs[srcIndex],
			mHighlightMaps[dstIndex].get(),
			mhHighlightMapGpuUavs[dstIndex],
			srcTexDimX,
			srcTexDimY,
			dstTexDimX,
			dstTexDimY,
			3));

		srcIndex = static_cast<Resource::Type>(static_cast<INT>(srcIndex) + 1);
		dstIndex = static_cast<Resource::Type>(static_cast<INT>(dstIndex) + 1);

		srcTexDimX = srcTexDimX >> 1;
		srcTexDimY = srcTexDimY >> 1;
		dstTexDimX = dstTexDimX >> 1;
		dstTexDimY = dstTexDimY >> 1;
	}

	return TRUE;
}

BOOL Bloom::BloomClass::UpSamplingWithBlur(
		Foundation::Resource::FrameResource* const pFrameResource,
		BlurFunc blurFunc) {
	CheckReturn(mpLogFile, blurFunc(
		mHighlightMaps[Resource::E_256thRes].get(),
		mhHighlightMapGpuSrvs[Resource::E_256thRes],
		mBloomMaps[Resource::E_256thRes].get(),
		mhBloomMapGpuUavs[Resource::E_256thRes],
		mInitData.ClientWidth >> 4,
		mInitData.ClientHeight >> 4));

	auto highSampIndex = Resource::E_64thRes;
	auto lowSampIndex = Resource::E_256thRes;

	auto texWidth = mInitData.ClientWidth >> 3;
	auto texHeight = mInitData.ClientHeight >> 3;

	for (UINT i = 0, end = Resource::Count - 1; i < end; ++i) {
		{
			CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
				pFrameResource->CommandAllocator(0),
				0,
				mPipelineStates[PipelineState::CP_BlendBloomWithDownSampled].Get()));

			const auto CmdList = mInitData.CommandObject->CommandList(0);
			mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

			{
				CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::GR_BlendBloomWithDownSampled].Get());

				const auto HighSampMap = mHighlightMaps[highSampIndex].get();
				const auto LowSampMap = mBloomMaps[lowSampIndex].get();

				LowSampMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

				HighSampMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				Foundation::Util::D3D12Util::UavBarrier(CmdList, HighSampMap);

				ShadingConvention::Bloom::RootConstant::BlendBloomWithDownSampled::Struct rc;
				rc.gInvTexDim = { static_cast<FLOAT>(1.f / texWidth), static_cast<FLOAT>(1.f / texHeight) };

				Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::Bloom::RootConstant::BlendBloomWithDownSampled::Struct>(
					RootSignature::BlendBloomWithDownSampled::RC_Consts,
					ShadingConvention::Bloom::RootConstant::BlendBloomWithDownSampled::Count,
					&rc,
					0,
					CmdList,
					TRUE);

				CmdList->SetComputeRootDescriptorTable(
					RootSignature::BlendBloomWithDownSampled::SI_LowerScaleMap, mhBloomMapGpuSrvs[lowSampIndex]);
				CmdList->SetComputeRootDescriptorTable(
					RootSignature::BlendBloomWithDownSampled::UIO_HigherScaleMap, mhHighlightMapGpuUavs[highSampIndex																									]);

				CmdList->Dispatch(
					Foundation::Util::D3D12Util::CeilDivide(
						texWidth, ShadingConvention::Bloom::ThreadGroup::Default::Width),
					Foundation::Util::D3D12Util::CeilDivide(
						texHeight, ShadingConvention::Bloom::ThreadGroup::Default::Height),
					ShadingConvention::Bloom::ThreadGroup::Default::Depth);
			}

			CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));
		}

		CheckReturn(mpLogFile, blurFunc(
			mHighlightMaps[highSampIndex].get(),
			mhHighlightMapGpuSrvs[highSampIndex],
			mBloomMaps[highSampIndex].get(),
			mhBloomMapGpuUavs[highSampIndex],
			texWidth, texHeight));

		highSampIndex = static_cast<Resource::Type>(std::max<UINT>(static_cast<UINT>(highSampIndex) - 1, 0));
		lowSampIndex = static_cast<Resource::Type>(std::max<UINT>(static_cast<UINT>(lowSampIndex) - 1, 0));

		texWidth = texWidth << 1;
		texHeight = texHeight << 1;
	}

	return TRUE;
}