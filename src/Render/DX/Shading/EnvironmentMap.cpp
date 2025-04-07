#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Vertex.h"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/Texture.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_DrawSkySphere = L"DrawSkySphere.hlsl";
}

EnvironmentMap::InitDataPtr EnvironmentMap::MakeInitData() {
	return std::unique_ptr<EnvironmentMapClass::InitData>(new EnvironmentMapClass::InitData());
}

UINT EnvironmentMap::EnvironmentMapClass::CbvSrvUavDescCount() const { 
	return 0 
		+ 1 // TemporaryEquirectangularMapSrv
		+ 1 // EquirectangularMapSrv
		; 
}

UINT EnvironmentMap::EnvironmentMapClass::RtvDescCount() const { 
	return 0
		+ ShadingConvention::MipmapGenerator::MaxMipLevel // EquirectangularMapRtvs
		; 
}

UINT EnvironmentMap::EnvironmentMapClass::DsvDescCount() const { return 0; }

EnvironmentMap::EnvironmentMapClass::EnvironmentMapClass() {
	mTemporaryEquirectangularMap = std::make_unique<Foundation::Resource::GpuResource>();
	mEquirectangularMap = std::make_unique<Foundation::Resource::GpuResource>();
}

BOOL EnvironmentMap::EnvironmentMapClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CompileShaders() {
	// DrawSkySphere
	{
		const auto vsInfo = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"VS", L"vs_6_3");
		const auto psInfo = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"PS", L"ps_6_3");
		mInitData.ShaderManager->AddShader(vsInfo, mShaderHashes[Shader::E_VS_DrawSkySphere]);
		mInitData.ShaderManager->AddShader(psInfo, mShaderHashes[Shader::E_PS_DrawSkySphere]);
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	// DrawSkySphere 
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::DrawSkySphere::Count] = {};
		slotRootParameter[RootSignature::DrawSkySphere::ECB_Pass].InitAsConstantBufferView(0);
		slotRootParameter[RootSignature::DrawSkySphere::ECB_Object].InitAsConstantBufferView(1);
		slotRootParameter[RootSignature::DrawSkySphere::ESI_EnvCubeMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::E_DrawSkySphere]),
			L"EnvironmentMap_RS_DrawSkySphere"
		));
	}
	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildPipelineStates() {
	// DrawSkySphere
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = Foundation::Util::D3D12Util::DefaultPsoDesc(
			Common::Foundation::Mesh::Vertex::InputLayoutDesc(), 
			ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat);

		psoDesc.pRootSignature = mRootSignatures[RootSignature::E_DrawSkySphere].Get();
		{
			const auto vs = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::E_VS_DrawSkySphere]);
			if (vs == nullptr) ReturnFalse(mpLogFile, L"Failed to get shader");
			const auto ps = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::E_PS_DrawSkySphere]);
			if (ps == nullptr) ReturnFalse(mpLogFile, L"Failed to get shader");
			psoDesc.VS = { reinterpret_cast<BYTE*>(vs->GetBufferPointer()), vs->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(ps->GetBufferPointer()), ps->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = HDR_FORMAT;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.DepthStencilState.StencilEnable = FALSE;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::EG_DrawSkySphere]),
			L"EnvironmentMap_GPS_DrawSkySphere"));
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhTemporaryEquirectangularMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhTemporaryEquirectangularMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);

	mhEquirectangularMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhEquirectangularMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	for (UINT i = 0; i < ShadingConvention::MipmapGenerator::MaxMipLevel; ++i) 
		mhEquirectangularMapCpuRtvs[i] = pDescHeap->RtvCpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::SetEnvironmentMap(LPCWSTR filePath) {
	auto tex = std::make_unique<Foundation::Resource::Texture>();
	
	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateTexture(
		mInitData.Device,
		mInitData.CommandObject,
		tex.get(),
		filePath));

	mTemporaryEquirectangularMap->Swap(tex->Resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CheckHRESULT(mpLogFile, mTemporaryEquirectangularMap->Resource()->SetName(L"EnvironmentMap_TemporaryEquirectangularMap"));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::GenerateMipmap() {


	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildDescriptors() {
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.MipLevels = 1;
	}

	return TRUE;
}