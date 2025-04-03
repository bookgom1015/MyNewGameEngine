#pragma once

#include "Render/VK/VkLowRenderer.hpp"

namespace Render {
	extern "C" RendererAPI Common::Render::Renderer* CreateRenderer();
	extern "C" RendererAPI void DestroyRenderer(Common::Render::Renderer* const renderer);

	namespace VK {
		class VkRenderer : public VkLowRenderer {
		public:
			VkRenderer();
			virtual ~VkRenderer();

		public:
			RendererAPI virtual BOOL Initialize(
				Common::Debug::LogFile* const pLogFile,
				Common::Foundation::Core::WindowsManager* const pWndManager,
				UINT width, UINT height) override;
			RendererAPI virtual void CleanUp() override;

		public: // Functions that called whenever a message is called
			RendererAPI virtual BOOL OnResize(UINT width, UINT height) override;

		public: // Functions that called in every frame
			RendererAPI virtual BOOL Update(FLOAT deltaTime) override;
			RendererAPI virtual BOOL Draw() override;

		public:
			RendererAPI virtual BOOL AddMesh() override;
			RendererAPI virtual BOOL RemoveMesh() override;
		};
	}
}