#pragma once

#include "Common/Foundation/Mesh/Mesh.hpp"
#include "Render/DX/DxLowRenderer.hpp"

namespace Render {
	extern "C" RendererAPI Common::Render::Renderer* CreateRenderer();
	extern "C" RendererAPI void DestroyRenderer(Common::Render::Renderer* const renderer);

	namespace DX {
		namespace Foundation::Mesh {
			struct MeshGeometry;
			struct SubmeshGeometry;
		}

		class DxRenderer : public Common::Render::Renderer, DxLowRenderer {
		public:
			DxRenderer();
			virtual ~DxRenderer();

		public: // Functions that is called only once
			RendererAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) override;
			RendererAPI virtual void CleanUp() override;

		public: // Functions that is called whenever a message is called
			RendererAPI virtual BOOL OnResize(UINT width, UINT height) override;

		public: // Functions that is called in every frame
			RendererAPI virtual BOOL Update(FLOAT deltaTime) override;
			RendererAPI virtual BOOL Draw() override;

		public:
			RendererAPI virtual BOOL AddMesh() override;
			RendererAPI virtual BOOL RemoveMesh() override;

		private:
			BOOL BuildMeshGeometry(
				Foundation::Mesh::SubmeshGeometry* const submesh, 
				const std::vector<Vertex>& vertices, 
				const std::vector<std::uint16_t>& indices,
				const std::string& name);

		private:
			BOOL BuildSkySphere();

		private:
			std::unordered_map<std::string, std::unique_ptr<Foundation::Mesh::MeshGeometry>> mMeshGeometries;
		};
	}
}