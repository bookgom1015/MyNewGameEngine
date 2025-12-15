#include "Render/DX/Shading/DOF.hpp"
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
	const WCHAR* const HLSL_CalcFocalDistance = L"CalcFocalDistance.hlsl";
}

DOF::InitDataPtr DOF::MakeInitData() {
	return std::unique_ptr<DOFClass::InitData>(new DOFClass::InitData());
}

DOF::DOFClass::DOFClass() {
	mFocalDistanceBuffer = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT DOF::DOFClass::CbvSrvUavDescCount() const { return 0 
	+ 2 // FocalDistancBuffer Srv Uav
	; 
}

UINT DOF::DOFClass::RtvDescCount() const { return 0; }

UINT DOF::DOFClass::DsvDescCount() const { return 0; }

BOOL DOF::DOFClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildFixedResources());

	return TRUE;
}

BOOL DOF::DOFClass::CompileShaders() {
	const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_CalcFocalDistance, L"CS", L"cs_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_CaclFocalDistance]));

	return TRUE;
}

BOOL DOF::DOFClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::CalcFocalDistance::Count] = {};
	slotRootParameter[RootSignature::CalcFocalDistance::CB_Pass].InitAsConstantBufferView(0);
	slotRootParameter[RootSignature::CalcFocalDistance::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::CalcFocalDistance::UO_FocalDistanceBuffer].InitAsUnorderedAccessView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_CalcFocalDistance]),
		L"DOF_GR_CalcFocalDistance"));

	return TRUE;
}

BOOL DOF::DOFClass::BuildPipelineStates() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_CalcFocalDistance].Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	{
		const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_CaclFocalDistance]);
		NullCheck(mpLogFile, CS);
		psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
	}

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
		mInitData.Device,
		psoDesc,
		IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_CalcFocalDistance]),
		L"DOF_CP_CalcFocalDistance"));

	return TRUE;
}

BOOL DOF::DOFClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhFocalDistanceBufferCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhFocalDistanceBufferGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhFocalDistanceBufferCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhFocalDistanceBufferGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());
	CheckReturn(mpLogFile, BuildFixedDescriptors());

	return TRUE;
}

BOOL DOF::DOFClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL DOF::DOFClass::BuildResources() {
	return TRUE;
}

BOOL DOF::DOFClass::BuildDescriptors() {


	return TRUE;
}

BOOL DOF::DOFClass::BuildFixedResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Format = DXGI_FORMAT_UNKNOWN;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	texDesc.Width = 1 * sizeof(float);
	texDesc.Height = 1;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CheckReturn(mpLogFile, mFocalDistanceBuffer->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"DOF_FocalDistanceBuffer"));

	return TRUE;
}

BOOL DOF::DOFClass::BuildFixedDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.NumElements = 1;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.StructureByteStride = sizeof(float);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.NumElements = 1;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.StructureByteStride = sizeof(float);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;


	const auto resource = mFocalDistanceBuffer->Resource();
	Foundation::Util::D3D12Util::CreateShaderResourceView(
		mInitData.Device, resource, &srvDesc, mhFocalDistanceBufferCpuSrv);
	Foundation::Util::D3D12Util::CreateUnorderedAccessView(
		mInitData.Device, resource, nullptr, &uavDesc, mhFocalDistanceBufferCpuUav);

	return TRUE;
}