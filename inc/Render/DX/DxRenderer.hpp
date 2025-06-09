#pragma once

#include "Common/Foundation/Mesh/Mesh.hpp"
#include "Common/Util/HashUtil.hpp"
#include "Render/DX/DxLowRenderer.hpp"

#include <array>
#include <queue>

#include <DirectXCollision.h>

namespace Common::Foundation {
	namespace Camera {
		class GameCamera;
	}

	namespace Mesh {
		struct Material;
	}
}

namespace ConstantBuffers {
	struct PassCB;
	struct LightCB;
	struct ProjectToCubeCB;
}

namespace Render {
	extern "C" RendererAPI Common::Render::Renderer* CreateRenderer();
	extern "C" RendererAPI void DestroyRenderer(Common::Render::Renderer* const renderer);

	namespace DX {
		namespace Foundation {
			struct Light;
			struct RenderItem;

			namespace Resource {
				struct MeshGeometry;
				struct SubmeshGeometry;

				struct MaterialData;

				class FrameResource;
			}
		}

		namespace Shading {
			namespace Util {
				class ShadingObjectManager;
				class ShaderManager;
				class AccelerationStructureManager;

				namespace MipmapGenerator { class MipmapGeneratorClass; }
				namespace EquirectangularConverter { class EquirectangularConverterClass; }
			}

			namespace EnvironmentMap { class EnvironmentMapClass; }
			namespace GammaCorrection { class GammaCorrectionClass; }
			namespace ToneMapping { class ToneMappingClass; }
			namespace GBuffer { class GBufferClass; }
			namespace BRDF { class BRDFClass; }
			namespace Shadow { class ShadowClass; }
			namespace TAA { class TAAClass; }
			namespace SSAO { class SSAOClass; }
			namespace RTAO { class RTAOClass; }
			namespace RaySorting { class RaySortingClass; }
			namespace SVGF { class SVGFClass; }
			namespace BlurFilter { class BlurFilterClass; }
		}

		class DxRenderer : public DxLowRenderer {
		public:
			DxRenderer();
			virtual ~DxRenderer();

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

		protected:
			virtual BOOL CreateDescriptorHeaps() override;

		private:
			BOOL UpdateConstantBuffers();
			BOOL UpdateMainPassCB();
			BOOL UpdateLightCB();
			BOOL UpdateObjectCB();
			BOOL UpdateMaterialCB();
			BOOL UpdateProjectToCubeCB();
			BOOL UpdateAmbientOcclusionCB();
			BOOL ResolvePendingLights();
			BOOL PopulateRendableItems();

		private:
			BOOL BuildMeshGeometry(
				ID3D12GraphicsCommandList6* const pCmdList,
				Foundation::Resource::SubmeshGeometry* const pSubMesh,
				const std::vector<Common::Foundation::Mesh::Vertex>& vertices, 
				const std::vector<std::uint16_t>& indices,
				const std::string& name,
				Foundation::Resource::MeshGeometry*& pMeshGeo);
			BOOL BuildMeshGeometry(
				ID3D12GraphicsCommandList6* const pCmdList,
				Common::Foundation::Mesh::Mesh* const pMesh,
				Foundation::Resource::MeshGeometry*& pMeshGeo);
			BOOL BuildMeshMaterial(
				ID3D12GraphicsCommandList6* const pCmdList,
				Common::Foundation::Mesh::Material* const pMaterial,
				Foundation::Resource::MaterialData*& pMatData);
			BOOL BuildMeshTextures(
				ID3D12GraphicsCommandList6* const pCmdList,
				Common::Foundation::Mesh::Material* const pMaterial,
				Foundation::Resource::MaterialData* const pMatData);

		private: // Functions that is called only once in Initialize
			BOOL InitShadingObjects();
			BOOL BuildFrameResources();

			BOOL BuildSkySphere();
			BOOL BuildLights();
			BOOL BuildScene();

			BOOL DrawImGui();

			BOOL DrawAO();
			BOOL IntegrateIrradiance();
			BOOL PresentAndSignal();

		private:
			BOOL mbInitialized = FALSE;

			std::unordered_map<Common::Foundation::Hash, std::unique_ptr<Foundation::Resource::MeshGeometry>> mMeshGeometries;
			std::vector<std::unique_ptr<Foundation::Resource::MaterialData>> mMaterials;

			// Frame resource
			std::vector<std::unique_ptr<Foundation::Resource::FrameResource>> mFrameResources;
			Foundation::Resource::FrameResource* mpCurrentFrameResource = nullptr;
			UINT mCurrentFrameResourceIndex = 0;

			// Constant buffers
			std::unique_ptr<ConstantBuffers::PassCB> mMainPassCB;
			std::unique_ptr<ConstantBuffers::LightCB> mLightCB;
			std::unique_ptr<ConstantBuffers::ProjectToCubeCB> mProjectToCubeCB;

			// Shading objects
			std::unique_ptr<Shading::Util::ShadingObjectManager> mShadingObjectManager;
			std::unique_ptr<Shading::Util::ShaderManager> mShaderManager;

			std::unique_ptr<Shading::Util::MipmapGenerator::MipmapGeneratorClass> mMipmapGenerator;
			std::unique_ptr<Shading::Util::EquirectangularConverter::EquirectangularConverterClass> mEquirectangularConverter;

			std::unique_ptr<Shading::EnvironmentMap::EnvironmentMapClass> mEnvironmentMap;
			std::unique_ptr<Shading::GammaCorrection::GammaCorrectionClass> mGammaCorrection;
			std::unique_ptr<Shading::ToneMapping::ToneMappingClass> mToneMapping;
			std::unique_ptr<Shading::GBuffer::GBufferClass> mGBuffer;
			std::unique_ptr<Shading::BRDF::BRDFClass> mBRDF;
			std::unique_ptr<Shading::Shadow::ShadowClass> mShadow;
			std::unique_ptr<Shading::TAA::TAAClass> mTAA;
			std::unique_ptr<Shading::SSAO::SSAOClass> mSSAO;
			std::unique_ptr<Shading::RTAO::RTAOClass> mRTAO;
			std::unique_ptr<Shading::RaySorting::RaySortingClass> mRaySorting;
			std::unique_ptr<Shading::SVGF::SVGFClass> mSVGF;
			std::unique_ptr<Shading::BlurFilter::BlurFilterClass> mBlurFilter;

			// Render items
			std::vector<std::unique_ptr<Foundation::RenderItem>> mRenderItems;
			std::unordered_map<Common::Foundation::Hash, Foundation::RenderItem*> mRenderItemRefs;
			std::array<std::vector<Foundation::RenderItem*>, Common::Foundation::Mesh::RenderType::Count> mRenderItemGroups;
			std::array<std::vector<Foundation::RenderItem*>, Common::Foundation::Mesh::RenderType::Count> mRendableItems;
			BOOL mbMeshGeometryAdded = FALSE;

			// Scene bounds
			DirectX::BoundingSphere mSceneBounds;

			// Acceleration structure manager
			std::unique_ptr<Shading::Util::AccelerationStructureManager> mAccelerationStructureManager;

			// Pending lights
			std::queue<std::shared_ptr<Foundation::Light>> mPendingLights;
		};
	}
}