#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/BRDF.hpp"
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
	const WCHAR* const HLSL_ComputeBRDF = L"ComputeBRDF.hlsl";
	const WCHAR* const HLSL_IntegrateIrradiance = L"IntegrateIrradiance.hlsl";
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

	NullCheck(pLogFile, pData);
	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL BRDF::BRDFClass::CompileShaders() {
	// ComputeBRDF
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ComputeBRDF, L"VS", L"vs_6_5");
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ComputeBRDF, L"MS", L"ms_6_5");

		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ComputeBRDF]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_ComputeBRDF]));

		// BlinnPhong
		{
			DxcDefine defines[] = {
				{ L"BLINN_PHONG", L"1" }
			};

			const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ComputeBRDF, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ComputeBRDF_BlinnPhong]));
		}
		// CookTorrance
		{
			DxcDefine defines[] = {
				{ L"COOK_TORRANCE", L"1" }
			};

			const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ComputeBRDF, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ComputeBRDF_CookTorrance]));
		}
	}
	// IntegrateIrradiance
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateIrradiance, L"VS", L"vs_6_5");
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateIrradiance, L"MS", L"ms_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateIrradiance, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_IntegrateIrradiance]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_IntegrateIrradiance]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_IntegrateIrradiance]));
	}

	return TRUE;
}

BOOL BRDF::BRDFClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// ComputeBRDF
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[7]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ComputeBRDF::Count]{};
		slotRootParameter[RootSignature::ComputeBRDF::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::ComputeBRDF::CB_Light].InitAsConstantBufferView(1);
		slotRootParameter[RootSignature::ComputeBRDF::RC_Consts].InitAsConstants(ShadingConvention::BRDF::RootConstant::ComputeBRDF::Count, 2);
		slotRootParameter[RootSignature::ComputeBRDF::SI_AlbedoMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeBRDF::SI_NormalMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeBRDF::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeBRDF::SI_SpecularMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeBRDF::SI_RoughnessMetalicMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeBRDF::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ComputeBRDF::SI_ShadowMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device, 
			rootSigDesc, 
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ComputeBRDF]),
			L"BRDF_GR_ComputeBRDF"));
	}
	// IntegrateIrradiance
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[12]{}; UINT index = 0;
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
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::IntegrateIrradiance::Count]{};
		slotRootParameter[RootSignature::IntegrateIrradiance::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::IntegrateIrradiance::RC_Consts].InitAsConstants(ShadingConvention::BRDF::RootConstant::IntegrateIrradiance::Count, 1);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_AlbedoMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_NormalMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_SpecularMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_RoughnessMetalicMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_AOMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_ReflectionMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_DiffuseIrradianceCubeMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_BrdfLutMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateIrradiance::SI_PrefilteredEnvCubeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_IntegrateIrradiance]),
			L"BRDF_GR_IntegrateIrradiance"));
	}

	return TRUE;
}

BOOL BRDF::BRDFClass::BuildPipelineStates() {
	// ComputeBRDF
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ComputeBRDF].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_ComputeBRDF]);
				NullCheck(mpLogFile, MS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			// BlinnPhong
			{
				{
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_ComputeBRDF_BlinnPhong]);
					NullCheck(mpLogFile, PS);
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_ComputeBRDF_BlinnPhong]),
					L"BRDF_MP_ComputeBRDF_BlinnPhong"));
			}
			// CookTorrance
			{
				{
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_ComputeBRDF_CookTorrance]);
					NullCheck(mpLogFile, PS);
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_ComputeBRDF_CookTorrance]),
					L"BRDF_MP_ComputeBRDF_CookTorrance"));
			}
		}
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ComputeBRDF].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ComputeBRDF]);
				NullCheck(mpLogFile, VS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			// BlinnPhong
			{
				{
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_ComputeBRDF_BlinnPhong]);
					NullCheck(mpLogFile, PS);
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ComputeBRDF_BlinnPhong]),
					L"BRDF_GP_ComputeBRDF_BlinnPhong"));
			}
			// CookTorrance
			{
				{
					const auto PS = mInitData.ShaderManager->GetShader(
						mShaderHashes[Shader::PS_ComputeBRDF_CookTorrance]);
					NullCheck(mpLogFile, PS);
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ComputeBRDF_CookTorrance]),
					L"BRDF_GP_ComputeBRDF_CookTorrance"));
			}
		}
		
	}
	// IntegrateIrradiance
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_IntegrateIrradiance].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_IntegrateIrradiance]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_IntegrateIrradiance]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_IntegrateIrradiance]),
				L"BRDF_MP_IntegrateIrradiance"));
		}
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_IntegrateIrradiance].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_IntegrateIrradiance]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_IntegrateIrradiance]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_IntegrateIrradiance]),
				L"BRDF_GP_IntegrateIrradiance"));
		}
		
	}

	return TRUE;
}

BOOL BRDF::BRDFClass::ComputeBRDF(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pAlbedoMap, D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap,
		Foundation::Resource::GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		Foundation::Resource::GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		Foundation::Resource::GpuResource* const pSpecularMap, D3D12_GPU_DESCRIPTOR_HANDLE si_specularMap,
		Foundation::Resource::GpuResource* const pRoughnessMetalnessMap, D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap,
		Foundation::Resource::GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		Foundation::Resource::GpuResource* const pShadowMap, D3D12_GPU_DESCRIPTOR_HANDLE si_shadowMap,
		BOOL bShadowEnabled) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? 
		PipelineState::MP_ComputeBRDF_CookTorrance : PipelineState::GP_ComputeBRDF_CookTorrance].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ComputeBRDF].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pAlbedoMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pSpecularMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRoughnessMetalnessMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pShadowMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		CmdList->SetGraphicsRootConstantBufferView(
			RootSignature::ComputeBRDF::CB_Pass, pFrameResource->MainPassCB.CBAddress());
		CmdList->SetGraphicsRootConstantBufferView(
			RootSignature::ComputeBRDF::CB_Light, pFrameResource->LightCB.CBAddress());

		ShadingConvention::BRDF::RootConstant::ComputeBRDF::Struct rc;
		rc.gShadowEnabled = bShadowEnabled;

		std::array<std::uint32_t, ShadingConvention::BRDF::RootConstant::ComputeBRDF::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::BRDF::RootConstant::ComputeBRDF::Struct));

		CmdList->SetGraphicsRoot32BitConstants(RootSignature::ComputeBRDF::RC_Consts, ShadingConvention::BRDF::RootConstant::ComputeBRDF::Count, consts.data(), 0);

		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ComputeBRDF::SI_AlbedoMap, si_albedoMap);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ComputeBRDF::SI_NormalMap, si_normalMap);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ComputeBRDF::SI_DepthMap, si_depthMap);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ComputeBRDF::SI_SpecularMap, si_specularMap);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ComputeBRDF::SI_RoughnessMetalicMap, si_roughnessMetalnessMap);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ComputeBRDF::SI_PositionMap, si_positionMap);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ComputeBRDF::SI_ShadowMap, si_shadowMap);

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

BOOL BRDF::BRDFClass::IntegrateIrradiance(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		Foundation::Resource::GpuResource* const pAlbedoMap, D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap,
		Foundation::Resource::GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		Foundation::Resource::GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		Foundation::Resource::GpuResource* const pSpecularMap, D3D12_GPU_DESCRIPTOR_HANDLE si_specularMap,
		Foundation::Resource::GpuResource* const pRoughnessMetalnessMap, D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap,
		Foundation::Resource::GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		Foundation::Resource::GpuResource* const pAOMap, D3D12_GPU_DESCRIPTOR_HANDLE si_aoMap,
		Foundation::Resource::GpuResource* const pDiffuseIrradianceMap, D3D12_GPU_DESCRIPTOR_HANDLE si_diffuseIrradianceMap,
		Foundation::Resource::GpuResource* const pBrdfLutMap, D3D12_GPU_DESCRIPTOR_HANDLE si_brdfLutMap,
		Foundation::Resource::GpuResource* const pPrefilteredEnvCubeMap, D3D12_GPU_DESCRIPTOR_HANDLE si_prefilteredEnvCubeMap,
		BOOL bAoEnabled) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? 
		PipelineState::MP_IntegrateIrradiance : PipelineState::GP_IntegrateIrradiance].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_IntegrateIrradiance].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pAlbedoMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pSpecularMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRoughnessMetalnessMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pAOMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pDiffuseIrradianceMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pBrdfLutMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPrefilteredEnvCubeMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		CmdList->SetGraphicsRootConstantBufferView(
			RootSignature::IntegrateIrradiance::CB_Pass, 
			pFrameResource->MainPassCB.CBAddress());

		ShadingConvention::BRDF::RootConstant::IntegrateIrradiance::Struct rc;
		rc.gAoEnabled = bAoEnabled;

		std::array<std::uint32_t, ShadingConvention::BRDF::RootConstant::IntegrateIrradiance::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::BRDF::RootConstant::IntegrateIrradiance::Struct));

		CmdList->SetGraphicsRoot32BitConstants(
			RootSignature::IntegrateIrradiance::RC_Consts, 
			ShadingConvention::BRDF::RootConstant::IntegrateIrradiance::Count, 
			consts.data(), 0);

		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_AlbedoMap, si_albedoMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_NormalMap, si_normalMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_DepthMap, si_depthMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_SpecularMap, si_specularMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_RoughnessMetalicMap, si_roughnessMetalnessMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_PositionMap, si_positionMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_AOMap, si_aoMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_DiffuseIrradianceCubeMap, si_diffuseIrradianceMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_BrdfLutMap, si_brdfLutMap);
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::IntegrateIrradiance::SI_PrefilteredEnvCubeMap, si_prefilteredEnvCubeMap);

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
