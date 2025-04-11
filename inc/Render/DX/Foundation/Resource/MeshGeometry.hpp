#pragma once

#include <string>
#include <unordered_map>

#include <wrl.h>
#include <Windows.h>

#include <d3d12.h>
#include <DirectXCollision.h>

#include "Common/Util/HashUtil.hpp"

namespace Render::DX::Foundation::Resource {
	struct SubmeshGeometry {
		UINT IndexCount			= 0;
		UINT StartIndexLocation = 0;
		UINT BaseVertexLocation = 0;

		DirectX::BoundingBox Bounds;
	};

	struct MeshGeometry {
		Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU  = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU  = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader  = nullptr;

		UINT		VertexByteStride	 = 0;
		UINT		VertexBufferByteSize = 0;
		DXGI_FORMAT IndexFormat			 = DXGI_FORMAT_R16_UINT;
		UINT		IndexBufferByteSize  = 0;

		std::unordered_map<std::string, SubmeshGeometry> Subsets;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;
		D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;
		void DisposeUploaders();

		static Common::Foundation::Hash Hash(MeshGeometry* ptr);
	};
}

namespace std {
	template <>
	struct hash<Render::DX::Foundation::Resource::MeshGeometry*> {
		Common::Foundation::Hash operator()(const Render::DX::Foundation::Resource::MeshGeometry* const ptr) const {
			uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
			return Common::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(addr));
		}
	};
}