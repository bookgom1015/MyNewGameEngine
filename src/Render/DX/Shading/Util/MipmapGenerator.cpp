#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"

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

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::CompileShaders() {
	// GenerateMipmap
	{
		const auto vsInfo = Util::ShaderManager::D3D12ShaderInfo(HLSL_GenerateMipmap, L"VS", L"vs_6_3");
		mInitData.ShaderManager->AddShader(vsInfo, mShaderHashes[Shader::E_VS_GenerateMipmap]);

		{
			const auto psInfo = Util::ShaderManager::D3D12ShaderInfo(HLSL_GenerateMipmap, L"PS_GenerateMipmap", L"ps_6_3");
			mInitData.ShaderManager->AddShader(psInfo, mShaderHashes[Shader::E_PS_GenerateMipmap]);
		}
		{
			const auto psInfo = Util::ShaderManager::D3D12ShaderInfo(HLSL_GenerateMipmap, L"PS_CopyMap", L"ps_6_3");
			mInitData.ShaderManager->AddShader(psInfo, mShaderHashes[Shader::E_PS_CopyMap]);
		}
	}

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::EC_Consts].InitAsConstants(ShadingConvention::MipmapGenerator::RootConstant::Default::Count, 0);
	slotRootParameter[RootSignature::Default::ESI_InputMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device, 
		rootSigDesc, 
		IID_PPV_ARGS(&mRootSignature), 
		L"MipmapGenerator_RS_Default"));

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::BuildPipelineStates() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = HDR_FORMAT;

	// GenerateMipmap
	{
		{
			const auto vs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::E_VS_GenerateMipmap]);
			if (vs == nullptr) ReturnFalse(mpLogFile, L"Failed to get shader");
			psoDesc.VS = { reinterpret_cast<BYTE*>(vs->GetBufferPointer()), vs->GetBufferSize() };
		}
		{
			const auto ps = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::E_PS_GenerateMipmap]);
			if (ps == nullptr) ReturnFalse(mpLogFile, L"Failed to get shader");
			psoDesc.PS = { reinterpret_cast<BYTE*>(ps->GetBufferPointer()), ps->GetBufferSize() };
		}
		
		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device, 
			psoDesc, 
			IID_PPV_ARGS(&mPipelineStates[PipelineState::EG_GenerateMipmap]), 
			L"MipmapGenerator_GPS_GenerateMipmap"));
	}
	// CopyMap
	{
		{
			const auto ps = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::E_PS_CopyMap]);
			if (ps == nullptr) ReturnFalse(mpLogFile, L"Failed to get shader");
			psoDesc.PS = { reinterpret_cast<BYTE*>(ps->GetBufferPointer()), ps->GetBufferSize() };
		}
		
		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::EG_CopyMap]),
			L"MipmapGenerator_GPS_CopyMap"));
	}

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::Run(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pOutput,
		D3D12_GPU_DESCRIPTOR_HANDLE si_input,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_outputs[],
		UINT maxMipLevel, UINT width, UINT height) {
	mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::EG_CopyMap].Get());

	const auto cmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(cmdList);

	CheckReturn(mpLogFile, CopyMap(cmdList, pOutput, si_input, ro_outputs[0], width, height));
	CheckReturn(mpLogFile, GenerateMipmap(cmdList, pOutput, si_input, ro_outputs, maxMipLevel, width, height));

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::CopyMap(
		ID3D12GraphicsCommandList4* const pCmdList,
		Foundation::Resource::GpuResource* const pOutput,
		D3D12_GPU_DESCRIPTOR_HANDLE si_input,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_output,
		UINT width, UINT height) {
	pCmdList->SetGraphicsRootSignature(mRootSignature.Get());

	{
		const D3D12_VIEWPORT viewport = { 0.f, 0.f, static_cast<FLOAT>(width), static_cast<FLOAT>(height), 0.f, 1.f };
		const D3D12_RECT scissorRect = { 0, 0, static_cast<INT>(width), static_cast<INT>(height) };

		pCmdList->RSSetViewports(1, &viewport);
		pCmdList->RSSetScissorRects(1, &scissorRect);
	}

	pOutput->Transite(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	pCmdList->OMSetRenderTargets(1, &ro_output, TRUE, nullptr);

	pCmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::ESI_InputMap, si_input);

	{
		FLOAT invW = 1.f / width;
		FLOAT invH = 1.f / height;

		ShadingConvention::MipmapGenerator::RootConstant::Default::Struct rc;
		rc.gInvTexSize.x = invW;
		rc.gInvTexSize.y = invH;
		rc.gInvMipmapTexSize.x = 0.f;
		rc.gInvMipmapTexSize.y = 0.f;

		std::array<std::uint32_t, ShadingConvention::MipmapGenerator::RootConstant::Default::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::MipmapGenerator::RootConstant::Default::Struct));

		pCmdList->SetGraphicsRoot32BitConstants(
			RootSignature::Default::EC_Consts,
			ShadingConvention::MipmapGenerator::RootConstant::Default::Count,
			consts.data(),
			0);
	}

	pCmdList->IASetVertexBuffers(0, 0, nullptr);
	pCmdList->IASetIndexBuffer(nullptr);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->DrawInstanced(6, 1, 0, 0);

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::GenerateMipmap(
		ID3D12GraphicsCommandList4* const pCmdList,
		Foundation::Resource::GpuResource* const pOutput,
		D3D12_GPU_DESCRIPTOR_HANDLE si_input,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_outputs[],
		UINT maxMipLevel, UINT width, UINT height) {
	pCmdList->SetPipelineState(mPipelineStates[PipelineState::EG_GenerateMipmap].Get());

	for (UINT mipLevel = 1; mipLevel < maxMipLevel; ++mipLevel) {
		pCmdList->OMSetRenderTargets(1, &ro_outputs[mipLevel], TRUE, nullptr);

		const UINT mw = static_cast<UINT>(width / std::pow(2, mipLevel));
		const UINT mh = static_cast<UINT>(height / std::pow(2, mipLevel));

		const D3D12_VIEWPORT viewport = { 0.f, 0.f, static_cast<FLOAT>(mw), static_cast<FLOAT>(mh), 0.f, 1.f };
		const D3D12_RECT scissorRect = { 0, 0, static_cast<INT>(mw), static_cast<INT>(mh) };

		pCmdList->RSSetViewports(1, &viewport);
		pCmdList->RSSetScissorRects(1, &scissorRect);

		const FLOAT invW = 1.f / width;
		const FLOAT invH = 1.f / height;
		const FLOAT invMW = 1.f / mw;
		const FLOAT invMH = 1.f / mh;

		ShadingConvention::MipmapGenerator::RootConstant::Default::Struct rc;
		rc.gInvTexSize.x = invW;
		rc.gInvTexSize.y = invH;
		rc.gInvMipmapTexSize.x = invMW;
		rc.gInvMipmapTexSize.y = invMH;

		std::array<std::uint32_t, ShadingConvention::MipmapGenerator::RootConstant::Default::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::MipmapGenerator::RootConstant::Default::Struct));

		pCmdList->SetGraphicsRoot32BitConstants(
			RootSignature::Default::EC_Consts, 
			ShadingConvention::MipmapGenerator::RootConstant::Default::Count, 
			consts.data(), 
			0);

		pCmdList->IASetVertexBuffers(0, 0, nullptr);
		pCmdList->IASetIndexBuffer(nullptr);
		pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCmdList->DrawInstanced(6, 1, 0, 0);
	}

	pOutput->Transite(pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	return TRUE;
}