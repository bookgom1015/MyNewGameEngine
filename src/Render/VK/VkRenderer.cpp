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

BOOL VkRenderer::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Common::Foundation::Core::WindowsManager* const pWndManager,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		UINT width, UINT height) {
	CheckReturn(mpLogFile, VkLowRenderer::Initialize(pLogFile, pWndManager, pArgSet, width, height));

	return TRUE;
}

void VkRenderer::CleanUp() {
	VkLowRenderer::CleanUp();
}

BOOL VkRenderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, VkLowRenderer::OnResize(width, height));

#ifdef _DEBUG
	std::cout << "VkRenderer resized (Width: " << width << " Height: " << height << ")" << std::endl;
#endif

	return TRUE;
}

BOOL VkRenderer::Update(FLOAT deltaTime) {
	return TRUE;
}

BOOL VkRenderer::Draw() {
	return TRUE;
}

BOOL VkRenderer::AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Hash& hash) {
	return TRUE;
}

void VkRenderer::RemoveMesh(Common::Foundation::Hash hash) {

}
