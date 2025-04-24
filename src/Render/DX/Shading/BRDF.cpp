#include "Render/DX/Shading/BRDF.hpp"
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
	const WCHAR* const HLSL_IntegrateDiffuse = L"IntegrateDiffuse.hlsl";
	const WCHAR* const HLSL_IntegrateSpecular = L"IntegrateSpecular.hlsl";
}

BRDF::InitDataPtr BRDF::MakeInitData() {
	return std::unique_ptr<BRDFClass::InitData>(new BRDFClass::InitData());
}

BRDF::BRDFClass::BRDFClass() {}

UINT BRDF::BRDFClass::CbvSrvUavDescCount() const { return 0; }

UINT BRDF::BRDFClass::RtvDescCount() const { return 0; }

UINT BRDF::BRDFClass::DsvDescCount() const { return 0; }

BOOL BRDF::BRDFClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL BRDF::BRDFClass::CompileShaders() {
	// IntegrateDiffuse
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateDiffuse, L"VS", L"vs_6_5");
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateDiffuse, L"MS", L"ms_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateDiffuse, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_IntegrateDiffuse]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_IntegrateDiffuse]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_IntegrateDiffuse]));
	}
	// IntegrateSpecular
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateSpecular, L"VS", L"vs_6_5");
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateSpecular, L"MS", L"ms_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateSpecular, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_IntegrateSpecular]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_IntegrateSpecular]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_IntegrateSpecular]));
	}

	return TRUE;
}

BOOL BRDF::BRDFClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	// IntegrateDiffuse
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[8] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::IntegrateDiffuse::Count] = {};
		slotRootParameter[RootSignature::IntegrateDiffuse::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_AlbedoMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_NormalMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_RMSMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_ShadowMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_AOMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_DiffuseIrradianceCubeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device, 
			rootSigDesc, 
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_IntegrateDiffuse]),
			L"BRDF_GR_IntegrateDiffuse"));
	}
	// IntegrateSpecular
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[10] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::IntegrateSpecular::Count] = {};
		slotRootParameter[RootSignature::IntegrateSpecular::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_AlbedoMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_NormalMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_RMSMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_AOMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_ReflectionMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_BrdfLutMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_PrefilteredEnvCubeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_IntegrateSpecular]),
			L"BRDF_GR_IntegrateSpecular"));
	}

	return TRUE;
}

BOOL BRDF::BRDFClass::BuildPipelineStates() {
	// IntegrateDiffuse
	{
		// GraphicsPipeline
		{
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_IntegrateDiffuse].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_IntegrateDiffuse]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_IntegrateDiffuse]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_IntegrateDiffuse]),
				L"BRDF_GP_IntegrateDiffuse"));
		}
	}
	// IntegrateSpecular
	{
		auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_IntegrateSpecular].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_IntegrateSpecular]);
			NullCheck(mpLogFile, VS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_IntegrateSpecular]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_IntegrateSpecular]),
			L"BRDF_GP_IntegrateSpecular"));
	}

	return TRUE;
}