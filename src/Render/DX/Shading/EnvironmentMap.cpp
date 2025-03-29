#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"

using namespace Render::DX::Shading;

UINT EnvironmentMap::CbvSrvUavDescCount() const { return 0; }
UINT EnvironmentMap::RtvDescCount() const {	return 0; }
UINT EnvironmentMap::DsvDescCount() const {	return 0; }

EnvironmentMap::InitDataPtr EnvironmentMap::MakeInitData() {
	return std::unique_ptr<InitData>(new InitData());
}

BOOL EnvironmentMap::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL EnvironmentMap::CompileShaders() {
	return TRUE;
}

BOOL EnvironmentMap::BuildRootSignatures() {
	return TRUE;
}

BOOL EnvironmentMap::BuildPipelineStates() {
	return TRUE;
}

BOOL EnvironmentMap::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	return TRUE;
}
