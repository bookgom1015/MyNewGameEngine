#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX::Shading::Util;

namespace {
	const WCHAR* const HLSL_GenerateMipmap = L"GenerateMipmap.hlsl";
}

MipmapGenerator::InitDataPtr MipmapGenerator::MakeInitData() {
	return std::unique_ptr<MipmapGeneratorClass::InitData>(new MipmapGeneratorClass::InitData());
}

UINT MipmapGenerator::MipmapGeneratorClass::CbvSrvUavDescCount() const { return 0; }

UINT MipmapGenerator::MipmapGeneratorClass::RtvDescCount() const { return 0; }

UINT MipmapGenerator::MipmapGeneratorClass::DsvDescCount() const { return 0; }

BOOL MipmapGenerator::MipmapGeneratorClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	NullCheck(pLogFile, pData);
	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

void MipmapGenerator::MipmapGeneratorClass::CleanUp() {
	for (UINT i = 0; i < PipelineState::Count; ++i)
		mPipelineStates[i].Reset();

	mRootSignature.Reset();
}

BOOL MipmapGenerator::MipmapGeneratorClass::CompileShaders() {
	// GenerateMipmap
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GenerateMipmap, L"VS", L"vs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_GenerateMipmap]));

		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GenerateMipmap, L"MS", L"ms_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_GenerateMipmap]));

		{
			const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GenerateMipmap, L"PS_GenerateMipmap", L"ps_6_5");
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_GenerateMipmap]));
		}
		{
			const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GenerateMipmap, L"PS_CopyMap", L"ps_6_5");
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_CopyMap]));
		}
	}

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// Default
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count]{};
		slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(
			ShadingConvention::MipmapGenerator::RootConstant::Default::Count, 0);
		slotRootParameter[RootSignature::Default::SI_InputMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignature),
			L"MipmapGenerator_GR_Default"));
	}

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::BuildPipelineStates() {
	// GenerateMipmap
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignature.Get();
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_GenerateMipmap]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_GenerateMipmap]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_GenerateMipmap]),
				L"MipmapGenerator_MP_GenerateMipmap"));
		}
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignature.Get();
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_GenerateMipmap]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_GenerateMipmap]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_GenerateMipmap]),
				L"MipmapGenerator_GP_GenerateMipmap"));
		}		
	}
	// CopyMap
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignature.Get();
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_GenerateMipmap]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_CopyMap]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_CopyMap]),
				L"MipmapGenerator_MP_CopyMap"));
		}
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignature.Get();
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_GenerateMipmap]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_CopyMap]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_CopyMap]),
				L"MipmapGenerator_GP_CopyMap"));
		}		
	}

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::GenerateMipmap(
		Foundation::Resource::GpuResource* const pOutput,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_outputs[],
		Foundation::Resource::GpuResource* const pInput,
		D3D12_GPU_DESCRIPTOR_HANDLE si_input,
		UINT maxMipLevel, UINT width, UINT height) {
	CheckReturn(mpLogFile, CopyMap(pOutput, ro_outputs[0], pInput, si_input, width, height));
	CheckReturn(mpLogFile, GenerateMipmap(pOutput, ro_outputs, si_input, maxMipLevel, width, height));

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::CopyMap(
		Foundation::Resource::GpuResource* const pOutput,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_output,
	Foundation::Resource::GpuResource* const pInput,
		D3D12_GPU_DESCRIPTOR_HANDLE si_input,
		UINT width, UINT height) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList(mPipelineStates[
		mInitData.MeshShaderSupported ? PipelineState::MP_CopyMap : PipelineState::GP_CopyMap].Get()));

	const auto CmdList = mInitData.CommandObject->DirectCommandList();
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignature.Get());

	const D3D12_VIEWPORT Viewport = { 0.f, 0.f, static_cast<FLOAT>(width), static_cast<FLOAT>(height), 0.f, 1.f };
	const D3D12_RECT ScissorRect = { 0, 0, static_cast<INT>(width), static_cast<INT>(height) };

	CmdList->RSSetViewports(1, &Viewport);
	CmdList->RSSetScissorRects(1, &ScissorRect);

	pOutput->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pInput->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->OMSetRenderTargets(1, &ro_output, TRUE, nullptr);

	CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_InputMap, si_input);

	if (mInitData.MeshShaderSupported) {
		CmdList->DispatchMesh(1, 1, 1);
	}
	else {
		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(6, 1, 0, 0);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::GenerateMipmap(
		Foundation::Resource::GpuResource* const pOutput,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_outputs[],
		D3D12_GPU_DESCRIPTOR_HANDLE si_input,
		UINT maxMipLevel, UINT width, UINT height) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList(mPipelineStates[
			mInitData.MeshShaderSupported ? PipelineState::MP_GenerateMipmap : PipelineState::GP_GenerateMipmap].Get()));

	const auto CmdList = mInitData.CommandObject->DirectCommandList();
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignature.Get());

	for (UINT mipLevel = 1; mipLevel < maxMipLevel; ++mipLevel) {
		CmdList->OMSetRenderTargets(1, &ro_outputs[mipLevel], TRUE, nullptr);

		const UINT MW = static_cast<UINT>(width / std::pow(2, mipLevel));
		const UINT MH = static_cast<UINT>(height / std::pow(2, mipLevel));

		const D3D12_VIEWPORT Viewport = { 0.f, 0.f, static_cast<FLOAT>(MW), static_cast<FLOAT>(MH), 0.f, 1.f };
		const D3D12_RECT ScissorRect = { 0, 0, static_cast<INT>(MW), static_cast<INT>(MH) };

		CmdList->RSSetViewports(1, &Viewport);
		CmdList->RSSetScissorRects(1, &ScissorRect);

		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_InputMap, si_input);

		const FLOAT InvW = 1.f / width;
		const FLOAT InvH = 1.f / height;
		const FLOAT InvMW = 1.f / MW;
		const FLOAT InvMH = 1.f / MH;

		ShadingConvention::MipmapGenerator::RootConstant::Default::Struct rc;
		rc.gInvTexSize.x = InvW;
		rc.gInvTexSize.y = InvH;
		rc.gInvMipmapTexSize.x = InvMW;
		rc.gInvMipmapTexSize.y = InvMH;

		std::array<std::uint32_t, ShadingConvention::MipmapGenerator::RootConstant::Default::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::MipmapGenerator::RootConstant::Default::Struct));

		CmdList->SetGraphicsRoot32BitConstants(
			RootSignature::Default::RC_Consts, 
			ShadingConvention::MipmapGenerator::RootConstant::Default::Count, 
			consts.data(), 
			0);

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

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());

	return TRUE;
}