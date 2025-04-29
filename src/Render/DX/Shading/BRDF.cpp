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

		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_IntegrateDiffuse]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_IntegrateDiffuse]));

		// BlinnPhong
		{
			DxcDefine defines[] = {
				{ L"BLINN_PHONG", L"1" }
			};			

			const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateDiffuse, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_IntegrateDiffuse_BlinnPhong]));
		}
		// CookTorrance
		{
			DxcDefine defines[] = {
				{ L"COOK_TORRANCE", L"1" }
			};

			const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateDiffuse, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_IntegrateDiffuse_CookTorrance]));
		}
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
		CD3DX12_DESCRIPTOR_RANGE texTables[9] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::IntegrateDiffuse::Count] = {};
		slotRootParameter[RootSignature::IntegrateDiffuse::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_AlbedoMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_NormalMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_SpecularMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateDiffuse::SI_RoughnessMetalicMap].InitAsDescriptorTable(1, &texTables[index++]);
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
		CD3DX12_DESCRIPTOR_RANGE texTables[11] = {}; UINT index = 0;
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

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::IntegrateSpecular::Count] = {};
		slotRootParameter[RootSignature::IntegrateSpecular::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_AlbedoMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_NormalMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_SpecularMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::IntegrateSpecular::SI_RoughnessMetalicMap].InitAsDescriptorTable(1, &texTables[index++]);
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
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			// BlinnPhong
			{
				{
					const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_IntegrateDiffuse_BlinnPhong]);
					NullCheck(mpLogFile, PS);
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_IntegrateDiffuse_BlinnPhong]),
					L"BRDF_GP_IntegrateDiffuse_BlinnPhong"));
			}
			// CookTorrance
			{
				{
					const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_IntegrateDiffuse_CookTorrance]);
					NullCheck(mpLogFile, PS);
					psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
				}

				CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_IntegrateDiffuse_CookTorrance]),
					L"BRDF_GP_IntegrateDiffuse_CookTorrance"));
			}
		}
	}
	// IntegrateSpecular
	{
		// GraphicsPipelineState
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
	}

	return TRUE;
}

BOOL BRDF::BRDFClass::IntegrateDiffuse(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pAlbedoMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap,
		Foundation::Resource::GpuResource* const pNormalMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		Foundation::Resource::GpuResource* const pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		Foundation::Resource::GpuResource* const pSpecularMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_specularMap,
		Foundation::Resource::GpuResource* const pRoughnessMetalnessMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		Foundation::Resource::GpuResource* const pDiffuseIrradianceMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_diffuseIrradianceMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::GP_IntegrateDiffuse_CookTorrance].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_IntegrateDiffuse].Get());

	CmdList->RSSetViewports(1, &viewport);
	CmdList->RSSetScissorRects(1, &scissorRect);

	pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pAlbedoMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pSpecularMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pRoughnessMetalnessMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pDiffuseIrradianceMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

	CmdList->SetGraphicsRootConstantBufferView(RootSignature::IntegrateDiffuse::CB_Pass, pFrameResource->MainPassCBAddress());
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateDiffuse::SI_AlbedoMap, si_albedoMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateDiffuse::SI_NormalMap, si_normalMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateDiffuse::SI_DepthMap, si_depthMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateDiffuse::SI_SpecularMap, si_specularMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateDiffuse::SI_RoughnessMetalicMap, si_roughnessMetalnessMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateDiffuse::SI_PositionMap, si_positionMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateDiffuse::SI_DiffuseIrradianceCubeMap, si_diffuseIrradianceMap);

	CmdList->IASetVertexBuffers(0, 0, nullptr);
	CmdList->IASetIndexBuffer(nullptr);
	CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CmdList->DrawInstanced(6, 1, 0, 0);

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL BRDF::BRDFClass::IntegrateSpecular(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		Foundation::Resource::GpuResource* const pAlbedoMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap,
		Foundation::Resource::GpuResource* const pNormalMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		Foundation::Resource::GpuResource* const pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		Foundation::Resource::GpuResource* const pSpecularMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_specularMap,
		Foundation::Resource::GpuResource* const pRoughnessMetalnessMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap,
		Foundation::Resource::GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		Foundation::Resource::GpuResource* const pBrdfLutMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_brdfLutMap,
		Foundation::Resource::GpuResource* const pPrefilteredEnvCubeMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_prefilteredEnvCubeMap) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[PipelineState::GP_IntegrateSpecular].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_IntegrateSpecular].Get());

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
	pBrdfLutMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pPrefilteredEnvCubeMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

	CmdList->SetGraphicsRootConstantBufferView(RootSignature::IntegrateSpecular::CB_Pass, pFrameResource->MainPassCBAddress());
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_BackBuffer, si_backBufferCopy);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_AlbedoMap, si_albedoMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_NormalMap, si_normalMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_DepthMap, si_depthMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_SpecularMap, si_specularMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_RoughnessMetalicMap, si_roughnessMetalnessMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_PositionMap, si_positionMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_BrdfLutMap, si_brdfLutMap);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::IntegrateSpecular::SI_PrefilteredEnvCubeMap, si_prefilteredEnvCubeMap);

	CmdList->IASetVertexBuffers(0, 0, nullptr);
	CmdList->IASetIndexBuffer(nullptr);
	CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CmdList->DrawInstanced(6, 1, 0, 0);

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}