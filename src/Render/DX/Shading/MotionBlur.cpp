#include "Render/DX/Shading/MotionBlur.hpp"
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
	const WCHAR* const HLSL_MotionBlur = L"MotionBlur.hlsl";
}

MotionBlur::InitDataPtr MotionBlur::MakeInitData() {
	return std::unique_ptr<MotionBlurClass::InitData>(new MotionBlurClass::InitData());
}

MotionBlur::MotionBlurClass::MotionBlurClass() {}

UINT MotionBlur::MotionBlurClass::CbvSrvUavDescCount() const { return 0; }

UINT MotionBlur::MotionBlurClass::RtvDescCount() const { return 0; }

UINT MotionBlur::MotionBlurClass::DsvDescCount() const { return 0; }


BOOL MotionBlur::MotionBlurClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;
	
	return TRUE;
}

BOOL MotionBlur::MotionBlurClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_MotionBlur, L"VS", L"vs_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_MotionBlur]));

	const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_MotionBlur, L"MS", L"ms_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_MotionBlur]));

	const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_MotionBlur, L"PS", L"ps_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_MotionBlur]));

	return TRUE;
}

BOOL MotionBlur::MotionBlurClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[3] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(
		ShadingConvention::MotionBlur::RootConstant::Default::Count, 0);
	slotRootParameter[RootSignature::Default::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[RootSignature::Default::SI_VelocityMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"MotionBlur_GR_Default"));

	return TRUE;
}

BOOL MotionBlur::MotionBlurClass::BuildPipelineStates() {
	// GraphicsPipelineState
	{
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_MotionBlur]);
			NullCheck(mpLogFile, VS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_MotionBlur]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = SDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_MotionBlur]),
			L"MotionBlur_GP_Default"));
	}
	// MeshPipelineState
	if (mInitData.MeshShaderSupported) {
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_MotionBlur]);
			NullCheck(mpLogFile, MS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_MotionBlur]);
			NullCheck(mpLogFile, PS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = SDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_MotionBlur]),
			L"MotionBlur_MP_Default"));
	}

	return TRUE;
}

BOOL MotionBlur::MotionBlurClass::ApplyMotionBlur(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		Foundation::Resource::GpuResource* const pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		Foundation::Resource::GpuResource* const pVelocityMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap,
		FLOAT intensity, FLOAT limit, FLOAT depthBias, INT sampleCount) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? PipelineState::MP_MotionBlur: PipelineState::GP_MotionBlur].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_READ);
		pVelocityMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		ShadingConvention::MotionBlur::RootConstant::Default::Struct rc;
		rc.gIntensity = intensity;
		rc.gLimit = limit;
		rc.gDepthBias = depthBias;
		rc.gSampleCount = sampleCount;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::MotionBlur::RootConstant::Default::Struct>(
			RootSignature::Default::RC_Consts,
			ShadingConvention::MotionBlur::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_DepthMap, si_depthMap);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::Default::SI_VelocityMap, si_velocityMap);

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