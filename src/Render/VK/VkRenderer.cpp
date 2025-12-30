#include "Render/VK/VkRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Render/VK/Shading/Util/ShadingObjectManager.hpp"
#include "Render/VK/Shading/Util/ShaderManager.hpp"
#include "Render/VK/Shading/GBuffer.hpp"

using namespace Render::VK;

extern "C" RendererAPI Common::Render::Renderer* Render::CreateRenderer() {
	return new VkRenderer();
}

extern "C" RendererAPI void Render::DestroyRenderer(Common::Render::Renderer* const renderer) {
	delete renderer;
}

VkRenderer::VkRenderer() {
	mShadingObjectManager = std::make_unique<Shading::Util::ShadingObjectManager>();
	mShaderManager = std::make_unique<Shading::Util::ShaderManager>();

	mGBuffer = std::make_unique<Shading::GBuffer::GBufferClass>();

	mShadingObjectManager->AddShadingObject(mGBuffer.get());
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

	CheckReturn(mpLogFile, InitShadingObjects());

	CheckReturn(mpLogFile, mShadingObjectManager->CompileShaders(mShaderManager.get(), L".\\..\\..\\..\\assets\\Shaders\\GLSL\\"));
	CheckReturn(mpLogFile, mShadingObjectManager->BuildDescriptorSets());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineLayouts());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildRenderPass());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineStates());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildFramebuffers());

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

BOOL VkRenderer::InitShadingObjects() {
	CheckReturn(mpLogFile, mShadingObjectManager->Initialize(mpLogFile));
	CheckReturn(mpLogFile, mShaderManager->Initialize(
		mpLogFile,
		mDevice.get(),
		static_cast<UINT>(mProcessor->Logical)));

	// GBuffer
	{
		auto initData = Shading::GBuffer::MakeInitData();
		initData->Device = mDevice.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mGBuffer->Initialize(mpLogFile, initData.get()));
	}

	return TRUE;
}