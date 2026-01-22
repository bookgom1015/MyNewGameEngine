#pragma once

#include <wrl.h>

#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation {
	namespace Core {
		class Device;
	}

	namespace Resource {
		template<typename T>
		class UploadBuffer {
		public:
			UploadBuffer() = default;
			virtual ~UploadBuffer();

		public:
			virtual BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				Core::Device* const pDevice, 
				UINT elementCount, 
				UINT instanceCount,
				BOOL isConstantBuffer,
				LPCWSTR name = nullptr);
			virtual void CleanUp();

			__forceinline ID3D12Resource* Resource() const;

			void CopyData(INT elementIndex, const T& data);

		protected:
			Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
			BYTE* mMappedData = nullptr;

			UINT mElementByteSize = 0;
			BOOL mIsConstantBuffer = FALSE;

			BOOL bIsDirty = TRUE;
		};
	}
}

#include "Render/DX/Foundation/Resource/UploadBuffer.inl"