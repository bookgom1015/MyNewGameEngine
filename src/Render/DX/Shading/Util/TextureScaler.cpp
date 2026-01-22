#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/Util/TextureScaler.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX::Shading::Util;

namespace {
	const WCHAR* const HLSL_DownSample2Nx2N = L"DownSample2Nx2N.hlsl";
}

TextureScaler::InitDataPtr TextureScaler::MakeInitData() {
	return std::unique_ptr<TextureScalerClass::InitData>(new TextureScalerClass::InitData());
}

UINT TextureScaler::TextureScalerClass::CbvSrvUavDescCount() const { return 0; }

UINT TextureScaler::TextureScalerClass::RtvDescCount() const { return 0; }

UINT TextureScaler::TextureScalerClass::DsvDescCount() const { return 0; }

BOOL TextureScaler::TextureScalerClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

void TextureScaler::TextureScalerClass::CleanUp() {
	for (UINT i = 0; i < PipelineState::Count; ++i)
		mPipelineStates[i].Reset();

	for (UINT i = 0; i < RootSignature::Count; ++i)
		mRootSignatures[i].Reset();
}

BOOL TextureScaler::TextureScalerClass::CompileShaders() {
	// DownSample2x2
	{
		DxcDefine defines[] = { { L"KERNEL_RADIUS", L"1" } };

		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_DownSample2Nx2N, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_DownSample2x2]));
	}
	// DownSample4x4
	{
		DxcDefine defines[] = { { L"KERNEL_RADIUS", L"2" } };

		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_DownSample2Nx2N, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_DownSample4x4]));
	}
	// DownSample6x6
	{
		DxcDefine defines[] = { { L"KERNEL_RADIUS", L"3" } };

		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_DownSample2Nx2N, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_DownSample6x6]));
	}

	return TRUE;
}

BOOL TextureScaler::TextureScalerClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// DownSample2Nx2N
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::DownSample6x6::Count]{};
		slotRootParameter[RootSignature::DownSample6x6::RC_Consts].InitAsConstants(
			ShadingConvention::TextureScaler::RootConstant::DownSample6x6::Count, 0);
		slotRootParameter[RootSignature::DownSample6x6::SI_InputMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::DownSample6x6::UO_OutputMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_DownSample2Nx2N]),
			L"TextureScaler_GR_DownSample2Nx2N"));
	}

	return TRUE;
}

BOOL TextureScaler::TextureScalerClass::BuildPipelineStates() {
	// DownSample2Nx2N
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_DownSample2Nx2N].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		// 2x2
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_DownSample2x2]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_DownSample2x2]),
				L"TextureScaler_CP_DownSample2x2"));
		}
		// 4x4
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_DownSample4x4]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_DownSample4x4]),
				L"TextureScaler_CP_DownSample4x4"));
		}
		// 6x6
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_DownSample6x6]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_DownSample6x6]),
				L"TextureScaler_CP_DownSample6x6"));
		}
	}

	return TRUE;
}

BOOL TextureScaler::TextureScalerClass::DownSample2Nx2N(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pInputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
		Foundation::Resource::GpuResource* const pOutputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
		UINT srcTexDimX, UINT srcTexDimY, UINT dstTexDimX, UINT dstTexDimY,
		UINT kernelRadius) {
	assert(kernelRadius > 0 && kernelRadius <= 3);

	PipelineState::Type type;
	if (kernelRadius == 1) type = PipelineState::CP_DownSample2x2;
	else if (kernelRadius == 2) type = PipelineState::CP_DownSample4x4;
	else if (kernelRadius == 3) type = PipelineState::CP_DownSample6x6;

	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[type].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignatures[RootSignature::GR_DownSample2Nx2N].Get());

		ShadingConvention::TextureScaler::RootConstant::DownSample6x6::Struct rc;
		rc.gSrcTexDim = { srcTexDimX, srcTexDimY };
		rc.gDstTexDim = { dstTexDimX, dstTexDimY };

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::TextureScaler::RootConstant::DownSample6x6::Struct>(
			RootSignature::DownSample6x6::RC_Consts,
			ShadingConvention::TextureScaler::RootConstant::DownSample6x6::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		pInputMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);;

		pOutputMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pOutputMap);

		CmdList->SetComputeRootDescriptorTable(RootSignature::DownSample6x6::SI_InputMap, si_inputMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::DownSample6x6::UO_OutputMap, uo_outputMap);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				dstTexDimX, ShadingConvention::TextureScaler::ThreadGroup::DownSample6x6::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				dstTexDimY, ShadingConvention::TextureScaler::ThreadGroup::DownSample6x6::Height),
			ShadingConvention::TextureScaler::ThreadGroup::DownSample6x6::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}