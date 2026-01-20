#pragma once

namespace Render::DX::Foundation::Resource {
	struct Texture {
		UINT DescriptorIndex{};

		Microsoft::WRL::ComPtr<ID3D12Resource> Resource{};
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap{};
	};
}