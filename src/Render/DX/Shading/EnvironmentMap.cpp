#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Vertex.h"
#include "Common/Util/StringUtil.hpp"
#include "Render/DX/Foundation/RenderItem.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Resource/Texture.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"
#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Render/DX/Shading/Util/EquirectangularConverter.hpp"

using namespace Render::DX::Shading;
using namespace DirectX;

namespace {
	const WCHAR* const HLSL_DrawSkySphere = L"DrawSkySphere.hlsl";
	const WCHAR* const HLSL_ConvoluteDiffuseIrradiance = L"ConvoluteDiffuseIrradiance.hlsl";
	const WCHAR* const HLSL_ConvoluteSpecularIrradiance = L"ConvoluteSpecularIrradiance.hlsl";
	const WCHAR* const HLSL_IntegrateBrdf = L"IntegrateBrdf.hlsl";

	const WCHAR* const EnvironmentCubeMapFileNameSuffix = L"_env_cube_map";
	const WCHAR* const DiffuseIrradianceCubeMapFileNameSuffix = L"_diff_irrad_cube_map";
	const WCHAR* const PrefilteredEnvironmentCubeMapFileNameSuffix = L"_prefiltered_env_cube_map";
	const WCHAR* const BrdfLutMapFileName = L"brdf_lut_map";

	const UINT CubeMapSize = 1024;
	const UINT BrdfLutMapSize = 1024;

	BOOL FileExists(const std::wstring& filePath) {		
		const auto& filePathStr = Common::Util::StringUtil::WStringToString(filePath);

		FILE* file;
		fopen_s(&file, filePathStr.c_str(), "r");
		if (file == NULL) return FALSE;

		fclose(file);

		return TRUE;
	}

	BOOL GetTextureResource(
			Common::Debug::LogFile* const pLogFile, 
			Render::DX::Foundation::Core::Device* const pDevice,
			Render::DX::Foundation::Core::CommandObject* const pCmdObject,
			Render::DX::Foundation::Resource::GpuResource* const pResource,
			LPCWSTR filePath,
			LPCWSTR texName) {
		auto tex = std::make_unique<Render::DX::Foundation::Resource::Texture>();

		CheckReturn(pLogFile, Render::DX::Foundation::Util::D3D12Util::CreateTexture(
			pDevice,
			pCmdObject,
			tex.get(),
			filePath));

		pResource->Swap(tex->Resource);
		CheckHRESULT(pLogFile, pResource->Resource()->SetName(texName));

		return TRUE;
	}
}

EnvironmentMap::InitDataPtr EnvironmentMap::MakeInitData() {
	return std::unique_ptr<EnvironmentMapClass::InitData>(new EnvironmentMapClass::InitData());
}

UINT EnvironmentMap::EnvironmentMapClass::CbvSrvUavDescCount() const { 
	return 0 
		+ 1 // TemporaryEquirectangularMapSrv
		+ 1 // EquirectangularMapSrv
		+ 1 // EnvironmentMapSrv
		+ 1 // DiffuseIrradianceCubeMapSrv
		+ 1 // SpecularIrradianceCubeMapSrv
		+ 1 // BrdfLutMapSrv
		; 
}

UINT EnvironmentMap::EnvironmentMapClass::RtvDescCount() const { 
	return 0
		+ ShadingConvention::MipmapGenerator::MaxMipLevel // EquirectangularMapRtvs
		+ ShadingConvention::MipmapGenerator::MaxMipLevel // EnvironmentMapRtvs
		+ 1 // DiffuseIrradianceCubeMapRtv
		+ ShadingConvention::MipmapGenerator::MaxMipLevel // SpecularIrradianceCubeMapRtvs
		+ 1 // EnvironmentMapRtv
		; 
}

UINT EnvironmentMap::EnvironmentMapClass::DsvDescCount() const { return 0; }

EnvironmentMap::EnvironmentMapClass::EnvironmentMapClass() {
	mTemporaryEquirectangularMap = std::make_unique<Foundation::Resource::GpuResource>();
	mEquirectangularMap = std::make_unique<Foundation::Resource::GpuResource>();
	mEnvironmentCubeMap = std::make_unique<Foundation::Resource::GpuResource>();
	mDiffuseIrradianceCubeMap = std::make_unique<Foundation::Resource::GpuResource>();
	mPrefilteredEnvironmentCubeMap = std::make_unique<Foundation::Resource::GpuResource>();
	mBrdfLutMap = std::make_unique<Foundation::Resource::GpuResource>();
}

BOOL EnvironmentMap::EnvironmentMapClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	mViewport = { 0.f, 0.f, static_cast<FLOAT>(CubeMapSize), static_cast<FLOAT>(CubeMapSize), 0.f, 1.f };
	mScissorRect = { 0, 0, static_cast<INT>(CubeMapSize), static_cast<INT>(CubeMapSize) };

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CompileShaders() {
	// DrawSkySphere
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"VS", L"vs_6_5");
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"MS", L"ms_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_DrawSkySphere]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_DrawSkySphere]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_DrawSkySphere]));
	}
	// ConvoluteDiffuseIrradiance
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvoluteDiffuseIrradiance, L"VS", L"vs_6_5");
		const auto GS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvoluteDiffuseIrradiance, L"GS", L"gs_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvoluteDiffuseIrradiance, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ConvoluteDiffuseIrradiance]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(GS, mShaderHashes[Shader::GS_ConvoluteDiffuseIrradiance]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ConvoluteDiffuseIrradiance]));
	}
	// ConvoluteSpecularIrradiance
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvoluteSpecularIrradiance, L"VS", L"vs_6_5");
		const auto GS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvoluteSpecularIrradiance, L"GS", L"gs_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ConvoluteSpecularIrradiance, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ConvoluteSpecularIrradiance]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(GS, mShaderHashes[Shader::GS_ConvoluteSpecularIrradiance]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ConvoluteSpecularIrradiance]));
	}
	// IntegrateBrdf
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"VS", L"vs_6_5");
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"MS", L"ms_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_IntegrateBrdf]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_IntegrateBrdf]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_IntegrateBrdf]));
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

	// DrawSkySphere 
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::DrawSkySphere::Count] = {};
		slotRootParameter[RootSignature::DrawSkySphere::CB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::DrawSkySphere::CB_Object].InitAsConstantBufferView(1);
		slotRootParameter[RootSignature::DrawSkySphere::RC_Consts].InitAsConstants(ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Count, 2);
		slotRootParameter[RootSignature::DrawSkySphere::SI_VertexBuffer].InitAsShaderResourceView(0);
		slotRootParameter[RootSignature::DrawSkySphere::SI_IndexBuffer].InitAsShaderResourceView(1);
		slotRootParameter[RootSignature::DrawSkySphere::SI_EnvCubeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_DrawSkySphere]),
			L"EnvironmentMap_GR_DrawSkySphere"));
	}
	// ConvoluteDiffuseIrradiance
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ConvoluteDiffuseIrradiance::Count] = {};
		slotRootParameter[RootSignature::ConvoluteDiffuseIrradiance::CB_ProjectToCube].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::ConvoluteDiffuseIrradiance::SI_CubeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ConvoluteDiffuseIrradiance]),
			L"EnvironmentMap_GR_ConvoluteDiffuseIrradiance"));
	}
	// ConvoluteSpecularIrradiance
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ConvoluteSpecularIrradiance::Count] = {};
		slotRootParameter[RootSignature::ConvoluteSpecularIrradiance::CB_ProjectToCube].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::ConvoluteSpecularIrradiance::RC_Consts].InitAsConstants(ShadingConvention::EnvironmentMap::RootConstant::ConvoluteSpecularIrradiance::Count, 1);
		slotRootParameter[RootSignature::ConvoluteSpecularIrradiance::SI_EnvCubeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ConvoluteSpecularIrradiance]),
			L"EnvironmentMap_GR_ConvoluteSpecularIrradiance"));
	}
	// IntegrateBrdf
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::IntegrateBrdf::Count] = {};
		slotRootParameter[RootSignature::IntegrateBrdf::CB_Pass].InitAsConstantBufferView(0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_IntegrateBrdf]),
			L"EnvironmentMap_GR_IntegrateBrdf"));
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildPipelineStates() {
	// DrawSkySphere
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::DefaultMeshPsoDesc(ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat);

			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_DrawSkySphere].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_DrawSkySphere]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_DrawSkySphere]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_DrawSkySphere]),
				L"EnvironmentMap_MP_DrawSkySphere"));
		}
		else {
			const auto inputLayout = Foundation::Util::D3D12Util::InputLayoutDesc();
			auto psoDesc = Foundation::Util::D3D12Util::DefaultPsoDesc(inputLayout, ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat);

			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_DrawSkySphere].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_DrawSkySphere]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_DrawSkySphere]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_DrawSkySphere]),
				L"EnvironmentMap_GP_DrawSkySphere"));
		}
	}
	// ConvoluteDiffuseIrradiance
	{
		const auto inputLayout = Foundation::Util::D3D12Util::InputLayoutDesc();
		auto psoDesc = Foundation::Util::D3D12Util::DefaultPsoDesc(inputLayout, ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ConvoluteDiffuseIrradiance].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ConvoluteDiffuseIrradiance]);
			NullCheck(mpLogFile, VS);
			const auto GS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::GS_ConvoluteDiffuseIrradiance]);
			NullCheck(mpLogFile, GS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ConvoluteDiffuseIrradiance]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.GS = { reinterpret_cast<BYTE*>(GS->GetBufferPointer()), GS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = HDR_FORMAT;
		psoDesc.DepthStencilState.DepthEnable = FALSE;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ConvoluteDiffuseIrradiance]),
			L"EnvironmentMap_GP_ConvoluteDiffuseIrradiance"));
	}
	// ConvoluteSpecularIrradiance
	{
		const auto inputLayout = Foundation::Util::D3D12Util::InputLayoutDesc();
		auto psoDesc = Foundation::Util::D3D12Util::DefaultPsoDesc(inputLayout, ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ConvoluteSpecularIrradiance].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ConvoluteSpecularIrradiance]);
			NullCheck(mpLogFile, VS);
			const auto GS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::GS_ConvoluteSpecularIrradiance]);
			NullCheck(mpLogFile, GS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ConvoluteSpecularIrradiance]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.GS = { reinterpret_cast<BYTE*>(GS->GetBufferPointer()), GS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = HDR_FORMAT;
		psoDesc.DepthStencilState.DepthEnable = FALSE;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ConvoluteSpecularIrradiance]),
			L"EnvironmentMap_GP_ConvoluteSpecularIrradiance"));
	}
	// IntegrateBrdf
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();

			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_IntegrateBrdf].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::MS_IntegrateBrdf]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::PS_IntegrateBrdf]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { 
					reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { 
					reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = ShadingConvention::EnvironmentMap::BrdfLutMapFormat;
			psoDesc.DepthStencilState.DepthEnable = FALSE;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_IntegrateBrdf]),
				L"EnvironmentMap_MP_IntegrateBrdf"));
		}
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();

			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_IntegrateBrdf].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::VS_IntegrateBrdf]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Shader::PS_IntegrateBrdf]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { 
					reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { 
					reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = ShadingConvention::EnvironmentMap::BrdfLutMapFormat;
			psoDesc.DepthStencilState.DepthEnable = FALSE;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_IntegrateBrdf]),
				L"EnvironmentMap_GP_IntegrateBrdf"));
		}				
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	// TemporaryEquirectangularMap
	{
		mhTemporaryEquirectangularMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
		mhTemporaryEquirectangularMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	}
	// EquirectangularMap
	{
		mhEquirectangularMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
		mhEquirectangularMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
		for (UINT i = 0; i < ShadingConvention::MipmapGenerator::MaxMipLevel; ++i)
			mhEquirectangularMapCpuRtvs[i] = pDescHeap->RtvCpuOffset(1);
	}
	// EnvironmentCubeMap
	{
		mhEnvironmentCubeMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
		mhEnvironmentCubeMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
		for (UINT i = 0; i < ShadingConvention::MipmapGenerator::MaxMipLevel; ++i)
			mhEnvironmentCubeMapCpuRtvs[i] = pDescHeap->RtvCpuOffset(1);
	}
	// DiffuseIrradianceCubeMap
	{
		mhDiffuseIrradianceCubeMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
		mhDiffuseIrradianceCubeMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
		mhDiffuseIrradianceCubeMapCpuRtv = pDescHeap->RtvCpuOffset(1);
	}
	// PrefilteredEnvironmentCubeMap
	{
		mhPrefilteredEnvironmentCubeMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
		mhPrefilteredEnvironmentCubeMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
		for (UINT i = 0; i < ShadingConvention::MipmapGenerator::MaxMipLevel; ++i)
			mhPrefilteredEnvironmentCubeMapCpuRtvs[i] = pDescHeap->RtvCpuOffset(1);
	}
	// BrdfLutMap
	{
		mhBrdfLutMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
		mhBrdfLutMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
		mhBrdfLutMapCpuRtv = pDescHeap->RtvCpuOffset(1);
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::SetEnvironmentMap(
		Foundation::Resource::FrameResource* const pFrameResource,
		Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator, 
		Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
		LPCWSTR fileName, LPCWSTR baseDir) {
	CheckReturn(mpLogFile, Load(fileName, baseDir));
	CheckReturn(mpLogFile, Generate(pFrameResource, pMipmapGenerator, pEquirectangularConverter, fileName, baseDir));
	CheckReturn(mpLogFile, Save(fileName, baseDir));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::DrawSkySphere(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const backBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const depthBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE di_depthStencil,
		Foundation::RenderItem* const sphere) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? 
		PipelineState::MP_DrawSkySphere : PipelineState::GP_DrawSkySphere].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_DrawSkySphere].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		backBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		mEnvironmentCubeMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, &di_depthStencil);

		CmdList->SetGraphicsRootConstantBufferView(
			RootSignature::DrawSkySphere::CB_Pass, 
			pFrameResource->MainPassCB.CBAddress());

		CmdList->SetGraphicsRootConstantBufferView(
			RootSignature::DrawSkySphere::CB_Object, 
			pFrameResource->ObjectCB.CBAddress(sphere->ObjectCBIndex));
		CmdList->SetGraphicsRootDescriptorTable(
			RootSignature::DrawSkySphere::SI_EnvCubeMap, 
			mhEnvironmentCubeMapGpuSrv);

		if (mInitData.MeshShaderSupported) {
			CmdList->SetGraphicsRootShaderResourceView(
				RootSignature::DrawSkySphere::SI_VertexBuffer, 
				sphere->Geometry->VertexBufferGPU->GetGPUVirtualAddress());
			CmdList->SetGraphicsRootShaderResourceView(
				RootSignature::DrawSkySphere::SI_IndexBuffer, 
				sphere->Geometry->IndexBufferGPU->GetGPUVirtualAddress());

			ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Struct rc;
			rc.gVertexCount = sphere->Geometry->VertexBufferByteSize / sphere->Geometry->VertexByteStride;
			rc.gIndexCount = sphere->Geometry->IndexBufferByteSize / sphere->Geometry->IndexByteStride;

			std::array<std::uint32_t, ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Count> consts;
			std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Struct));

			CmdList->SetGraphicsRoot32BitConstants(
				RootSignature::DrawSkySphere::RC_Consts,
				ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Count,
				consts.data(),
				0);

			const UINT PrimCount = rc.gIndexCount / 3;

			CmdList->DispatchMesh(
				Foundation::Util::D3D12Util::CeilDivide(
					PrimCount, 
					ShadingConvention::EnvironmentMap::ThreadGroup::MeshShader::ThreadsPerGroup),
				1,
				1);
		}
		else {
			CmdList->IASetVertexBuffers(0, 1, &sphere->Geometry->VertexBufferView());
			CmdList->IASetIndexBuffer(&sphere->Geometry->IndexBufferView());
			CmdList->IASetPrimitiveTopology(sphere->PrimitiveType);

			CmdList->DrawIndexedInstanced(sphere->IndexCount, 1, sphere->StartIndexLocation, sphere->BaseVertexLocation, 0);
		}
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::Load(LPCWSTR fileName, LPCWSTR baseDir) {
	// Load envrionment cube map if one exists
	{
		std::wstringstream filePath;
		filePath << baseDir << fileName << EnvironmentCubeMapFileNameSuffix << L".dds";

		if (FileExists(filePath.str())) {
			WLogln(mpLogFile, L"Environment cube-map detected. Loading it...");

			CheckReturn(mpLogFile, GetTextureResource(
				mpLogFile,
				mInitData.Device,
				mInitData.CommandObject,
				mEnvironmentCubeMap.get(),
				filePath.str().c_str(),
				L"EnvironmentMap_EnvironmentCubeMap"));
			CheckReturn(mpLogFile, BuildEnvironmentMapDescriptors(FALSE));

			WLogln(mpLogFile, L"Loading environment cube-map completed");
		}
		else {
			mTasks = static_cast<TaskType>(mTasks | TaskType::E_NeedToGenEnvCubeMap);
			WLogln(mpLogFile, L"Environment cube-map not found. Generating new one...");
		}
	}
	// Load diffuse irradiance cube map if one exists
	{
		std::wstringstream filePath;
		filePath << baseDir << fileName << DiffuseIrradianceCubeMapFileNameSuffix << L".dds";

		if (FileExists(filePath.str())) {
			WLogln(mpLogFile, L"Diffuse irradiance cube-map detected. Loading it...");

			CheckReturn(mpLogFile, GetTextureResource(
				mpLogFile,
				mInitData.Device,
				mInitData.CommandObject,
				mDiffuseIrradianceCubeMap.get(),
				filePath.str().c_str(),
				L"EnvironmentMap_DiffuseIrradianceCubeMap"));
			CheckReturn(mpLogFile, BuildDiffuseIrradianceCubeMapDescriptors(FALSE));

			WLogln(mpLogFile, L"Loading diffuse irradiance cube-map completed");
		}
		else {
			mTasks = static_cast<TaskType>(mTasks | TaskType::E_NeedToGenDiffuseIrradianceCubeMap);
			WLogln(mpLogFile, L"Diffuse irradiance cube-map not found. Generating new one...");
		}
	}
	// Load prefiltered environment cube map if one exists
	{
		std::wstringstream filePath;
		filePath << baseDir << fileName << PrefilteredEnvironmentCubeMapFileNameSuffix << L".dds";

		if (FileExists(filePath.str())) {
			WLogln(mpLogFile, L"Prefiltered environment cube-map detected. Loading it...");

			CheckReturn(mpLogFile, GetTextureResource(
				mpLogFile,
				mInitData.Device,
				mInitData.CommandObject,
				mPrefilteredEnvironmentCubeMap.get(),
				filePath.str().c_str(),
				L"EnvironmentMap_PrefilteredEnvironmentCubeMap"));
			CheckReturn(mpLogFile, BuildPrefilteredEnvironmentCubeMapDescriptors(FALSE));

			WLogln(mpLogFile, L"Loading prefiltered environment cube-map completed");
		}
		else {
			mTasks = static_cast<TaskType>(mTasks | TaskType::E_NeedToGenPrefilteredEnvCubeMap);
			WLogln(mpLogFile, L"Prefiltered environment cube-map not found. Generating new one...");
		}
	}
	// Load BRDF-LUT map if one exists
	{
		std::wstringstream filePath;
		filePath << baseDir << BrdfLutMapFileName << L".dds";

		if (FileExists(filePath.str())) {
			WLogln(mpLogFile, L"BRDF-LUT map detected. Loading it...");

			CheckReturn(mpLogFile, GetTextureResource(
				mpLogFile,
				mInitData.Device,
				mInitData.CommandObject,
				mBrdfLutMap.get(),
				filePath.str().c_str(),
				L"EnvironmentMap_BrdfLutMap"));
			CheckReturn(mpLogFile, BuildBrdfLutMapDescriptors(FALSE));

			WLogln(mpLogFile, L"Loading BRDF-LUT map completed");
		}
		else {
			mTasks = static_cast<TaskType>(mTasks | TaskType::E_NeedToGenBrdfLutMap);
			WLogln(mpLogFile, L"BRDF-LUT map not found. Generating new one...");
		}
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::Generate(
		Foundation::Resource::FrameResource* const pFrameResource,
		Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator,
		Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
		LPCWSTR fileName, LPCWSTR baseDir) {
	if (mTasks & TaskType::E_NeedToGenEnvCubeMap) {
		{
			std::wstringstream filePath;
			filePath << baseDir << fileName << ".dds";

			CheckReturn(mpLogFile, GetTextureResource(
				mpLogFile,
				mInitData.Device,
				mInitData.CommandObject,
				mTemporaryEquirectangularMap.get(),
				filePath.str().c_str(),
				L"EnvironmentMap_TemporaryEquirectangularMap"));
		}

		CheckReturn(mpLogFile, CreateEquirectangularMap());
		CheckReturn(mpLogFile, BuildEquirectangularMapDescriptors());

		CheckReturn(mpLogFile, CreateEnvironmentCubeMap());
		CheckReturn(mpLogFile, BuildEnvironmentMapDescriptors(TRUE));

		CheckReturn(mpLogFile, GenerateMipmap(pMipmapGenerator));
		CheckReturn(mpLogFile, ConvertEquirectangularMapToCubeMap(
			pEquirectangularConverter, pFrameResource->ProjectToCubeCB.CBAddress()));

		mTasks = static_cast<TaskType>(mTasks | TaskType::E_NeedToSaveEnvCubeMap);
		WLogln(mpLogFile, L"Generating environment cube-map completed");
	}
	if (mTasks & TaskType::E_NeedToGenDiffuseIrradianceCubeMap) {
		CheckReturn(mpLogFile, CreateDiffuseIrradianceCubeMap());
		CheckReturn(mpLogFile, BuildDiffuseIrradianceCubeMapDescriptors(TRUE));

		CheckReturn(mpLogFile, DrawDiffuseIrradianceCubeMap(
			pFrameResource->ProjectToCubeCB.CBAddress()));

		mTasks = static_cast<TaskType>(mTasks | TaskType::E_NeedToSaveDiffuseIrradianceCubeMap);
		WLogln(mpLogFile, L"Generating diffuse irradiance cube-map completed");
	}
	if (mTasks & TaskType::E_NeedToGenPrefilteredEnvCubeMap) {
		CheckReturn(mpLogFile, CreatePrefilteredEnvironmentCubeMap());
		CheckReturn(mpLogFile, BuildPrefilteredEnvironmentCubeMapDescriptors(TRUE));

		CheckReturn(mpLogFile, DrawPrefilteredEnvironmentCubeMap(
			pFrameResource->ProjectToCubeCB.CBAddress()));

		mTasks = static_cast<TaskType>(mTasks | TaskType::E_NeedToSavePrefilteredEnvCubeMap);
		WLogln(mpLogFile, L"Generating prefiltered environment cube-map completed");
	}
	if (mTasks & TaskType::E_NeedToGenBrdfLutMap) {
		CheckReturn(mpLogFile, CreateBrdfLutMap());
		CheckReturn(mpLogFile, BuildBrdfLutMapDescriptors(TRUE));

		CheckReturn(mpLogFile, DrawBrdfLutMap(pFrameResource->MainPassCB.CBAddress()));

		mTasks = static_cast<TaskType>(mTasks | TaskType::E_NeedToSaveBrdfLutMap);
		WLogln(mpLogFile, L"Generating BRDF-LUT map completed");
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::Save(LPCWSTR fileName, LPCWSTR baseDir) {
	// Save environment cube map
	if (mTasks & TaskType::E_NeedToSaveEnvCubeMap) {
		ScratchImage image;
		
		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CaptureTexture(
			mInitData.CommandObject,
			mEnvironmentCubeMap->Resource(),
			TRUE,
			image,
			mEnvironmentCubeMap->State(),
			mEnvironmentCubeMap->State()));
		
		std::wstringstream filePath;
		filePath << baseDir << fileName << EnvironmentCubeMapFileNameSuffix << L".dds";

		CheckHRESULT(mpLogFile, SaveToDDSFile(
			image.GetImages(),
			image.GetImageCount(),
			image.GetMetadata(),
			DDS_FLAGS_NONE,
			filePath.str().c_str()));

		WLogln(mpLogFile, L"Environment cube-map saved");
	}
	// Save diffuse irradiance cube map
	if (mTasks & TaskType::E_NeedToSaveDiffuseIrradianceCubeMap) {
		ScratchImage image;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CaptureTexture(
			mInitData.CommandObject,
			mDiffuseIrradianceCubeMap->Resource(),
			TRUE,
			image,
			mDiffuseIrradianceCubeMap->State(),
			mDiffuseIrradianceCubeMap->State()));

		std::wstringstream filePath;
		filePath << baseDir << fileName << DiffuseIrradianceCubeMapFileNameSuffix << L".dds";

		CheckHRESULT(mpLogFile, SaveToDDSFile(
			image.GetImages(),
			image.GetImageCount(),
			image.GetMetadata(),
			DDS_FLAGS_NONE,
			filePath.str().c_str()));

		WLogln(mpLogFile, L"Diffuse irradiance cube-map saved");
	}
	// Save prefiltered environment cube map
	if (mTasks & TaskType::E_NeedToSavePrefilteredEnvCubeMap) {
		ScratchImage image;
	
		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CaptureTexture(
			mInitData.CommandObject,
			mPrefilteredEnvironmentCubeMap->Resource(),
			TRUE,
			image,
			mPrefilteredEnvironmentCubeMap->State(),
			mPrefilteredEnvironmentCubeMap->State()));
	
		std::wstringstream filePath;
		filePath << baseDir << fileName << PrefilteredEnvironmentCubeMapFileNameSuffix << L".dds";
	
		CheckHRESULT(mpLogFile, SaveToDDSFile(
			image.GetImages(),
			image.GetImageCount(),
			image.GetMetadata(),
			DDS_FLAGS_NONE,
			filePath.str().c_str()));
	
		WLogln(mpLogFile, L"Prefiltered environment cube-map saved");
	}
	// Save BRDF-LUT map
	if (mTasks & TaskType::E_NeedToSaveBrdfLutMap) {
		ScratchImage image;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CaptureTexture(
			mInitData.CommandObject,
			mBrdfLutMap->Resource(),
			FALSE,
			image,
			mBrdfLutMap->State(),
			mBrdfLutMap->State()));

		std::wstringstream filePath;
		filePath << baseDir << BrdfLutMapFileName << L".dds";

		CheckHRESULT(mpLogFile, SaveToDDSFile(
			image.GetImages(),
			image.GetImageCount(),
			image.GetMetadata(),
			DDS_FLAGS_NONE,
			filePath.str().c_str()));

		WLogln(mpLogFile, L"BRDF-LUT map saved");
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CreateEquirectangularMap() {
	const auto desc = mTemporaryEquirectangularMap->Desc();

	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Format = ShadingConvention::EnvironmentMap::EquirectangularMapFormat;
	rscDesc.Width = desc.Width;
	rscDesc.Height = desc.Height;
	rscDesc.MipLevels = ShadingConvention::MipmapGenerator::MaxMipLevel;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CheckReturn(mpLogFile, mEquirectangularMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"EnvironmentMap_EquirectangularMap"));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildEquirectangularMapDescriptors() {
	// TemporaryEquirectangularMap
	{
		const auto desc = mTemporaryEquirectangularMap->Desc();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Format = desc.Format;

		Foundation::Util::D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			mTemporaryEquirectangularMap->Resource(),
			&srvDesc,
			mhTemporaryEquirectangularMapCpuSrv);
	}
	// EquirectangularMap
	{
		const auto desc = mEquirectangularMap->Desc();

		// Srv 
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
			srvDesc.Texture2D.MipLevels = desc.MipLevels;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Format = desc.Format;

			Foundation::Util::D3D12Util::CreateShaderResourceView(
				mInitData.Device,
				mEquirectangularMap->Resource(),
				&srvDesc,
				mhEquirectangularMapCpuSrv);
		}
		// Rtv
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = desc.Format;
			rtvDesc.Texture2D.PlaneSlice = 0;

			for (UINT i = 0; i < desc.MipLevels; ++i) {
				rtvDesc.Texture2D.MipSlice = i;

				Foundation::Util::D3D12Util::CreateRenderTargetView(
					mInitData.Device,
					mEquirectangularMap->Resource(),
					&rtvDesc,
					mhEquirectangularMapCpuRtvs[i]);
			}
		}
	}
	
	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CreateEnvironmentCubeMap() {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Format = ShadingConvention::EnvironmentMap::EnvironmentCubeMapFormat;
	rscDesc.Width = CubeMapSize;
	rscDesc.Height = CubeMapSize;
	rscDesc.DepthOrArraySize = 6;
	rscDesc.MipLevels = ShadingConvention::MipmapGenerator::MaxMipLevel;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CheckReturn(mpLogFile, mEnvironmentCubeMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"EnvironmentMap_EnvironmentCubeMap"));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildEnvironmentMapDescriptors(BOOL bNeedRtv) {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Format = HDR_FORMAT;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		srvDesc.TextureCube.MipLevels = ShadingConvention::MipmapGenerator::MaxMipLevel;

		Foundation::Util::D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			mEnvironmentCubeMap->Resource(),
			&srvDesc,
			mhEnvironmentCubeMapCpuSrv);
	}
	// Rtv
	if (bNeedRtv) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = HDR_FORMAT;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 6;

		for (UINT mipLevel = 0; mipLevel < ShadingConvention::MipmapGenerator::MaxMipLevel; ++mipLevel) {
			rtvDesc.Texture2DArray.MipSlice = mipLevel;

			Foundation::Util::D3D12Util::CreateRenderTargetView(
				mInitData.Device,
				mEnvironmentCubeMap->Resource(),
				&rtvDesc,
				mhEnvironmentCubeMapCpuRtvs[mipLevel]);
		}
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CreateDiffuseIrradianceCubeMap() {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Format = ShadingConvention::EnvironmentMap::DiffuseIrradianceCubeMapFormat;
	rscDesc.Width = CubeMapSize;
	rscDesc.Height = CubeMapSize;
	rscDesc.DepthOrArraySize = 6;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CheckReturn(mpLogFile, mDiffuseIrradianceCubeMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"EnvironmentMap_DiffuseIrradianceCubeMap"));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildDiffuseIrradianceCubeMapDescriptors(BOOL bNeedRtv) {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Format = ShadingConvention::EnvironmentMap::DiffuseIrradianceCubeMapFormat;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		Foundation::Util::D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			mDiffuseIrradianceCubeMap->Resource(),
			&srvDesc,
			mhDiffuseIrradianceCubeMapCpuSrv);
	}
	// Rtv
	if (bNeedRtv) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = ShadingConvention::EnvironmentMap::DiffuseIrradianceCubeMapFormat;
		rtvDesc.Texture2DArray.ArraySize = 6;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;

		Foundation::Util::D3D12Util::CreateRenderTargetView(
			mInitData.Device,
			mDiffuseIrradianceCubeMap->Resource(),
			&rtvDesc,
			mhDiffuseIrradianceCubeMapCpuRtv);
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CreatePrefilteredEnvironmentCubeMap() {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Format = ShadingConvention::EnvironmentMap::PrefilteredEnvironmentCubeMapFormat;
	rscDesc.Width = CubeMapSize;
	rscDesc.Height = CubeMapSize;
	rscDesc.DepthOrArraySize = 6;
	rscDesc.MipLevels = ShadingConvention::MipmapGenerator::MaxMipLevel;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CheckReturn(mpLogFile, mPrefilteredEnvironmentCubeMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"EnvironmentMap_PrefilteredEnvironmentCubeMap"));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildPrefilteredEnvironmentCubeMapDescriptors(BOOL bNeedRtv) {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Format = ShadingConvention::EnvironmentMap::PrefilteredEnvironmentCubeMapFormat;
		srvDesc.TextureCube.MipLevels = ShadingConvention::MipmapGenerator::MaxMipLevel;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		Foundation::Util::D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			mPrefilteredEnvironmentCubeMap->Resource(),
			&srvDesc,
			mhPrefilteredEnvironmentCubeMapCpuSrv);
	}
	// Rtv
	if (bNeedRtv) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = ShadingConvention::EnvironmentMap::PrefilteredEnvironmentCubeMapFormat;
		rtvDesc.Texture2DArray.ArraySize = 6;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;

		for (UINT i = 0; i < ShadingConvention::MipmapGenerator::MaxMipLevel; ++i) {
			rtvDesc.Texture2DArray.MipSlice = i;

			Foundation::Util::D3D12Util::CreateRenderTargetView(
				mInitData.Device,
				mPrefilteredEnvironmentCubeMap->Resource(),
				&rtvDesc,
				mhPrefilteredEnvironmentCubeMapCpuRtvs[i]);
		}
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CreateBrdfLutMap() {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Format = ShadingConvention::EnvironmentMap::BrdfLutMapFormat;
	rscDesc.Width = BrdfLutMapSize;
	rscDesc.Height = BrdfLutMapSize;
	rscDesc.MipLevels = 1;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CheckReturn(mpLogFile, mBrdfLutMap->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"EnvironmentMap_BrdfLutMap"));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildBrdfLutMapDescriptors(BOOL bNeedRtv) {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = ShadingConvention::EnvironmentMap::BrdfLutMapFormat;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.MipLevels = 1;

		Foundation::Util::D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			mBrdfLutMap->Resource(),
			&srvDesc,
			mhBrdfLutMapCpuSrv);
	}
	// Rtv
	if (bNeedRtv) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = ShadingConvention::EnvironmentMap::BrdfLutMapFormat;
		rtvDesc.Texture2D.PlaneSlice = 0;
		rtvDesc.Texture2D.MipSlice = 0;

		Foundation::Util::D3D12Util::CreateRenderTargetView(
			mInitData.Device,
			mBrdfLutMap->Resource(),
			&rtvDesc,
			mhBrdfLutMapCpuRtv);
	}

	return TRUE;
}


BOOL EnvironmentMap::EnvironmentMapClass::GenerateMipmap(Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator) {
	const auto desc = mTemporaryEquirectangularMap->Desc();

	CheckReturn(mpLogFile, pMipmapGenerator->GenerateMipmap(
		mEquirectangularMap.get(),
		mhEquirectangularMapCpuRtvs,
		mTemporaryEquirectangularMap.get(),
		mhTemporaryEquirectangularMapGpuSrv,
		ShadingConvention::MipmapGenerator::MaxMipLevel,
		static_cast<UINT>(desc.Width),
		static_cast<UINT>(desc.Height)));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::ConvertEquirectangularMapToCubeMap(
		Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
		D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube) {
	CheckReturn(mpLogFile, pEquirectangularConverter->ConvertEquirectangularToCube(
		mEnvironmentCubeMap.get(),
		mhEnvironmentCubeMapCpuRtvs,
		mEquirectangularMap.get(),
		mhEquirectangularMapGpuSrv,
		cbProjectToCube,
		CubeMapSize,
		CubeMapSize,
		ShadingConvention::MipmapGenerator::MaxMipLevel
	));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::DrawDiffuseIrradianceCubeMap(D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList(
		mPipelineStates[PipelineState::GP_ConvoluteDiffuseIrradiance].Get()));

	const auto CmdList = mInitData.CommandObject->DirectCommandList();
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ConvoluteDiffuseIrradiance].Get());

	CmdList->RSSetViewports(1, &mViewport);
	CmdList->RSSetScissorRects(1, &mScissorRect);

	mDiffuseIrradianceCubeMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mEnvironmentCubeMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->OMSetRenderTargets(1, &mhDiffuseIrradianceCubeMapCpuRtv, TRUE, nullptr);

	CmdList->SetGraphicsRootConstantBufferView(RootSignature::ConvoluteDiffuseIrradiance::CB_ProjectToCube, cbProjectToCube);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::ConvoluteDiffuseIrradiance::SI_CubeMap, mhEnvironmentCubeMapGpuSrv);

	CmdList->IASetVertexBuffers(0, 0, nullptr);
	CmdList->IASetIndexBuffer(nullptr);
	CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CmdList->DrawInstanced(36, 1, 0, 0);

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::DrawPrefilteredEnvironmentCubeMap(D3D12_GPU_VIRTUAL_ADDRESS cbProjectToCube) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList(
		mPipelineStates[PipelineState::GP_ConvoluteSpecularIrradiance].Get()));

	const auto CmdList = mInitData.CommandObject->DirectCommandList();
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ConvoluteSpecularIrradiance].Get());

	CmdList->RSSetViewports(1, &mViewport);
	CmdList->RSSetScissorRects(1, &mScissorRect);

	mPrefilteredEnvironmentCubeMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mEnvironmentCubeMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CmdList->SetGraphicsRootConstantBufferView(RootSignature::ConvoluteSpecularIrradiance::CB_ProjectToCube, cbProjectToCube);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::ConvoluteSpecularIrradiance::SI_EnvCubeMap, mhEnvironmentCubeMapGpuSrv);

	for (UINT mipLevel = 0; mipLevel < ShadingConvention::MipmapGenerator::MaxMipLevel; ++mipLevel) {
		UINT size = static_cast<UINT>(CubeMapSize / std::pow(2.0, mipLevel));

		D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<FLOAT>(size), static_cast<FLOAT>(size), 0.0f, 1.0f };
		D3D12_RECT rect = { 0, 0, static_cast<INT>(size), static_cast<INT>(size) };

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &rect);

		ShadingConvention::EnvironmentMap::RootConstant::ConvoluteSpecularIrradiance::Struct rc;
		rc.gMipLevel = mipLevel;
		rc.gResolution = CubeMapSize;
		rc.gRoughness = 0.16667f * static_cast<FLOAT>(mipLevel);

		std::array<std::uint32_t, ShadingConvention::EnvironmentMap::RootConstant::ConvoluteSpecularIrradiance::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::EnvironmentMap::RootConstant::ConvoluteSpecularIrradiance::Struct));

		CmdList->SetGraphicsRoot32BitConstants(
			RootSignature::ConvoluteSpecularIrradiance::RC_Consts, 
			ShadingConvention::EnvironmentMap::RootConstant::ConvoluteSpecularIrradiance::Count, 
			consts.data(), 
			0);

		CmdList->OMSetRenderTargets(1, &mhPrefilteredEnvironmentCubeMapCpuRtvs[mipLevel], TRUE, nullptr);

		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(36, 1, 0, 0);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::DrawBrdfLutMap(D3D12_GPU_VIRTUAL_ADDRESS cbPass) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetDirectCommandList(
		mPipelineStates[mInitData.MeshShaderSupported ? 
		PipelineState::MP_IntegrateBrdf : PipelineState::GP_IntegrateBrdf].Get()));

	const auto CmdList = mInitData.CommandObject->DirectCommandList();
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_IntegrateBrdf].Get());

	CmdList->RSSetViewports(1, &mViewport);
	CmdList->RSSetScissorRects(1, &mScissorRect);

	mBrdfLutMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	CmdList->OMSetRenderTargets(1, &mhBrdfLutMapCpuRtv, TRUE, nullptr);

	CmdList->SetGraphicsRootConstantBufferView(RootSignature::IntegrateBrdf::CB_Pass, cbPass);

	if (mInitData.MeshShaderSupported) {
		CmdList->DispatchMesh(1, 1, 1);
	}
	else {
		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(6, 1, 0, 0);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteDirectCommandList());

	return TRUE;
}