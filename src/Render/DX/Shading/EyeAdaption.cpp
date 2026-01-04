#include "Render/DX/Shading/EyeAdaption.hpp"
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
	const WCHAR* const HLSL_ClearHistogram = L"ClearHistogram.hlsl";
	const WCHAR* const HLSL_LuminanceHistogram = L"LuminanceHistogram.hlsl";
	const WCHAR* const HLSL_PercentileExtract = L"PercentileExtract.hlsl";
	const WCHAR* const HLSL_TemporalSmoothing = L"TemporalSmoothing.hlsl";
}

EyeAdaption::InitDataPtr EyeAdaption::MakeInitData() {
	return std::unique_ptr<EyeAdaptionClass::InitData>(new EyeAdaptionClass::InitData());
}

EyeAdaption::EyeAdaptionClass::EyeAdaptionClass() {
	mHistogramBuffer = std::make_unique<Foundation::Resource::GpuResource>();
	mAvgLogLuminance = std::make_unique<Foundation::Resource::GpuResource>();
	mPrevLuminance = std::make_unique<Foundation::Resource::GpuResource>();
	mSmoothedLuminance = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT EyeAdaption::EyeAdaptionClass::CbvSrvUavDescCount() const { return 0; }

UINT EyeAdaption::EyeAdaptionClass::RtvDescCount() const { return 0; }

UINT EyeAdaption::EyeAdaptionClass::DsvDescCount() const { return 0; }


BOOL EyeAdaption::EyeAdaptionClass::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::CompileShaders() {
	// ClearHistogram
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_ClearHistogram, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Shader::CS_ClearHistogram]));
	}
	// LuminanceHistogram
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_LuminanceHistogram, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Shader::CS_LuminanceHistogram]));
	}
	// PercentileExtract
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_PercentileExtract, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Shader::CS_PercentileExtract]));
	}
	// TemporalSmoothing
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_TemporalSmoothing, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Shader::CS_TemporalSmoothing]));
	}
	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// LuminanceHistogram
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::LuminanceHistogram::Count] = {};
		slotRootParameter[RootSignature::LuminanceHistogram::RC_Consts]
			.InitAsConstants(
				ShadingConvention::EyeAdaption::RootConstant::LuminanceHistogram::Count,
				0);
		slotRootParameter[RootSignature::LuminanceHistogram::SI_BackBuffer].
			InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::LuminanceHistogram::UO_HistogramBuffer].
			InitAsUnorderedAccessView(0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_LuminanceHistogram]),
			L"EyeAdaption_GR_LuminanceHistogram"));
	}
	// PercentileExtract
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::PercentileExtract::Count] = {};
		slotRootParameter[RootSignature::PercentileExtract::RC_Consts]
			.InitAsConstants(
				ShadingConvention::EyeAdaption::RootConstant::PercentileExtract::Count,
				0);
		slotRootParameter[RootSignature::PercentileExtract::UI_HistogramBuffer].
			InitAsUnorderedAccessView(0);
		slotRootParameter[RootSignature::PercentileExtract::UO_AvgLogLuminance].
			InitAsUnorderedAccessView(1);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_PercentileExtract]),
			L"EyeAdaption_GR_PercentileExtract"));
	}
	// TemporalSmoothing
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::TemporalSmoothing::Count] = {};
		slotRootParameter[RootSignature::TemporalSmoothing::RC_Consts]
			.InitAsConstants(
				ShadingConvention::EyeAdaption::RootConstant::TemporalSmoothing::Count,
				0);
		slotRootParameter[RootSignature::TemporalSmoothing::UI_AvgLogLuminance].
			InitAsUnorderedAccessView(0);
		slotRootParameter[RootSignature::TemporalSmoothing::UO_SmoothedLum].
			InitAsUnorderedAccessView(1);
		slotRootParameter[RootSignature::TemporalSmoothing::UIO_PrevLum].
			InitAsUnorderedAccessView(2);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_TemporalSmoothing]),
			L"EyeAdaption_GR_TemporalSmoothing"));
	}

	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::BuildPipelineStates() {
	// ClearHistogram
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[
			RootSignature::GR_LuminanceHistogram].Get();
			psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::CS_ClearHistogram]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = {
					reinterpret_cast<BYTE*>(CS->GetBufferPointer()),
					CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_ClearHistogram]),
				L"EyeAdaption_CP_ClearHistogram"));
	}
	// LuminanceHistogram
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[
			RootSignature::GR_LuminanceHistogram].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Shader::CS_LuminanceHistogram]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { 
				reinterpret_cast<BYTE*>(CS->GetBufferPointer()), 
				CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_LuminanceHistogram]),
			L"EyeAdaption_CP_LuminanceHistogram"));
	}
	// PercentileExtract
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[
			RootSignature::GR_PercentileExtract].Get();
			psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::CS_PercentileExtract]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = {
					reinterpret_cast<BYTE*>(CS->GetBufferPointer()),
					CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_PercentileExtract]),
				L"EyeAdaption_CP_PercentileExtract"));
	}
	// TemporalSmoothing
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[
			RootSignature::GR_TemporalSmoothing].Get();
			psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::CS_TemporalSmoothing]);
				NullCheck(mpLogFile, CS);
				psoDesc.CS = {
					reinterpret_cast<BYTE*>(CS->GetBufferPointer()),
					CS->GetBufferSize() };
			}

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_TemporalSmoothing]),
				L"EyeAdaption_CP_TemporalSmoothing"));
	}

	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::ClearHistogram(
		Foundation::Resource::FrameResource* const pFrameResource) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_ClearHistogram].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[RootSignature::GR_LuminanceHistogram].Get());

		const auto Histogram = mHistogramBuffer.get();
		Histogram->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, Histogram);

		ShadingConvention::EyeAdaption::RootConstant::LuminanceHistogram::Struct rc;
		rc.gTexDim = { mInitData.ClientWidth, mInitData.ClientHeight };
		rc.gMinLogLum = -8.f;
		rc.gMaxLogLum = 4.f;
		rc.gBinCount = 64;
		Foundation::Util::D3D12Util::SetRoot32BitConstants<
			ShadingConvention::EyeAdaption::RootConstant::LuminanceHistogram::Struct>(
				RootSignature::LuminanceHistogram::RC_Consts,
				ShadingConvention::EyeAdaption::RootConstant::LuminanceHistogram::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::LuminanceHistogram::UO_HistogramBuffer,
			mHistogramBuffer->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(MAX_BIN_COUNT, 1, 1);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::BuildLuminanceHistogram(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_LuminanceHistogram].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[RootSignature::GR_LuminanceHistogram].Get());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const auto Histogram = mHistogramBuffer.get();
		Histogram->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, Histogram);

		ShadingConvention::EyeAdaption::RootConstant::LuminanceHistogram::Struct rc;
		rc.gTexDim = { mInitData.ClientWidth >> 1, mInitData.ClientHeight >> 1 };
		rc.gMinLogLum = -8.f;
		rc.gMaxLogLum = 4.f;
		rc.gBinCount = 64;
		Foundation::Util::D3D12Util::SetRoot32BitConstants<
			ShadingConvention::EyeAdaption::RootConstant::LuminanceHistogram::Struct>(
				RootSignature::LuminanceHistogram::RC_Consts,
				ShadingConvention::EyeAdaption::RootConstant::LuminanceHistogram::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		CmdList->SetComputeRootDescriptorTable(
			RootSignature::LuminanceHistogram::SI_BackBuffer, si_backBuffer);
		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::LuminanceHistogram::UO_HistogramBuffer,
			mHistogramBuffer->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientWidth >> 1, 
				ShadingConvention::EyeAdaption::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientHeight >> 1, 
				ShadingConvention::EyeAdaption::ThreadGroup::Default::Height),
			ShadingConvention::EyeAdaption::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::PercentileExtract(
		Foundation::Resource::FrameResource* const pFrameResource) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_PercentileExtract].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[RootSignature::GR_PercentileExtract].Get());

		const auto Histogram = mHistogramBuffer.get();
		Histogram->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, Histogram);

		ShadingConvention::EyeAdaption::RootConstant::PercentileExtract::Struct rc;
		rc.gMinLogLum = -8.f;
		rc.gMaxLogLum = 4.f;
		rc.gLowPercent = 0.1f;
		rc.gHighPercent = 0.99f;
		rc.gBinCount = 64;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<
			ShadingConvention::EyeAdaption::RootConstant::PercentileExtract::Struct>(
				RootSignature::PercentileExtract::RC_Consts,
				ShadingConvention::EyeAdaption::RootConstant::PercentileExtract::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::PercentileExtract::UI_HistogramBuffer, 
			mHistogramBuffer->Resource()->GetGPUVirtualAddress());
		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::PercentileExtract::UO_AvgLogLuminance,
			mAvgLogLuminance->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(1, 1, 1);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::TemporalSmoothing(
		Foundation::Resource::FrameResource* const pFrameResource,
		FLOAT deltaTime) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_TemporalSmoothing].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[RootSignature::GR_TemporalSmoothing].Get());

		const auto AvgLogLum = mAvgLogLuminance.get();
		AvgLogLum->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, AvgLogLum);

		const auto Smoothed = mSmoothedLuminance.get();
		Smoothed->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, Smoothed);

		const auto Prev = mPrevLuminance.get();
		Prev->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, Prev);

		ShadingConvention::EyeAdaption::RootConstant::TemporalSmoothing::Struct rc;
		rc.gUpSpeed = 1.25f;
		rc.gGlareUpSpeed = 0.8f;
		rc.gDownSpeed = 3.f;
		rc.gDeltaTime = deltaTime;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<
			ShadingConvention::EyeAdaption::RootConstant::TemporalSmoothing::Struct>(
				RootSignature::TemporalSmoothing::RC_Consts,
				ShadingConvention::EyeAdaption::RootConstant::TemporalSmoothing::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::TemporalSmoothing::UI_AvgLogLuminance,
			mAvgLogLuminance->Resource()->GetGPUVirtualAddress());
		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::TemporalSmoothing::UO_SmoothedLum,
			mSmoothedLuminance->Resource()->GetGPUVirtualAddress());
		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::TemporalSmoothing::UIO_PrevLum,
			mPrevLuminance->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(1, 1, 1);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Format = DXGI_FORMAT_UNKNOWN;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	texDesc.Height = 1;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// HistogramBuffer
	{
		texDesc.Width = MAX_BIN_COUNT * sizeof(ShadingConvention::EyeAdaption::HistogramBin);

		CheckReturn(mpLogFile, mHistogramBuffer->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EyeAdaption_HistogramBuffer"));
	}
	// AvgLogLuminance
	{
		texDesc.Width = sizeof(ShadingConvention::EyeAdaption::Result);

		CheckReturn(mpLogFile, mAvgLogLuminance->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EyeAdaption_AvgLogLuminance"));
	}
	// Smoothed/Prev-Luminance
	{
		texDesc.Width = sizeof(FLOAT);

		CheckReturn(mpLogFile, mPrevLuminance->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EyeAdaption_PrevLuminance"));
		CheckReturn(mpLogFile, mSmoothedLuminance->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EyeAdaption_SmoothedLuminance"));
	}


	return TRUE;
}