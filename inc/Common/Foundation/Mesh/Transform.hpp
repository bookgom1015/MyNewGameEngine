#pragma once

#include <SimpleMath.h>

namespace Common::Foundation::Mesh {
	struct Transform {
		DirectX::SimpleMath::Vector3 Position;
		DirectX::SimpleMath::Vector3 Rotation;
		DirectX::SimpleMath::Vector3 Scale;
	};
}