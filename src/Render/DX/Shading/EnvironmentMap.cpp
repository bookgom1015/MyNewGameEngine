#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Vertex.h"
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
#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Render/DX/Shading/Util/EquirectangularConverter.hpp"

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_DrawSkySphere = L"DrawSkySphere.hlsl";

	const UINT CubeMapSize = 1024;
}

EnvironmentMap::InitDataPtr EnvironmentMap::MakeInitData() {
	return std::unique_ptr<EnvironmentMapClass::InitData>(new EnvironmentMapClass::InitData());
}

UINT EnvironmentMap::EnvironmentMapClass::CbvSrvUavDescCount() const { 
	return 0 
		+ 1 // TemporaryEquirectangularMapSrv
		+ 1 // EquirectangularMapSrv
		+ 1 // EnvironmentMapSrv
		; 
}

UINT EnvironmentMap::EnvironmentMapClass::RtvDescCount() const { 
	return 0
		+ ShadingConvention::MipmapGenerator::MaxMipLevel // EquirectangularMapRtvs
		+ ShadingConvention::MipmapGenerator::MaxMipLevel // EnvironmentMapRtvs
		; 
}

UINT EnvironmentMap::EnvironmentMapClass::DsvDescCount() const { return 0; }

EnvironmentMap::EnvironmentMapClass::EnvironmentMapClass() {
	mTemporaryEquirectangularMap = std::make_unique<Foundation::Resource::GpuResource>();
	mEquirectangularMap = std::make_unique<Foundation::Resource::GpuResource>();
	mEnvironmentCubeMap = std::make_unique<Foundation::Resource::GpuResource>();
}

BOOL EnvironmentMap::EnvironmentMapClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

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

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
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
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_DrawSkySphere]),
			L"EnvironmentMap_GR_DrawSkySphere"
		));
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildPipelineStates() {
	// DrawSkySphere
	{
		// GraphicsPipelineState
		{
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
		// MeshShaderPiepelineState
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

	mhEnvironmentCubeMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhEnvironmentCubeMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	for (UINT i = 0; i < ShadingConvention::MipmapGenerator::MaxMipLevel; ++i)
		mhEnvironmentCubeMapCpuRtvs[i] = pDescHeap->RtvCpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::SetEnvironmentMap(
		Util::MipmapGenerator::MipmapGeneratorClass* const pMipmapGenerator, 
		Util::EquirectangularConverter::EquirectangularConverterClass* const pEquirectangularConverter,
		D3D12_GPU_VIRTUAL_ADDRESS cbEquirectConv,
		LPCWSTR filePath) {	
	CheckReturn(mpLogFile, CreateTemporaryEquirectangularMap(filePath));
	CheckReturn(mpLogFile, CreateEquirectangularMap());
	CheckReturn(mpLogFile, BuildEquirectangularMapDescriptors());
	CheckReturn(mpLogFile, GenerateMipmap(pMipmapGenerator));
	CheckReturn(mpLogFile, ConvertEquirectangularMapToCubeMap(pEquirectangularConverter, cbEquirectConv));

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
		mPipelineStates[mInitData.MeshShaderSupported ? PipelineState::MP_DrawSkySphere : PipelineState::GP_DrawSkySphere].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_DrawSkySphere].Get());

	CmdList->RSSetViewports(1, &viewport);
	CmdList->RSSetScissorRects(1, &scissorRect);

	backBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	depthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, &di_depthStencil);

	CmdList->SetGraphicsRootConstantBufferView(RootSignature::DrawSkySphere::CB_Pass, pFrameResource->MainPassCBAddress());
	
	D3D12_GPU_VIRTUAL_ADDRESS ritemObjCBAddress = pFrameResource->ObjectCBAddress(sphere->ObjCBIndex);
	CmdList->SetGraphicsRootConstantBufferView(RootSignature::DrawSkySphere::CB_Object, ritemObjCBAddress);
	CmdList->SetGraphicsRootDescriptorTable(RootSignature::DrawSkySphere::SI_EnvCubeMap, mhEnvironmentCubeMapGpuSrv);

	if (mInitData.MeshShaderSupported) {
		CmdList->SetGraphicsRootShaderResourceView(RootSignature::DrawSkySphere::SI_VertexBuffer, sphere->Geometry->VertexBufferGPU->GetGPUVirtualAddress());
		CmdList->SetGraphicsRootShaderResourceView(RootSignature::DrawSkySphere::SI_IndexBuffer, sphere->Geometry->IndexBufferGPU->GetGPUVirtualAddress());

		ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Struct rc;	
		rc.gVertexCount = sphere->Geometry->VertexBufferByteSize / sphere->Geometry->VertexByteStride;
		rc.gIndexCount = sphere->Geometry->IndexBufferByteSize / sizeof(std::uint16_t);

		std::array<std::uint32_t, ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Struct));

		CmdList->SetGraphicsRoot32BitConstants(
			RootSignature::DrawSkySphere::RC_Consts,
			ShadingConvention::EnvironmentMap::RootConstant::DrawSkySphere::Count,
			consts.data(),
			0);

		const UINT PrimCount = rc.gIndexCount / 3;

		CmdList->DispatchMesh(
			Foundation::Util::D3D12Util::CeilDivide(PrimCount, ShadingConvention::EnvironmentMap::ThreadGroup::MeshShader::ThreadsPerGroup),
			1, 
			1);
	}
	else {
		CmdList->IASetVertexBuffers(0, 1, &sphere->Geometry->VertexBufferView());
		CmdList->IASetIndexBuffer(&sphere->Geometry->IndexBufferView());
		CmdList->IASetPrimitiveTopology(sphere->PrimitiveType);

		CmdList->DrawIndexedInstanced(sphere->IndexCount, 1, sphere->StartIndexLocation, sphere->BaseVertexLocation, 0);
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	
	rscDesc.Alignment = 0;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// EnvironmentCubeMap
	{
		rscDesc.Width = CubeMapSize;
		rscDesc.Height = CubeMapSize;
		rscDesc.DepthOrArraySize = 6;
		rscDesc.MipLevels = ShadingConvention::MipmapGenerator::MaxMipLevel;
		rscDesc.Format = ShadingConvention::EnvironmentMap::EnvironmentCubeMapFormat;

		CheckReturn(mpLogFile, mEnvironmentCubeMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EnvironmentMap_EnvironmentCubeMap"));
	}

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildDescriptors() {
	

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CreateTemporaryEquirectangularMap(LPCWSTR filePath) {
	auto tex = std::make_unique<Foundation::Resource::Texture>();

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateTexture(
		mInitData.Device,
		mInitData.CommandObject,
		tex.get(),
		filePath));

	mTemporaryEquirectangularMap->Swap(tex->Resource);
	CheckHRESULT(mpLogFile, mTemporaryEquirectangularMap->Resource()->SetName(L"EnvironmentMap_TemporaryEquirectangularMap"));

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
	// EnvironmentCubeMap
	{
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
		{
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
		D3D12_GPU_VIRTUAL_ADDRESS cbEquirectConv) {
	CheckReturn(mpLogFile, pEquirectangularConverter->ConvertEquirectangularToCube(
		mEnvironmentCubeMap.get(),
		mhEnvironmentCubeMapCpuRtvs,
		mEquirectangularMap.get(),
		mhEquirectangularMapGpuSrv,
		cbEquirectConv,
		CubeMapSize,
		CubeMapSize,
		ShadingConvention::MipmapGenerator::MaxMipLevel
	));

	return TRUE;
}