#ifndef __MATHUTIL_INL__
#define __MATHUTIL_INL__

namespace Common::Util::MathUtil {
	float Sin(float t) { return sinf(t); }

	float ASin(float t) { return asinf(t); }

	float Cos(float t) { return cosf(t); }

	float ACos(float t) { return acosf(t); }

	float Tan(float t) { return tanf(t); }

	float ATan2(float x, float y) { return atan2f(y, x); }

	constexpr float DegreesToRadians(float degrees) { return degrees * DegToRad; }

	constexpr float RadiansToDegrees(float radians) { return radians * RadToDeg; }

	float RandF() { return static_cast<float>(rand()) / static_cast<float>(RAND_MAX); }

	float RandF(float a, float b) { return a + RandF() * (b - a); }

	int Rand(int a, int b) { return a + rand() % ((b - a) + 1); }

	template<typename T>
	T Min(const T& a, const T& b) { return a < b ? a : b; }

	template<typename T>
	T Max(const T& a, const T& b) { return a > b ? a : b; }

	template<typename T>
	T Lerp(const T& a, const T& b, float t) { return a + (b - a) * t; }

	template<typename T>
	T Clamp(const T& x, const T& low, const T& high) { 
		return x < low ? low : (x > high ? high : x);
	}

	float Abs(float param) { return static_cast<float>(fabs(param)); }

	constexpr bool IsZero(float value) { return value * value < Epsilon * Epsilon; }

	constexpr bool IsNotZero(float value) { return !IsZero(value); }

	bool IsEqual(float a, float b) { return Abs(a - b) < Epsilon; }

	bool IsNotEqual(float a, float b) { return Abs(a - b) >= Epsilon; }

	bool IsEqual(const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs) {
		return IsEqual(lhs.x, rhs.x) && IsEqual(lhs.y, rhs.y);
	}

	bool IsNotEqual(const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs) {
		return IsNotEqual(lhs, rhs);
	}

	bool IsEqual(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
		return IsEqual(lhs.x, rhs.x) && IsEqual(lhs.y, rhs.y) && IsEqual(lhs.z, rhs.z);
	}

	bool IsNotEqual(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
		return IsNotEqual(lhs, rhs);
	}

	bool IsEqual(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs) {
		return IsEqual(lhs.x, rhs.x) && IsEqual(lhs.y, rhs.y) && IsEqual(lhs.z, rhs.z) && IsEqual(lhs.w, rhs.w);
	}

	bool IsNotEqual(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs) {
		return IsNotEqual(lhs, rhs);
	}

	DirectX::PackedVector::XMFLOAT3PK PackXMFLOAT3(const DirectX::XMFLOAT3& v) {
		return DirectX::PackedVector::XMFLOAT3PK(v.x, v.y, v.z);
	}

	DirectX::XMVECTOR CalcUpVector(const DirectX::XMFLOAT3& dir) {
		const DirectX::XMVECTOR dirVec = XMLoadFloat3(&dir);

		DirectX::XMVECTOR up = UnitVector::UpVector;
		if (dir.z >= 0.0f) {
			const float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dirVec, up));
			if (dot > 0.99f) up = UnitVector::BackwardVector;
			else if (dot < -0.99f) up = UnitVector::ForwardVector;
		}
		else {
			up = UnitVector::DownVector;

			const float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dirVec, up));
			if (dot > 0.99f) up = UnitVector::ForwardVector;
			else if (dot < -0.99f) up = UnitVector::BackwardVector;
		}

		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, dirVec));
		up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dirVec, right));

		return up;
	}

	DirectX::XMVECTOR CalcUpVector(const DirectX::XMVECTOR& dir) {
		DirectX::XMFLOAT3 dirf;
		DirectX::XMStoreFloat3(&dirf, dir);

		DirectX::XMVECTOR up = UnitVector::UpVector;
		if (dirf.z >= 0.0f) {
			const float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, up));
			if (dot > 0.99f) up = UnitVector::BackwardVector;
			else if (dot < -0.99f) up = UnitVector::ForwardVector;
		}
		else {
			up = UnitVector::DownVector;

			const float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, up));
			if (dot > 0.99f) up = UnitVector::ForwardVector;
			else if (dot < -0.99f) up = UnitVector::BackwardVector;
		}

		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, dir));
		up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dir, right));

		return up;
	}

	void CalcUpVector(DirectX::XMFLOAT3& dst, const DirectX::XMFLOAT3& dir) {
		const DirectX::XMVECTOR dirVec = XMLoadFloat3(&dir);

		DirectX::XMVECTOR up = UnitVector::UpVector;
		if (dir.z >= 0.0f) {
			const float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dirVec, up));
			if (dot > 0.99f) up = UnitVector::BackwardVector;
			else if (dot < -0.99f) up = UnitVector::ForwardVector;
		}
		else {
			up = UnitVector::DownVector;

			const float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dirVec, up));
			if (dot > 0.99f) up = UnitVector::ForwardVector;
			else if (dot < -0.99f) up = UnitVector::BackwardVector;
		}

		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, dirVec));
		up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dirVec, right));

		DirectX::XMStoreFloat3(&dst, up);
	}

	void CalcUpVector(DirectX::XMFLOAT3& dst, const DirectX::XMVECTOR& dir) {
		DirectX::XMFLOAT3 dirf;
		DirectX::XMStoreFloat3(&dirf, dir);

		DirectX::XMVECTOR up = UnitVector::UpVector;
		if (dirf.z >= 0.0f) {
			const float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, up));
			if (dot > 0.99f) up = UnitVector::BackwardVector;
			else if (dot < -0.99f) up = UnitVector::ForwardVector;
		}
		else {
			up = UnitVector::DownVector;

			const float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, up));
			if (dot > 0.99f) up = UnitVector::ForwardVector;
			else if (dot < -0.99f) up = UnitVector::BackwardVector;
		}

		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, dir));
		up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dir, right));

		DirectX::XMStoreFloat3(&dst, up);
	}
}

#endif // __MATHUTIL_INL__