#include "Render/DX/Shading/Bloom.hpp"
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
	const WCHAR* const HLSL_ExtractHighlights = L"ExtractHighlights.hlsl";
	const WCHAR* const HLSL_ApplyBloom = L"ApplyBloom.hlsl";
}

Bloom::InitDataPtr Bloom::MakeInitData() {
	return std::unique_ptr<BloomClass::InitData>(new BloomClass::InitData());
}

Bloom::BloomClass::BloomClass() {
	for (UINT i = 0; i < Resource::Count; ++i)
		mHighlightMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT Bloom::BloomClass::CbvSrvUavDescCount() const { return Resource::Count; }

UINT Bloom::BloomClass::RtvDescCount() const { return Resource::Count; }

UINT Bloom::BloomClass::DsvDescCount() const { return 0; }

BOOL Bloom::BloomClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL Bloom::BloomClass::CompileShaders() {
	// ExtractHighlights
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ExtractHighlights, L"VS", L"vs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ExtractHighlights]));
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ExtractHighlights, L"MS", L"ms_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_ExtractHighlights]));
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ExtractHighlights, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ExtractHighlights]));
	}
	// ApplyBloom
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"VS", L"vs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_ApplyBloom]));
		const auto MS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"MS", L"ms_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(MS, mShaderHashes[Shader::MS_ApplyBloom]));
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_ApplyBloom]));
	}

	return TRUE;
}

BOOL Bloom::BloomClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	// ExtractHighlights
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ExtractHighlights::Count] = {};
		slotRootParameter[RootSignature::ExtractHighlights::RC_Consts].InitAsConstants(
			ShadingConvention::SSCS::RootConstant::ComputeContactShadow::Count, 0);
		slotRootParameter[RootSignature::ExtractHighlights::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
		
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ExtractHighlights]),
			L"Bloom_GR_ExtractHighlights"));
	}
	// ApplyBloom
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2] = {}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[RootSignature::ApplyBloom::Count] = {};
		slotRootParameter[RootSignature::ApplyBloom::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[RootSignature::ApplyBloom::SI_BloomMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			static_cast<UINT>(samplers.size()), samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[RootSignature::GR_ApplyBloom]),
			L"Bloom_GR_ApplyBloom"));
	}

	return TRUE;
}

BOOL Bloom::BloomClass::BuildPipelineStates() {
	// ExtractHighlights
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ExtractHighlights].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_ExtractHighlights]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ExtractHighlights]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_ExtractHighlights]),
				L"Bloom_MP_ExtractHighlights"));
		}
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ExtractHighlights].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ExtractHighlights]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ExtractHighlights]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ExtractHighlights]),
				L"Bloom_GP_ExtractHighlights"));
		}
	}
	// ApplyBloom
	{
		if (mInitData.MeshShaderSupported) {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ApplyBloom].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::MS_ApplyBloom]);
				NullCheck(mpLogFile, MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ApplyBloom]);
				NullCheck(mpLogFile, PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::MP_ApplyBloom]),
				L"Bloom_MP_ApplyBloom"));
		}
		else {
			auto psoDesc = Foundation::Util::D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[RootSignature::GR_ApplyBloom].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_ApplyBloom]);
				NullCheck(mpLogFile, VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_ApplyBloom]);
				NullCheck(mpLogFile, PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[PipelineState::GP_ApplyBloom]),
				L"Bloom_GP_ApplyBloom"));
		}
	}

	return TRUE;
}

BOOL Bloom::BloomClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	// HighlightMap
	for (UINT i = 0; i < Resource::Count; ++i) {
		mhHighlightMapCpuSrvs[i] = pDescHeap->CbvSrvUavCpuOffset(1);
		mhHighlightMapGpuSrvs[i] = pDescHeap->CbvSrvUavGpuOffset(1);

		mhHighlightMapCpuRtvs[i] = pDescHeap->RtvCpuOffset(1);
	}

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL Bloom::BloomClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL Bloom::BloomClass::ExtractHighlights(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer,
		FLOAT threshold, FLOAT softknee) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ? 
		PipelineState::MP_ExtractHighlights : PipelineState::GP_ExtractHighlights].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ExtractHighlights].Get());

		D3D12_VIEWPORT viewport = { 
			0, 0, static_cast<FLOAT>(mInitData.ClientWidth), static_cast<FLOAT>(mInitData.ClientHeight), 0.f, 1.f };
		D3D12_RECT scissorRect = { 
			0,0,static_cast<LONG>(mInitData.ClientWidth), static_cast<LONG>(mInitData.ClientHeight) };

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		const auto HighlightMap = mHighlightMaps[0].get();

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		HighlightMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

		CmdList->OMSetRenderTargets(1, &mhHighlightMapCpuRtvs[0], TRUE, nullptr);

		ShadingConvention::Bloom::RootConstant::ExtractHighlights::Struct rc;
		rc.gThreshold = threshold;
		rc.gSoftKnee = softknee;

		Foundation::Util::D3D12Util::SetRoot32BitConstants<ShadingConvention::Bloom::RootConstant::ExtractHighlights::Struct>(
			RootSignature::ExtractHighlights::RC_Consts,
			ShadingConvention::Bloom::RootConstant::ExtractHighlights::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ExtractHighlights::SI_BackBuffer, si_backBuffer);

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

BOOL Bloom::BloomClass::BlurHighlights(
	std::function<BOOL(
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const,
		D3D12_GPU_DESCRIPTOR_HANDLE,
		Foundation::Resource::GpuResource* const,
		D3D12_CPU_DESCRIPTOR_HANDLE,
		UINT, UINT)> func) {
	Resource::Type input = Resource::E_FullRes;
	Resource::Type output = Resource::E_QuaterRes;

	UINT inputWidth = mInitData.ClientWidth;
	UINT inputHeight = mInitData.ClientHeight;

	UINT outputWidth = mInitData.ClientWidth >> 1; 
	UINT outputHeight = mInitData.ClientHeight >> 1;

	for (UINT i = 1; i < Resource::Count; ++i) {
		const auto pInputMap = mHighlightMaps[input].get();
		const auto si_inputMap = mhHighlightMapGpuSrvs[input];

		const auto pOutputMap = mHighlightMaps[output].get();
		const auto ro_outputMap = mhHighlightMapCpuRtvs[output];

		D3D12_VIEWPORT viewport = {
			0, 0, static_cast<FLOAT>(outputWidth), static_cast<FLOAT>(outputHeight), 0.f, 1.f };
		D3D12_RECT scissorRect = {
			0, 0, static_cast<LONG>(outputWidth), static_cast<LONG>(outputHeight) };

		CheckReturn(mpLogFile, func(
			viewport,
			scissorRect,
			pInputMap,
			si_inputMap,
			pOutputMap,
			ro_outputMap,
			inputWidth,
			inputHeight));

		input = static_cast<Resource::Type>(static_cast<UINT>(input) + 1);
		output = static_cast<Resource::Type>(static_cast<UINT>(output) + 1);

		inputWidth = inputWidth >> 1;
		inputHeight = inputHeight >> 1;

		outputWidth = outputWidth >> 1;
		outputHeight = outputHeight >> 1;
	}

	return TRUE;
}

BOOL Bloom::BloomClass::ApplyBloom(
		Foundation::Resource::FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		Foundation::Resource::GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		Foundation::Resource::GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0,
		mPipelineStates[mInitData.MeshShaderSupported ?
		PipelineState::MP_ApplyBloom : PipelineState::GP_ApplyBloom].Get()));

	const auto CmdList = mInitData.CommandObject->CommandList(0);
	mInitData.DescriptorHeap->SetDescriptorHeap(CmdList);

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[RootSignature::GR_ApplyBloom].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		const auto HighlightMap = mHighlightMaps[0].get();
		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		HighlightMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ApplyBloom::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(RootSignature::ApplyBloom::SI_BloomMap, mhHighlightMapGpuSrvs[0]);

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

BOOL Bloom::BloomClass::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// HighlightMap
	{
		texDesc.Format = ShadingConvention::Bloom::HighlightMapFormat;

		UINT texW = mInitData.ClientWidth;
		UINT texH = mInitData.ClientHeight;

		for (UINT i = 0; i < Resource::Count; ++i) {
			texDesc.Width = texW;
			texDesc.Height = texH;

			std::wstringstream name;

			name << L"Bloom_HighlightMap_" << i;

			CheckReturn(mpLogFile, mHighlightMaps[i]->Initialize(
				mInitData.Device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				name.str().c_str()));

			texW = texW >> 1;
			texH = texH >> 1;
		}
	}

	return TRUE;
}

BOOL Bloom::BloomClass::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	// HighlightMap
	{
		srvDesc.Format = ShadingConvention::Bloom::HighlightMapFormat;
		rtvDesc.Format = ShadingConvention::Bloom::HighlightMapFormat;

		for (UINT i = 0; i < Resource::Count; ++i) {
			const auto resource = mHighlightMaps[i]->Resource();

			Foundation::Util::D3D12Util::CreateShaderResourceView(
				mInitData.Device, resource, &srvDesc, mhHighlightMapCpuSrvs[i]);
			Foundation::Util::D3D12Util::CreateRenderTargetView(
				mInitData.Device, resource, &rtvDesc, mhHighlightMapCpuRtvs[i]);
		}
	}

	return TRUE;
}
