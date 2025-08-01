#pragma once

#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

#include "Render/DX/Foundation/ConstantBuffer.h"
#include "Render/DX/Foundation/Resource/UploadBuffer.hpp"

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
			FrameResource() = default;
			virtual ~FrameResource() = default;

		public:
			__forceinline ID3D12CommandAllocator* CommandAllocator(UINT index) const;
			__forceinline void CommandAllocators(std::vector<ID3D12CommandAllocator*>& allocs) const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS MainPassCBAddress() const;
			UINT MainPassCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS LightCBAddress() const;
			UINT LightCBByteSize() const;
			
			__forceinline D3D12_GPU_VIRTUAL_ADDRESS ObjectCBAddress() const;
			__forceinline D3D12_GPU_VIRTUAL_ADDRESS ObjectCBAddress(UINT index) const;
			UINT ObjectCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS MaterialCBAddress() const;
			__forceinline D3D12_GPU_VIRTUAL_ADDRESS MaterialCBAddress(UINT index) const;
			UINT MaterialCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS ProjectToCubeCBAddress() const;
			UINT ProjectToCubeCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS AmbientOcclusionCBAddress() const;
			UINT AmbientOcclusionCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS RayGenCBAddress() const;
			UINT RayGenCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS RaySortingCBAddress() const;
			UINT RaySortingCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS CrossBilateralFilterCBAddress() const;
			UINT CrossBilateralFilterCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS CalcLocalMeanVarianceCBAddress() const;
			UINT CalcLocalMeanVarianceCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS BlendWithCurrentFrameCBAddress() const;
			UINT BlendWithCurrentFrameCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS AtrousWaveletTransformFilterCBAddress() const;
			UINT AtrousWaveletTransformFilterCBByteSize() const;

		public:
			BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				Core::Device* const pDevice,
				UINT numThreads,
				UINT numPasses, 
				UINT numObjects,
				UINT numMaterials);

		public:
			BOOL ResetCommandListAllocators();

		public:
			__forceinline void CopyMainPassCB(INT elementIndex, const ConstantBuffers::PassCB& data);
			__forceinline void CopyLightCB(INT elementIndex, const ConstantBuffers::LightCB& data);
			__forceinline void CopyObjectCB(INT elementIndex, const ConstantBuffers::ObjectCB& data);
			__forceinline void CopyMaterialCB(INT elementIndex, const ConstantBuffers::MaterialCB& data);
			__forceinline void CopyProjectToCubeCB(const ConstantBuffers::ProjectToCubeCB& data);
			__forceinline void CopyAmbientOcclusionCB(const ConstantBuffers::AmbientOcclusionCB& data);
			__forceinline void CopyRayGenCB(const ConstantBuffers::RayGenCB& data);
			__forceinline void CopyRaySortingCB(const ConstantBuffers::RaySortingCB& data);
			__forceinline void CopyCrossBilateralFilterCB(const ConstantBuffers::SVGF::CrossBilateralFilterCB& data);
			__forceinline void CopyCalcLocalMeanVarianceCB(const ConstantBuffers::SVGF::CalcLocalMeanVarianceCB& data);
			__forceinline void CopyBlendWithCurrentFrameCB(const ConstantBuffers::SVGF::BlendWithCurrentFrameCB& data);
			__forceinline void CopyAtrousWaveletTransformFilterCB(const ConstantBuffers::SVGF::AtrousWaveletTransformFilterCB& data);

		private:
			BOOL CreateCommandListAllocators();
			BOOL BuildConstantBuffres(
				UINT numPasses,
				UINT numObjects,
				UINT numMaterials);

		public:
			UINT64 mFence = 0;

		private:
			Common::Debug::LogFile* mpLogFile = nullptr;
			Core::Device* mpDevice = nullptr;

			std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> mCmdAllocators;

			UINT mThreadCount = 0;

			UploadBuffer<ConstantBuffers::PassCB> mMainPassCB;
			UploadBuffer<ConstantBuffers::LightCB> mLightCB;
			UploadBuffer<ConstantBuffers::ObjectCB> mObjectCB;
			UploadBuffer<ConstantBuffers::MaterialCB> mMaterialCB;
			UploadBuffer<ConstantBuffers::ProjectToCubeCB> mProjectToCubeCB;
			UploadBuffer<ConstantBuffers::AmbientOcclusionCB> mAmbientOcclusionCB;
			UploadBuffer<ConstantBuffers::RayGenCB> mRayGenCB;
			UploadBuffer<ConstantBuffers::RaySortingCB> mRaySortingCB;
			UploadBuffer<ConstantBuffers::SVGF::CrossBilateralFilterCB> mCrossBilateralFilterCB;
			UploadBuffer<ConstantBuffers::SVGF::CalcLocalMeanVarianceCB> mCalcLocalMeanVarianceCB;
			UploadBuffer<ConstantBuffers::SVGF::BlendWithCurrentFrameCB> mBlendWithCurrentFrameCB;
			UploadBuffer<ConstantBuffers::SVGF::AtrousWaveletTransformFilterCB> mAtrousWaveletTransformFilterCB;
		};
	}
}

#include "Render/DX/Foundation/Resource/FrameResource.inl"