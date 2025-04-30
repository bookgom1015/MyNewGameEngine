#include "Render/DX/Shading/Shadow.hpp"
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

using namespace Render::DX::Shading;

namespace {
	const WCHAR* const HLSL_DrawZDepth = L"DrawZDepth.hlsl";
	const WCHAR* const HLSL_DrawShadow = L"DrawShadow.hlsl";
}

Shadow::InitDataPtr Shadow::MakeInitData() {
	return std::unique_ptr<ShadowClass::InitData>(new ShadowClass::InitData());
}

Shadow::ShadowClass::ShadowClass() {
	for (UINT i = 0; i < MaxLights; ++i) {
		mZDepthMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
		mZDepthCubeMaps[i] = std::make_unique<Foundation::Resource::GpuResource>();
	}
	mShadowMap = std::make_unique<Foundation::Resource::GpuResource>();
}

UINT Shadow::ShadowClass::CbvSrvUavDescCount() const { return 0
	+ MaxLights // ZDepthMaps
	+ MaxLights // ZDepthCubeMaps
	+ 1 // ShadowMap
	; 
}

UINT Shadow::ShadowClass::RtvDescCount() const { return 0; }

UINT Shadow::ShadowClass::DsvDescCount() const { return 0
	+ MaxLights // ZDepthMaps
	+ MaxLights // ZDepthCubeMaps
	+ 1 // ShadowMap
	; 
}

BOOL Shadow::ShadowClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());

	return TRUE;
}

BOOL Shadow::ShadowClass::CompileShaders() {
	// DrawZDepth
	{
		const auto VS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawZDepth, L"VS", L"vs_6_5");
		const auto GS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawZDepth, L"GS", L"gs_6_5");
		const auto PS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawZDepth, L"PS", L"ps_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_DrawZDepth]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(GS, mShaderHashes[Shader::GS_DrawZDepth]));
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_DrawZDepth]));
	}
	// DrawShadow
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_DrawShadow, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_DrawShadow]));
	}

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	return TRUE;
}

BOOL Shadow::ShadowClass::BuildPipelineStates() {
	return TRUE;
}

BOOL Shadow::ShadowClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {


	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildResources() {
	return TRUE;
}

BOOL Shadow::ShadowClass::BuildDescriptors() {
	return TRUE;
}
