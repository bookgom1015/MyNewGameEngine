#include "Render/DX/Shading/DOF.hpp"
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
	const WCHAR* const HLSL_CalcFocalDistance = L"CalcFocalDistance.hlsl";
	const WCHAR* const HLSL_CircleOfConfusion = L"CircleOfConfusion.hlsl";
	const WCHAR* const HLSL_Bokeh = L"Bokeh.hlsl";
	const WCHAR* const HLSL_BokehBlur3x3 = L"BokehBlur3x3.hlsl";
}

DOF::InitDataPtr DOF::MakeInitData() {
	return std::unique_ptr<DOFClass::InitData>(new DOFClass::InitData());
}

DOF::DOFClass::DOFClass() {
	mFocalDistanceBuffer = std::make_unique<Foundation::Resource::GpuResource>();
	mCircleOfConfusionMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT DOF::DOFClass::CbvSrvUavDescCount() const { return 0 
	+ 2 // CircleOfConfusionMap Srv Uav
	; 
}

UINT DOF::DOFClass::RtvDescCount() const { return 0; }

UINT DOF::DOFClass::DsvDescCount() const { return 0; }

BOOL DOF::DOFClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildFixedResources());

	return TRUE;
}

BOOL DOF::DOFClass::CompileShaders() {
	// CaclFocalDistance
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_CalcFocalDistance, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Shader::CS_CaclFocalDistance]));
	}
	// CircleOfConfusion
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_CircleOfConfusion, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Shader::CS_CircleOfConfusion]));
	}
	// Bokeh
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_Bokeh, L"VS", L"vs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[Shader::VS_Bokeh]));
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_Bokeh, L"MS", L"ms_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			MS, mShaderHashes[Shader::MS_Bokeh]));
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_Bokeh, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[Shader::PS_Bokeh]));
	}
	// BokehBlur3x3
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_BokehBlur3x3, L"VS", L"vs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[Shader::VS_BokehBlur3x3]));
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_BokehBlur3x3, L"MS", L"ms_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			MS, mShaderHashes[Shader::MS_BokehBlur3x3]));
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_BokehBlur3x3, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[Shader::PS_BokehBlur3x3]));
	}

	return TRUE;
}

BOOL DOF::DOFClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	// CalcFocalDistance
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::CalcFocalDistance::Count] = {};
		slotRootParameter[RootSignature::CalcFocalDistance::CB_Pass].
			InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::CalcFocalDistance::SI_PositionMap].
			InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::CalcFocalDistance::UO_FocalDistanceBuffer].
			InitAsUnorderedAccessView(0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_CalcFocalDistance]),
			L"DOF_GR_CalcFocalDistance"));
	}
	// CircleOfConfusion
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::CircleOfConfusion::Count] = {};
		slotRootParameter[RootSignature::CircleOfConfusion::CB_Pass].
			InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::CircleOfConfusion::RC_Consts].
			InitAsConstants(ShadingConvention::DOF::RootConstant::CircleOfConfusion::Count, 1);
		slotRootParameter[RootSignature::CircleOfConfusion::SI_DepthMap].
			InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::CircleOfConfusion::UI_FocalDistanceBuffer].
			InitAsUnorderedAccessView(0);
		slotRootParameter[RootSignature::CircleOfConfusion::UO_CircleOfConfusionMap].
			InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_CircleOfConfusion]),
			L"DOF_GR_CircleOfConfusion"));
	}
	// Bokeh
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Bokeh::Count] = {};
		slotRootParameter[RootSignature::Bokeh::RC_Consts].
			InitAsConstants(ShadingConvention::DOF::RootConstant::Bokeh::Count, 0);
		slotRootParameter[RootSignature::Bokeh::SI_BackBuffer].
			InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::Bokeh::SI_CoCMap].
			InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_Bokeh]),
			L"DOF_GR_Bokeh"));
	}
	// BokehBlur3x3
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::BokehBlur3x3::Count] = {};
		slotRootParameter[RootSignature::BokehBlur3x3::RC_Consts].
			InitAsConstants(ShadingConvention::DOF::RootConstant::BokehBlur3x3::Count, 0);
		slotRootParameter[RootSignature::BokehBlur3x3::SI_InputMap].
			InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::BokehBlur3x3::SI_CoCMap].
			InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_BokehBlur3x3]),
			L"DOF_GR_BokehBlur3x3"));
	}

	return TRUE;
}

BOOL DOF::DOFClass::BuildPipelineStates() {
	// CalcFocalDistance
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_CalcFocalDistance].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_CaclFocalDistance]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_CalcFocalDistance]),
			L"DOF_CP_CalcFocalDistance"));
	}
	// CircleOfConfusion
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_CircleOfConfusion].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::CS_CircleOfConfusion]);
			NullCheck(mpLogFile, CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::CP_CircleOfConfusion]),
			L"DOF_CP_CircleOfConfusion"));
	}
	// Bokeh
	{
		// MeshPipelineState
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_Bokeh].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::MS_Bokeh]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::PS_Bokeh]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_Bokeh]),
				L"DOF_MP_Bokeh"));
		}
		// GraphicsPipelineState
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_Bokeh].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::VS_Bokeh]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::PS_Bokeh]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_Bokeh]),
				L"DOF_GP_Bokeh"));
		}
	}
	// BokehBlur3x3
	{
		// MeshPipelineState
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_BokehBlur3x3].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::MS_BokehBlur3x3]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::PS_BokehBlur3x3]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_BokehBlur3x3]),
				L"DOF_MP_BokehBlur3x3"));
		}
		// GraphicsPipelineState
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_BokehBlur3x3].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::VS_BokehBlur3x3]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::PS_BokehBlur3x3]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_BokehBlur3x3]),
				L"DOF_GP_BokehBlur3x3"));
		}
	}
	return TRUE;
}

BOOL DOF::DOFClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhCircleOfConfusionMapCpuUav = pDescHeap->CbvSrvUavCpuOffset(1);
	mhCircleOfConfusionMapGpuUav = pDescHeap->CbvSrvUavGpuOffset(1);
	mhCircleOfConfusionMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhCircleOfConfusionMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL DOF::DOFClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL DOF::DOFClass::CalcFocalDistance(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_CalcFocalDistance].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[RootSignature::GR_CalcFocalDistance].Get());

		CmdList->SetComputeRootConstantBufferView(
			RootSignature::CalcFocalDistance::CB_Pass, pFrameResource->MainPassCBAddress());

		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const auto FocalDistBuff = mFocalDistanceBuffer.get();
		FocalDistBuff->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, FocalDistBuff);

		CmdList->SetComputeRootDescriptorTable(
			RootSignature::CalcFocalDistance::SI_PositionMap, si_positionMap);
		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::CalcFocalDistance::UO_FocalDistanceBuffer, 
			mFocalDistanceBuffer->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientWidth, ShadingConvention::DOF::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientHeight, ShadingConvention::DOF::ThreadGroup::Default::Height),
			ShadingConvention::DOF::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL DOF::DOFClass::CircleOfConfusion(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::CP_CircleOfConfusion].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[RootSignature::GR_CircleOfConfusion].Get());

		CmdList->SetComputeRootConstantBufferView(
			RootSignature::CalcFocalDistance::CB_Pass, pFrameResource->MainPassCBAddress());

		ShadingConvention::DOF::RootConstant::CircleOfConfusion::Struct rc;
		rc.gFocusRange = 16.f;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<
			ShadingConvention::DOF::RootConstant::CircleOfConfusion::Struct>(
				RootSignature::CircleOfConfusion::RC_Consts,			
				ShadingConvention::DOF::RootConstant::CircleOfConfusion::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const auto FocalDistBuff = mFocalDistanceBuffer.get();
		FocalDistBuff->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Foundation::Util::D3D12Util::UavBarrier(CmdList, FocalDistBuff);

		CmdList->SetComputeRootDescriptorTable(
			RootSignature::CircleOfConfusion::SI_DepthMap, si_depthMap);
		CmdList->SetComputeRootUnorderedAccessView(
			RootSignature::CircleOfConfusion::UI_FocalDistanceBuffer,
			mFocalDistanceBuffer->Resource()->GetGPUVirtualAddress());
		CmdList->SetComputeRootDescriptorTable(
			RootSignature::CircleOfConfusion::UO_CircleOfConfusionMap, 
			mhCircleOfConfusionMapGpuUav);

		CmdList->Dispatch(
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientWidth, ShadingConvention::DOF::ThreadGroup::Default::Width),
			Foundation::Util::D3D12Util::CeilDivide(
				mInitData.ClientHeight, ShadingConvention::DOF::ThreadGroup::Default::Height),
			ShadingConvention::DOF::ThreadGroup::Default::Depth);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL DOF::DOFClass::Bokeh(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? 
		PipelineState::MP_Bokeh : PipelineState::GP_Bokeh].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_Bokeh].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mCircleOfConfusionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		ShadingConvention::DOF::RootConstant::Bokeh::Struct rc;
		rc.gInvTexDim = { 
			1.f / static_cast<float>(mInitData.ClientWidth),
			1.f / static_cast<float>(mInitData.ClientHeight) };
		rc.gSampleCount = 4;
		rc.gBokehRadius = 2.f;
		rc.gThreshold = 0.9f;
		rc.gHighlightPower = 2.f;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<
			ShadingConvention::DOF::RootConstant::Bokeh::Struct>(
				RootSignature::Bokeh::RC_Consts,
				ShadingConvention::DOF::RootConstant::Bokeh::Count,
				&rc,
				0,
				CmdList,
				FALSE);

		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::Bokeh::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::Bokeh::SI_CoCMap, mhCircleOfConfusionMapGpuSrv);

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

BOOL DOF::DOFClass::BokehBlur(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ?
		PipelineState::MP_BokehBlur3x3 : PipelineState::GP_BokehBlur3x3].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_BokehBlur3x3].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mCircleOfConfusionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		ShadingConvention::DOF::RootConstant::BokehBlur3x3::Struct rc;
		rc.gTexDim = { mInitData.ClientWidth, mInitData.ClientHeight };

		Foundation::Util::D3D12Util::SetRoot32BitConstants<
			ShadingConvention::DOF::RootConstant::BokehBlur3x3::Struct>(
				RootSignature::BokehBlur3x3::RC_Consts,
				ShadingConvention::DOF::RootConstant::BokehBlur3x3::Count,
				&rc,
				0,
				CmdList,
				FALSE);

		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::BokehBlur3x3::SI_InputMap, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::BokehBlur3x3::SI_CoCMap, mhCircleOfConfusionMapGpuSrv);

		for (UINT i = 0; i < 3; ++i) {
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
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL DOF::DOFClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = mInitData.ClientWidth;
	texDesc.Height = mInitData.ClientHeight;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// CircleOfConfusion
	{
		texDesc.Format = ShadingConvention::DOF::CircleOfConfusionMapFormat;

		CheckReturn(mpLogFile, mCircleOfConfusionMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"DOF_CircleOfConfusionMap"));
	}

	return TRUE;
}

BOOL DOF::DOFClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	// CircleOfConfusion
	{
		srvDesc.Format = ShadingConvention::DOF::CircleOfConfusionMapFormat;
		uavDesc.Format = ShadingConvention::DOF::CircleOfConfusionMapFormat;

		const auto resource = mCircleOfConfusionMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(
			mInitData.Device, resource, &srvDesc, mhCircleOfConfusionMapCpuSrv);
		Foundation::Util::D3D12Util::CreateUnorderedAccessView(
			mInitData.Device, resource, nullptr, &uavDesc, mhCircleOfConfusionMapCpuUav);
	}

	return TRUE;
}

BOOL DOF::DOFClass::BuildFixedResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Format = DXGI_FORMAT_UNKNOWN;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	texDesc.Width = 1 * sizeof(float);
	texDesc.Height = 1;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CheckReturn(mpLogFile, mFocalDistanceBuffer->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"DOF_FocalDistanceBuffer"));

	return TRUE;
}