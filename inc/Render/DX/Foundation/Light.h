#ifndef __LIGHT_H__
#define __LIGHT_H__

#ifdef _HLSL
	#include "./../../../../inc/Common/Render/LightType.h"
#else
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif // WIN32_LEAN_AND_MEAN
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif // NOMINMAX
	#include <Windows.h>

	#include <DirectXMath.h>

	#include "Common/Render/LightType.h"
#endif

#define MaxLights 8

namespace Render {
	namespace DX {
		namespace Foundation {
			struct Light {
				DirectX::XMFLOAT4X4 Mat0;
				DirectX::XMFLOAT4X4 Mat1;
				DirectX::XMFLOAT4X4 Mat2;
				DirectX::XMFLOAT4X4 Mat3;
				DirectX::XMFLOAT4X4 Mat4;
				DirectX::XMFLOAT4X4 Mat5;

				DirectX::XMFLOAT3	Color;
				FLOAT				Intensity;

				DirectX::XMFLOAT3	Direction;			// directional/spot light only
				FLOAT				Radius;				// point/tube light only

				DirectX::XMFLOAT3	Position;			// point/spot/tube/rectangle light only
				UINT				Type;

				DirectX::XMFLOAT3	Position1;			// tube/rectangle light only (End Point)
				FLOAT				AttenuationRadius;	// point/spot light only

				DirectX::XMFLOAT3	Position2;			// rectangle light only (End Point)
				FLOAT				InnerConeAngle;		// spot light only (degrees)

				DirectX::XMFLOAT3	Position3;			// rectangle light only (End Point)
				FLOAT				OuterConeAngle;		// spot light only (degrees)

				DirectX::XMFLOAT2	Size;				// rectangle light only
				FLOAT				ConstantPad1;
				FLOAT				ConstantPad2;

				DirectX::XMFLOAT3	Up;					// rentangle light only
				FLOAT				ConstantPad3;

				DirectX::XMFLOAT3	Right;				// rentangle light only
				FLOAT				ConstantPad4;

				DirectX::XMFLOAT3	Center;				// rentangle light only
				FLOAT				ConstantPad5;
			};
		}
	}
}

#endif // __LIGHT_H__