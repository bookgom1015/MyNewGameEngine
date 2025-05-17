#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3dx12/d3dx12.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX {
	namespace Foundation::Core {
		class Device;
	}

	namespace Shading::Util {
		class GpuUploadBuffer {
		protected:
			GpuUploadBuffer(Common::Debug::LogFile* const pLogFile);
			virtual ~GpuUploadBuffer();

		public:
			__forceinline Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() const;

		protected:
			BOOL Allocate(Foundation::Core::Device* const pDevice, UINT bufferSize, LPCWSTR resourceName = nullptr);
			BOOL MapCpuWriteOnly(std::uint8_t*& pData);

		protected:
			Common::Debug::LogFile* mpLogFile = nullptr;

			Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
		};

		// Shader record = {{Shader ID}, {RootArguments}}
		class ShaderRecord {
		public:
			struct PointerWithSize {
				void* Ptr;
				UINT Size;

				PointerWithSize();
				PointerWithSize(void* const ptr, UINT size);
			};

		public:
			ShaderRecord(void* const pShaderIdentifier, UINT shaderIdentifierSize);
			ShaderRecord(void* const pShaderIdentifier, UINT shaderIdentifierSize, void* const pLocalRootArguments, UINT localRootArgumentsSize);

		public:
			void CopyTo(void* const dest) const;

		public:
			PointerWithSize mShaderIdentifier;
			PointerWithSize mLocalRootArguments;
		};

		// Shader table = {{ ShaderRecord 1}, {ShaderRecord 2}, ...}
		class ShaderTable : public GpuUploadBuffer {
		public:
			ShaderTable(
				Common::Debug::LogFile* const pLogFile,
				Foundation::Core::Device* const pDevice, 
				UINT numShaderRecords, 
				UINT shaderRecordSize, 
				LPCWSTR resourceName = nullptr);

		public:
			BOOL Initialze();
			BOOL push_back(const ShaderRecord& shaderRecord);

			__forceinline constexpr std::uint8_t* GetMappedShaderRecords() const;
			__forceinline constexpr UINT GetShaderRecordSize() const;

			// Pretty-print the shader records.
			void DebugPrint(std::unordered_map<void*, std::wstring>& shaderIdToStringMap);

		protected:
			Foundation::Core::Device* mpDevice = nullptr;

			std::uint8_t* mMappedShaderRecords;
			UINT mShaderRecordSize;
			UINT mBufferSize;

			// Debug support
			std::wstring mName;
			std::vector<ShaderRecord> mShaderRecords;
		};
	}
}

#include "ShaderTable.inl"