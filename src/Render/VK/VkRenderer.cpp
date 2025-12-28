#include "Render/VK/VkRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Render/VK/Shading/Util/ShaderManager.hpp"

using namespace Render::VK;

extern "C" RendererAPI Common::Render::Renderer* Render::CreateRenderer() {
	return new VkRenderer();
}

extern "C" RendererAPI void Render::DestroyRenderer(Common::Render::Renderer* const renderer) {
	delete renderer;
}

VkRenderer::VkRenderer() {
	mShaderManager = std::make_unique<Shading::Util::ShaderManager>();
}

VkRenderer::~VkRenderer() {
	CleanUp();
}

BOOL VkRenderer::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Common::Foundation::Core::WindowsManager* const pWndManager,
		Common::ImGuiManager::ImGuiManager* const pImGuiManager,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		UINT width, UINT height) {
	CheckReturn(mpLogFile, VkLowRenderer::Initialize(pLogFile, pWndManager, pImGuiManager, pArgSet, width, height));

	CheckReturn(mpLogFile, mShaderManager->Initialize(
		mpLogFile, 
		mDevice.get(), 
		static_cast<UINT>(mProcessor->Logical)));

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

BOOL VkRenderer::AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Mesh::Transform* const pTransform, Common::Foundation::Hash& hash) {
	return TRUE;
}

BOOL VkRenderer::UpdateMeshTransform(Common::Foundation::Hash hash, Common::Foundation::Mesh::Transform* const pTransform) {
	return TRUE;
}

void VkRenderer::RemoveMesh(Common::Foundation::Hash hash) {

}
