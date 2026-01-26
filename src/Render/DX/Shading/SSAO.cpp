#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/SSAO.hpp"
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
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace {
	const WCHAR* const HLSL_SSAO = L"SSAO.hlsl";
}

SSAO::InitDataPtr SSAO::MakeInitData() {
	return std::unique_ptr<SSAOClass::InitData>(new SSAOClass::InitData());
}

SSAO::SSAOClass::SSAOClass() {
	mRandomVectorMap = std::make_unique<Foundation::Resource::GpuResource>();
	mRandomVectorMapUploadBuffer = std::make_unique<Foundation::Resource::GpuResource>();

	for (UINT resource = 0; resource < Resource::AO::Count; ++resource)
		mAOResources[resource] = std::make_unique<Foundation::Resource::GpuResource>();

	for (UINT frame = 0; frame < 2; ++frame) {
		for (UINT resource = 0; resource < Resource::TemporalCache::Count; ++resource) {
			mTemporalCaches[frame][resource] = std::make_unique<Foundation::Resource::GpuResource>();
		}
		mTemporalAOResources[frame] = std::make_unique<Foundation::Resource::GpuResource>();
	}

	mDebugMap = std::make_unique<Foundation::Resource::GpuResource>();
}

SSAO::SSAOClass::~SSAOClass() { CleanUp(); }

UINT SSAO::SSAOClass::CbvSrvUavDescCount() const { return 0
	+ Descriptor::AO::Count
	+ Descriptor::TemporalCache::Count // Frame 0
	+ Descriptor::TemporalCache::Count // Frame 1
	+ Descriptor::TemporalAO::Count // Frame 0
	+ Descriptor::TemporalAO::Count // Frame 1
	+ 1 // RandomVectorMap
	+ 1 // DebugMapUav
	; 
}

UINT SSAO::SSAOClass::RtvDescCount() const { return 0; }

UINT SSAO::SSAOClass::DsvDescCount() const { return 0; }

BOOL SSAO::SSAOClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildRandomVectorMapResource());
	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildRandomVectorTexture());

	BuildOffsetVecotrs();

	return TRUE;
}

void SSAO::SSAOClass::CleanUp() {
	if (mbCleanedUp) return;

	if (mDebugMap) mDebugMap.reset();

	for (UINT i = 0; i < 2; ++i) {
		auto& resource = mTemporalAOResources[i];
		if (resource) resource.reset();
	}

	for (UINT i = 0; i < Resource::TemporalCache::Count; ++i) {
		for (UINT j = 0; j < 2; ++j) {
			auto& resource = mTemporalCaches[j][i];
			if (resource) resource.reset();
		}
	}

	for (UINT i = 0; i < Resource::AO::Count; ++i) {
		auto& resource = mAOResources[i];
		if (resource) resource.reset();
	}

	if (mRandomVectorMapUploadBuffer) mRandomVectorMapUploadBuffer.reset();
	if (mRandomVectorMap) mRandomVectorMap.reset();

	mPipelineState.Reset();
	mRootSignature.Reset();

	mbCleanedUp = TRUE;
}

BOOL SSAO::SSAOClass::CompileShaders() {
	const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_SSAO, L"CS", L"cs_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_SSAO]));

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[6]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count]{};
	slotRootParameter[RootSignature::Default::CB_AO].InitAsConstantBufferView(0);
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(
		ShadingConvention::SSAO::RootConstant::Default::Count, 1);
	slotRootParameter[RootSignature::Default::SI_NormalDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_RandomVectorMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::UO_AOCoefficientMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::UO_RayHitDistMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::UO_DebugMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"SSAO_GR_DrawAO"));

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildPipelineStates() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	{
		const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_SSAO]);
		NullCheck(mpLogFile, CS);
		psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
	}

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
		mInitData.Device,
		psoDesc,
		IID_PPV_ARGS(&mPipelineState),
		L"SSAO_CP_DrawAO"));
	
	return TRUE;
}

BOOL SSAO::SSAOClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhRandomVectorMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhRandomVectorMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	// AO
	{
		// AOCoefficient
		mhAOResourceCpus[Descriptor::AO::ES_AOCoefficient] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhAOResourceGpus[Descriptor::AO::ES_AOCoefficient] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhAOResourceCpus[Descriptor::AO::EU_AOCoefficient] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhAOResourceGpus[Descriptor::AO::EU_AOCoefficient] = pDescHeap->CbvSrvUavGpuOffset(1);
		// RayHitDistance
		mhAOResourceCpus[Descriptor::AO::ES_RayHitDistance] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhAOResourceGpus[Descriptor::AO::ES_RayHitDistance] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhAOResourceCpus[Descriptor::AO::EU_RayHitDistance] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhAOResourceGpus[Descriptor::AO::EU_RayHitDistance] = pDescHeap->CbvSrvUavGpuOffset(1);
	}
	// TemporalCache
	for (UINT frame = 0; frame < 2; ++frame) {
		// TSPP
		mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_TSPP] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalCacheGpus[frame][Descriptor::TemporalCache::ES_TSPP] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_TSPP] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalCacheGpus[frame][Descriptor::TemporalCache::EU_TSPP] = pDescHeap->CbvSrvUavGpuOffset(1);
		// RayHitDistance
		mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_RayHitDistance] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalCacheGpus[frame][Descriptor::TemporalCache::ES_RayHitDistance] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_RayHitDistance] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalCacheGpus[frame][Descriptor::TemporalCache::EU_RayHitDistance] = pDescHeap->CbvSrvUavGpuOffset(1);
		// AOCoefficientSquaredMean
		mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_AOCoefficientSquaredMean] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalCacheGpus[frame][Descriptor::TemporalCache::ES_AOCoefficientSquaredMean] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_AOCoefficientSquaredMean] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalCacheGpus[frame][Descriptor::TemporalCache::EU_AOCoefficientSquaredMean] = pDescHeap->CbvSrvUavGpuOffset(1);
		// TemporalAOResource
		mhTemporalAOResourceCpus[frame][Descriptor::TemporalAO::E_Srv] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalAOResourceGpus[frame][Descriptor::TemporalAO::E_Srv] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhTemporalAOResourceCpus[frame][Descriptor::TemporalAO::E_Uav] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalAOResourceGpus[frame][Descriptor::TemporalAO::E_Uav] = pDescHeap->CbvSrvUavGpuOffset(1);
	}

	mhDebugMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhDebugMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildRandomVectorMapDescriptor());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL SSAO::SSAOClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildRandomVectorTexture() {
	//
	// In order to copy CPU memory data into our default buffer,
	//  we need to create an intermediate upload heap. 
	//
	const UINT num2DSubresources = 1;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(mRandomVectorMap->Resource(), 0, num2DSubresources);

	CheckReturn(mpLogFile, mRandomVectorMapUploadBuffer->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr));

	std::vector<XMCOLOR> initData(256 * 256);
	for (INT i = 0; i < 256; ++i) {
		for (INT j = 0; j < 256; ++j) {
			// Random vector in [0,1].  We will decompress in shader to [-1,1].
			XMFLOAT3 v(Common::Util::MathUtil::RandF(), Common::Util::MathUtil::RandF(), Common::Util::MathUtil::RandF());

			initData[static_cast<INT64>(i) * 256 + static_cast<INT64>(j)] = XMCOLOR(v.x, v.y, v.z, 0.f);
		}
	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData.data();
	subResourceData.RowPitch = 256 * sizeof(XMCOLOR);
	subResourceData.SlicePitch = subResourceData.RowPitch * 256;

	{
		CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList());

		const auto CmdList = mInitData.CommandObject->DirectCommandList();

		{
			//
			// Schedule to copy the data to the default resource, and change states.
			// Note that mCurrSol is put in the GENERIC_READ state so it can be 
			// read by a shader.
			//
			mRandomVectorMapUploadBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
			mRandomVectorMap->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

			UpdateSubresources(
				CmdList,
				mRandomVectorMap->Resource(),
				mRandomVectorMapUploadBuffer->Resource(),
				0,
				0,
				num2DSubresources,
				&subResourceData);
		}

		CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());
	}

	return TRUE;
}

BOOL SSAO::SSAOClass::DrawAO(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pCurrNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_currNormalDepthMap,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineState.Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignature.Get());

		CmdList->SetComputeRootConstantBufferView(
			RootSignature::Default::CB_AO, 
			pFrameResource->AmbientOcclusionCB.CBAddress());

		ShadingConvention::SSAO::RootConstant::Default::Struct rc;
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(mInitData.ClientWidth);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(mInitData.ClientHeight);

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::SSAO::RootConstant::Default::Struct>(
			RootSignature::Default::RC_Consts,
			ShadingConvention::SSAO::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		const auto AOCoefficient = mAOResources[Resource::AO::E_AOCoefficient].get();
		AOCoefficient->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, AOCoefficient);

		const auto RayHitDistance = mAOResources[Resource::AO::E_RayHitDistance].get();
		RayHitDistance->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, RayHitDistance);

		pCurrNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mRandomVectorMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_NormalDepthMap, si_currNormalDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_PositionMap, si_positionMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_RandomVectorMap, mhRandomVectorMapGpuSrv);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::Default::UO_AOCoefficientMap, mhAOResourceGpus[Descriptor::AO::EU_AOCoefficient]);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::Default::UO_RayHitDistMap, mhAOResourceGpus[Descriptor::AO::EU_RayHitDistance]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::UO_DebugMap, mhDebugMapGpuUav);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::SSAO::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::SSAO::ThreadGroup::Default::Height),
			ShadingConvention::SSAO::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

UINT SSAO::SSAOClass::MoveToNextTemporalCacheFrame() {
	mCurrentTemporalCacheFrameIndex = (mCurrentTemporalCacheFrameIndex + 1) % 2;
	return mCurrentTemporalCacheFrameIndex;
}

UINT SSAO::SSAOClass::MoveToNextTemporalAOFrame() {
	mCurrentTemporalAOFrameIndex = (mCurrentTemporalAOFrameIndex + 1) % 2;
	return mCurrentTemporalAOFrameIndex;
}

BOOL SSAO::SSAOClass::BuildRandomVectorMapResource() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Format = ShadingConvention::SSAO::RandomVectorMapFormat;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CheckReturn(mpLogFile, mRandomVectorMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"SSAO_RandomVectorMap"));

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildRandomVectorMapDescriptor() {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = ShadingConvention::SSAO::RandomVectorMapFormat;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, mRandomVectorMap->Resource(), &srvDesc, mhRandomVectorMapCpuSrv);
	}

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Format = ShadingConvention::SSAO::AOCoefficientMapFormat;
	texDesc.Width = mInitData.ClientWidth;
	texDesc.Height = mInitData.ClientHeight;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// AO
	{
		// AOCoefficientMap
		{
			texDesc.Format = ShadingConvention::SSAO::AOCoefficientMapFormat;

			CheckReturn(mpLogFile, mAOResources[Resource::AO::E_AOCoefficient]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"SSAO_AOCoefficientMap"));
		}
		// RayHitDistanceMap
		{
			texDesc.Format = ShadingConvention::SVGF::RayHitDistanceMapFormat;

			CheckReturn(mpLogFile, mAOResources[Resource::AO::E_RayHitDistance]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"SSAO_RayHitDistanceMap"));
		}
	}
	for (UINT frame = 0; frame < 2; ++frame) {
		// TemporalCache
		{
			// TSPPMap
			{
				texDesc.Format = ShadingConvention::SVGF::TSPPMapFormat;

				std::wstringstream name;
				name << L"SSAO_TSPPMap_" << frame;

				CheckReturn(mpLogFile, mTemporalCaches[frame][Resource::TemporalCache::E_TSPP]->Initialize(
					mInitData.Device,
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					name.str().c_str()));
			}
			// TemporalRayHitDistanceMap
			{
				texDesc.Format = ShadingConvention::SVGF::RayHitDistanceMapFormat;

				std::wstringstream name;
				name << L"SSAO_TemporalRayHitDistanceMap_" << frame;

				CheckReturn(mpLogFile, mTemporalCaches[frame][Resource::TemporalCache::E_RayHitDistance]->Initialize(
					mInitData.Device,
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					name.str().c_str()));
			}
			// TemporalAOCoefficientSquaredMeanMap
			{
				texDesc.Format = ShadingConvention::SSAO::AOCoefficientSquaredMeanMapFormat;

				std::wstringstream name;
				name << L"SSAO_TemporalAOCoefficientSquaredMeanMap_" << frame;

				CheckReturn(mpLogFile, mTemporalCaches[frame][Resource::TemporalCache::E_AOCoefficientSquaredMean]->Initialize(
					mInitData.Device,
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					name.str().c_str()));
			}
		}
		// TemporalAOCoefficientMap
		{
			texDesc.Format = ShadingConvention::SSAO::AOCoefficientMapFormat;

			std::wstringstream name;
			name << L"SSAO_TemporalAOCoefficientMap_" << frame;

			CheckReturn(mpLogFile, mTemporalAOResources[frame]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				name.str().c_str()));
		}
	}
	// DebugMap
	{
		CheckReturn(mpLogFile, mDebugMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SSAO_DebugMap"));
	}

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	// AO
	{
		// AOCoefficientMap
		{
			srvDesc.Format = ShadingConvention::SSAO::AOCoefficientMapFormat;
			uavDesc.Format = ShadingConvention::SSAO::AOCoefficientMapFormat;
	
			const auto resource = mAOResources[Resource::AO::E_AOCoefficient]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhAOResourceCpus[Descriptor::AO::ES_AOCoefficient]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhAOResourceCpus[Descriptor::AO::EU_AOCoefficient]);
		}
		// RayHitDistanceMap
		{
			srvDesc.Format = ShadingConvention::SVGF::RayHitDistanceMapFormat;
			uavDesc.Format = ShadingConvention::SVGF::RayHitDistanceMapFormat;
	
			const auto resource = mAOResources[Resource::AO::E_RayHitDistance]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhAOResourceCpus[Descriptor::AO::ES_RayHitDistance]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhAOResourceCpus[Descriptor::AO::EU_RayHitDistance]);
		}
	}
	for (UINT frame = 0; frame < 2; ++frame) {
		// TemporalCache
		{
			// TSSPMap
			{
				srvDesc.Format = ShadingConvention::SVGF::TSPPMapFormat;
				uavDesc.Format = ShadingConvention::SVGF::TSPPMapFormat;
	
				const auto resource = mTemporalCaches[frame][Resource::TemporalCache::E_TSPP]->Resource();
				Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_TSPP]);
				Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_TSPP]);
			}
			// RayHitDistanceMap
			{
				srvDesc.Format = ShadingConvention::SVGF::RayHitDistanceMapFormat;
				uavDesc.Format = ShadingConvention::SVGF::RayHitDistanceMapFormat;
	
				const auto resource = mTemporalCaches[frame][Resource::TemporalCache::E_RayHitDistance]->Resource();
				Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_RayHitDistance]);
				Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_RayHitDistance]);
			}
			// AOCoefficientSquaredMeanMap
			{
				srvDesc.Format = ShadingConvention::SSAO::AOCoefficientSquaredMeanMapFormat;
				uavDesc.Format = ShadingConvention::SSAO::AOCoefficientSquaredMeanMapFormat;
	
				const auto resource = mTemporalCaches[frame][Resource::TemporalCache::E_AOCoefficientSquaredMean]->Resource();
				Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_AOCoefficientSquaredMean]);
				Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_AOCoefficientSquaredMean]);
			}
		}
		// TemporalAOCoefficientMap
		{
			srvDesc.Format = ShadingConvention::SSAO::AOCoefficientMapFormat;
			uavDesc.Format = ShadingConvention::SSAO::AOCoefficientMapFormat;
	
			const auto resource = mTemporalAOResources[frame]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhTemporalAOResourceCpus[frame][Descriptor::TemporalAO::E_Srv]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhTemporalAOResourceCpus[frame][Descriptor::TemporalAO::E_Uav]);
		}
	}
	// DebugMap
	{
		Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, mDebugMap->Resource(), nullptr, &uavDesc, mhDebugMapCpuUav);
	}

	return TRUE;
}

void SSAO::SSAOClass::BuildOffsetVecotrs() {
	// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
	// and the 6 center points along each cube face.  We always alternate the points on 
	// opposites sides of the cubes.  This way we still get the vectors spread out even
	// if we choose to use less than 14 samples.

	// 8 cube corners
	mOffsets[0] = XMFLOAT4(+1.f, +1.f, +1.f, 0.f);
	mOffsets[1] = XMFLOAT4(-1.f, -1.f, -1.f, 0.f);

	mOffsets[2] = XMFLOAT4(-1.f, +1.f, +1.f, 0.f);
	mOffsets[3] = XMFLOAT4(+1.f, -1.f, -1.f, 0.f);

	mOffsets[4] = XMFLOAT4(+1.f, +1.f, -1.f, 0.f);
	mOffsets[5] = XMFLOAT4(-1.f, -1.f, +1.f, 0.f);

	mOffsets[6] = XMFLOAT4(-1.f, +1.f, -1.f, 0.f);
	mOffsets[7] = XMFLOAT4(+1.f, -1.f, +1.f, 0.f);

	// 6 centers of cube faces
	mOffsets[8] = XMFLOAT4(-1.f, 0.f, 0.f, 0.f);
	mOffsets[9] = XMFLOAT4(+1.f, 0.f, 0.f, 0.f);

	mOffsets[10] = XMFLOAT4(0.f, -1.f, 0.f, 0.f);
	mOffsets[11] = XMFLOAT4(0.f, +1.f, 0.f, 0.f);

	mOffsets[12] = XMFLOAT4(0.f, 0.f, -1.f, 0.f);
	mOffsets[13] = XMFLOAT4(0.f, 0.f, +1.f, 0.f);

	for (UINT i = 0; i < 14; ++i) {
		// Create random lengths in [0.25, 1.0].
		const FLOAT s = Common::Util::MathUtil::RandF(0.25f, 1.f);
		const XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&mOffsets[i]));

		XMStoreFloat4(&mOffsets[i], v);
	}
}
