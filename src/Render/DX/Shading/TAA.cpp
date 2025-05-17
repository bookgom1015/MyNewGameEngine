#include "Render/DX/Shading/TAA.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/RenderItem.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Resource/MaterialData.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"

using namespace Render::DX::Shading;
using namespace DirectX;

namespace {
	const WCHAR* const HLSL_TAA = L"TAA.hlsl";
}

TAA::InitDataPtr TAA::MakeInitData() {
	return std::unique_ptr<TAAClass::InitData>(new TAAClass::InitData());
}

TAA::TAAClass::TAAClass() {
	mHistoryMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT TAA::TAAClass::CbvSrvUavDescCount() const { return 1; }

UINT TAA::TAAClass::RtvDescCount() const { return 0; }

UINT TAA::TAAClass::DsvDescCount() const { return 0; }

BOOL TAA::TAAClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	for (size_t i = 0, end = mHaltonSequence.size(); i < end; ++i) {
		auto offset = mHaltonSequence[i];
		mFittedToBakcBufferHaltonSequence[i] = XMFLOAT2(((offset.x - 0.5f) / mInitData.ClientWidth) * 2.f, ((offset.y - 0.5f) / mInitData.ClientHeight) * 2.f);
	}

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL TAA::TAAClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_TAA, L"VS", L"vs_6_5");
	const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_TAA, L"MS", L"ms_6_5");
	const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_TAA, L"PS", L"ps_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_TAA]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_TAA]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_TAA]));

	return TRUE;
}

BOOL TAA::TAAClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[3] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(ShadingConvention::TAA::RootConstant::Default::Count, 0);
	slotRootParameter[RootSignature::Default::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_HistoryMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_VelocityMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"TAA_GR_Default"));

	return TRUE;
}

BOOL TAA::TAAClass::BuildPipelineStates() {
	// GraphicsPipelineState
	{
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_TAA]);
			NullCheck(mpLogFile, VS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_TAA]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_TAA]),
			L"TAA_GP_Default"));
	}
	// MeshShaderPipelineState
	if (mInitData.MeshShaderSupported) {
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_TAA]);
			NullCheck(mpLogFile, MS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_TAA]);
			NullCheck(mpLogFile, PS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_TAA]),
			L"TAA_MP_Default"));
	}

	return TRUE;
}

BOOL TAA::TAAClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhHistoryMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhHistoryMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL TAA::TAAClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	for (size_t i = 0, end = mHaltonSequence.size(); i < end; ++i) {
		auto offset = mHaltonSequence[i];
		mFittedToBakcBufferHaltonSequence[i] = XMFLOAT2(((offset.x - 0.5f) / mInitData.ClientWidth) * 2.f, ((offset.y - 0.5f) / mInitData.ClientHeight) * 2.f);
	}

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL TAA::TAAClass::ApplyTAA(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		Foundation::Resource::GpuResource* const pVelocityMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap,
		FLOAT factor) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? PipelineState::MP_TAA : PipelineState::GP_TAA].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mHistoryMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		ShadingConvention::TAA::RootConstant::Default::Struct rc;
		rc.gModulationFactor = factor;
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(mInitData.ClientWidth);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(mInitData.ClientHeight);

		std::array<std::uint32_t, ShadingConvention::TAA::RootConstant::Default::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::TAA::RootConstant::Default::Struct));

		CmdList->SetGraphicsRoot32BitConstants(RootSignature::Default::RC_Consts, ShadingConvention::TAA::RootConstant::Default::Count, consts.data(), 0);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_HistoryMap, mhHistoryMapGpuSrv);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_VelocityMap, si_velocityMap);

		if (mInitData.MeshShaderSupported) {
			CmdList->DispatchMesh(1, 1, 1);
		}
		else {
			CmdList->IASetVertexBuffers(0, 0, nullptr);
			CmdList->IASetIndexBuffer(nullptr);
			CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CmdList->DrawInstanced(6, 1, 0, 0);
		}

		mHistoryMap->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CmdList->CopyResource(mHistoryMap->Resource(), pBackBuffer->Resource());
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL TAA::TAAClass::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc = {};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Format = HDR_FORMAT;
	rscDesc.Alignment = 0;
	rscDesc.Width = mInitData.ClientWidth;
	rscDesc.Height = mInitData.ClientHeight;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CheckReturn(mpLogFile, mHistoryMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"TAA_HistoryMap"));

	return TRUE;
}

BOOL TAA::TAAClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = HDR_FORMAT;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.MipLevels = 1;

	Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, mHistoryMap->Resource(), &srvDesc, mhHistoryMapCpuSrv);

	return TRUE;
}
