#ifndef __LIGHT_H__
#define __LIGHT_H__

#ifndef MAX_LIGHTS
#define MAX_LIGHTS 8
#endif 

static const UINT LightType_Directional = 1 << 0;
static const UINT LightType_Point		= 1 << 1;
static const UINT LightType_Spot		= 1 << 2;

struct Light {
	DirectX::XMFLOAT4X4 Mat0;
	DirectX::XMFLOAT4X4 Mat1;

	DirectX::XMFLOAT3	Color;
	FLOAT				Intensity;

	DirectX::XMFLOAT3	Direction;
	UINT				Type;

	DirectX::XMFLOAT3	Position;
	UINT				Index;
};

#endif // __LIGHT_H__