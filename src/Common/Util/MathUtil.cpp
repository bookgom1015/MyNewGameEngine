#include "Common/Util/MathUtil.hpp"

using namespace Common::Util;
using namespace DirectX;

#include <sstream>

FLOAT MathUtil::AngleFromXY(FLOAT x, FLOAT y) {
	FLOAT theta = 0.f;

	// Quadrant I or IV
	if (x >= 0.f) {
		// If x = 0, then atanf(y/x) = +pi/2 if y > 0
		//                atanf(y/x) = -pi/2 if y < 0
		theta = atanf(y / x); // in [-pi/2, +pi/2]

		if (theta < 0.f)
			theta += 2.f * Pi; // in [0, 2*pi).
	}
	// Quadrant II or III
	else {
		theta = atanf(y / x) + Pi; // in [0, 2*pi).
	}

	return theta;
}

XMVECTOR MathUtil::SphericalToCartesian(FLOAT radius, FLOAT theta, FLOAT phi) {
	return XMVectorSet(
		radius * sinf(phi) * cosf(theta),
		radius * cosf(phi),
		radius * sinf(phi) * sinf(theta),
		1.f);
}

XMMATRIX MathUtil::InverseTranspose(CXMMATRIX M) {
	// Inverse-transpose is just applied to normals.  So zero out 
	// translation row so that it doesn't get into our inverse-transpose
	// calculation--we don't want the inverse-transpose of the translation.
	XMMATRIX A = M;
	A.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	XMVECTOR det = XMMatrixDeterminant(A);

	return XMMatrixTranspose(XMMatrixInverse(&det, A));
}

XMFLOAT4X4 MathUtil::Identity4x4() {
	static XMFLOAT4X4 I(
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f);

	return I;
}

XMVECTOR MathUtil::RandUnitVec3() {
	const XMVECTOR One = XMVectorSet(1.f, 1.f, 1.f, 1.f);
	const XMVECTOR Zero = XMVectorZero();

	// Keep trying until we get a point on/in the hemisphere.
	while (TRUE) {
		// Generate random point in the cube [-1,1]^3.
		const XMVECTOR v = XMVectorSet(MathUtil::RandF(-1.f, 1.f), MathUtil::RandF(-1.f, 1.f), MathUtil::RandF(-1.f, 1.f), 0.f);

		// Ignore points outside the unit sphere in order to get an even distribution 
		// over the unit sphere.  Otherwise points will clump more on the sphere near 
		// the corners of the cube.

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		return XMVector3Normalize(v);
	}
}

XMVECTOR MathUtil::RandHemisphereUnitVec3(XMVECTOR n) {
	const XMVECTOR One = XMVectorSet(1.f, 1.f, 1.f, 1.f);
	const XMVECTOR Zero = XMVectorZero();

	// Keep trying until we get a point on/in the hemisphere.
	while (TRUE) {
		// Generate random point in the cube [-1,1]^3.
		const XMVECTOR v = XMVectorSet(MathUtil::RandF(-1.f, 1.f), MathUtil::RandF(-1.f, 1.f), MathUtil::RandF(-1.f, 1.f), 0.f);

		// Ignore points outside the unit sphere in order to get an even distribution 
		// over the unit sphere.  Otherwise points will clump more on the sphere near 
		// the corners of the cube.

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		// Ignore points in the bottom hemisphere.
		if (XMVector3Less(XMVector3Dot(n, v), Zero))
			continue;

		return XMVector3Normalize(v);
	}
}

std::string MathUtil::to_string(DirectX::XMFLOAT2 vec) {
	std::stringstream sstream;
	sstream << "[ " << vec.x << ", " << vec.y << " ]";

	return sstream.str();
}

std::string MathUtil::to_string(DirectX::XMFLOAT3 vec) {
	std::stringstream sstream;
	sstream << "[ " << vec.x << ", " << vec.y << ", " << vec.z << " ]";

	return sstream.str();
}

std::string MathUtil::to_string(DirectX::XMFLOAT4 vec) {
	std::stringstream sstream;
	sstream << "[ " << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << " ]";

	return sstream.str();
}

std::string MathUtil::to_string(DirectX::XMFLOAT4X4 mat) {
	std::stringstream sstream;
	sstream << "| " << mat.m[0][0] << ' ' << mat.m[0][1] << ' ' << mat.m[0][2] << ' ' << mat.m[0][3] << " |" << std::endl;
	sstream << "| " << mat.m[1][0] << ' ' << mat.m[1][1] << ' ' << mat.m[1][2] << ' ' << mat.m[1][3] << " |" << std::endl;
	sstream << "| " << mat.m[2][0] << ' ' << mat.m[2][1] << ' ' << mat.m[2][2] << ' ' << mat.m[2][3] << " |" << std::endl;
	sstream << "| " << mat.m[3][0] << ' ' << mat.m[3][1] << ' ' << mat.m[3][2] << ' ' << mat.m[3][3] << " |";

	return sstream.str();
}