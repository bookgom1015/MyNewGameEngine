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
#ifndef __BVH_H__
#define __BVH_H__

#include <list>
#include <ctime>
#include "Common/AccelerationStructure/LinearAlgebra.h"
#include "Common/AccelerationStructure/Geometry.h"

namespace Common::AccelerationStructure {
	struct Clock {
		unsigned firstValue;
		Clock() { reset(); }
		void reset() { firstValue = clock(); }
		unsigned readMS() { return (clock() - firstValue) / (CLOCKS_PER_SEC / 1000); }
	};

	// The nice version of the BVH - a shallow hierarchy of inner and leaf nodes
	struct BVHNode {
		Vector3Df Bottom;
		Vector3Df Top;
		virtual bool IsLeaf() = 0; // pure virtual
	};

	struct BVHInner : BVHNode {
		BVHNode* Left;
		BVHNode* Right;
		virtual bool IsLeaf() { return false; }
	};

	struct BVHLeaf : BVHNode {
		std::list<const Triangle*> Triangles;
		virtual bool IsLeaf() { return true; }
	};

	struct CacheFriendlyBVHNode {
		// bounding box
		Vector3Df Bottom;
		Vector3Df Top;

		// parameters for leafnodes and innernodes occupy same space (union) to save memory
		// top bit discriminates between leafnode and innernode
		// no pointers, but indices (int): faster

		union {
			// inner node - stores indexes to array of CacheFriendlyBVHNode
			struct {
				unsigned IdxLeft;
				unsigned IdxRight;
			} Inner;
			// leaf node: stores triangle count and starting index in triangle list
			struct {
				unsigned Count; // Top-most bit set, leafnode if set, innernode otherwise
				unsigned StartIndexInTriIndexList;
			} Leaf;
		} u;
	};

	// The ugly, cache-friendly form of the BVH: 32 bytes
	void CreateCFBVH(); // CacheFriendlyBVH

	// The single-point entrance to the BVH - call only this
	void UpdateBoundingVolumeHierarchy(const char* filename);
}

#endif // __BVH_H__