/*
*  CUDA based triangle mesh path tracer using BVH acceleration by Sam lapere, 2016
*  BVH implementation based on real-time CUDA ray tracer by Thanassis Tsiodras,
*  http://users.softlab.ntua.gr/~ttsiod/cudarenderer-BVH.html
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <cstdint>

#include "Common/AccelerationStructure/LinearAlgebra.h"

namespace Common::AccelerationStructure {
	struct Vertex : public Vector3Df {
		// Normal vector of this vertex
		Vector3Df Normal;

		Vertex(float x, float y, float z, float nx, float ny, float nz, float amb = 60.f) 
			: Vector3Df(x, y, z), Normal(Vector3Df(nx, ny, nz)) { /* assert |nx,ny,nz| = 1 */ }
	};

	struct Triangle {
		// Indexes in vertices array
		std::uint32_t Index1;
		std::uint32_t Index2;
		std::uint32_t Index3;
		// RGB Color Vector3Df 
		Vector3Df Color;
		// Center point
		Vector3Df Center;
		// Triangle normal
		Vector3Df Normal;
		// Ignore back-face culling flag
		bool TwoSided;
		// Raytracing intersection pre-computed cache:
		float d0, d1, d2, d3;
		Vector3Df e1, e2, e3;
		// bounding box
		Vector3Df Bottom;
		Vector3Df Top;
	};
}

#endif // __GEOMETRY_H__
