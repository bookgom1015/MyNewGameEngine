#ifndef __LIGHT_H__
#define __LIGHT_H__

#ifndef _HLSL
	#include <DirectXMath.h>

	typedef unsigned int UINT;
	typedef float FLOAT;
#endif

#define MaxLights 8

namespace Common {
	namespace Foundation {
#ifdef HLSL_VERSION_UNDER_6
		static const UINT LightType_None		= 0;
		static const UINT LightType_Directional = 1;
		static const UINT LightType_Point		= 2;
		static const UINT LightType_Spot		= 3;
		static const UINT LightType_Tube		= 4;
		static const UINT LightType_Rect		= 5;
#else
		enum LightType {
			E_None = 0,
			E_Directional,
			E_Point,
			E_Spot,
			E_Tube,
			E_Rect,
			Count
		};
#endif 

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
			FLOAT				Index;				// for d3d11
			FLOAT				__ConstantPad0__;

			DirectX::XMFLOAT3	Up;					// rentangle light only
			FLOAT				__ConstantPad1__;

			DirectX::XMFLOAT3	Right;				// rentangle light only
			FLOAT				__ConstantPad2__;

			DirectX::XMFLOAT3	Center;				// rentangle light only
			FLOAT				__ConstantPad3__;
		};
	}
}

#endif // __LIGHT_H__