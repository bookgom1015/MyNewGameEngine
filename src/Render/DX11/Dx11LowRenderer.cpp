#include "Render/DX11/Dx11LowRenderer.hpp"

using namespace Render::DX11;

Dx11LowRenderer::Dx11LowRenderer() {}

Dx11LowRenderer::~Dx11LowRenderer() {}

BOOL Dx11LowRenderer::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Common::Foundation::Core::WindowsManager* const pWndManager,
		Common::ImGuiManager::ImGuiManager* const pImGuiManager,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pShadingArgSet,
		UINT width, UINT height) {


	return TRUE;}

void Dx11LowRenderer::CleanUp() {}

BOOL Dx11LowRenderer::OnResize(UINT width, UINT height) {
	return TRUE;
}

BOOL Dx11LowRenderer::Update(FLOAT deltaTime) {
	return TRUE;
}

BOOL Dx11LowRenderer::Draw() {
	return TRUE;
}