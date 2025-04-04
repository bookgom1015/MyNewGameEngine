#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX {
	namespace Foundation {
		namespace Core {
			class Device;
			class CommandObject;
		}

		namespace Resource {
			class GpuResource;
		}
	}


	namespace Shading::Util {
		class ShaderManager;

		namespace MipmapGenerator {
			namespace RootSignature {
				namespace Default {
					enum {
						EC_Consts = 0,
						ESI_InputMap,
						Count
					};
				}
			}

			namespace PipelineState {
				enum Type {
					EG_GenerateMipmap = 0,
					EG_CopyMap,
					Count
				};
			}

			class MipmapGeneratorClass : public Render::DX::Foundation::ShadingObject {
			public:
				struct InitData {
					Foundation::Core::Device* Device = nullptr;
					Foundation::Core::CommandObject* CommandObject = nullptr;
					Util::ShaderManager* ShaderManager = nullptr;
				};

			public:
				MipmapGeneratorClass() = default;
				virtual ~MipmapGeneratorClass() = default;

			public:
				virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

				virtual BOOL CompileShaders() override;
				virtual BOOL BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) override;
				virtual BOOL BuildPipelineStates() override;

			private:
				InitData mInitData;

				Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
				std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineState::Count> mPipelineStates;
			};

			using InitDataPtr = std::unique_ptr<MipmapGeneratorClass::InitData>;

			static InitDataPtr MakeInitData();
		}
	}
}