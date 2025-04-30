#pragma once

#include <DirectXMath.h>

namespace Common::Foundation::Mesh {
	struct Transform {
		DirectX::XMVECTOR Position;
		DirectX::XMVECTOR Rotation;
		DirectX::XMVECTOR Scale;
	};
}