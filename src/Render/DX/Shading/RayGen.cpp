#include "Render/DX/Shading/RayGen.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Sampler/Sampler.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"

using namespace Render::DX::Shading;
using namespace DirectX;

namespace {
	const UINT gcNumSampleSets = 83;

	const WCHAR* const HLSL_RayGen = L"RayGen.hlsl";
}

RayGen::InitDataPtr RayGen::MakeInitData() {
	return std::unique_ptr<RayGenClass::InitData>(new RayGenClass::InitData());
}

RayGen::RayGenClass::RayGenClass() {
	mRandomSampler = std::make_unique<Common::Foundation::MultiJittered>();
	mRayDirectionOriginDepthMap = std::make_unique<Render::DX::Foundation::Resource::GpuResource>();
}

UINT RayGen::RayGenClass::CbvSrvUavDescCount() const { return 1; }

UINT RayGen::RayGenClass::RtvDescCount() const { return 0; }

UINT RayGen::RayGenClass::DsvDescCount() const { return 0; }

BOOL RayGen::RayGenClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	const UINT MaxPixelsInSampleSet1D = mInitData.MaxSampleSetDistributedAcrossPixels;
	const UINT MaxSamplesPerSet = mInitData.MaxSamplesPerPixel * MaxPixelsInSampleSet1D * MaxPixelsInSampleSet1D;

	CheckReturn(mpLogFile, mSamplesGPUBuffer.Initialize(
		mpLogFile, 
		mInitData.Device, 
		MaxSamplesPerSet * gcNumSampleSets, 
		Foundation::Core::SwapChain::SwapChainBufferCount, 
		FALSE,
		L"RayGen_SB_SamplesGpuBuffer"));
	CheckReturn(mpLogFile, mHemisphereSamplesGPUBuffer.Initialize(
		mpLogFile, 
		mInitData.Device, 
		MaxSamplesPerSet * gcNumSampleSets, 
		Foundation::Core::SwapChain::SwapChainBufferCount, 
		FALSE,
		L"RayGen_SB_HemisamplesGpuBuffer"));

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL RayGen::RayGenClass::CompileShaders() {
	const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_RayGen, L"CS", L"cs_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_RayGen]));

	return TRUE;
}

BOOL RayGen::RayGenClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[3] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::CB_RayGen].InitAsConstantBufferView(0);
	slotRootParameter[RootSignature::Default::SB_SampleSets].InitAsShaderResourceView(0);
	slotRootParameter[RootSignature::Default::SI_NormalDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::UO_RayDirectionOriginDepthMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"RayGen_GR_Default"));

	return TRUE;
}

BOOL RayGen::RayGenClass::BuildPipelineStates() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	{
		const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_RayGen]);
		NullCheck(mpLogFile, CS);
		psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
	}

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
		mInitData.Device,
		psoDesc,
		IID_PPV_ARGS(&mPipelineState),
		L"RayGen_CP_Default"));

	return TRUE;
}

BOOL RayGen::RayGenClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhRayDirectionOriginDepthMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhRayDirectionOriginDepthMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhRayDirectionOriginDepthMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhRayDirectionOriginDepthMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL RayGen::RayGenClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL RayGen::RayGenClass::Update() {
	BuildSamples();

	return TRUE;
}

UINT Render::DX::Shading::RayGen::RayGenClass::NumSampleSets() const {
	return mRandomSampler->NumSampleSets();
}

UINT Render::DX::Shading::RayGen::RayGenClass::NumSamples() const {
	return mRandomSampler->NumSamples();
}

BOOL RayGen::RayGenClass::GenerateRays(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		UINT currFrameResourceIndex) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineState.Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignature.Get());
	
		CmdList->SetComputeRootConstantBufferView(RootSignature::Default::CB_RayGen, pFrameResource->RayGenCBAddress());
	
		pNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
		mRayDirectionOriginDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mRayDirectionOriginDepthMap.get());
	
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_NormalDepthMap, si_normalDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_PositionMap, si_positionMap);
		CmdList->SetComputeRootShaderResourceView(RootSignature::Default::SB_SampleSets, mSamplesGPUBuffer.GpuVirtualAddress(0, currFrameResourceIndex));
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::UO_RayDirectionOriginDepthMap, mhRayDirectionOriginDepthMapGpuUav);
	
		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::RayGen::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::RayGen::ThreadGroup::Default::Height),
			ShadingConvention::RayGen::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));


	return TRUE;
}

void RayGen::RayGenClass::BuildSamples() {
	static UINT prevSampleSetDistributedAcrossPixels = UINT_MAX;
	static UINT prevSamplesPerPixel = UINT_MAX;

	UINT currSampleSetDistributedAcrossPixels = *mInitData.SampleSetDistributedAcrossPixels;
	UINT currSamplesPerPixel = *mInitData.SamplesPerPixel;

	if (prevSampleSetDistributedAcrossPixels != currSampleSetDistributedAcrossPixels || prevSamplesPerPixel != currSamplesPerPixel) {
		prevSampleSetDistributedAcrossPixels = currSampleSetDistributedAcrossPixels;
		prevSamplesPerPixel= currSamplesPerPixel;
	}
	else {
		return;
	}

	const UINT PixelsInSampleSet1D = currSampleSetDistributedAcrossPixels;
	const UINT SamplesPerSet = currSamplesPerPixel * PixelsInSampleSet1D * PixelsInSampleSet1D;
	mRandomSampler->Reset(SamplesPerSet, gcNumSampleSets, Common::Foundation::HemisphereDistribution::E_Cosine);

	const UINT NumSamples = mRandomSampler->NumSamples() * mRandomSampler->NumSampleSets();
	for (UINT i = 0; i < NumSamples; i++) {
		const XMFLOAT3 P = mRandomSampler->GetHemisphereSample3D();
		// Convert [-1,1] to [0,1].
		mSamplesGPUBuffer[i].Value = XMFLOAT2(P.x * 0.5f + 0.5f, P.y * 0.5f + 0.5f);
		mHemisphereSamplesGPUBuffer[i].Value = P;
	}
}

BOOL RayGen::RayGenClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mInitData.ClientWidth;
	texDesc.Height = mInitData.ClientHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// RayDirectionOriginDepthMap
	{
		texDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;

		CheckReturn(mpLogFile, mRayDirectionOriginDepthMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"RayGen_RayDirectionOriginDepthMap"));
	}

	return TRUE;
}

BOOL RayGen::RayGenClass::BuildDescriptors() {
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

	// RayDirectionOriginDepthMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
		uavDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;

		const auto resource = mRayDirectionOriginDepthMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhRayDirectionOriginDepthMapCpuSrv);
		Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhRayDirectionOriginDepthMapCpuUav);
	}

	return TRUE;
}
