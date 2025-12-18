#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace EyeAdaption {
		namespace Shader {
			enum Type {
				Count = 0
			};
		}

		namespace RootSignature {
			enum Type {
				Count = 0
			};
		}

		namespace PipelineState {
			enum Type {
				Count = 0
			};
		}

		class EyeAdaptionClass : public Foundation::ShadingObject {
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
			EyeAdaptionClass();
			virtual ~EyeAdaptionClass() = default;

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

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;
		};

		using InitDataPtr = std::unique_ptr<EyeAdaptionClass::InitData>;

		InitDataPtr MakeInitData();
	}
}