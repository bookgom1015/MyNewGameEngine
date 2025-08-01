#include "Render/DX/Shading/ToneMapping.hpp"
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
	const WCHAR* const HLSL_ToneMapping = L"ToneMapping.hlsl";
}

ToneMapping::InitDataPtr ToneMapping::MakeInitData() {
	return std::unique_ptr<ToneMappingClass::InitData>(new ToneMappingClass::InitData());
}

ToneMapping::ToneMappingClass::ToneMappingClass() {
	mIntermediateMap = std::make_unique<Foundation::Resource::GpuResource>();
	mIntermediateCopyMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT ToneMapping::ToneMappingClass::CbvSrvUavDescCount() const { return 0 +
	+ 1 // IntermediateMapSrv
	+ 1 // IntermediateCopyMapSrv
	; 
}

UINT ToneMapping::ToneMappingClass::RtvDescCount() const { return 1; }

UINT ToneMapping::ToneMappingClass::DsvDescCount() const { return 0; }

BOOL ToneMapping::ToneMappingClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ToneMapping, L"VS", L"vs_6_5");
	const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ToneMapping, L"MS", L"ms_6_5");
	const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ToneMapping, L"PS", L"ps_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ToneMapping]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_ToneMapping]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ToneMapping]));
	
	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::RC_Cosnts].InitAsConstants(ShadingConvention::ToneMapping::RootConstant::Default::Count, 0);
	slotRootParameter[RootSignature::Default::SI_Intermediate].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device, 
		rootSigDesc, 
		IID_PPV_ARGS(&mRootSignature),
		L"ToneMapping_GR_Default"));

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::BuildPipelineStates() {
	// ToneMapping
	{
		// GraphicsPipelineState
		{
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignature.Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ToneMapping]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ToneMapping]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = SDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ToneMapping]),
				L"ToneMapping_GP_ToneMapping"));
		}
		// MeshShaderPipelineState
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignature.Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_ToneMapping]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ToneMapping]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = SDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_ToneMapping]),
				L"ToneMapping_MP_ToneMapping"));
		}
	}

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhIntermediateMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhIntermediateMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhIntermediateMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	mhIntermediateCopyMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhIntermediateCopyMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::Resolve(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		FLOAT exposure) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? PipelineState::MP_ToneMapping : PipelineState::GP_ToneMapping].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mIntermediateMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		ShadingConvention::ToneMapping::RootConstant::Default::Struct rc;
		rc.gExposure = exposure;

		std::array<std::uint32_t, ShadingConvention::ToneMapping::RootConstant::Default::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::ToneMapping::RootConstant::Default::Struct));

		CmdList->SetGraphicsRoot32BitConstants(RootSignature::Default::RC_Cosnts, ShadingConvention::ToneMapping::RootConstant::Default::Count, consts.data(), 0);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_Intermediate, mhIntermediateMapGpuSrv);

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

BOOL ToneMapping::ToneMappingClass::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc = {};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Width = mInitData.ClientWidth;
	rscDesc.Height = mInitData.ClientHeight;
	rscDesc.Format = ShadingConvention::ToneMapping::IntermediateMapFormat;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	// IntermediateMap
	{
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE optClear(ShadingConvention::ToneMapping::IntermediateMapFormat, ShadingConvention::ToneMapping::IntermediateMapClearValues);

		CheckReturn(mpLogFile, mIntermediateMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			L"ToneMapping_IntermediateMap"));
	}
	// IntermediateCopyMap
	{
		rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		CheckReturn(mpLogFile, mIntermediateCopyMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"ToneMapping_IntermediateCopyMap"));
	}

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::BuildDescriptors() {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = ShadingConvention::ToneMapping::IntermediateMapFormat;

		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, mIntermediateMap->Resource(), &srvDesc, mhIntermediateMapCpuSrv);
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, mIntermediateCopyMap->Resource(), &srvDesc, mhIntermediateCopyMapCpuSrv);
	}
	// Rtv
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		rtvDesc.Format = ShadingConvention::ToneMapping::IntermediateMapFormat;

		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, mIntermediateMap->Resource(), &rtvDesc, mhIntermediateMapCpuRtv);
	}

	return TRUE;
}
