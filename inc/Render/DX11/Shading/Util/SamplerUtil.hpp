#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX11 {
	namespace SamplerState {
		enum Type {
			E_PointWrap = 0,
			E_PointClamp,
			E_LinearWrap,
			E_LinearClamp,
			E_AnisotropicWrap,
			E_AnisotropicClamp,
			E_AnisotropicBorder,
			E_Depth,
			E_Shadow,
			E_PointMirror,
			E_LinearMirror,
			Count
		};
	}

	namespace Foundation::Core {
		class Device;
	}

	namespace Shading::Util {
		class SamplerUtil {
		public:
			static BOOL Initialize(Common::Debug::LogFile* const pLogFile, Foundation::Core::Device* const pDevice);
			static void CleanUp();

			static __forceinline constexpr ID3D11SamplerState** GetSamplers() noexcept;
			static __forceinline constexpr UINT SamplerCount() noexcept;

		private:
			static Microsoft::WRL::ComPtr<ID3D11SamplerState> msSamplers[SamplerState::Type::Count];
			static ID3D11SamplerState* msSamplerPtrs[SamplerState::Type::Count];
		};
	}
}

#include "SamplerUtil.inl"