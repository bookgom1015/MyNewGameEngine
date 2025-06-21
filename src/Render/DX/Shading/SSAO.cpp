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

#include <DirectXColors.h>

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
	for (UINT i = 0; i < 2; ++i) 
		mAOMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT SSAO::SSAOClass::CbvSrvUavDescCount() const { return 0
	+ 2 // AOMapSrvs
	+ 2 // AOMapUavs
	+ 1 // RandomVectorMap
	; 
}

UINT SSAO::SSAOClass::RtvDescCount() const { return 2; }

UINT SSAO::SSAOClass::DsvDescCount() const { return 0; }

BOOL SSAO::SSAOClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	mTexWidth = mInitData.ClientWidth / 2;
	mTexHeight = mInitData.ClientHeight / 2;

	mViewport = { 0.0f, 0.0f, static_cast<FLOAT>(mTexWidth), static_cast<FLOAT>(mTexHeight), 0.0f, 1.0f };
	mScissorRect = { 0, 0, static_cast<INT>(mTexWidth), static_cast<INT>(mTexHeight) };

	CheckReturn(mpLogFile, BuildRandomVectorMapResource());
	CheckReturn(mpLogFile, BuildAOMapResources());

	BuildOffsetVecotrs();

	return TRUE;
}

BOOL SSAO::SSAOClass::CompileShaders() {
	const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_SSAO, L"CS", L"cs_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_SSAO]));

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[4] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::CB_AO].InitAsConstantBufferView(0);
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(ShadingConvention::SSAO::RootConstant::Default::Count, 1);
	slotRootParameter[RootSignature::Default::SI_NormalMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_RandomVectorMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::UO_AOMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"SSAO_GR_Default"));

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildPipelineStates() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
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
		L"SSAO_CP_Default"));

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhRandomVectorMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhRandomVectorMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);

	for (UINT i = 0; i < 2; ++i) {
		mhAOMapCpuSrvs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhAOMapGpuSrvs[i] = pDescHeap->CbvSrvUavGpuOffset(1);
		mhAOMapCpuUavs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhAOMapGpuUavs[i] = pDescHeap->CbvSrvUavGpuOffset(1);
	}

	CheckReturn(mpLogFile, BuildRandomVectorMapDescriptor());
	CheckReturn(mpLogFile, BuildAOMapDescriptors());

	return TRUE;
}

BOOL SSAO::SSAOClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	mTexWidth = mInitData.ClientWidth / 2;
	mTexHeight = mInitData.ClientHeight / 2;

	mViewport = { 0.0f, 0.0f, static_cast<FLOAT>(mTexWidth), static_cast<FLOAT>(mTexHeight), 0.0f, 1.0f };
	mScissorRect = { 0, 0, static_cast<INT>(mTexWidth), static_cast<INT>(mTexHeight) };

	CheckReturn(mpLogFile, BuildAOMapResources());
	CheckReturn(mpLogFile, BuildAOMapDescriptors());

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
		Foundation::Resource::GpuResource* const pNormalMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
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

		CmdList->SetComputeRootConstantBufferView(RootSignature::Default::CB_AO, pFrameResource->AmbientOcclusionCBAddress());

		ShadingConvention::SSAO::RootConstant::Default::Struct rc;
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(mTexWidth);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(mTexHeight);

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::SSAO::RootConstant::Default::Struct>(
			RootSignature::Default::RC_Consts,
			ShadingConvention::SSAO::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		mAOMaps[0]->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, mAOMaps[0].get());

		pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mRandomVectorMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_NormalMap, si_normalMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_PositionMap, si_positionMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_RandomVectorMap, mhRandomVectorMapGpuSrv);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::UO_AOMap, mhAOMapGpuUavs[0]);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(mTexWidth, ShadingConvention::SSAO::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(mTexHeight, ShadingConvention::SSAO::ThreadGroup::Default::Height),
			ShadingConvention::SSAO::ThreadGroup::Default::Depth);

		Foundation::Util::D3D12Util::UavBarrier(CmdList, mAOMaps[0].get());
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
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

BOOL SSAO::SSAOClass::BuildAOMapResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Format = ShadingConvention::SSAO::AOMapFormat;
	texDesc.Width = mTexWidth;
	texDesc.Height = mTexHeight;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// AOMaps
	for (UINT i = 0; i < 2; ++i) {
		std::wstringstream wsstream;
		wsstream << L"SSAO_AOMap_" << i;

		CheckReturn(mpLogFile, mAOMaps[i]->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			wsstream.str().c_str()));
	}

	return TRUE;
}

BOOL SSAO::SSAOClass::BuildAOMapDescriptors() {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = ShadingConvention::SSAO::AOMapFormat;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		for (UINT i = 0; i < 2; ++i)
			Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, mAOMaps[i]->Resource(), &srvDesc, mhAOMapCpuSrvs[i]);
	}
	// Uav
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Format = ShadingConvention::SSAO::AOMapFormat;
		uavDesc.Texture2D.MipSlice = 0;
		uavDesc.Texture2D.PlaneSlice = 0;

		for (UINT i = 0; i < 2; ++i) 
			Foundation::Util::D3D12Util::CreateUnorderedAccessView(mInitData.Device, mAOMaps[i]->Resource(), nullptr, &uavDesc, mhAOMapCpuUavs[i]);
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