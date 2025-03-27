#ifndef __MATHUTIL_INL__
#define __MATHUTIL_INL__

FLOAT Common::Foundation::Util::MathUtil::Sin(FLOAT t) {
	return sinf(t);
}

FLOAT Common::Foundation::Util::MathUtil::ASin(FLOAT t) {
	return asinf(t);
}

FLOAT Common::Foundation::Util::MathUtil::Cos(FLOAT t) {
	return cosf(t);
}

FLOAT Common::Foundation::Util::MathUtil::ACos(FLOAT t) {
	return acosf(t);
}

FLOAT Common::Foundation::Util::MathUtil::Tan(FLOAT t) {
	return tanf(t);
}

FLOAT Common::Foundation::Util::MathUtil::ATan2(FLOAT x, FLOAT y) {
	return atan2f(y, x);
}

constexpr FLOAT Common::Foundation::Util::MathUtil::DegreesToRadians(FLOAT degrees) {
	return degrees * DegToRad;
}

constexpr FLOAT Common::Foundation::Util::MathUtil::RadiansToDegrees(FLOAT radians) {
	return radians * RadToDeg;
}

FLOAT Common::Foundation::Util::MathUtil::RandF() {
	return static_cast<FLOAT>(rand()) / static_cast<FLOAT>(RAND_MAX);
}

FLOAT Common::Foundation::Util::MathUtil::RandF(FLOAT a, FLOAT b) {
	return a + RandF() * (b - a);
}

INT Common::Foundation::Util::MathUtil::Rand(INT a, INT b) {
	return a + rand() % ((b - a) + 1);
}

template<typename T>
T Common::Foundation::Util::MathUtil::Min(const T& a, const T& b) {
	return a < b ? a : b;
}

template<typename T>
T Common::Foundation::Util::MathUtil::Max(const T& a, const T& b) {
	return a > b ? a : b;
}

template<typename T>
T Common::Foundation::Util::MathUtil::Lerp(const T& a, const T& b, FLOAT t) {
	return a + (b - a) * t;
}

template<typename T>
T Common::Foundation::Util::MathUtil::Clamp(const T& x, const T& low, const T& high) {
	return x < low ? low : (x > high ? high : x);
}

FLOAT Common::Foundation::Util::MathUtil::Abs(FLOAT param) {
	return static_cast<FLOAT>(fabs(param));
}

constexpr BOOL Common::Foundation::Util::MathUtil::IsZero(FLOAT value) {
	return value * value < Epsilon * Epsilon;
}

constexpr BOOL Common::Foundation::Util::MathUtil::IsNotZero(FLOAT value) {
	return !IsZero(value);
}

BOOL Common::Foundation::Util::MathUtil::IsEqual(FLOAT a, FLOAT b) {
	return Abs(a - b) < Epsilon;
}

BOOL Common::Foundation::Util::MathUtil::IsNotEqual(FLOAT a, FLOAT b) {
	return Abs(a - b) >= Epsilon;
}

BOOL Common::Foundation::Util::MathUtil::IsEqual(const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs) {
	return IsEqual(lhs.x, rhs.x) && IsEqual(lhs.y, rhs.y);
}

BOOL Common::Foundation::Util::MathUtil::IsNotEqual(const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs) {
	return IsNotEqual(lhs, rhs);
}

BOOL Common::Foundation::Util::MathUtil::IsEqual(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
	return IsEqual(lhs.x, rhs.x) && IsEqual(lhs.y, rhs.y) && IsEqual(lhs.z, rhs.z);
}

BOOL Common::Foundation::Util::MathUtil::IsNotEqual(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) {
	return IsNotEqual(lhs, rhs);
}

BOOL Common::Foundation::Util::MathUtil::IsEqual(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs) {
	return IsEqual(lhs.x, rhs.x) && IsEqual(lhs.y, rhs.y) && IsEqual(lhs.z, rhs.z) && IsEqual(lhs.w, rhs.w);
}

BOOL Common::Foundation::Util::MathUtil::IsNotEqual(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs) {
	return IsNotEqual(lhs, rhs);
}

DirectX::PackedVector::XMFLOAT3PK Common::Foundation::Util::MathUtil::PackXMFLOAT3(const DirectX::XMFLOAT3& v) {
	return DirectX::PackedVector::XMFLOAT3PK(v.x, v.y, v.z);
}

DirectX::XMVECTOR Common::Foundation::Util::MathUtil::CalcUpVector(const DirectX::XMFLOAT3& dir) {
	const DirectX::XMVECTOR dirVec = XMLoadFloat3(&dir);

	DirectX::XMVECTOR up = UnitVector::UpVector;
	if (dir.z >= 0.0f) {
		const FLOAT dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dirVec, up));
		if (dot > 0.99f) up = UnitVector::BackwardVector;
		else if (dot < -0.99f) up = UnitVector::ForwardVector;
	}
	else {
		up = UnitVector::DownVector;

		const FLOAT dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dirVec, up));
		if (dot > 0.99f) up = UnitVector::ForwardVector;
		else if (dot < -0.99f) up = UnitVector::BackwardVector;
	}

	DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, dirVec));
	up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dirVec, right));

	return up;
}

DirectX::XMVECTOR Common::Foundation::Util::MathUtil::CalcUpVector(const DirectX::XMVECTOR& dir) {
	DirectX::XMFLOAT3 dirf;
	DirectX::XMStoreFloat3(&dirf, dir);

	DirectX::XMVECTOR up = UnitVector::UpVector;
	if (dirf.z >= 0.0f) {
		const FLOAT dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, up));
		if (dot > 0.99f) up = UnitVector::BackwardVector;
		else if (dot < -0.99f) up = UnitVector::ForwardVector;
	}
	else {
		up = UnitVector::DownVector;

		const FLOAT dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, up));
		if (dot > 0.99f) up = UnitVector::ForwardVector;
		else if (dot < -0.99f) up = UnitVector::BackwardVector;
	}

	DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, dir));
	up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dir, right));

	return up;
}

void Common::Foundation::Util::MathUtil::CalcUpVector(DirectX::XMFLOAT3& dst, const DirectX::XMFLOAT3& dir) {
	const DirectX::XMVECTOR dirVec = XMLoadFloat3(&dir);

	DirectX::XMVECTOR up = UnitVector::UpVector;
	if (dir.z >= 0.0f) {
		const FLOAT dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dirVec, up));
		if (dot > 0.99f) up = UnitVector::BackwardVector;
		else if (dot < -0.99f) up = UnitVector::ForwardVector;
	}
	else {
		up = UnitVector::DownVector;

		const FLOAT dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dirVec, up));
		if (dot > 0.99f) up = UnitVector::ForwardVector;
		else if (dot < -0.99f) up = UnitVector::BackwardVector;
	}

	DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, dirVec));
	up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dirVec, right));

	DirectX::XMStoreFloat3(&dst, up);
}

void Common::Foundation::Util::MathUtil::CalcUpVector(DirectX::XMFLOAT3& dst, const DirectX::XMVECTOR& dir) {
	DirectX::XMFLOAT3 dirf;
	DirectX::XMStoreFloat3(&dirf, dir);

	DirectX::XMVECTOR up = UnitVector::UpVector;
	if (dirf.z >= 0.0f) {
		const FLOAT dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, up));
		if (dot > 0.99f) up = UnitVector::BackwardVector;
		else if (dot < -0.99f) up = UnitVector::ForwardVector;
	}
	else {
		up = UnitVector::DownVector;

		const FLOAT dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, up));
		if (dot > 0.99f) up = UnitVector::ForwardVector;
		else if (dot < -0.99f) up = UnitVector::BackwardVector;
	}

	DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, dir));
	up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dir, right));

	DirectX::XMStoreFloat3(&dst, up);
}

#endif // __MATHUTIL_INL__