#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/BlurFilter.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/MathUtil.hpp"
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
	const WCHAR* const HLSL_GaussianBlurFilter3x3 = L"GaussianBlurFilter3x3.hlsl";
	const WCHAR* const HLSL_GaussianBlurFilterNxN = L"GaussianBlurFilterNxN.hlsl";
}

BlurFilter::InitDataPtr BlurFilter::MakeInitData() {
	return std::unique_ptr<BlurFilterClass::InitData>(new BlurFilterClass::InitData());
}

BlurFilter::BlurFilterClass::BlurFilterClass() {}

UINT BlurFilter::BlurFilterClass::CbvSrvUavDescCount() const { return 0; }

UINT BlurFilter::BlurFilterClass::RtvDescCount() const { return 0; }

UINT BlurFilter::BlurFilterClass::DsvDescCount() const { return 0; }

BOOL BlurFilter::BlurFilterClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

void BlurFilter::BlurFilterClass::CleanUp() {
	for (UINT i = 0; i < PipelineState::Count; ++i)
		mPipelineStates[i].Reset();

	mRootSignature.Reset();
}

BOOL BlurFilter::BlurFilterClass::CompileShaders() {
	// GaussianBlurFilter3x3
	{
		DxcDefine defines[] = { { L"ValueType", L"float" } };

		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_GaussianBlurFilter3x3, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_GaussianBlurFilter3x3]));
	}
	// GaussianBlurFilterRGBA3x3
	{
		DxcDefine defines[] = { { L"ValueType", L"float4" } };

		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_GaussianBlurFilter3x3, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_GaussianBlurFilterRGBA3x3]));
	}
	// GaussianBlurFilterNxN
	{
		// R
		{
			// 3x3
			{
				DxcDefine defines[] = { 
					{ L"KERNEL_RADIUS", L"1" },
					{ L"ValueType", L"float" }
				};

				const auto CS = Util::ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[Shader::CS_GaussianBlurFilterNxN3x3]));
			}
			// 5x5
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"2" },
					{ L"ValueType", L"float" }
				};

				const auto CS = Util::ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[Shader::CS_GaussianBlurFilterNxN5x5]));
			}
			// 7x7
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"3" },
					{ L"ValueType", L"float" }
				};

				const auto CS = Util::ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[Shader::CS_GaussianBlurFilterNxN7x7]));
			}
			// 9x9
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"4" },
					{ L"ValueType", L"float" }
				};

				const auto CS = Util::ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[Shader::CS_GaussianBlurFilterNxN9x9]));
			}
		}
		// RGBA
		{
			// 3x3
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"1" },
					{ L"ValueType", L"float4" }
				};

				const auto CS = Util::ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[Shader::CS_GaussianBlurFilterRGBANxN3x3]));
			}
			// 5x5
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"2" },
					{ L"ValueType", L"float4" }
				};

				const auto CS = Util::ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[Shader::CS_GaussianBlurFilterRGBANxN5x5]));
			}
			// 7x7
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"3" },
					{ L"ValueType", L"float4" }
				};

				const auto CS = Util::ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[Shader::CS_GaussianBlurFilterRGBANxN7x7]));
			}
			// 9x9
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"4" },
					{ L"ValueType", L"float4" }
				};

				const auto CS = Util::ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[Shader::CS_GaussianBlurFilterRGBANxN9x9]));
			}
		}
	}

	return TRUE;
}

BOOL BlurFilter::BlurFilterClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count]{};
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(ShadingConvention::BlurFilter::RootConstant::Default::Count, 0);
	slotRootParameter[RootSignature::Default::SI_InputMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::UO_OutputMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"BlurFilter_GR_Default"));

	return TRUE;
}

BOOL BlurFilter::BlurFilterClass::BuildPipelineStates() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	// GaussianBlurFilter3x3
	{
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilter3x3]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilter3x3]),
			L"BlurFilter_CP_GaussianBlurFilter3x3"));
	}
	// GaussianBlurFilterRGBA3x3
	{
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterRGBA3x3]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterRGBA3x3]),
			L"BlurFilter_CP_GaussianBlurFilterRGBA3x3"));
	}
	// GaussianBlurFilterNxN
	{
		// R
		{
			// 3x3
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterNxN3x3]);
					NullCheck(mpLogFile, CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterNxN3x3]),
					L"BlurFilter_CP_GaussianBlurFilterNxN3x3"));
			}
			// 5x5
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterNxN5x5]);
					NullCheck(mpLogFile, CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterNxN5x5]),
					L"BlurFilter_CP_GaussianBlurFilterNxN5x5"));
			}
			// 7x7
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterNxN7x7]);
					NullCheck(mpLogFile, CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterNxN7x7]),
					L"BlurFilter_CP_GaussianBlurFilterNxN7x7"));
			}
			// 9x9
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterNxN9x9]);
					NullCheck(mpLogFile, CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterNxN9x9]),
					L"BlurFilter_CP_GaussianBlurFilterNxN9x9"));
			}
		}
		// RGBA
		{
			// 3x3
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterRGBANxN3x3]);
					NullCheck(mpLogFile, CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterRGBANxN3x3]),
					L"BlurFilter_CP_GaussianBlurFilterRGBANxN3x3"));
			}
			// 5x5
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterRGBANxN5x5]);
					NullCheck(mpLogFile, CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterRGBANxN5x5]),
					L"BlurFilter_CP_GaussianBlurFilterRGBANxN5x5"));
			}
			// 7x7
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterRGBANxN7x7]);
					NullCheck(mpLogFile, CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterRGBANxN7x7]),
					L"BlurFilter_CP_GaussianBlurFilterRGBANxN7x7"));
			}
			// 9x9
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterRGBANxN9x9]);
					NullCheck(mpLogFile, CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterRGBANxN9x9]),
					L"BlurFilter_CP_GaussianBlurFilterRGBANxN9x9"));
			}
		}
	}
	
	return TRUE;
}

BOOL BlurFilter::BlurFilterClass::GaussianBlur(
		Foundation::Resource::FrameResource* const pFrameResource,
		PipelineState::Type type,
		Foundation::Resource::GpuResource* const pInputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
		Foundation::Resource::GpuResource* const pOutputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
		UINT texWidth, UINT texHeight) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[type].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(mRootSignature.Get());

		ShadingConvention::BlurFilter::RootConstant::Default::Struct rc;
		rc.gTexDim.x = static_cast<FLOAT>(texWidth);
		rc.gTexDim.y = static_cast<FLOAT>(texHeight);
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(texWidth);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(texHeight);

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::BlurFilter::RootConstant::Default::Struct>(
			RootSignature::Default::RC_Consts,
			ShadingConvention::BlurFilter::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		pInputMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pOutputMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, pOutputMap);

		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::SI_InputMap, si_inputMap);
		CmdList->SetComputeRootDescriptorTable(RootSignature::Default::UO_OutputMap, uo_outputMap);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(texWidth, ShadingConvention::BlurFilter::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(texHeight, ShadingConvention::BlurFilter::ThreadGroup::Default::Height),
			ShadingConvention::BlurFilter::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}