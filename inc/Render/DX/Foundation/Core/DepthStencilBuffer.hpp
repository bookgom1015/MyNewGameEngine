#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Render::DX::Foundation {
	namespace Util {
		class GpuResource;
	}

	namespace Core {
		class DepthStencilBuffer : public ShadingObject {
		public:
			struct InitData {
				Common::Debug::LogFile* LogFile;
				ID3D12Device5* Device;
				UINT Width;
				UINT Height;
			};

		public:
			DepthStencilBuffer();
			virtual ~DepthStencilBuffer();

		public:
			virtual UINT CbvSrvUavDescCount() const override;
			virtual UINT RtvDescCount() const override;
			virtual UINT DsvDescCount() const override;

		public:
			virtual BOOL Initialize(void* const pData) override;

			virtual BOOL BuildDescriptors(DescriptorHeap* const pDescHeap) override;
			virtual BOOL OnResize(UINT width, UINT height) override;

		private:
			BOOL BuildDepthStencilBuffer();
			BOOL BuildDescriptors();

		private:
			InitData mInitData;

			std::unique_ptr<Util::GpuResource> mDepthStencilBuffer;
			D3D12_CPU_DESCRIPTOR_HANDLE mhDepthStencilBufferCpuDsv;
		};
	}
}