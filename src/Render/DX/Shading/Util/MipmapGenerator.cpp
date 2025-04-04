#include "Render/DX/Shading/Util/MipmapGenerator.hpp"

using namespace Render::DX::Shading::Util;

MipmapGenerator::InitDataPtr MipmapGenerator::MakeInitData() {
	return std::unique_ptr<MipmapGeneratorClass::InitData>(new MipmapGeneratorClass::InitData());
}

BOOL MipmapGenerator::MipmapGeneratorClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::CompileShaders() {

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) {

	return TRUE;
}

BOOL MipmapGenerator::MipmapGeneratorClass::BuildPipelineStates() {

	return TRUE;
}
