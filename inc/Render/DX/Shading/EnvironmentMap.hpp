#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX {
	namespace Foundation::Core {
		class Device;
	}

	namespace Shading {
		namespace Util {
			class ShaderManager;
		}

		class EnvironmentMap : public Render::DX::Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device   = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
			};

			using InitDataPtr = std::unique_ptr<InitData>;

		public:
			EnvironmentMap() = default;
			virtual ~EnvironmentMap() = default;

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			static InitDataPtr MakeInitData();

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildRootSignatures() override;
			virtual BOOL BuildPipelineStates() override;
			virtual BOOL BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) override;

		private:
			InitData mInitData;
		};
	}
}