#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/ChromaticAberration.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_ChromaticAberrationClass = L"ChromaticAberration.hlsl";
}

ChromaticAberration::InitDataPtr ChromaticAberration::MakeInitData() {
	return std::unique_ptr<ChromaticAberrationClass::InitData>(
		new ChromaticAberrationClass::InitData());
}

ChromaticAberration::ChromaticAberrationClass::ChromaticAberrationClass() {}

ChromaticAberration::ChromaticAberrationClass::~ChromaticAberrationClass() { CleanUp(); }

UINT ChromaticAberration::ChromaticAberrationClass::CbvSrvUavDescCount() const { return 0; }

UINT ChromaticAberration::ChromaticAberrationClass::RtvDescCount() const { return 0; }

UINT ChromaticAberration::ChromaticAberrationClass::DsvDescCount() const { return 0; }

BOOL ChromaticAberration::ChromaticAberrationClass::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

void ChromaticAberration::ChromaticAberrationClass::CleanUp() {
	if (mbCleanedUp) return;

	for (UINT i = 0; i < PipelineState::Count; ++i)
		mPipelineStates[i].Reset();

	mRootSignature.Reset();

	mbCleanedUp = TRUE;
}

BOOL ChromaticAberration::ChromaticAberrationClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D12ShaderInfo(
		HLSL_ChromaticAberrationClass, L"VS", L"vs_6_5");
	const auto MS = Util::ShaderManager::D3D12ShaderInfo(
		HLSL_ChromaticAberrationClass, L"MS", L"ms_6_5");
	const auto PS = Util::ShaderManager::D3D12ShaderInfo(
		HLSL_ChromaticAberrationClass, L"PS", L"ps_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
		VS, mShaderHashes[Shader::VS_ChromaticAberration]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
		MS, mShaderHashes[Shader::MS_ChromaticAberration]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
		PS, mShaderHashes[Shader::PS_ChromaticAberration]));

	return TRUE;
}

BOOL ChromaticAberration::ChromaticAberrationClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count]{};
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(
		ShadingConvention::ChromaticAberration::RootConstant::Default::Count, 0);
	slotRootParameter[RootSignature::Default::SI_BackBuffer].InitAsDescriptorTable(
		1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"ChromaticAberration_GR_Default"));

	return TRUE;
}

BOOL ChromaticAberration::ChromaticAberrationClass::BuildPipelineStates() {
	if (mInitData.MeshShaderSupported) {
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Shader::MS_ChromaticAberration]);
			NullCheck(mpLogFile, MS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Shader::PS_ChromaticAberration]);
			NullCheck(mpLogFile, PS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_ChromaticAberration]),
			L"ChromaticAberration_MP_Default"));
	}
	else {
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Shader::VS_ChromaticAberration]);
			NullCheck(mpLogFile, VS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Shader::PS_ChromaticAberration]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ChromaticAberration]),
			L"ChromaticAberration_GP_Default"));
	}

	return TRUE;
}

BOOL ChromaticAberration::ChromaticAberrationClass::ApplyChromaticAberration(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		FLOAT strength, FLOAT threshold, FLOAT feather,
		UINT maxShiftPx, FLOAT exponent) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? 
		PipelineState::MP_ChromaticAberration : PipelineState::GP_ChromaticAberration].Get()));

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

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		ShadingConvention::ChromaticAberration::RootConstant::Default::Struct rc;
		rc.gInvTexDim = {
			1.f / static_cast<FLOAT>(mInitData.ClientWidth),
			1.f / static_cast<FLOAT>(mInitData.ClientHeight) };
		rc.gStrength = strength;
		rc.gThreshold = threshold;
		rc.gFeather = feather;
		rc.gMaxShiftPx = maxShiftPx;
		rc.gExponent = exponent;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<
			ShadingConvention::ChromaticAberration::RootConstant::Default::Struct>(
				RootSignature::Default::RC_Consts,
				ShadingConvention::ChromaticAberration::RootConstant::Default::Count,
				&rc,
				0,
				CmdList,
				FALSE);

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
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}