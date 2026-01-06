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
#include <vector>
#include "Common/AccelerationStructure/LinearAlgebra.h"
#include "Common/AccelerationStructure/Geometry.h"

namespace Common::AccelerationStructure {
	struct Clock {
		std::uint32_t FirstValue;
		Clock() { reset(); }
		void reset() { FirstValue = clock(); }
		std::uint32_t readMS() { return (clock() - FirstValue) / (CLOCKS_PER_SEC / 1000); }
	};

	class CacheFriendlyBVH {
	public:
		// The nice version of the BVH - a shallow hierarchy of inner and leaf nodes
		struct BVHNode {
			Vector3Df Bottom;
			Vector3Df Top;
			virtual bool IsLeaf() = 0;
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
					std::uint32_t IdxLeft;
					std::uint32_t IdxRight;
				} Inner;
				// leaf node: stores triangle count and starting index in triangle list
				struct {
					std::uint32_t Count; // Top-most bit set, leafnode if set, innernode otherwise
					std::uint32_t StartIndexInTriIndexList;
				} Leaf;
			} u;
		};

		// Work item for creation of BVH:
		struct BBoxTmp {
			// Bottom point (ie minx,miny,minz)
			Vector3Df Bottom;
			// Top point (ie maxx,maxy,maxz)
			Vector3Df Top;
			// Center point, ie 0.5*(top-bottom)
			Vector3Df Center; // = bbox centroid
			// Triangle
			const Triangle* pTriangles;  // triangle list
			BBoxTmp() :
				Bottom{ FLT_MAX, FLT_MAX, FLT_MAX },
				Top{ -FLT_MAX, -FLT_MAX, -FLT_MAX },
				pTriangles{} {
			}
		};
		// vector of triangle bounding boxes needed during BVH construction
		using BBoxEntries = std::vector<BBoxTmp>;  

		static const std::uint32_t InvalidAxis = std::numeric_limits<std::uint32_t>::max();

	public:
		// The single-point entrance to the BVH - call only this
		void UpdateBoundingVolumeHierarchy();

	private:
		BVHNode* CreateBVH();

		BVHNode* Recurse(BBoxEntries& work, std::uint32_t depth = 0);

		// The ugly, cache-friendly form of the BVH: 32 bytes
		void CreateCFBVH(); // CacheFriendlyBVH

		// Writes in the gpCFBVH and gTriIndexListNo arrays,
		// creating a cache-friendly version of the BVH
		void PopulateCacheFriendlyBVH(
			const Triangle* pFirstTriangle,
			BVHNode* root,
			std::uint32_t& idxBoxes,
			std::uint32_t& idxTriList);

		// recursively count bboxes
		std::uint32_t CountBoxes(BVHNode* root);
		// recursively count triangles
		std::uint32_t CountTriangles(BVHNode* root);
		// recursively count depth
		void CountDepth(BVHNode* root, std::uint32_t depth, std::uint32_t& maxDepth);

	private:
		Triangle* mpTriangles{};

		std::uint32_t* mpTriIndexList{};
		std::uint32_t mNumTriIndexList{};

		BVHNode* mpSceneBVH{};

		Vertex* mpVertices{};

		std::uint32_t mpNumCFBVH{};
		CacheFriendlyBVHNode* mpCFBVH{};
	};
}

#endif // __BVH_H__