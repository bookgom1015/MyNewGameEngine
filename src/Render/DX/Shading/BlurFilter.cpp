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

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_GaussianBlurFilter3x3CS = L"GaussianBlurFilter3x3CS.hlsl";
	const WCHAR* const HLSL_GaussianBlurFilterRG3x3CS = L"GaussianBlurFilterRG3x3CS.hlsl";
	const WCHAR* const HLSL_GaussianBlurFilterNxNCS = L"GaussianBlurFilterNxNCS.hlsl";
	const WCHAR* const HLSL_GaussianBlurFilterRGBANxN = L"GaussianBlurFilterRGBANxN.hlsl";
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

BOOL BlurFilter::BlurFilterClass::CompileShaders() {
	// GaussianBlurFilter3x3CS
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GaussianBlurFilter3x3CS, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_GaussianBlurFilter3x3CS]));
	}
	// GaussianBlurFilterRG3x3CS
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GaussianBlurFilterRG3x3CS, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_GaussianBlurFilterRG3x3CS]));
	}
	// GaussianBlurFilterNxNCS
	{
		// 3x3
		{
			DxcDefine defines[] = { { L"KERNEL_RADIUS", L"1" } };

			const auto CS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterNxNCS, L"CS", L"cs_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				CS, mShaderHashes[Shader::CS_GaussianBlurFilterNxNCS_3x3]));
		}
		// 5x5
		{
			DxcDefine defines[] = { { L"KERNEL_RADIUS", L"2" } };

			const auto CS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterNxNCS, L"CS", L"cs_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				CS, mShaderHashes[Shader::CS_GaussianBlurFilterNxNCS_5x5]));
		}
		// 7x7
		{
			DxcDefine defines[] = { { L"KERNEL_RADIUS", L"3" } };

			const auto CS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterNxNCS, L"CS", L"cs_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				CS, mShaderHashes[Shader::CS_GaussianBlurFilterNxNCS_7x7]));
		}
		// 9x9
		{
			DxcDefine defines[] = { { L"KERNEL_RADIUS", L"4" } };

			const auto CS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterNxNCS, L"CS", L"cs_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				CS, mShaderHashes[Shader::CS_GaussianBlurFilterNxNCS_9x9]));
		}
	}
	// GaussianBlurFilterRGBANxN
	{
		// 3x3
		{
			DxcDefine defines[] = { { L"KERNEL_RADIUS", L"1" } };

			const auto VS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"VS", L"vs_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				VS, mShaderHashes[Shader::VS_GaussianBlurFilterRGBANxN_3x3]));
			const auto MS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"MS", L"ms_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				MS, mShaderHashes[Shader::MS_GaussianBlurFilterRGBANxN_3x3]));
			const auto PS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				PS, mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_3x3]));
		}
		// 5x5
		{
			DxcDefine defines[] = { { L"KERNEL_RADIUS", L"2" } };

			const auto VS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"VS", L"vs_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				VS, mShaderHashes[Shader::VS_GaussianBlurFilterRGBANxN_5x5]));
			const auto MS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"MS", L"ms_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				MS, mShaderHashes[Shader::MS_GaussianBlurFilterRGBANxN_5x5]));
			const auto PS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				PS, mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_5x5]));
		}
		// 7x7
		{
			DxcDefine defines[] = { { L"KERNEL_RADIUS", L"3" } };

			const auto VS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"VS", L"vs_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				VS, mShaderHashes[Shader::VS_GaussianBlurFilterRGBANxN_7x7]));
			const auto MS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"MS", L"ms_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				MS, mShaderHashes[Shader::MS_GaussianBlurFilterRGBANxN_7x7]));
			const auto PS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				PS, mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_7x7]));
		}
		// 9x9
		{
			DxcDefine defines[] = { { L"KERNEL_RADIUS", L"4" } };

			const auto VS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"VS", L"vs_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				VS, mShaderHashes[Shader::VS_GaussianBlurFilterRGBANxN_9x9]));
			const auto MS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"MS", L"ms_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				MS, mShaderHashes[Shader::MS_GaussianBlurFilterRGBANxN_9x9]));
			const auto PS = Util::ShaderManager::D3D12ShaderInfo(
				HLSL_GaussianBlurFilterRGBANxN, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
				PS, mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_9x9]));
		}
	}

	return TRUE;
}

BOOL BlurFilter::BlurFilterClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[2] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(ShadingConvention::BlurFilter::RootConstant::Default::Count, 0);
	slotRootParameter[RootSignature::Default::SI_InputMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::UO_OutputMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
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
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	// GaussianBlurFilter3x3
	{
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilter3x3CS]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilter3x3]),
			L"BlurFilter_CP_GaussianBlurFilter3x3"));
	}
	// GaussianBlurFilterRG3x3
	{
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterRG3x3CS]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterRG3x3]),
			L"BlurFilter_CP_GaussianBlurFilterRG3x3"));
	}
	// GaussianBlurFilterNxN
	{
		// 3x3
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterNxNCS_3x3]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterNxN_3x3]),
				L"BlurFilter_CP_GaussianBlurFilterNxN_3x3"));
		}
		// 5x5
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterNxNCS_5x5]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterNxN_5x5]),
				L"BlurFilter_CP_GaussianBlurFilterNxN_5x5"));
		}
		// 7x7
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterNxNCS_7x7]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterNxN_7x7]),
				L"BlurFilter_CP_GaussianBlurFilterNxN_7x7"));
		}
		// 9x9
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_GaussianBlurFilterNxNCS_9x9]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_GaussianBlurFilterNxN_9x9]),
				L"BlurFilter_CP_GaussianBlurFilterNxN_9x9"));
		}
	}
	// GaussianBlurFilterRGBANxN
	{
		// 3x3
		{
			if (mInitData.MeshShaderSupported) {
				auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
				psoDesc.pRootSignature = mRootSignature.Get();
				{
					const auto MS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::MS_GaussianBlurFilterRGBANxN_3x3]);
					NullCheck(mpLogFile, MS);
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_3x3]);
					NullCheck(mpLogFile, PS);
					psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}
				psoDesc.RTVFormats[0] = HDR_FORMAT;

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_GaussianBlurFilterNxN_3x3]),
					L"BlurFilter_MP_GaussianBlurFilterNxN_3x3"));
			}
			else {
				auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
				psoDesc.pRootSignature = mRootSignature.Get();
				{
					const auto VS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::VS_GaussianBlurFilterRGBANxN_3x3]);
					NullCheck(mpLogFile, VS);
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_3x3]);
					NullCheck(mpLogFile, PS);
					psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}
				psoDesc.RTVFormats[0] = HDR_FORMAT;

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_GaussianBlurFilterNxN_3x3]),
					L"BlurFilter_GP_GaussianBlurFilterNxN_3x3"));
			}
		}
		// 5x5
		{
			if (mInitData.MeshShaderSupported) {
				auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
				psoDesc.pRootSignature = mRootSignature.Get();
				{
					const auto MS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::MS_GaussianBlurFilterRGBANxN_5x5]);
					NullCheck(mpLogFile, MS);
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_5x5]);
					NullCheck(mpLogFile, PS);
					psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}
				psoDesc.RTVFormats[0] = HDR_FORMAT;

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_GaussianBlurFilterNxN_5x5]),
					L"BlurFilter_MP_GaussianBlurFilterNxN_5x5"));
			}
			else {
				auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
				psoDesc.pRootSignature = mRootSignature.Get();
				{
					const auto VS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::VS_GaussianBlurFilterRGBANxN_5x5]);
					NullCheck(mpLogFile, VS);
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_5x5]);
					NullCheck(mpLogFile, PS);
					psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}
				psoDesc.RTVFormats[0] = HDR_FORMAT;

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_GaussianBlurFilterNxN_5x5]),
					L"BlurFilter_GP_GaussianBlurFilterNxN_5x5"));
			}
		}
		// 7x7
		{
			if (mInitData.MeshShaderSupported) {
				auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
				psoDesc.pRootSignature = mRootSignature.Get();
				{
					const auto MS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::MS_GaussianBlurFilterRGBANxN_7x7]);
					NullCheck(mpLogFile, MS);
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_7x7]);
					NullCheck(mpLogFile, PS);
					psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}
				psoDesc.RTVFormats[0] = HDR_FORMAT;

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_GaussianBlurFilterNxN_7x7]),
					L"BlurFilter_MP_GaussianBlurFilterNxN_7x7"));
			}
			else {
				auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
				psoDesc.pRootSignature = mRootSignature.Get();
				{
					const auto VS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::VS_GaussianBlurFilterRGBANxN_7x7]);
					NullCheck(mpLogFile, VS);
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_7x7]);
					NullCheck(mpLogFile, PS);
					psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}
				psoDesc.RTVFormats[0] = HDR_FORMAT;

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_GaussianBlurFilterNxN_7x7]),
					L"BlurFilter_GP_GaussianBlurFilterNxN_7x7"));
			}
		}
		// 9x9
		{
			if (mInitData.MeshShaderSupported) {
				auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
				psoDesc.pRootSignature = mRootSignature.Get();
				{
					const auto MS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::MS_GaussianBlurFilterRGBANxN_9x9]);
					NullCheck(mpLogFile, MS);
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_9x9]);
					NullCheck(mpLogFile, PS);
					psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}
				psoDesc.RTVFormats[0] = HDR_FORMAT;

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_GaussianBlurFilterNxN_9x9]),
					L"BlurFilter_MP_GaussianBlurFilterNxN_9x9"));
			}
			else {
				auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
				psoDesc.pRootSignature = mRootSignature.Get();
				{
					const auto VS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::VS_GaussianBlurFilterRGBANxN_9x9]);
					NullCheck(mpLogFile, VS);
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_GaussianBlurFilterRGBANxN_9x9]);
					NullCheck(mpLogFile, PS);
					psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}
				psoDesc.RTVFormats[0] = HDR_FORMAT;

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_GaussianBlurFilterNxN_9x9]),
					L"BlurFilter_GP_GaussianBlurFilterNxN_9x9"));
			}
		}
	}
	return TRUE;
}

BOOL BlurFilter::BlurFilterClass::GaussianBlur(
		Foundation::Resource::FrameResource* const pFrameResource,
		BlurType::Compute type,
		Foundation::Resource::GpuResource* const pInputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
		Foundation::Resource::GpuResource* const pOutputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
		UINT texWidth, UINT texHeight) {
	PipelineState::Type ps;
	switch (type) {
	case BlurType::Compute::C_GaussianBlurFilter3x3:
		ps = PipelineState::CP_GaussianBlurFilter3x3; break;
	case BlurType::Compute::C_GaussianBlurFilterRG3x3:
		ps = PipelineState::CP_GaussianBlurFilterRG3x3; break;
	case BlurType::Compute::C_GaussianBlurFilterNxN_3x3:
		ps = PipelineState::CP_GaussianBlurFilterNxN_3x3; break;
	case BlurType::Compute::C_GaussianBlurFilterNxN_5x5:
		ps = PipelineState::CP_GaussianBlurFilterNxN_5x5; break;
	case BlurType::Compute::C_GaussianBlurFilterNxN_7x7:
		ps = PipelineState::CP_GaussianBlurFilterNxN_7x7; break;
	case BlurType::Compute::C_GaussianBlurFilterNxN_9x9:
		ps = PipelineState::CP_GaussianBlurFilterNxN_9x9; break;
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[ps].Get()));

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

BOOL BlurFilter::BlurFilterClass::GaussianBlur(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		BlurType::Graphics type,
		Foundation::Resource::GpuResource* const pInputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
		Foundation::Resource::GpuResource* const pOutputMap,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_outputMap,
		UINT texWidth, UINT texHeight) {
	PipelineState::Type ps;
	if (mInitData.MeshShaderSupported) {
		switch (type) {
		case BlurType::Graphics::G_GaussianBlurFilterNxN_3x3:
			ps = PipelineState::MP_GaussianBlurFilterNxN_3x3; break;
		case BlurType::Graphics::G_GaussianBlurFilterNxN_5x5:
			ps = PipelineState::MP_GaussianBlurFilterNxN_5x5; break;
		case BlurType::Graphics::G_GaussianBlurFilterNxN_7x7:
			ps = PipelineState::MP_GaussianBlurFilterNxN_7x7; break;
		case BlurType::Graphics::G_GaussianBlurFilterNxN_9x9:
			ps = PipelineState::MP_GaussianBlurFilterNxN_9x9; break;
		}
	}
	else {
		switch (type) {
		case BlurType::Graphics::G_GaussianBlurFilterNxN_3x3:
			ps = PipelineState::GP_GaussianBlurFilterNxN_3x3; break;
		case BlurType::Graphics::G_GaussianBlurFilterNxN_5x5:
			ps = PipelineState::GP_GaussianBlurFilterNxN_5x5; break;
		case BlurType::Graphics::G_GaussianBlurFilterNxN_7x7:
			ps = PipelineState::GP_GaussianBlurFilterNxN_7x7; break;
		case BlurType::Graphics::G_GaussianBlurFilterNxN_9x9:
			ps = PipelineState::GP_GaussianBlurFilterNxN_9x9; break;
		}
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[ps].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pInputMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pOutputMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

		CmdList->OMSetRenderTargets(1, &ro_outputMap, TRUE, nullptr);

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
			FALSE);

		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_InputMap, si_inputMap);

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