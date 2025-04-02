#pragma once

#include <wrl.h>
#include <Windows.h>

#include <d3d12.h>

namespace Render::DX::Foundation::Resource {
	struct Texture {
		UINT DescriptorIndex = 0;

		Microsoft::WRL::ComPtr<ID3D12Resource> Resource	  = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
	};
}