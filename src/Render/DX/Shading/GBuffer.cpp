#include "Render/DX/Shading/GBuffer.hpp"
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
	const UINT NumRenderTargtes = 5;

	const WCHAR* const HLSL_GBuffer = L"GBuffer.hlsl";
}

GBuffer::InitDataPtr GBuffer::MakeInitData() {
	return std::unique_ptr<GBufferClass::InitData>(new GBufferClass::InitData());
}

GBuffer::GBufferClass::GBufferClass() {
	mAlbedoMap = std::make_unique<Foundation::Resource::GpuResource>();
	mNormalMap = std::make_unique<Foundation::Resource::GpuResource>();
	mVelocityMap = std::make_unique<Foundation::Resource::GpuResource>();
	mPositionMap = std::make_unique<Foundation::Resource::GpuResource>();
	mRMSMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT GBuffer::GBufferClass::CbvSrvUavDescCount() const { return NumRenderTargtes; }

UINT GBuffer::GBufferClass::RtvDescCount() const { return NumRenderTargtes; }

UINT GBuffer::GBufferClass::DsvDescCount() const { return 0; }

BOOL GBuffer::GBufferClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL GBuffer::GBufferClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GBuffer, L"VS", L"vs_6_5");
	const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GBuffer, L"MS", L"ms_6_5");
	const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_GBuffer, L"PS", L"ps_6_5");
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_GBuffer]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_GBuffer]));
	CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_GBuffer]));

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, ShadingConvention::GBuffer::MaxNumTextures, 0, 1);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::Default::Count] = {};
	slotRootParameter[RootSignature::Default::CB_Pass].InitAsConstantBufferView(0);
	slotRootParameter[RootSignature::Default::CB_Object].InitAsConstantBufferView(1);
	slotRootParameter[RootSignature::Default::CB_Material].InitAsConstantBufferView(2);
	slotRootParameter[RootSignature::Default::RC_Consts].InitAsConstants(ShadingConvention::GBuffer::RootConstant::Default::Count, 3);
	slotRootParameter[RootSignature::Default::SI_VertexBuffer].InitAsShaderResourceView(0);
	slotRootParameter[RootSignature::Default::SI_IndexBuffer].InitAsShaderResourceView(1);
	slotRootParameter[RootSignature::Default::SI_Textures].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		static_cast<UINT>(samplers.size()), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
		mInitData.Device, 
		rootSigDesc, 
		IID_PPV_ARGS(&mRootSignature),
		L"GBuffer_GR_Default"));

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildPipelineStates() {
	// GraphicsPipelineState
	{
		const auto inputLayout = Foundation::Util::D3D12Util::InputLayoutDesc();
		auto psoDesc = Foundation::Util::D3D12Util::DefaultPsoDesc(inputLayout, ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_GBuffer]);
			NullCheck(mpLogFile, VS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_GBuffer]);
			NullCheck(mpLogFile, PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = NumRenderTargtes;
		psoDesc.RTVFormats[0] = ShadingConvention::GBuffer::AlbedoMapFormat;
		psoDesc.RTVFormats[1] = ShadingConvention::GBuffer::NormalMapFormat;
		psoDesc.RTVFormats[2] = ShadingConvention::GBuffer::RMSMapFormat;
		psoDesc.RTVFormats[3] = ShadingConvention::GBuffer::VelocityMapFormat;
		psoDesc.RTVFormats[4] = ShadingConvention::GBuffer::PositionMapFormat;
		psoDesc.DSVFormat = ShadingConvention::DepthStencilBuffer::DepthBufferFormat;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc, 
			IID_PPV_ARGS(&mPipelineState), 
			L"GBuffer_GP_Default"));
	}

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	mhAlbedoMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhAlbedoMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhAlbedoMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	mhNormalMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhNormalMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhNormalMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	mhRMSMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhRMSMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhRMSMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	mhVelocityMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhVelocityMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhVelocityMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	mhPositionMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhPositionMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhPositionMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL GBuffer::GBufferClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc = {};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Width = mInitData.ClientWidth;
	rscDesc.Height = mInitData.ClientHeight;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// AlbedoMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::AlbedoMapFormat;

		const CD3DX12_CLEAR_VALUE AlbedoMapOptClear(
			ShadingConvention::GBuffer::AlbedoMapFormat, 
			ShadingConvention::GBuffer::AlbedoMapClearValues);

		CheckReturn(mpLogFile, mAlbedoMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&AlbedoMapOptClear,
			L"GBuffer_AlbedoMap"));
	}
	// NormalMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::NormalMapFormat;

		const CD3DX12_CLEAR_VALUE NormalMapOptClear(
			ShadingConvention::GBuffer::NormalMapFormat,
			ShadingConvention::GBuffer::NormalMapClearValues);

		CheckReturn(mpLogFile, mNormalMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&NormalMapOptClear,
			L"GBuffer_NormalMap"));
	}
	// RMSMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::RMSMapFormat;

		const CD3DX12_CLEAR_VALUE RMSMapOptClear(
			ShadingConvention::GBuffer::RMSMapFormat,
			ShadingConvention::GBuffer::RMSMapClearValues);

		CheckReturn(mpLogFile, mRMSMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&RMSMapOptClear,
			L"GBuffer_RMSMap"));
	}
	// VelocityMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::VelocityMapFormat;

		const CD3DX12_CLEAR_VALUE VelocityMapOptClear(
			ShadingConvention::GBuffer::VelocityMapFormat,
			ShadingConvention::GBuffer::VelocityMapClearValues);

		CheckReturn(mpLogFile, mVelocityMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&VelocityMapOptClear,
			L"GBuffer_VelocityMap"));
	}
	// PositionMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::PositionMapFormat;

		const CD3DX12_CLEAR_VALUE PositionMapOptClear(
			ShadingConvention::GBuffer::PositionMapFormat,
			ShadingConvention::GBuffer::PositionMapClearValues);

		CheckReturn(mpLogFile, mPositionMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&PositionMapOptClear,
			L"GBuffer_PositionMap"));
	}

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	// AlbedoMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::AlbedoMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::AlbedoMapFormat;

		const auto AlbedoMap = mAlbedoMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, AlbedoMap, &srvDesc, mhAlbedoMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, AlbedoMap, &rtvDesc, mhAlbedoMapCpuRtv);
	}
	// NormalMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::NormalMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::NormalMapFormat;

		const auto NormalMap = mNormalMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, NormalMap, &srvDesc, mhNormalMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, NormalMap, &rtvDesc, mhNormalMapCpuRtv);
	}
	// RMSMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::RMSMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::RMSMapFormat;

		const auto RMSMap = mRMSMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, RMSMap, &srvDesc, mhRMSMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, RMSMap, &rtvDesc, mhRMSMapCpuRtv);
	}
	// PositionMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::PositionMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::PositionMapFormat;

		const auto PositionMap = mPositionMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, PositionMap, &srvDesc, mhPositionMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, PositionMap, &rtvDesc, mhPositionMapCpuRtv);
	}
	// VelocityMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::VelocityMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::VelocityMapFormat;

		const auto VelocityMap = mVelocityMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, VelocityMap, &srvDesc, mhVelocityMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, VelocityMap, &rtvDesc, mhVelocityMapCpuRtv);
	}

	return TRUE;
}
