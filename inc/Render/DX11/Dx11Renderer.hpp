#pragma once

#include "Common/Foundation/Mesh/Mesh.hpp"
#include "Render/DX11/Dx11LowRenderer.hpp"

namespace Render {
	extern "C" RendererAPI Common::Render::Renderer* CreateRenderer();
	extern "C" RendererAPI void DestroyRenderer(Common::Render::Renderer* const renderer);

	namespace DX11 {
		namespace Foundation {
			struct RenderItem;

			namespace Resource {
				class FrameResource;
				class MeshGeometry;
				struct MaterialData;
			}
		}

		namespace Shading {
			namespace Util {
				class ShaderManager;
				class ShadingObjectManager;
			}
		}

		class Dx11Renderer : public Dx11LowRenderer {
		public:
			Dx11Renderer();
			virtual ~Dx11Renderer();

		public: // Functions that is called only once
			RendererAPI virtual BOOL Initialize(
				Common::Debug::LogFile* const pLogFile,
				Common::Foundation::Core::WindowsManager* const pWndManager,
				Common::ImGuiManager::ImGuiManager* const pImGuiManager,
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
				UINT width, UINT height) override;
			RendererAPI virtual void CleanUp() override;

		public: // Functions that is called whenever a message is called
			RendererAPI virtual BOOL OnResize(UINT width, UINT height) override;

		public: // Functions that is called in every frame
			RendererAPI virtual BOOL Update(FLOAT deltaTime) override;
			RendererAPI virtual BOOL Draw() override;

		public:
			RendererAPI virtual BOOL AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Mesh::Transform* const pTransform, Common::Foundation::Hash& hash) override;
			RendererAPI virtual BOOL UpdateMeshTransform(Common::Foundation::Hash hash, Common::Foundation::Mesh::Transform* const pTransform) override;
			RendererAPI virtual void RemoveMesh(Common::Foundation::Hash hash) override;

		private:
			BOOL UpdateCB();
			BOOL UpdatePassCB();
			BOOL UpdateObjectCB();
			BOOL UpdateMaterialCB();
			BOOL UpdateLightCB();
			BOOL UpdateGBufferCB();

		private:
			BOOL BuildMeshGeometry(
				Common::Foundation::Mesh::Mesh* const pMesh,
				Foundation::Resource::MeshGeometry* const pMeshGeo);
			BOOL BuildRenderItem(
				Common::Foundation::Mesh::Mesh* const pMesh, 
				Common::Foundation::Mesh::Transform* const pTransform,
				Common::Foundation::Hash& hash,
				Foundation::Resource::MeshGeometry* pMeshGeo);
			BOOL BuildRenderItem(
				Common::Foundation::Mesh::Mesh* const pMesh,
				Foundation::Resource::MeshGeometry*& pMeshGeo);
			BOOL BuildMeshMaterial(
				Common::Foundation::Mesh::Material* const pMaterial,
				Foundation::Resource::MaterialData*& pMatData);
			BOOL BuildMeshTextures(
				Common::Foundation::Mesh::Material* const pMaterial,
				Foundation::Resource::MaterialData* const pMatData);

			BOOL BuildSkySphere();

		private:
			std::unique_ptr<Shading::Util::ShadingObjectManager> mShadingObjectManager{};
			std::unique_ptr<Shading::Util::ShaderManager> mShaderManager{};

			std::unique_ptr<Foundation::Resource::FrameResource> mFrameResource;

			std::unordered_map<Common::Foundation::Hash, std::unique_ptr<Foundation::Resource::MeshGeometry>> mMeshGeometries{};
			std::vector<std::unique_ptr<Foundation::Resource::MaterialData>> mMaterials{};

			std::vector<std::unique_ptr<Foundation::RenderItem>> mRenderItems{};
			std::unordered_map<Common::Foundation::Hash, Foundation::RenderItem*> mRenderItemRefs{};
			std::array<std::vector<Foundation::RenderItem*>, Common::Foundation::Mesh::RenderType::Count> mRenderItemGroups{};
			std::array<std::vector<Foundation::RenderItem*>, Common::Foundation::Mesh::RenderType::Count> mRendableItems{};
			std::unique_ptr<Foundation::RenderItem> mSkySphere{};
			BOOL mbMeshGeometryAdded{};

			DirectX::BoundingSphere mSceneBounds{};
		};
	}
}