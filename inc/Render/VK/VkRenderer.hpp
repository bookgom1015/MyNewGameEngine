#pragma once

#include "Render/VK/VkLowRenderer.hpp"

namespace Common::Foundation {
	namespace Camera {
		class GameCamera;
	}

	namespace Mesh {
		class Mesh;
		struct Material;
	}
}

namespace Render {
	extern "C" RendererAPI Common::Render::Renderer* CreateRenderer();
	extern "C" RendererAPI void DestroyRenderer(Common::Render::Renderer* const renderer);

	namespace VK {
		namespace Foundation {
			namespace Resource {
				struct MeshGeometry;
			}
		}

		namespace Shading {
			namespace Util {
				class ShadingObjectManager;
				class ShaderManager;
			}

			namespace GBuffer { class GBufferClass; }
		}

		class VkRenderer : public VkLowRenderer {
		public:
			VkRenderer();
			virtual ~VkRenderer();

		public:
			RendererAPI virtual BOOL Initialize(
				Common::Debug::LogFile* const pLogFile,
				Common::Foundation::Core::WindowsManager* const pWndManager,
				Common::ImGuiManager::ImGuiManager* const pImGuiManager,
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
				UINT width, UINT height) override;
			RendererAPI virtual void CleanUp() override;

		public: // Functions that called whenever a message is called
			RendererAPI virtual BOOL OnResize(UINT width, UINT height) override;

		public: // Functions that called in every frame
			RendererAPI virtual BOOL Update(FLOAT deltaTime) override;
			RendererAPI virtual BOOL Draw() override;

		public:
			RendererAPI virtual BOOL AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Mesh::Transform* const pTransform, Common::Foundation::Hash& hash) override;
			RendererAPI virtual BOOL UpdateMeshTransform(Common::Foundation::Hash hash, Common::Foundation::Mesh::Transform* const pTransform) override;
			RendererAPI virtual void RemoveMesh(Common::Foundation::Hash hash) override;

		private:
			BOOL InitShadingObjects();

			BOOL BuildMeshGeometry(
				Common::Foundation::Mesh::Mesh* const pMesh, 
				Foundation::Resource::MeshGeometry*& pMeshGeo);

		private:
			std::unique_ptr<Shading::Util::ShadingObjectManager> mShadingObjectManager;
			std::unique_ptr<Shading::Util::ShaderManager> mShaderManager;

			std::unique_ptr<Shading::GBuffer::GBufferClass> mGBuffer;
		};
	}
}