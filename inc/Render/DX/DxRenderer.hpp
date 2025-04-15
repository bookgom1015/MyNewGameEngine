#pragma once

#include "Common/Foundation/Mesh/Mesh.hpp"
#include "Common/Util/HashUtil.hpp"
#include "Render/DX/DxLowRenderer.hpp"

namespace Common::Foundation::Camera {
	class GameCamera;
}

namespace ConstantBuffers {
	struct PassCB;
	struct EquirectangularConverterCB;
}

namespace Render {
	extern "C" RendererAPI Common::Render::Renderer* CreateRenderer();
	extern "C" RendererAPI void DestroyRenderer(Common::Render::Renderer* const renderer);

	namespace DX {
		namespace Foundation {
			struct RenderItem;

			namespace Resource {
				struct MeshGeometry;
				struct SubmeshGeometry;

				class FrameResource;
			}
		}

		namespace Shading {
			namespace Util {
				class ShadingObjectManager;
				class ShaderManager;

				namespace MipmapGenerator { class MipmapGeneratorClass; }
				namespace EquirectangularConverter { class EquirectangularConverterClass; }
			}

			namespace EnvironmentMap { class EnvironmentMapClass; }
		}

		class DxRenderer : public DxLowRenderer {
		public:
			DxRenderer();
			virtual ~DxRenderer();

		public: // Functions that is called only once
			RendererAPI virtual BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				Common::Foundation::Core::WindowsManager* const pWndManager, 
				UINT width, UINT height) override;
			RendererAPI virtual void CleanUp() override;

		public: // Functions that is called whenever a message is called
			RendererAPI virtual BOOL OnResize(UINT width, UINT height) override;

		public: // Functions that is called in every frame
			RendererAPI virtual BOOL Update(FLOAT deltaTime) override;
			RendererAPI virtual BOOL Draw() override;

		public:
			RendererAPI virtual BOOL AddMesh() override;
			RendererAPI virtual BOOL RemoveMesh() override;

		protected:
			virtual BOOL CreateDescriptorHeaps() override;

		private:
			BOOL UpdateConstantBuffers();
			BOOL UpdateMainPassCB();
			BOOL UpdateObjectCB();
			BOOL UpdateEquirectangularConverterCB();

		private:
			BOOL BuildMeshGeometry(
				Foundation::Resource::SubmeshGeometry* const submesh,
				const std::vector<Common::Foundation::Mesh::Vertex>& vertices, 
				const std::vector<std::uint16_t>& indices,
				const std::string& name,
				Common::Foundation::Hash& hash);

		private: // Functions that is called only once in Initialize
			BOOL InitShadingObjects();
			BOOL BuildFrameResources();

			BOOL BuildSkySphere();
			BOOL FinishUpInitializing();

			BOOL PresentAndSignal();

		private:
			std::unordered_map<Common::Foundation::Hash, std::unique_ptr<Foundation::Resource::MeshGeometry>> mMeshGeometries;

			// Frame resource
			std::vector<std::unique_ptr<Foundation::Resource::FrameResource>> mFrameResources;
			Foundation::Resource::FrameResource* mCurrentFrameResource = nullptr;
			UINT mCurrentFrameResourceIndex = 0;

			// Constant buffers
			std::unique_ptr<ConstantBuffers::PassCB> mMainPassCB;
			std::unique_ptr<ConstantBuffers::EquirectangularConverterCB> mEquirectConvCB;

			// Shading objects
			std::unique_ptr<Shading::Util::ShadingObjectManager> mShadingObjectManager;
			std::unique_ptr<Shading::Util::ShaderManager> mShaderManager;

			std::unique_ptr<Shading::Util::MipmapGenerator::MipmapGeneratorClass> mMipmapGenerator;
			std::unique_ptr<Shading::Util::EquirectangularConverter::EquirectangularConverterClass> mEquirectangularConverter;

			std::unique_ptr<Shading::EnvironmentMap::EnvironmentMapClass> mEnvironmentMap;

			// Render items
			std::vector<std::unique_ptr<Foundation::RenderItem>> mRenderItems;
		};
	}
}