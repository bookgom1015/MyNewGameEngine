#pragma once

#include "Common/Util/HashUtil.hpp"

namespace Render::DX::Foundation::Resource {
	struct SubmeshGeometry {
		UINT IndexCount{};
		UINT StartIndexLocation{};
		UINT BaseVertexLocation{};

		DirectX::BoundingBox Bounds{};
	};

	struct MeshGeometry {
		Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU{};
		Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU{};

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU{};
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU{};

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader{};
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader{};

		UINT VertexByteStride{};
		UINT VertexBufferByteSize{};

		DXGI_FORMAT IndexFormat{ DXGI_FORMAT_R16_UINT };
		UINT IndexBufferByteSize{};
		UINT IndexByteStride{};

		UINT64 Fence{};

		std::unordered_map<std::string, SubmeshGeometry> Subsets{};

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;
		D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;
		void DisposeUploaders();

		static Common::Foundation::Hash Hash(MeshGeometry* ptr);
	};
}