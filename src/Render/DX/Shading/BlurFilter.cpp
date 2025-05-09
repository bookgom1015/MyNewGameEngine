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

		std::array<std::uint32_t, ShadingConvention::BlurFilter::RootConstant::Default::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::BlurFilter::RootConstant::Default::Struct));

		CmdList->SetComputeRoot32BitConstants(RootSignature::Default::RC_Consts, ShadingConvention::BlurFilter::RootConstant::Default::Count, consts.data(), 0);

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