#include "Render/DX/Shading/GammaCorrection.hpp"
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
	const WCHAR* const HLSL_GammaCorrection = L"GammaCorrection.hlsl";
}

GammaCorrection::InitDataPtr GammaCorrection::MakeInitData() {
	return std::unique_ptr<GammaCorrectionClass::InitData>(new GammaCorrectionClass::InitData());
}

GammaCorrection::GammaCorrectionClass::GammaCorrectionClass() {}

UINT GammaCorrection::GammaCorrectionClass::CbvSrvUavDescCount() const { return 0; }

UINT GammaCorrection::GammaCorrectionClass::RtvDescCount() const { return 0; }

UINT GammaCorrection::GammaCorrectionClass::DsvDescCount() const { return 0; }

BOOL GammaCorrection::GammaCorrectionClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL GammaCorrection::GammaCorrectionClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GammaCorrection, L"VS", L"vs_6_5");
	const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GammaCorrection, L"MS", L"ms_6_5");
	const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GammaCorrection, L"PS", L"ps_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_GammaCorrect]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_GammaCorrect]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_GammaCorrect]));

	return TRUE;
}

BOOL GammaCorrection::GammaCorrectionClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(ShadingConvention::GammaCorrection::RootConstant::Default::Count, 0);
	slotRootParameter[RootSignature::Default::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device, 
		rootSigDesc, 
		IID_PPV_ARGS(&mRootSignature),
		L"GammaCorrection_GR_Default"));

	return TRUE;
}

BOOL GammaCorrection::GammaCorrectionClass::BuildPipelineStates() {
	// GraphicsPipelineState
	{
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_GammaCorrect]);
			NullCheck(mpLogFile, VS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_GammaCorrect]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = SDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_GammaCorrect]),
			L"GammaCorrection_GP_Default"));
	}
	// MeshShaderPipelineState
	if (mInitData.MeshShaderSupported) {
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_GammaCorrect]);
			NullCheck(mpLogFile, MS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_GammaCorrect]);
			NullCheck(mpLogFile, PS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = SDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_GammaCorrect]),
			L"GammaCorrection_MP_Default"));
	}

	return TRUE;
}

BOOL GammaCorrection::GammaCorrectionClass::ApplyCorrection(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		FLOAT gamma) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? PipelineState::MP_GammaCorrect : PipelineState::GP_GammaCorrect].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignature.Get());

	CmdList->RSSetViewports(1, &viewport);
	CmdList->RSSetScissorRects(1, &scissorRect);

	pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

	CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

	pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

	ShadingConvention::GammaCorrection::RootConstant::Default::Struct rc;
	rc.gGamma = gamma;

	std::array<std::uint32_t, ShadingConvention::GammaCorrection::RootConstant::Default::Count> consts;
	std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::GammaCorrection::RootConstant::Default::Struct));

	CmdList->SetGraphicsRoot32BitConstants(RootSignature::Default::RC_Consts, ShadingConvention::GammaCorrection::RootConstant::Default::Count, consts.data(), 0);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_BackBuffer, si_backBufferCopy);

	if (mInitData.MeshShaderSupported) {
		CmdList->DispatchMesh(1, 1, 1);
	}
	else {
		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(6, 1, 0, 0);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}
