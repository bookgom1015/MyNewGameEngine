#include "Render/DX/Shading/GBuffer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/RenderItem.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Resource/MaterialData.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX::Shading;

namespace {
	const UINT NumRenderTargtes = 8;

	const WCHAR* const HLSL_GBuffer = L"GBuffer.hlsl";
}

GBuffer::InitDataPtr GBuffer::MakeInitData() {
	return std::unique_ptr<GBufferClass::InitData>(new GBufferClass::InitData());
}

GBuffer::GBufferClass::GBufferClass() {
	mAlbedoMap = std::make_unique<Foundation::Resource::GpuResource>();
	mNormalMap = std::make_unique<Foundation::Resource::GpuResource>();
	mNormalDepthMap = std::make_unique<Foundation::Resource::GpuResource>();
	mReprojNormalDepthMap = std::make_unique<Foundation::Resource::GpuResource>();
	mCachedNormalDepthMap = std::make_unique<Foundation::Resource::GpuResource>();
	mSpecularMap = std::make_unique<Foundation::Resource::GpuResource>();
	mRoughnessMetalnessMap = std::make_unique<Foundation::Resource::GpuResource>();
	mVelocityMap = std::make_unique<Foundation::Resource::GpuResource>();
	mPositionMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT GBuffer::GBufferClass::CbvSrvUavDescCount() const { return NumRenderTargtes
	+ 1 // CachedNormalDepthMap
	; 
}

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

BOOL GBuffer::GBufferClass::BuildRootSignatures() {
	decltype(auto) samplers = Util::SamplerUtil::GetStaticSamplers();

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
		Util::StaticSamplerCount, samplers,
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
	if (mInitData.MeshShaderSupported) {
		auto psoDesc = Foundation::Util::D3D12Util::DefaultMeshPsoDesc(ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_GBuffer]);
			NullCheck(mpLogFile, MS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_GBuffer]);
			NullCheck(mpLogFile, PS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = NumRenderTargtes;
		psoDesc.RTVFormats[0] = ShadingConvention::GBuffer::AlbedoMapFormat;
		psoDesc.RTVFormats[1] = ShadingConvention::GBuffer::NormalMapFormat;
		psoDesc.RTVFormats[2] = ShadingConvention::GBuffer::NormalDepthMapFormat;
		psoDesc.RTVFormats[3] = ShadingConvention::GBuffer::NormalDepthMapFormat;
		psoDesc.RTVFormats[4] = ShadingConvention::GBuffer::SpecularMapFormat;
		psoDesc.RTVFormats[5] = ShadingConvention::GBuffer::RoughnessMetalnessMapFormat;
		psoDesc.RTVFormats[6] = ShadingConvention::GBuffer::VelocityMapFormat;
		psoDesc.RTVFormats[7] = ShadingConvention::GBuffer::PositionMapFormat;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_GBuffer]),
			L"GBuffer_MP_Default"));
	}
	else {
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
		psoDesc.RTVFormats[2] = ShadingConvention::GBuffer::NormalDepthMapFormat;
		psoDesc.RTVFormats[3] = ShadingConvention::GBuffer::NormalDepthMapFormat;
		psoDesc.RTVFormats[4] = ShadingConvention::GBuffer::SpecularMapFormat;
		psoDesc.RTVFormats[5] = ShadingConvention::GBuffer::RoughnessMetalnessMapFormat;
		psoDesc.RTVFormats[6] = ShadingConvention::GBuffer::VelocityMapFormat;
		psoDesc.RTVFormats[7] = ShadingConvention::GBuffer::PositionMapFormat;

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc, 
			IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_GBuffer]),
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

	mhNormalDepthMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhNormalDepthMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhNormalDepthMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	mhReprojNormalDepthMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhReprojNormalDepthMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhReprojNormalDepthMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	mhCachedNormalDepthMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhCachedNormalDepthMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);

	mhSpecularMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhSpecularMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhSpecularMapCpuRtv = pDescHeap->RtvCpuOffset(1);

	mhRoughnessMetalnessMapCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhRoughnessMetalnessMapGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhRoughnessMetalnessMapCpuRtv = pDescHeap->RtvCpuOffset(1);

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

BOOL GBuffer::GBufferClass::DrawGBuffer(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport, 
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const backBuffer, 
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const depthBuffer, 
		D3D12_CPU_DESCRIPTOR_HANDLE do_depthBuffer,
		const std::vector<Render::DX::Foundation::RenderItem*>& ritems,
		FLOAT ditheringMaxDist, FLOAT ditheringMinDist) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? PipelineState::MP_GBuffer : PipelineState::GP_GBuffer].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	CheckReturn(mpLogFile, CacheNormalDepth(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		mAlbedoMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mReprojNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mSpecularMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mRoughnessMetalnessMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mVelocityMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		CmdList->ClearRenderTargetView(mhAlbedoMapCpuRtv, ShadingConvention::GBuffer::AlbedoMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(mhNormalMapCpuRtv, ShadingConvention::GBuffer::NormalMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(mhNormalDepthMapCpuRtv, ShadingConvention::GBuffer::NormalDepthMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(mhReprojNormalDepthMapCpuRtv, ShadingConvention::GBuffer::NormalDepthMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(mhSpecularMapCpuRtv, ShadingConvention::GBuffer::SpecularMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(mhRoughnessMetalnessMapCpuRtv, ShadingConvention::GBuffer::RoughnessMetalnessMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(mhVelocityMapCpuRtv, ShadingConvention::GBuffer::VelocityMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(mhPositionMapCpuRtv, ShadingConvention::GBuffer::PositionMapClearValues, 0, nullptr);
		CmdList->ClearDepthStencilView(
			do_depthBuffer, 
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
			ShadingConvention::DepthStencilBuffer::InvalidDepthValue, 
			ShadingConvention::DepthStencilBuffer::InvalidStencilValue,
			0, nullptr);

		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, NumRenderTargtes> renderTargets = {
			mhAlbedoMapCpuRtv,
			mhNormalMapCpuRtv,
			mhNormalDepthMapCpuRtv,
			mhReprojNormalDepthMapCpuRtv,
			mhSpecularMapCpuRtv,
			mhRoughnessMetalnessMapCpuRtv,
			mhVelocityMapCpuRtv,
			mhPositionMapCpuRtv
		};

		CmdList->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), TRUE, &do_depthBuffer);

		CmdList->SetGraphicsRootConstantBufferView(
			RootSignature::Default::CB_Pass, 
			pFrameResource->MainPassCB.CBAddress());

		CheckReturn(mpLogFile, DrawRenderItems(pFrameResource, CmdList, ritems, ditheringMaxDist, ditheringMinDist));
	}

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

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

	// AlbedoMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::AlbedoMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

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
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

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
	// NormalDepthMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE NormalDepthMapOptClear(
			ShadingConvention::GBuffer::NormalDepthMapFormat,
			ShadingConvention::GBuffer::NormalDepthMapClearValues);

		CheckReturn(mpLogFile, mNormalDepthMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&NormalDepthMapOptClear,
			L"GBuffer_NormalDepthMap"));
	}
	// ReprojectedNormalDepthMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE NormalDepthMapOptClear(
			ShadingConvention::GBuffer::NormalDepthMapFormat,
			ShadingConvention::GBuffer::NormalDepthMapClearValues);

		CheckReturn(mpLogFile, mReprojNormalDepthMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&NormalDepthMapOptClear,
			L"GBuffer_ReprojectedNormalDepthMap"));
	}
	// CachedNormalDepthMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		CheckReturn(mpLogFile, mCachedNormalDepthMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr ,
			L"GBuffer_CachedNormalDepthMap"));
	}
	// SpecularMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::SpecularMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE RMSMapOptClear(
			ShadingConvention::GBuffer::SpecularMapFormat,
			ShadingConvention::GBuffer::SpecularMapClearValues);

		CheckReturn(mpLogFile, mSpecularMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&RMSMapOptClear,
			L"GBuffer_SpecularMap"));
	}
	// RoughnessMetallicMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::RoughnessMetalnessMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE RMSMapOptClear(
			ShadingConvention::GBuffer::RoughnessMetalnessMapFormat,
			ShadingConvention::GBuffer::RoughnessMetalnessMapClearValues);

		CheckReturn(mpLogFile, mRoughnessMetalnessMap->Initialize(
			mInitData.Device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&RMSMapOptClear,
			L"GBuffer_RoughnessMetallicMap"));
	}
	// VelocityMap
	{
		rscDesc.Format = ShadingConvention::GBuffer::VelocityMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

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
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

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
	// NormalDepthMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;

		const auto NormalDepthMap = mNormalDepthMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, NormalDepthMap, &srvDesc, mhNormalDepthMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, NormalDepthMap, &rtvDesc, mhNormalDepthMapCpuRtv);
	}
	// ReprojectedNormalDepthMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;

		const auto PrevNormalDepthMap = mReprojNormalDepthMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, PrevNormalDepthMap, &srvDesc, mhReprojNormalDepthMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, PrevNormalDepthMap, &rtvDesc, mhReprojNormalDepthMapCpuRtv);
	}
	// CachedNormalDepthMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::NormalDepthMapFormat;

		const auto CachedNormalDepthMap = mCachedNormalDepthMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, CachedNormalDepthMap, &srvDesc, mhCachedNormalDepthMapCpuSrv);
	}
	// SpecularMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::SpecularMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::SpecularMapFormat;

		const auto RMSMap = mSpecularMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, RMSMap, &srvDesc, mhSpecularMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, RMSMap, &rtvDesc, mhSpecularMapCpuRtv);
	}
	// RoughnessMetallicMap
	{
		srvDesc.Format = ShadingConvention::GBuffer::RoughnessMetalnessMapFormat;
		rtvDesc.Format = ShadingConvention::GBuffer::RoughnessMetalnessMapFormat;

		const auto RMSMap = mRoughnessMetalnessMap->Resource();
		Foundation::Util::D3D12Util::CreateShaderResourceView(mInitData.Device, RMSMap, &srvDesc, mhRoughnessMetalnessMapCpuSrv);
		Foundation::Util::D3D12Util::CreateRenderTargetView(mInitData.Device, RMSMap, &rtvDesc, mhRoughnessMetalnessMapCpuRtv);
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

BOOL GBuffer::GBufferClass::DrawRenderItems(
		Foundation::Resource::FrameResource* const pFrameResource,
		ID3D12GraphicsCommandList6* const pCmdList,
		const std::vector<Render::DX::Foundation::RenderItem*>& ritems,
		FLOAT ditheringMaxDist, FLOAT ditheringMinDist) {
	for (size_t i = 0, end = ritems.size(); i < end; ++i) {
		const auto ri = ritems[i];

		pCmdList->SetGraphicsRootConstantBufferView(
			RootSignature::Default::CB_Object, 
			pFrameResource->ObjectCB.CBAddress(ri->ObjectCBIndex));
		
		pCmdList->SetGraphicsRootConstantBufferView(
			RootSignature::Default::CB_Material, 
			pFrameResource->MaterialCB.CBAddress(ri->Material->MaterialCBIndex));

		ShadingConvention::GBuffer::RootConstant::Default::Struct rc;
		rc.gTexDim = { mInitData.ClientWidth, mInitData.ClientHeight };
		rc.gVertexCount = ri->Geometry->VertexBufferByteSize / ri->Geometry->VertexByteStride;
		rc.gIndexCount = ri->Geometry->IndexBufferByteSize / ri->Geometry->IndexByteStride;
		rc.gDitheringMaxDist = ditheringMaxDist;
		rc.gDitheringMinDist = ditheringMinDist;

		std::array<std::uint32_t, ShadingConvention::GBuffer::RootConstant::Default::Count> consts;
		std::memcpy(consts.data(), &rc, sizeof(ShadingConvention::GBuffer::RootConstant::Default::Struct));

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::GBuffer::RootConstant::Default::Struct>(
			RootSignature::Default::RC_Consts,
			ShadingConvention::GBuffer::RootConstant::Default::Count,
			consts.data(),
			0,
			pCmdList,
			FALSE);

		if (mInitData.MeshShaderSupported) {
			pCmdList->SetGraphicsRootShaderResourceView(RootSignature::Default::SI_VertexBuffer, ri->Geometry->VertexBufferGPU->GetGPUVirtualAddress());
			pCmdList->SetGraphicsRootShaderResourceView(RootSignature::Default::SI_IndexBuffer, ri->Geometry->IndexBufferGPU->GetGPUVirtualAddress());

			const UINT PrimCount = rc.gIndexCount / 3;

			pCmdList->DispatchMesh(
				Foundation::Util::D3D12Util::CeilDivide(PrimCount, ShadingConvention::GBuffer::ThreadGroup::MeshShader::ThreadsPerGroup),
				1,
				1);
		}
		else {
			pCmdList->IASetVertexBuffers(0, 1, &ri->Geometry->VertexBufferView());
			pCmdList->IASetIndexBuffer(&ri->Geometry->IndexBufferView());
			pCmdList->IASetPrimitiveTopology(ri->PrimitiveType);

			pCmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
		}
	}

	return TRUE;
}

BOOL GBuffer::GBufferClass::CacheNormalDepth(ID3D12GraphicsCommandList6* const pCmdList) {
	mNormalDepthMap->Transite(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	mCachedNormalDepthMap->Transite(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

	pCmdList->CopyResource(mCachedNormalDepthMap->Resource(), mNormalDepthMap->Resource());

	return TRUE;
}