#include "Render/DX/Shading/EyeAdaption.hpp"
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
	const WCHAR* const HLSL_LuminanceHistogram = L"LuminanceHistogram.hlsl";
	const WCHAR* const HLSL_PercentileExtract = L"PercentileExtract.hlsl";
}

EyeAdaption::InitDataPtr EyeAdaption::MakeInitData() {
	return std::unique_ptr<EyeAdaptionClass::InitData>(new EyeAdaptionClass::InitData());
}

EyeAdaption::EyeAdaptionClass::EyeAdaptionClass() {

}

UINT EyeAdaption::EyeAdaptionClass::CbvSrvUavDescCount() const { return 0; }

UINT EyeAdaption::EyeAdaptionClass::RtvDescCount() const { return 0; }

UINT EyeAdaption::EyeAdaptionClass::DsvDescCount() const { return 0; }


BOOL EyeAdaption::EyeAdaptionClass::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::CompileShaders() {
	// LuminanceHistogram
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_LuminanceHistogram, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Shader::CS_LuminanceHistogram]));
	}
	// PercentileExtract
	{
		const auto CS = Util::ShaderManager::D3D12ShaderInfo(
			HLSL_PercentileExtract, L"CS", L"cs_6_5");
		CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Shader::CS_PercentileExtract]));
	}
	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	return TRUE;
}

BOOL EyeAdaption::EyeAdaptionClass::BuildPipelineStates() {
	return TRUE;
}
