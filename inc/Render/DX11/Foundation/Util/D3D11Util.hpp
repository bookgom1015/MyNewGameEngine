#pragma once

namespace Render::DX11::Foundation::Util {
	class D3D11Util {
	public:
		static void GetDefaultInputLayout(
			const D3D11_INPUT_ELEMENT_DESC*& outDesc, UINT& outCount);
	};
}