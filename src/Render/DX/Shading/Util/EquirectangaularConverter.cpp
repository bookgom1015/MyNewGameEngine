#include "Render/DX/Shading/Util/EquirectangularConverter.hpp"
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
	const WCHAR* const HLSL_ConvertEquirectangularToCubeMap = L"ConvertEquirectangularToCubeMap.hlsl";
	const WCHAR* const HLSL_ConvertCubeMapToEquirectangular = L"ConvertCubeToEquirectangularMap.hlsl";
}

EquirectangularConverter::InitDataPtr EquirectangularConverter::MakeInitData() {
	return std::unique_ptr<EquirectangularConverterClass::InitData>(new EquirectangularConverterClass::InitData());
}

UINT EquirectangularConverter::EquirectangularConverterClass::CbvSrvUavDescCount() const { return 0; }

UINT EquirectangularConverter::EquirectangularConverterClass::RtvDescCount() const { return 0; }

UINT EquirectangularConverter::EquirectangularConverterClass::DsvDescCount() const { return 0; }

BOOL EquirectangularConverter::EquirectangularConverterClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL EquirectangularConverter::EquirectangularConverterClass::CompileShaders() {
	// ConvertEquirectangularToCubeMap
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvertEquirectangularToCubeMap, L"VS", L"vs_6_5");
		const auto GS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvertEquirectangularToCubeMap, L"GS", L"gs_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvertEquirectangularToCubeMap, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ConvEquirectToCube]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(GS, mShaderHashes[Shader::GS_ConvEquirectToCube]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ConvEquirectToCube]));
	}
	// ConvertCubeMapToEquirectangular
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvertCubeMapToEquirectangular, L"VS", L"vs_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvertCubeMapToEquirectangular, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ConvCubeToEquirect]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ConvCubeToEquirect]));
	}

	return TRUE;
}

BOOL EquirectangularConverter::EquirectangularConverterClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// ConvEquirectToCube
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ConvEquirectToCube::Count] = {};
		slotRootParameter[RootSignature::ConvEquirectToCube::CB_ProjectToCube].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::ConvEquirectToCube::SI_EquirectangularMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ConvEquirectToCube]),
			L"EquirectangularConverter_GR_ConvEquirectToCube"));
	}
	// ConvCubeToEquirect
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ConvCubeToEquirect::Count] = {};
		slotRootParameter[RootSignature::ConvCubeToEquirect::RC_Consts].InitAsConstants(ShadingConvention::EquirectangularConverter::RootConstant::ConvCubeToEquirect::Count, 0);
		slotRootParameter[RootSignature::ConvCubeToEquirect::SI_CubeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ConvCubeToEquirect]),
			L"EquirectangularConverter_GR_ConvCubeToEquirect"));
	}

	return TRUE;
}

BOOL EquirectangularConverter::EquirectangularConverterClass::BuildPipelineStates() {
	// ConvEquirectToCube
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = Foundation::Util::D3D12Util::DefaultPsoDesc({ nullptr, 0 }, DXGI_FORMAT_UNKNOWN);
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ConvEquirectToCube].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ConvEquirectToCube]);
			NullCheck(mpLogFile, VS);
			const auto GS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::GS_ConvEquirectToCube]);
			NullCheck(mpLogFile, GS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ConvEquirectToCube]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.GS = { reinterpret_cast<BYTE*>(GS->GetBufferPointer()), GS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		psoDesc.RTVFormats[0] = HDR_FORMAT;
		psoDesc.DepthStencilState.DepthEnable = FALSE;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ConvEquirectToCube]),
			L"EquirectangularConverter_GP_ConvEquirectToCube"));
	}
	// ConvCubeToEquirect
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ConvCubeToEquirect].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ConvCubeToEquirect]);
			NullCheck(mpLogFile, VS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ConvCubeToEquirect]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ConvCubeToEquirect]),
			L"EquirectangularConverter_GP_ConvCubeToEquirect"));
	}

	return TRUE;
}

BOOL EquirectangularConverter::EquirectangularConverterClass::ConvertEquirectangularToCube(
		Foundation::Resource::GpuResource* const pCube,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_cubes[],
		Foundation::Resource::GpuResource* const pEquirect,
		D3D12_GPU_DESCRIPTOR_HANDLE si_equirect,
		D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube,
		UINT width, UINT height,
		UINT maxMipLevel) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList(mPipelineStates[PipelineState::GP_ConvEquirectToCube].Get()));

	const auto CmdList = mInitData.CommandObject->DirectCommandList();
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ConvEquirectToCube].Get());

	pCube->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pEquirect->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->SetGraphicsRootConstantBufferView(RootSignature::ConvEquirectToCube::CB_ProjectToCube, cbProjectToCube);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::ConvEquirectToCube::SI_EquirectangularMap, si_equirect);
	
	for (UINT mipLevel = 0; mipLevel < maxMipLevel; ++mipLevel) {
		const UINT MW = static_cast<UINT>(width / std::pow(2, mipLevel));
		const UINT MH = static_cast<UINT>(height / std::pow(2, mipLevel));
	
		const D3D12_VIEWPORT Viewport = { 0.f, 0.f, static_cast<FLOAT>(MW), static_cast<FLOAT>(MH), 0.f, 1.f };
		const D3D12_RECT ScissorRect = { 0, 0, static_cast<INT>(MW), static_cast<INT>(MH) };
	
		CmdList->RSSetViewports(1, &Viewport);
		CmdList->RSSetScissorRects(1, &ScissorRect);
	
		CmdList->OMSetRenderTargets(1, &ro_cubes[mipLevel], TRUE, nullptr);
	
		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(36, 1, 0, 0);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());

	return TRUE;
}

BOOL EquirectangularConverter::EquirectangularConverterClass::ConvertEquirectangularToCube(
		Foundation::Resource::GpuResource* const pCube,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_cube,
		Foundation::Resource::GpuResource* const pEquirect,
		D3D12_GPU_DESCRIPTOR_HANDLE si_equirect,
		D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		UINT maxMipLevel) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList(mPipelineStates[PipelineState::GP_ConvEquirectToCube].Get()));

	const auto CmdList = mInitData.CommandObject->DirectCommandList();
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ConvEquirectToCube].Get());

	CmdList->RSSetViewports(1, &viewport);
	CmdList->RSSetScissorRects(1, &scissorRect);

	pCube->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pEquirect->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->SetGraphicsRootConstantBufferView(RootSignature::ConvEquirectToCube::CB_ProjectToCube, cbProjectToCube);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::ConvEquirectToCube::SI_EquirectangularMap, si_equirect);

	CmdList->OMSetRenderTargets(1, &ro_cube, TRUE, nullptr);

	CmdList->IASetVertexBuffers(0, 0, nullptr);
	CmdList->IASetIndexBuffer(nullptr);
	CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CmdList->DrawInstanced(36, 1, 0, 0);

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());

	return TRUE;
}

BOOL EquirectangularConverter::EquirectangularConverterClass::ConvertCubeToEquirectangular(
		Foundation::Resource::GpuResource* const pEquirect,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_equirect,
		Foundation::Resource::GpuResource* const pCube,
		D3D12_GPU_DESCRIPTOR_HANDLE si_cube,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		UINT mipLevel) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList(mPipelineStates[PipelineState::GP_ConvCubeToEquirect].Get()));

	const auto CmdList = mInitData.CommandObject->DirectCommandList();
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ConvCubeToEquirect].Get());

	CmdList->RSSetViewports(1, &viewport);
	CmdList->RSSetScissorRects(1, &scissorRect);

	pEquirect->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCube->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->OMSetRenderTargets(1, &ro_equirect, TRUE, nullptr);

	ShadingConvention::EquirectangularConverter::RootConstant::ConvCubeToEquirect::Struct rc;
	rc.gMipLevel = mipLevel;

	std::array<std::uint32_t, ShadingConvention::EquirectangularConverter::RootConstant::ConvCubeToEquirect::Count> consts;
	std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::EquirectangularConverter::RootConstant::ConvCubeToEquirect::Struct));

	CmdList->SetComputeRoot32BitConstants(RootSignature::ConvCubeToEquirect::RC_Consts, ShadingConvention::EquirectangularConverter::RootConstant::ConvCubeToEquirect::Count, consts.data(), 0);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::ConvCubeToEquirect::SI_CubeMap, si_cube);

	CmdList->IASetVertexBuffers(0, 0, nullptr);
	CmdList->IASetIndexBuffer(nullptr);
	CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CmdList->DrawInstanced(6, 1, 0, 0);

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());

	return TRUE;
}