#pragma once

#include <unordered_map>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3dx12/d3dx12.h>

#include "Common/Util/HashUtil.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX {
	namespace Foundation {
		struct RenderItem;

		namespace Resource {
			struct MeshGeometry;
			struct SubmeshGeometry;

			struct MaterialData;

			class FrameResource;
		}

		namespace Core {
			class Device;
			class CommandObject;
		}

		namespace Resource {
			class FrameResource;
		}
	}

	namespace Shading::Util {
		class AccelerationStructureBuffer {
		private:
			friend class AccelerationStructureManager;

		public:
			AccelerationStructureBuffer() = default;
			virtual ~AccelerationStructureBuffer();

		private:
			BOOL BuildBLAS(
				Common::Debug::LogFile* const pLogFile, 
				Foundation::Core::Device* const pDevice, 
				ID3D12GraphicsCommandList6* const pCmdList,
				Foundation::Resource::MeshGeometry* const pMeshGeo);
			BOOL BuildTLAS(
				Common::Debug::LogFile* const pLogFile,
				Foundation::Core::Device* const pDevice,
				ID3D12GraphicsCommandList6* const pCmdList,
				D3D12_RAYTRACING_INSTANCE_DESC instanceDescs[],
				UINT numInstanceDescs);
			BOOL UpdateTLAS(
				Common::Debug::LogFile* const pLogFile,
				ID3D12GraphicsCommandList6* const pCmdList,
				D3D12_RAYTRACING_INSTANCE_DESC instanceDescs[],
				UINT numInstanceDescs);

		private:
			Microsoft::WRL::ComPtr<ID3D12Resource> mScratch;
			Microsoft::WRL::ComPtr<ID3D12Resource> mResult;
			Microsoft::WRL::ComPtr<ID3D12Resource> mInstanceDesc;	// only used in top-level AS

			UINT64 mResultDataMaxSizeInBytes;

			void* mpMappedData = nullptr;
		};

		class AccelerationStructureManager {
		public:
			AccelerationStructureManager();
			virtual ~AccelerationStructureManager() = default;

		public:
			__forceinline D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure() const;

		public:
			BOOL Initialize(
				Common::Debug::LogFile* const pLogFile,
				Foundation::Core::Device* const pDevice,
				Foundation::Core::CommandObject* const pCmdObject);

			BOOL BuildBLAS(
				ID3D12GraphicsCommandList6* const pCmdList,
				Foundation::Resource::MeshGeometry* const pMeshGeo);
			BOOL Update(
				Foundation::Resource::FrameResource* const pFrameResource,
				Foundation::RenderItem* const ritems[],
				UINT numRitems);

		private:
			BOOL BuildTLAS(
				ID3D12GraphicsCommandList6* const pCmdList,
				Foundation::RenderItem* const ritems[],
				UINT numRitems);
			BOOL UpdateTLAS(
				ID3D12GraphicsCommandList6* const pCmdList,
				Foundation::RenderItem* const ritems[],
				UINT numRitems);
			BOOL BuildInstanceDescriptors(
				std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instaneDescs,
				Foundation::RenderItem* const ritems[],
				UINT numRitems);

		private:
			BOOL mbNeedToRebuildTLAS = FALSE;

			Common::Debug::LogFile* mpLogFile = nullptr;
			Foundation::Core::Device* mpDevice = nullptr;
			Foundation::Core::CommandObject* mpCommandObject = nullptr;
			
			std::vector<std::unique_ptr<AccelerationStructureBuffer>> mBLASes;
			std::unordered_map<Common::Foundation::Hash, AccelerationStructureBuffer*> mBLASRefs;
			std::unique_ptr<AccelerationStructureBuffer> mTLAS;
		};
	}
}

#include "AccelerationStructure.inl"