#pragma once

#include "Render/DX/Foundation/Util/UploadBufferWrapper.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation {
	namespace Core {
		class Device;
	}

	namespace Resource {
		class FrameResource {
		public:
			static const UINT Count = 3;

		public:
			FrameResource();
			virtual ~FrameResource();

		public:
			__forceinline ID3D12CommandAllocator* CommandAllocator(UINT index) const;
			__forceinline void CommandAllocators(std::vector<ID3D12CommandAllocator*>& allocs) const;

		public:
			BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				Core::Device* const pDevice,
				UINT numThreads,
				UINT numPasses, 
				UINT numObjects,
				UINT numMaterials);
			void CleanUp();

		public:
			BOOL ResetCommandListAllocators();

		private:
			BOOL CreateCommandListAllocators();
			BOOL BuildConstantBuffres(
				UINT numPasses,
				UINT numObjects,
				UINT numMaterials);

		public:
			UINT64 mFence{};

		private:
			Common::Debug::LogFile* mpLogFile{};
			Core::Device* mpDevice{};

			std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> mCmdAllocators{};

			UINT mThreadCount{};

		public:
			UploadBufferWrapper<ConstantBuffers::PassCB> MainPassCB{};
			UploadBufferWrapper<ConstantBuffers::LightCB> LightCB{};
			UploadBufferWrapper<ConstantBuffers::ObjectCB> ObjectCB{};
			UploadBufferWrapper<ConstantBuffers::MaterialCB> MaterialCB{};
			UploadBufferWrapper<ConstantBuffers::ProjectToCubeCB> ProjectToCubeCB{};
			UploadBufferWrapper<ConstantBuffers::AmbientOcclusionCB> AmbientOcclusionCB{};
			UploadBufferWrapper<ConstantBuffers::RayGenCB> RayGenCB{};
			UploadBufferWrapper<ConstantBuffers::RaySortingCB> RaySortingCB{};
			UploadBufferWrapper<ConstantBuffers::SVGF::CrossBilateralFilterCB> CrossBilateralFilterCB{};
			UploadBufferWrapper<ConstantBuffers::SVGF::CalcLocalMeanVarianceCB> CalcLocalMeanVarianceCB{};
			UploadBufferWrapper<ConstantBuffers::SVGF::BlendWithCurrentFrameCB> BlendWithCurrentFrameCB{};
			UploadBufferWrapper<ConstantBuffers::SVGF::AtrousWaveletTransformFilterCB> AtrousWaveletTransformFilterCB{};
			UploadBufferWrapper<ConstantBuffers::ContactShadowCB> ContactShadowCB{};
		};
	}
}

#include "Render/DX/Foundation/Resource/FrameResource.inl"