#pragma once

#include "Common/Foundation/Mesh/Mesh.hpp"
#include "Common/Foundation/Util/HashUtil.hpp"
#include "Render/DX/DxLowRenderer.hpp"

namespace Render {
	extern "C" RendererAPI Common::Render::Renderer* CreateRenderer();
	extern "C" RendererAPI void DestroyRenderer(Common::Render::Renderer* const renderer);

	namespace DX {
		namespace Foundation {
			namespace Resource {
				struct MeshGeometry;
				struct SubmeshGeometry;

				class FrameResource;
			}

			namespace Util {
				class ShadingObjectManager;
			}
		}

		namespace Shading {
			namespace Util {
				class ShaderManager;

				namespace MipmapGenerator { class MipmapGeneratorClass; }
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
			BOOL BuildMeshGeometry(
				Foundation::Resource::SubmeshGeometry* const submesh,
				const std::vector<Common::Foundation::Mesh::Vertex>& vertices, 
				const std::vector<std::uint16_t>& indices,
				const std::string& name);

		private: // Functions that is called only once in Initialize
			BOOL InitShadingObjects();
			BOOL BuildFrameResources();

			BOOL BuildSkySphere();
			BOOL FinishUpInitializing();

		private:
			std::unordered_map<Common::Foundation::Hash, std::unique_ptr<Foundation::Resource::MeshGeometry>> mMeshGeometries;

			// Frame resource
			std::vector<std::unique_ptr<Foundation::Resource::FrameResource>> mFrameResources;

			// Shading objects
			std::unique_ptr<Foundation::Util::ShadingObjectManager> mShadingObjectManager;
			std::unique_ptr<Shading::Util::ShaderManager> mShaderManager;

			std::unique_ptr<Shading::Util::MipmapGenerator::MipmapGeneratorClass> mMipmapGenerator;
			std::unique_ptr<Shading::EnvironmentMap::EnvironmentMapClass> mEnvironmentMap;
		};
	}
}