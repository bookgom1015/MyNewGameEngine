#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Dx11Renderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Core/SwapChain.hpp"

using namespace Render::DX11;

extern "C" RendererAPI Common::Render::Renderer* Render::CreateRenderer() {
	return new Dx11Renderer();
}

extern "C" RendererAPI void Render::DestroyRenderer(Common::Render::Renderer* const renderer) {
	delete renderer;
}

Dx11Renderer::Dx11Renderer() {}

Dx11Renderer::~Dx11Renderer() {}

BOOL Dx11Renderer::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Common::Foundation::Core::WindowsManager* const pWndManager,
		Common::ImGuiManager::ImGuiManager* const pImGuiManager,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		UINT width, UINT height) {
	CheckReturn(pLogFile, Dx11LowRenderer::Initialize(
		pLogFile, pWndManager, pImGuiManager, pArgSet, width, height));

	return TRUE;
}

void Dx11Renderer::CleanUp() {
	Dx11LowRenderer::CleanUp();
}

BOOL Dx11Renderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, Dx11LowRenderer::OnResize(width, height));

#ifdef _DEBUG
	std::cout << std::format("Dx11Renderer resized (Width: {}, Height: {}", width, height) << std::endl;
#endif

	return TRUE;
}

BOOL Dx11Renderer::Update(FLOAT deltaTime) {
	return TRUE;
}

BOOL Dx11Renderer::Draw() {
	return TRUE;
}

BOOL Dx11Renderer::AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Mesh::Transform* const pTransform, Common::Foundation::Hash& hash) {
	return TRUE;
}

BOOL Dx11Renderer::UpdateMeshTransform(Common::Foundation::Hash hash, Common::Foundation::Mesh::Transform* const pTransform) {
	return TRUE;
}

void Dx11Renderer::RemoveMesh(Common::Foundation::Hash hash) {}
