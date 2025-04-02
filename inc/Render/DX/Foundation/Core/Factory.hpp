#pragma once

#include <vector>

#include <wrl.h>

#include <dxgi1_6.h>
#include <d3d12.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation {
	namespace Util {
		class D3D12Util;
	}

	namespace Core {
		class Device;

		class Factory {
		private:
			friend class Util::D3D12Util;

		public:
			using Adapters = std::vector<std::pair<UINT, IDXGIAdapter*>>;

		public:
			Factory() = default;
			virtual ~Factory() = default;

		public:
			BOOL Initialize(Common::Debug::LogFile* const pLogFile);
			BOOL SortAdapters();
			BOOL SelectAdapter(Device* const pDevice);

		public:
			__forceinline BOOL AllowTearing() const;

		private:
			BOOL CreateFactory();

		private:
			Common::Debug::LogFile* mpLogFile = nullptr;

			// Debugging
			Microsoft::WRL::ComPtr<ID3D12Debug> mDebugController;

			Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
			UINT mdxgiFactoryFlags = 0;

			BOOL mbAllowTearing = FALSE;

			Adapters mAdapters;
		};
	}
}

#include "Render/DX/Foundation/Core/Factory.inl"