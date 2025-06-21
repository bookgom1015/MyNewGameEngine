#include "Render/DX/Shading/RaySorting.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_CountingSort_Rays_64x128 = L"CountingSort_Rays_64x128.hlsl";
}

RaySorting::InitDataPtr RaySorting::MakeInitData() {
	return std::unique_ptr<RaySortingClass::InitData>(new RaySortingClass::InitData());
}

RaySorting::RaySortingClass::RaySortingClass() {
	mRayIndexOffsetMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT RaySorting::RaySortingClass::CbvSrvUavDescCount() const { return 0; }

UINT RaySorting::RaySortingClass::RtvDescCount() const { return 0; }

UINT RaySorting::RaySortingClass::DsvDescCount() const { return 0; }

BOOL RaySorting::RaySortingClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL RaySorting::RaySortingClass::CompileShaders() {
	const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_CountingSort_Rays_64x128, L"CS", L"cs_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_CountingSort]));

	return TRUE;
}

BOOL RaySorting::RaySortingClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[4] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::CB_RaySorting].InitAsConstantBufferView(0);
	slotRootParameter[RootSignature::Default::SI_NormalDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::UO_RayIndexOffsetMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"RaySorting_GR_Default"));

	return TRUE;
}

BOOL RaySorting::RaySortingClass::BuildPipelineStates() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	{
		const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_CountingSort]);
		NullCheck(mpLogFile, CS);
		psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
	}

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
		mInitData.Device,
		psoDesc,
		IID_PPV_ARGS(&mPipelineState),
		L"RaySorting_CP_Default"));

	return TRUE;
}

BOOL RaySorting::RaySortingClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhRayIndexOffsetMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhRayIndexOffsetMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhRayIndexOffsetMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhRayIndexOffsetMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL RaySorting::RaySortingClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL RaySorting::RaySortingClass::CalcRayIndexOffset(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineState.Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignature.Get());
		
		pNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		mRayIndexOffsetMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mRayIndexOffsetMap.get());

		CmdList->SetComputeRootConstantBufferView(RootSignature::Default::CB_RaySorting, pFrameResource->RaySortingCBAddress());	
		
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_NormalDepthMap, si_normalDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::UO_RayIndexOffsetMap, mhRayIndexOffsetMapGpuUav);
		
		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientWidth, ShadingConvention::RaySorting::RayGroup::Width),
			Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientHeight, ShadingConvention::RaySorting::RayGroup::Height),
			ShadingConvention::RaySorting::RayGroup::Depth);
		
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mRayIndexOffsetMap.get());
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL RaySorting::RaySortingClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
	texDesc.Width = mInitData.ClientWidth;
	texDesc.Height = mInitData.ClientHeight;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CheckReturn(mpLogFile, mRayIndexOffsetMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"RaySorting_RayIndexOffsetMap"));

	return TRUE;
}

BOOL RaySorting::RaySortingClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	const auto resource = mRayIndexOffsetMap->Resource();
	Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhRayIndexOffsetMapCpuSrv);
	Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhRayIndexOffsetMapCpuUav);

	return TRUE;
}