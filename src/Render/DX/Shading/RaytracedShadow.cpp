#include "Render/DX/Shading/RaytracedShadow.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"
#include "Render/DX/Shading/Util/ShaderTable.hpp"

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_RaytracedShadow = L"RaytracedShadow.hlsl";

	const WCHAR* const RaytracedShadow_RayGenName = L"RaytracedShadow_RayGen";
	const WCHAR* const RaytracedShadow_ClosestHitName = L"RaytracedShadow_ClosestHit";
	const WCHAR* const RaytracedShadow_MissName = L"RaytracedShadow_Miss";
	const WCHAR* const RaytracedShadow_HitGroupName = L"RaytracedShadow_HitGroup";
}

RaytracedShadow::InitDataPtr RaytracedShadow::MakeInitData() {
	return std::unique_ptr<RaytracedShadowClass::InitData>(new RaytracedShadowClass::InitData());
}

RaytracedShadow::RaytracedShadowClass::RaytracedShadowClass() {
	mShadowMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT RaytracedShadow::RaytracedShadowClass::CbvSrvUavDescCount() const { return 2; }

UINT RaytracedShadow::RaytracedShadowClass::RtvDescCount() const { return 0; }

UINT RaytracedShadow::RaytracedShadowClass::DsvDescCount() const { return 0; }

BOOL RaytracedShadow::RaytracedShadowClass::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::CompileShaders() {
	const auto Lib = Util::ShaderManager::D3D12ShaderInfo(
		HLSL_RaytracedShadow, L"", L"lib_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
		Lib, mShaderHashes[Shader::Lib_RaytracedShadow]));

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[4] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Count] = {};
	slotRootParameter[RootSignature::CB_Light].
		InitAsConstantBufferView(0);
	slotRootParameter[RootSignature::SI_AccelerationStructure].
		InitAsShaderResourceView(0);
	slotRootParameter[RootSignature::SI_PositionMap].
		InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::SI_NormalMap].
		InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::SI_DepthMap].
		InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::UO_ShadowMap].
		InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
		_countof(slotRootParameter), slotRootParameter,
		Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_NONE
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSignatureDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"RaytracedShadow_GR_Default"));

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::BuildPipelineStates() {
	if (!mInitData.RaytracingSupported) return TRUE;

	CD3DX12_STATE_OBJECT_DESC stateObject = { 
		D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	// RaytraceShadow-Library
	const auto library = stateObject.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	const auto shader = mInitData.ShaderManager->
		GetShader(mShaderHashes[Shader::Lib_RaytracedShadow]);
	const D3D12_SHADER_BYTECODE rtaoLibDxil = CD3DX12_SHADER_BYTECODE(
		shader->GetBufferPointer(), shader->GetBufferSize());
	library->SetDXILLibrary(&rtaoLibDxil);
	LPCWSTR exports[] = { 
		RaytracedShadow_RayGenName, 
		RaytracedShadow_ClosestHitName, 
		RaytracedShadow_MissName };
	library->DefineExports(exports);

	// RaytraceShadow-HitGroup
	const auto hitGroup = stateObject.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	hitGroup->SetClosestHitShaderImport(RaytracedShadow_ClosestHitName);
	hitGroup->SetHitGroupExport(RaytracedShadow_HitGroupName);
	hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// ShaderConfig
	const auto shaderConfig = stateObject.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	const UINT payloadSize = 4; // tHit(FLOAT)
	const UINT attribSize = sizeof(DirectX::XMFLOAT2);
	shaderConfig->Config(payloadSize, attribSize);

	// Global-RootSignature
	const auto glbalRootSig = stateObject.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	glbalRootSig->SetRootSignature(mRootSignature.Get());

	// Pipeline-Configuration
	const auto pipelineConfig = stateObject.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	const UINT maxRecursionDepth = 1;
	pipelineConfig->Config(maxRecursionDepth);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateStateObject(
		mInitData.Device, stateObject, IID_PPV_ARGS(&mStateObject)));
	CheckHRESULT(mpLogFile, mStateObject->QueryInterface(
		IID_PPV_ARGS(&mStateObjectProp)));

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::BuildDescriptors(
		Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhShadowMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhShadowMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhShadowMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhShadowMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::BuildShaderTables(UINT numRitems) {
#ifdef _DEBUG
	// A shader name look-up table for shader table debug print out.
	std::unordered_map<void*, std::wstring> shaderIdToStringMap;
#endif

	const UINT ShaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	{
		// RayGenShaderTable
		{
			void* const rayGenShaderIdentifier = mStateObjectProp->
				GetShaderIdentifier(RaytracedShadow_RayGenName);

			Util::ShaderTable rayGenShaderTable(
				mpLogFile, mInitData.Device, 2, ShaderIdentifierSize);
			CheckReturn(mpLogFile, rayGenShaderTable.Initialze());
			rayGenShaderTable.push_back(Util::ShaderRecord(
				rayGenShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[rayGenShaderIdentifier] = RaytracedShadow_RayGenName;

			WLogln(mpLogFile, L"RaytracedShadow - Ray Gen");
			rayGenShaderTable.DebugPrint(shaderIdToStringMap);
			WLogln(mpLogFile, L"");
#endif

			mShaderTables[ShaderTable::E_RayGenShader] = rayGenShaderTable.GetResource();
		}
		// MissShaderTable
		{
			void* const missShaderIdentifier = mStateObjectProp->GetShaderIdentifier(
				RaytracedShadow_MissName);

			Util::ShaderTable missShaderTable(
				mpLogFile, mInitData.Device, 1, ShaderIdentifierSize);
			CheckReturn(mpLogFile, missShaderTable.Initialze());
			missShaderTable.push_back(Util::ShaderRecord(
				missShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[missShaderIdentifier] = RaytracedShadow_MissName;

			WLogln(mpLogFile, L"RaytracedShadow - Miss");
			missShaderTable.DebugPrint(shaderIdToStringMap);
			WLogln(mpLogFile, L"");
#endif

			mShaderTables[ShaderTable::E_MissShader] = missShaderTable.GetResource();
		}
		// HitGroupShaderTable
		{
			void* const hitGroupShaderIdentifier = mStateObjectProp->GetShaderIdentifier(
				RaytracedShadow_HitGroupName);

			Util::ShaderTable hitGroupTable(
				mpLogFile, mInitData.Device, numRitems, ShaderIdentifierSize);
			CheckReturn(mpLogFile, hitGroupTable.Initialze());

			for (UINT i = 0; i < numRitems; ++i)
				hitGroupTable.push_back(Util::ShaderRecord(
					hitGroupShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[hitGroupShaderIdentifier] = RaytracedShadow_HitGroupName;

			WLogln(mpLogFile, L"RaytracedShadow - Hit Group");
			hitGroupTable.DebugPrint(shaderIdToStringMap);
			WLogln(mpLogFile, L"");
#endif

			mShaderTables[ShaderTable::E_HitGroupShader] = hitGroupTable.GetResource();
			mHitGroupShaderTableStrideInBytes = hitGroupTable.GetShaderRecordSize();
		}
	}

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::CalcShadow(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_GPU_VIRTUAL_ADDRESS accelStruct,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		Foundation::Resource::GpuResource* const pNormalMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		Foundation::Resource::GpuResource* const pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		nullptr));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetPipelineState1(mStateObject.Get());
		CmdList->SetComputeRootSignature(mRootSignature.Get());

		const auto shadow = mShadowMap.get();
		shadow->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, shadow);

		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->SetComputeRootConstantBufferView(
			RootSignature::CB_Light, pFrameResource->LightCB.CBAddress());
		CmdList->SetComputeRootShaderResourceView(
			RootSignature::SI_AccelerationStructure, accelStruct);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::SI_PositionMap, si_positionMap);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::SI_NormalMap, si_normalMap);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::SI_DepthMap, si_depthMap);
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::UO_ShadowMap, mhShadowMapGpuUav);

		D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
		const auto& rayGen = mShaderTables[ShaderTable::E_RayGenShader];
		const auto& miss = mShaderTables[ShaderTable::E_MissShader];
		const auto& hitGroup = mShaderTables[ShaderTable::E_HitGroupShader];

		dispatchDesc.RayGenerationShaderRecord.StartAddress = rayGen->GetGPUVirtualAddress();
		dispatchDesc.RayGenerationShaderRecord.SizeInBytes = rayGen->GetDesc().Width;
		dispatchDesc.MissShaderTable.StartAddress = miss->GetGPUVirtualAddress();
		dispatchDesc.MissShaderTable.SizeInBytes = miss->GetDesc().Width;
		dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
		dispatchDesc.HitGroupTable.StartAddress = hitGroup->GetGPUVirtualAddress();
		dispatchDesc.HitGroupTable.SizeInBytes = hitGroup->GetDesc().Width;
		dispatchDesc.HitGroupTable.StrideInBytes = mHitGroupShaderTableStrideInBytes;

		dispatchDesc.Width = mInitData.ClientWidth;
		dispatchDesc.Height = mInitData.ClientHeight;
		dispatchDesc.Depth = 1;

		CmdList->DispatchRays(&dispatchDesc);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.DepthOrArraySize = 1;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Format = ShadingConvention::Shadow::ShadowMapFormat;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	texDesc.Width = mInitData.ClientWidth;
	texDesc.Height = mInitData.ClientHeight;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;

	CheckReturn(mpLogFile, mShadowMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"RaytracedShadowMap_ShadowMap"));

	return TRUE;
}

BOOL RaytracedShadow::RaytracedShadowClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = ShadingConvention::Shadow::ShadowMapFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = ShadingConvention::Shadow::ShadowMapFormat;

	const auto resource = mShadowMap->Resource();
	Foundation::Util::D3D12Util::CreateShaderResourceView(
		mInitData.Device, resource, &srvDesc, mhShadowMapCpuSrv);
	Foundation::Util::D3D12Util::CreateUnorderedAccessView(
		mInitData.Device, resource, nullptr, &uavDesc, mhShadowMapCpuUav);

	return TRUE;
}