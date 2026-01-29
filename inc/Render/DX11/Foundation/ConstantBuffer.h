#ifndef __CONSTANTBUFFER_H__
#define __CONSTANTBUFFER_H__

#ifdef _HLSL
	#include "./../../../inc/Common/Foundation/Light.h"
#else
	#include "Common/Foundation/Light.h"
#endif

#ifndef PassCB_Struct
#define PassCB_Struct {							\
	DirectX::XMFLOAT4X4 View;					\
	DirectX::XMFLOAT4X4 InvView;				\
	DirectX::XMFLOAT4X4 Proj;					\
	DirectX::XMFLOAT4X4 InvProj;				\
	DirectX::XMFLOAT4X4 ViewProj;				\
	DirectX::XMFLOAT4X4 PrevViewProj;			\
	DirectX::XMFLOAT3	EyePosW;				\
	FLOAT				__PassCB_ContantPad0__;	\
	DirectX::XMFLOAT2	JitteredOffset;			\
	UINT FrameCount;							\
	FLOAT				__PassCB_ContantPad1__;	\
};
#endif // PassCB_Struct

#ifndef PassCB_register
#define PassCB_register(__reg) cbuffer PassCB : register(__reg) PassCB_Struct
#endif // PassCB_register

struct PassCB PassCB_Struct

#ifndef ObjectCB_Struct
#define ObjectCB_Struct {				\
		DirectX::XMFLOAT4X4 World;		\
		DirectX::XMFLOAT4X4 PrevWorld;	\
};
#endif // ObjectCB_Struct

#ifndef ObjectCB_register
#define ObjectCB_register(__reg) cbuffer ObjectCB : register(__reg) ObjectCB_Struct
#endif // ObjectCB_register

struct ObjectCB ObjectCB_Struct

#ifndef MaterialCB_Struct
#define MaterialCB_Struct {					\
		DirectX::XMFLOAT4 Albedo;			\
		DirectX::XMFLOAT3 Specular;			\
		FLOAT __MatCB_ContantPad0__;		\
		FLOAT Roughness;					\
		FLOAT Metalness;					\
		FLOAT __MatCB_ContantPad1__;		\
		FLOAT __MatCB_ContantPad2__;		\
};
#endif // MaterialCB_Struct

#ifndef MaterialCB_register
#define MaterialCB_register(__reg) cbuffer MaterialCB : register(__reg) MaterialCB_Struct
#endif // MaterialCB_register

struct MaterialCB MaterialCB_Struct

#ifndef LightCB_Struct
#define LightCB_Struct {							\
	Common::Foundation::Light Lights[MaxLights];	\
	UINT LightCount;								\
	UINT __ConstantPad4__;							\
	UINT __ConstantPad5__;							\
	UINT __ConstantPad6__;							\
};
#endif // LightCB_Struct

#ifndef LightCB_register
#define LightCB_register(__reg) cbuffer LightCB : register(__reg) LightCB_Struct
#endif // LightCB_register

struct LightCB LightCB_Struct

#ifndef GBufferCB_Struct
#define GBufferCB_Struct {		\
	DirectX::XMFLOAT2 TexDim;	\
	FLOAT DitheringMaxDist;		\
	FLOAT DitheringMinDist;		\
};
#endif // GBufferCB_Struct

#ifndef GBufferCB_register
#define GBufferCB_register(__reg) cbuffer GBufferCB : register(__reg) GBufferCB_Struct
#endif // GBufferCB_register

struct GBufferCB GBufferCB_Struct

#ifndef ShadowCB_Struct
#define ShadowCB_Struct {	\
	UINT LightIndex;		\
	UINT __ConstantPad0__;	\
	UINT __ConstantPad1__;	\
	UINT __ConstantPad2__;	\
};
#endif // ShadowCB_Struct

#ifndef ShadowCB_register
#define ShadowCB_register(__reg) cbuffer ShadowCB : register(__reg) ShadowCB_Struct
#endif // ShadowCB_register

struct ShadowCB ShadowCB_Struct

#endif // __CONSTANTBUFFER_H__