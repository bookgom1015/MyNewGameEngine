#pragma once

#ifndef XM_CONSTEXPR
#define XM_CONSTEXPR
#endif

#include <SimpleMath.h>
#include <random>

namespace UnitVector {
	const DirectX::XMVECTOR RightVector		= DirectX::XMVectorSet( 1.f,  0.f,  0.f, 0.f);
	const DirectX::XMVECTOR UpVector		= DirectX::XMVectorSet( 0.f,  1.f,  0.f, 0.f);
	const DirectX::XMVECTOR ForwardVector	= DirectX::XMVectorSet( 0.f,  0.f,  1.f, 0.f);
	const DirectX::XMVECTOR LeftVector		= DirectX::XMVectorSet(-1.f,  0.f,  0.f, 0.f);
	const DirectX::XMVECTOR DownVector		= DirectX::XMVectorSet( 0.f, -1.f,  0.f, 0.f);
	const DirectX::XMVECTOR BackwardVector	= DirectX::XMVectorSet( 0.f,  0.f, -1.f, 0.f);
}

const float Infinity = FLT_MAX;
const float Pi		 = 3.14159265359f;
const float Epsilon	 = 0.0000000001f;
const float RadToDeg = 180.f / 3.14159265359f;
const float DegToRad = 3.14159265359f / 180.f;

namespace Common::Util {
	namespace MathUtil {
		__forceinline float Sin(float t);
		__forceinline float ASin(float t);
		__forceinline float Cos(float t);
		__forceinline float ACos(float t);
		__forceinline float Tan(float t);
		__forceinline float ATan2(float x, float y);

		__forceinline constexpr float DegreesToRadians(float degrees);
		__forceinline constexpr float RadiansToDegrees(float radians);

		// Returns random float in [0, 1).
		__forceinline float RandF();
		// Returns random float in [a, b).
		__forceinline float RandF(float a, float b);
		__forceinline int Rand(int a, int b);

		template<typename T>
		T Min(const T& a, const T& b);
		template<typename T>
		T Max(const T& a, const T& b);
		template<typename T>
		T Lerp(const T& a, const T& b, float t);
		template<typename T>
		T Clamp(const T& x, const T& low, const T& high);

		__forceinline float Abs(float param);

		__forceinline constexpr bool IsZero(float value);
		__forceinline constexpr bool IsNotZero(float value);

		__forceinline bool IsEqual(float a, float b);
		__forceinline bool IsNotEqual(float a, float b);
		__forceinline bool IsEqual(const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs);
		__forceinline bool IsNotEqual(const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs);
		__forceinline bool IsEqual(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
		__forceinline bool IsNotEqual(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
		__forceinline bool IsEqual(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs);
		__forceinline bool IsNotEqual(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs);

		__forceinline DirectX::PackedVector::XMFLOAT3PK PackXMFLOAT3(const DirectX::XMFLOAT3& v);

		__forceinline DirectX::XMVECTOR CalcUpVector(const DirectX::XMFLOAT3& dir);
		__forceinline DirectX::XMVECTOR CalcUpVector(const DirectX::XMVECTOR& dir);
		__forceinline void CalcUpVector(DirectX::XMFLOAT3& dst, const DirectX::XMFLOAT3& dir);
		__forceinline void CalcUpVector(DirectX::XMFLOAT3& dst, const DirectX::XMVECTOR& dir);

		// Returns the polar angle of the point (x,y) in [0, 2*PI).
		float AngleFromXY(float x, float y);

		DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi);
		DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M);
		DirectX::XMFLOAT4X4 Identity4x4();
		DirectX::XMVECTOR RandUnitVec3();
		DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);
	};
}

#include "MathUtil.inl"