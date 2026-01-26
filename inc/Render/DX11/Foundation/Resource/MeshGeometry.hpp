#pragma once

#include "Common/Util/HashUtil.hpp"

namespace Common {
	namespace Debug {
		struct LogFile;
	}

	namespace Foundation::Mesh {
		struct Vertex;
	}
}

namespace Render::DX11::Foundation {
	namespace Core {
		class Device;
	}

	namespace Resource {
		struct SubmeshGeometry {
			UINT IndexCount{};
			UINT StartIndexLocation{};
			UINT BaseVertexLocation{};

			DirectX::BoundingBox Bounds{};
		};

		class MeshGeometry {
		public:
			MeshGeometry();
			virtual ~MeshGeometry();

		public:
			__forceinline ID3D11Buffer** VertexBufferAddress() noexcept;
			__forceinline ID3D11Buffer* IndexBufferAddress() noexcept;
			__forceinline constexpr UINT IndexCount() const noexcept;

		public:
			BOOL Initialize(
				Common::Debug::LogFile* const pLogFile,
				Core::Device* const pDevice,
				Common::Foundation::Mesh::Vertex const *const pVertexData, UINT vertexByteSize,
				UINT const *const pIndexData, UINT indexByteSize,
				UINT indexCount);
			void CleanUp();

			static Common::Foundation::Hash Hash(MeshGeometry* ptr);

		public:
			BOOL mbCleanedUp{};

			Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer{};
			Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer{};
			UINT mIndexCount{};

			std::unordered_map<std::string, SubmeshGeometry> Subsets{};
		};
	}
}

#include "MeshGeometry.inl"