#pragma once

#include <wrl.h>

#include <d3d12.h>

namespace Render::DX::Foundation::Core {
	class Device {
	private:
		friend class Factory;

	public:
		Device() = default;
		virtual ~Device() = default;

	public:
		__forceinline ID3D12Device5* GetDevice() const;

	private:
		Microsoft::WRL::ComPtr<ID3D12Device5> md3dDevice;
	};
}

#include "Render/DX/Foundation/Core/Device.inl"