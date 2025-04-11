#pragma once

#include <DirectXMath.h>

struct Transform {
	DirectX::XMVECTOR Position;
	DirectX::XMVECTOR Rotation;
	DirectX::XMVECTOR Scale;
};