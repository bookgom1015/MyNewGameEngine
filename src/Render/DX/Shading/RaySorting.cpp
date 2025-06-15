#include "Render/DX/Shading/RaySorting.hpp"
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
	const WCHAR* const HLSL_CountingSort_Rays_64x128 = L"CountingSort_Rays_64x128.hlsl";
}

RaySorting::InitDataPtr RaySorting::MakeInitData() {
	return std::unique_ptr<RaySortingClass::InitData>(new RaySortingClass::InitData());
}

RaySorting::RaySortingClass::RaySortingClass() {

}

UINT RaySorting::RaySortingClass::CbvSrvUavDescCount() const { return 0; }

UINT RaySorting::RaySortingClass::RtvDescCount() const { return 0; }

UINT RaySorting::RaySortingClass::DsvDescCount() const { return 0; }

BOOL RaySorting::RaySortingClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL RaySorting::RaySortingClass::CompileShaders() {
	//const auto CS = Util::ShaderManager::D3D12ShaderInfo(HLSL_CountingSort_Rays_64x128, L"CS", L"cs_6_5");
	//CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(CS, mShaderHashes[Shader::CS_CountingSort]));

	return TRUE;
}

BOOL RaySorting::RaySortingClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {
	return TRUE;
}

BOOL RaySorting::RaySortingClass::BuildPipelineStates() {
	return TRUE;
}

BOOL RaySorting::RaySortingClass::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	return TRUE;
}

BOOL RaySorting::RaySortingClass::OnResize(UINT width, UINT height) {
	return TRUE;
}
