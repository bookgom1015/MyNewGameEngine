#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX11::Foundation {
	namespace Core {
		class Device;
	}

	namespace Util {
		class D3D11Util {
		public:
			static BOOL Initialize(Common::Debug::LogFile* const pLogFile);

		public:
			static void GetDefaultInputLayout(
				const D3D11_INPUT_ELEMENT_DESC*& outDesc, UINT& outCount);

			__forceinline static UINT CeilDivide(UINT value, UINT divisor);

			static BOOL LoadDDStoTexture(
				LPCWSTR filePath,
				Core::Device* const pDevice,
				Microsoft::WRL::ComPtr<ID3D11Texture2D>& tex);

		private:
			static Common::Debug::LogFile* msLogFile;
		};
	}
}

#include "D3D11Util.inl"