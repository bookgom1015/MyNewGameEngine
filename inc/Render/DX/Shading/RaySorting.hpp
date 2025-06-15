#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Shading {
	namespace RaySorting {
		namespace Shader {
			enum Type {
				CS_CountingSort = 0,
				Count
			};
		}

		namespace RootSignature {

		}

		class RaySortingClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device = nullptr;
				Foundation::Core::CommandObject* CommandObject = nullptr;
				Foundation::Core::DescriptorHeap* DescriptorHeap = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
				UINT ClientWidth = 0;
				UINT ClientHeight = 0;
			};

		public:
			RaySortingClass();
			virtual ~RaySortingClass() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) override;
			virtual BOOL BuildPipelineStates() override;
			virtual BOOL BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) override;
			virtual BOOL OnResize(UINT width, UINT height) override;

		private:
			InitData mInitData;

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;
		};

		using InitDataPtr = std::unique_ptr<RaySortingClass::InitData>;

		InitDataPtr MakeInitData();

	}
}