#include "Render/VK/VkRenderer.hpp"
#include "Common/Debug/Logger.hpp"

using namespace Render::VK;

extern "C" RendererAPI Common::Render::Renderer* Render::CreateRenderer() {
	return new VkRenderer();
}

extern "C" RendererAPI void Render::DestroyRenderer(Common::Render::Renderer* const renderer) {
	delete renderer;
}

VkRenderer::VkRenderer() {

}

VkRenderer::~VkRenderer() {
	CleanUp();
}

BOOL VkRenderer::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) {
	CheckReturn(mpLogFile, VkLowRenderer::Initialize(pLogFile, hWnd, width, height));

	return TRUE;
}

void VkRenderer::CleanUp() {
	VkLowRenderer::CleanUp();
}

BOOL VkRenderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, VkLowRenderer::OnResize(width, height));

	return TRUE;
}

BOOL VkRenderer::Update(FLOAT deltaTime) {
	return TRUE;
}

BOOL VkRenderer::Draw() {
	return TRUE;
}

BOOL VkRenderer::AddMesh() {
	return TRUE;
}

BOOL VkRenderer::RemoveMesh() {
	return TRUE;
}
