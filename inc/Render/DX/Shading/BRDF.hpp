#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace BRDF {
		namespace Shader {
			enum Type {
				VS_IntegrateDiffuse = 0,
				MS_IntegrateDiffuse,
				PS_IntegrateDiffuse,
				VS_IntegrateSpecular,
				MS_IntegrateSpecular,
				PS_IntegrateSpecular,
				Count
			};
		}

		namespace RootSignature {
			enum Type {
				GR_IntegrateDiffuse = 0,
				GR_IntegrateSpecular,
				Count
			};

			namespace IntegrateDiffuse {
				enum {
					CB_Pass = 0,
					SI_AlbedoMap,
					SI_NormalMap,
					SI_DepthMap,
					SI_RMSMap,
					SI_PositionMap,
					SI_ShadowMap,
					SI_AOMap,
					SI_DiffuseIrradianceCubeMap,
					Count
				};
			}
			namespace IntegrateSpecular {
				enum {
					CB_Pass = 0,
					SI_BackBuffer,
					SI_AlbedoMap,
					SI_NormalMap,
					SI_DepthMap,
					SI_RMSMap,
					SI_PositionMap,
					SI_AOMap,
					SI_ReflectionMap,
					SI_BrdfLutMap,
					SI_PrefilteredEnvCubeMap,
					Count
				};
			}
		}

		namespace PipelineState {
			enum Type {
				GP_IntegrateDiffuse = 0,
				GP_IntegrateSpecular,
				Count
			};
		}

		class BRDFClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				BOOL MeshShaderSupported = FALSE;
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;

				UINT ClientWidth = 0;
				UINT ClientHeight = 0;
			};

		public:
			BRDFClass();
			virtual ~BRDFClass() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) override;
			virtual BOOL BuildPipelineStates() override;

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes = {};

			std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignature::Count> mRootSignatures;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;
		};

		using InitDataPtr = std::unique_ptr<BRDFClass::InitData>;

		InitDataPtr MakeInitData();
	}
}