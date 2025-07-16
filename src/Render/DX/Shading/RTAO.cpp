#include "Render/DX/Shading/RTAO.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/MathUtil.hpp"
#include "Common/Render/ShadingArgument.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/ShaderTable.hpp"

using namespace Render::DX::Shading;
using namespace DirectX;

namespace {
	const WCHAR* const HLSL_RTAO				= L"RTAO.hlsl";

	const WCHAR* const RTAO_RayGenName			= L"RTAO_RayGen";
	const WCHAR* const RTAO_RayGenRaySortedName = L"RTAO_RayGen_RaySorted";
	const WCHAR* const RTAO_ClosestHitName		= L"RTAO_ClosestHit";
	const WCHAR* const RTAO_MissName			= L"RTAO_Miss";
	const WCHAR* const RTAO_HitGroupName		= L"RTAO_HitGroup";
}

RTAO::InitDataPtr RTAO::MakeInitData() {
	return std::unique_ptr<RTAOClass::InitData>(new RTAOClass::InitData());
}

RTAO::RTAOClass::RTAOClass() {
	for (UINT resource = 0; resource < Resource::AO::Count; ++resource)
		mAOResources[resource] = std::make_unique<Foundation::Resource::GpuResource>();

	for (UINT frame = 0; frame < 2; ++frame) {
		for (UINT resource = 0; resource < Resource::TemporalCache::Count; ++resource) {
			mTemporalCaches[frame][resource] = std::make_unique<Foundation::Resource::GpuResource>();
		}
	}

	mDebugMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT RTAO::RTAOClass::CbvSrvUavDescCount() const { return 0
	+ Descriptor::AO::Count
	+ Descriptor::TemporalCache::Count // Frame 0
	+ Descriptor::TemporalCache::Count // Frame 1
	+ 1 // DebugMap
	; 
}

UINT RTAO::RTAOClass::RtvDescCount() const { return 0; }

UINT RTAO::RTAOClass::DsvDescCount() const { return 0; }

BOOL RTAO::RTAOClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL RTAO::RTAOClass::CompileShaders() {
	const auto Lib = Util::ShaderManager::D3D12ShaderInfo(HLSL_RTAO, L"", L"lib_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(Lib, mShaderHashes[Shader::Lib_RTAO]));

	return TRUE;
}

BOOL RTAO::RTAOClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[7] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Count] = {};
	slotRootParameter[RootSignature::SI_AccelerationStructure].InitAsShaderResourceView(0);
	slotRootParameter[RootSignature::CB_AO].InitAsConstantBufferView(0);
	slotRootParameter[RootSignature::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::SI_NormalDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::SI_RayDirectionOriginDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::SI_RayIndexOffsetMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::UO_AOCoefficientMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::UO_RayHitDistanceMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::UO_DebugMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_NONE
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSignatureDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"RTAO_GR_Default"));

	return TRUE;
}

BOOL RTAO::RTAOClass::BuildPipelineStates() {
	if (!mInitData.RaytracingSupported) return TRUE;

	CD3DX12_STATE_OBJECT_DESC rtaoStateObject = { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	// RTAO-Library
	const auto rtaoLib = rtaoStateObject.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	const auto rtaoShader = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::Lib_RTAO]);
	const D3D12_SHADER_BYTECODE rtaoLibDxil = CD3DX12_SHADER_BYTECODE(rtaoShader->GetBufferPointer(), rtaoShader->GetBufferSize());
	rtaoLib->SetDXILLibrary(&rtaoLibDxil);
	LPCWSTR rtaoExports[] = { RTAO_RayGenName, RTAO_RayGenRaySortedName, RTAO_ClosestHitName, RTAO_MissName };
	rtaoLib->DefineExports(rtaoExports);

	// RTAO-HitGroup
	const auto rtaoHitGroup = rtaoStateObject.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	rtaoHitGroup->SetClosestHitShaderImport(RTAO_ClosestHitName);
	rtaoHitGroup->SetHitGroupExport(RTAO_HitGroupName);
	rtaoHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// ShaderConfig
	const auto shaderConfig = rtaoStateObject.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	const UINT payloadSize = 4; // tHit(FLOAT)
	const UINT attribSize = sizeof(XMFLOAT2);
	shaderConfig->Config(payloadSize, attribSize);

	// Global-RootSignature
	const auto glbalRootSig = rtaoStateObject.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	glbalRootSig->SetRootSignature(mRootSignature.Get());

	// Pipeline-Configuration
	const auto pipelineConfig = rtaoStateObject.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	const UINT maxRecursionDepth = 1;
	pipelineConfig->Config(maxRecursionDepth);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateStateObject(mInitData.Device, rtaoStateObject, IID_PPV_ARGS(&mStateObject)));
	CheckHRESULT(mpLogFile, mStateObject->QueryInterface(IID_PPV_ARGS(&mStateObjectProp)));

	return TRUE;
}

BOOL RTAO::RTAOClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
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
		// AOCoefficient
		mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_AOCoefficient] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalCacheGpus[frame][Descriptor::TemporalCache::ES_AOCoefficient] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_AOCoefficient] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporalCacheGpus[frame][Descriptor::TemporalCache::EU_AOCoefficient] = pDescHeap->CbvSrvUavGpuOffset(1);
	}
	// DebugMap
	mhDebugMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhDebugMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL RTAO::RTAOClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL RTAO::RTAOClass::BuildShaderTables(UINT numRitems) {
#ifdef _DEBUG
	// A shader name look-up table for shader table debug print out.
	std::unordered_map<void*, std::wstring> shaderIdToStringMap;
#endif

	const UINT ShaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	{
		// RayGenShaderTable
		{
			void* const rayGenShaderIdentifier = mStateObjectProp->GetShaderIdentifier(RTAO_RayGenName);

			Util::ShaderTable rayGenShaderTable(mpLogFile, mInitData.Device, 2, ShaderIdentifierSize);
			CheckReturn(mpLogFile, rayGenShaderTable.Initialze());
			rayGenShaderTable.push_back(Util::ShaderRecord(rayGenShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[rayGenShaderIdentifier] = RTAO_RayGenName;

			WLogln(mpLogFile, L"RTAO - Ray Gen");
			rayGenShaderTable.DebugPrint(shaderIdToStringMap);
			WLogln(mpLogFile, L"");
#endif

			mShaderTables[ShaderTable::E_RayGenShader] = rayGenShaderTable.GetResource();
		}
		// SortedRayGenShaderTable
		{
			void* const rayGenRaySortedShaderIdentifier = mStateObjectProp->GetShaderIdentifier(RTAO_RayGenRaySortedName);

			Util::ShaderTable rayGenShaderTable(mpLogFile, mInitData.Device, 2, ShaderIdentifierSize);
			CheckReturn(mpLogFile, rayGenShaderTable.Initialze());
			rayGenShaderTable.push_back(Util::ShaderRecord(rayGenRaySortedShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[rayGenRaySortedShaderIdentifier] = RTAO_RayGenRaySortedName;

			WLogln(mpLogFile, L"RTAO - Sorted Ray Gen");
			rayGenShaderTable.DebugPrint(shaderIdToStringMap);
			WLogln(mpLogFile, L"");
#endif

			mShaderTables[ShaderTable::E_SortedRayGenShader] = rayGenShaderTable.GetResource();
		}
		// MissShaderTable
		{
			void* const missShaderIdentifier = mStateObjectProp->GetShaderIdentifier(RTAO_MissName);

			Util::ShaderTable missShaderTable(mpLogFile, mInitData.Device, 1, ShaderIdentifierSize);
			CheckReturn(mpLogFile, missShaderTable.Initialze());
			missShaderTable.push_back(Util::ShaderRecord(missShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[missShaderIdentifier] = RTAO_MissName;

			WLogln(mpLogFile, L"RTAO - Miss");
			missShaderTable.DebugPrint(shaderIdToStringMap);
			WLogln(mpLogFile, L"");
#endif

			mShaderTables[ShaderTable::E_MissShader] = missShaderTable.GetResource();
		}
		// HitGroupShaderTable
		{
			void* const hitGroupShaderIdentifier = mStateObjectProp->GetShaderIdentifier(RTAO_HitGroupName);

			Util::ShaderTable hitGroupTable(mpLogFile, mInitData.Device, numRitems, ShaderIdentifierSize);
			CheckReturn(mpLogFile, hitGroupTable.Initialze());

			for (UINT i = 0; i < numRitems; ++i)
				hitGroupTable.push_back(Util::ShaderRecord(hitGroupShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[hitGroupShaderIdentifier] = RTAO_HitGroupName;

			WLogln(mpLogFile, L"RTAO - Hit Group");
			hitGroupTable.DebugPrint(shaderIdToStringMap);
			WLogln(mpLogFile, L"");
#endif

			mShaderTables[ShaderTable::E_HitGroupShader] = hitGroupTable.GetResource();
			mHitGroupShaderTableStrideInBytes = hitGroupTable.GetShaderRecordSize();
		}
	}

	return TRUE;
}

BOOL RTAO::RTAOClass::BuildResources() {
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

	// AO
	{
		// AOCoefficientMap
		{
			texDesc.Format = ShadingConvention::RTAO::AOCoefficientMapFormat;

			CheckReturn(mpLogFile, mAOResources[Resource::AO::E_AOCoefficient]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"RTAO_AOCoefficientMap"));
		}
		// RayHitDistanceMap
		{
			texDesc.Format = ShadingConvention::RTAO::RayHitDistanceFormat;

			CheckReturn(mpLogFile, mAOResources[Resource::AO::E_RayHitDistance]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"RTAO_RayHitDistanceMap"));
		}
	}
	// TemporalCache
	for (UINT frame = 0; frame < 2; ++frame) {
		// TSPPMap
		{
			texDesc.Format = ShadingConvention::RTAO::TSPPMapFormat;

			std::wstringstream name;
			name << L"RTAO_TSPPMap_" << frame;

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
			texDesc.Format = ShadingConvention::RTAO::RayHitDistanceFormat;

			std::wstringstream name;
			name << L"RTAO_TemporalRayHitDistanceMap_" << frame;

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
			texDesc.Format = ShadingConvention::RTAO::AOCoefficientSquaredMeanMapFormat;

			std::wstringstream name;
			name << L"RTAO_TemporalAOCoefficientSquaredMeanMap_" << frame;

			CheckReturn(mpLogFile, mTemporalCaches[frame][Resource::TemporalCache::E_AOCoefficientSquaredMean]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				name.str().c_str()));
		}
		// TemporalAOCoefficientMap
		{
			texDesc.Format = ShadingConvention::RTAO::AOCoefficientMapFormat;

			std::wstringstream name;
			name << L"RTAO_TemporalAOCoefficientMap_" << frame;

			CheckReturn(mpLogFile, mTemporalCaches[frame][Resource::TemporalCache::E_AOCoefficient]->Initialize(
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
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		CheckReturn(mpLogFile, mDebugMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"RTAO_AOCoefficientMap"));
	}

	return TRUE;
}
BOOL RTAO::RTAOClass::BuildDescriptors() {
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

	// AO
	{
		// AOCoefficient
		{
			srvDesc.Format = ShadingConvention::RTAO::AOCoefficientMapFormat;
			uavDesc.Format = ShadingConvention::RTAO::AOCoefficientMapFormat;

			const auto resource = mAOResources[Resource::AO::E_AOCoefficient]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhAOResourceCpus[Descriptor::AO::ES_AOCoefficient]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhAOResourceCpus[Descriptor::AO::EU_AOCoefficient]);
		}
		// RayHitDistance
		{
			srvDesc.Format = ShadingConvention::RTAO::RayHitDistanceFormat;
			uavDesc.Format = ShadingConvention::RTAO::RayHitDistanceFormat;

			const auto resource = mAOResources[Resource::AO::E_RayHitDistance]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhAOResourceCpus[Descriptor::AO::ES_RayHitDistance]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr , &uavDesc, mhAOResourceCpus[Descriptor::AO::EU_RayHitDistance]);
		}
	}
	// TemporalCache
	for (UINT frame = 0; frame < 2; ++frame) {
		// TSSP
		{
			srvDesc.Format = ShadingConvention::RTAO::TSPPMapFormat;
			uavDesc.Format = ShadingConvention::RTAO::TSPPMapFormat;

			const auto resource = mTemporalCaches[frame][Resource::TemporalCache::E_TSPP]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_TSPP]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_TSPP]);
		}
		// RayHitDistance
		{
			srvDesc.Format = ShadingConvention::RTAO::RayHitDistanceFormat;
			uavDesc.Format = ShadingConvention::RTAO::RayHitDistanceFormat;

			const auto resource = mTemporalCaches[frame][Resource::TemporalCache::E_RayHitDistance]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_RayHitDistance]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_RayHitDistance]);
		}
		// AOCoefficientSquaredMean
		{
			srvDesc.Format = ShadingConvention::RTAO::AOCoefficientSquaredMeanMapFormat;
			uavDesc.Format = ShadingConvention::RTAO::AOCoefficientSquaredMeanMapFormat;

			const auto resource = mTemporalCaches[frame][Resource::TemporalCache::E_AOCoefficientSquaredMean]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_AOCoefficientSquaredMean]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_AOCoefficientSquaredMean]);
		}
		// AOCoefficient
		{
			srvDesc.Format = ShadingConvention::RTAO::AOCoefficientMapFormat;
			uavDesc.Format = ShadingConvention::RTAO::AOCoefficientMapFormat;

			const auto resource = mTemporalCaches[frame][Resource::TemporalCache::E_AOCoefficient]->Resource();
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::ES_AOCoefficient]);
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhTemporalCacheCpus[frame][Descriptor::TemporalCache::EU_AOCoefficient]);
		}
	}
	// DebugMap
	{
		uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		const auto resource = mDebugMap->Resource();
		Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, mhDebugMapCpuUav);
	}

	return TRUE;
}

BOOL RTAO::RTAOClass::DrawAO(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_GPU_VIRTUAL_ADDRESS accelStruct,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		Foundation::Resource::GpuResource* const pNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
		Foundation::Resource::GpuResource* const pRayDirectionOriginDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_rayDirectionOriginDepthMap,
		Foundation::Resource::GpuResource* const pRayInexOffsetMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_rayIndexOffsetMap,
		BOOL bRaySortingEnabled) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		nullptr));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetPipelineState1(mStateObject.Get());
		CmdList->SetComputeRootSignature(mRootSignature.Get());

		const auto& ao = mAOResources[RTAO::Resource::AO::E_AOCoefficient].get();
		ao->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, ao);

		const auto& rayHitDist = mAOResources[RTAO::Resource::AO::E_RayHitDistance].get();
		rayHitDist->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, rayHitDist);

		const auto& debugMap = mDebugMap.get();
		debugMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, debugMap);

		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRayDirectionOriginDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->SetComputeRootShaderResourceView(RootSignature::SI_AccelerationStructure, accelStruct);
		CmdList->SetComputeRootConstantBufferView(RootSignature::CB_AO, pFrameResource->AmbientOcclusionCBAddress());
		CmdList->SetComputeRootDescriptorTable(RootSignature::SI_PositionMap, si_positionMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::SI_NormalDepthMap, si_normalDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::SI_RayDirectionOriginDepthMap, si_rayDirectionOriginDepthMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::SI_RayIndexOffsetMap, si_rayIndexOffsetMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::UO_AOCoefficientMap, mhAOResourceGpus[RTAO::Descriptor::AO::EU_AOCoefficient]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::UO_RayHitDistanceMap, mhAOResourceGpus[RTAO::Descriptor::AO::EU_RayHitDistance]);
		CmdList->SetComputeRootDescriptorTable(RootSignature::UO_DebugMap, mhDebugMapGpuUav);

		D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
		const auto& rayGen = mShaderTables[bRaySortingEnabled ? ShaderTable::E_SortedRayGenShader : ShaderTable::E_RayGenShader];
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
		
		if (bRaySortingEnabled) {
			const UINT ActvieWidth =
				mInitData.ShadingArgumentSet->RTAO.CheckboardRayGeneration ?
				Foundation::Util::D3D12Util::CeilDivide(mInitData.ClientWidth, 2)
				: mInitData.ClientWidth;

			dispatchDesc.Width = ActvieWidth * mInitData.ClientHeight;
			dispatchDesc.Height = 1;
			dispatchDesc.Depth = 1;
		}
		else {
			dispatchDesc.Width = mInitData.ClientWidth;
			dispatchDesc.Height = mInitData.ClientHeight;
			dispatchDesc.Depth = 1;
		}
		
		CmdList->DispatchRays(&dispatchDesc);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

UINT RTAO::RTAOClass::MoveToNextTemporalCacheFrame() {
	mCurrentTemporalCacheFrameIndex = (mCurrentTemporalCacheFrameIndex + 1) % 2;
	return mCurrentTemporalCacheFrameIndex;
}

UINT RTAO::RTAOClass::MoveToNextAOResourceFrame() {
	mCurrentAOResourceFrameIndex = (mCurrentAOResourceFrameIndex + 1) % 2;
	return mCurrentAOResourceFrameIndex;
}