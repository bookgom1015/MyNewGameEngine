#include "Common/AccelerationStructure/BVH.h"

#include <string>
#include <ctime>
#include <cfloat>
#include <algorithm>
#include <iostream>

#define BVH_STACK_SIZE 32

using namespace Common::AccelerationStructure;

// BVH CONSTRUCTION
// This builds the BVH, finding optimal split planes for each depth
// uses binning: divide the work bounding box into a number of equally sized "bins" along one of the axes
// choose axis and splitting plane resulting in least cost (determined by surface area heuristic or SAH)
// SAH (surface area heuristic): the larger the surface area of a bounding box, the costlier it is to raytrace
// find the bbox with the minimum surface area
//
// I strongly recommend reading Ingo Wald's 2007 paper "On fast SAH based BVH construction",  
// http://www.sci.utah.edu/~wald/Publications/2007/ParallelBVHBuild/fastbuild.pdf, to understand the code below

// The gateway - creates the "pure" BVH, and then copies the results in the cache-friendly one
void CacheFriendlyBVH::UpdateBoundingVolumeHierarchy() {
	//if (!mpSceneBVH) {
	//	std::string BVHcacheFilename(filename);
	//	BVHcacheFilename += ".bvh";
	//	FILE* fp;
	//	fopen_s(&fp, BVHcacheFilename.c_str(), "rb");
	//	if (!fp) {
	//		// No cached BVH data - we need to calculate them
	//		Clock me;
	//		mpSceneBVH = CreateBVH();
	//
	//		// Now that the BVH has been created, copy its data into a more cache-friendly format
	//		// (CacheFriendlyBVHNode occupies exactly 32 bytes, i.e. a cache-line)
	//		Common::AccelerationStructure::CreateCFBVH();
	//
	//		// Now store the results, if possible...
	//		fopen_s(&fp, BVHcacheFilename.c_str(), "wb");
	//		if (!fp) return;
	//		if (1 != fwrite(&mpNumCFBVH, sizeof(std::uint32_t), 1, fp)) return;
	//		if (1 != fwrite(&mpTriIndexListNo, sizeof(std::uint32_t), 1, fp)) return;
	//		if (mpNumCFBVH != fwrite(mpCFBVH, sizeof(CacheFriendlyBVHNode), mpNumCFBVH, fp)) return;
	//		if (mpTriIndexListNo != fwrite(mpTriIndexList, sizeof(std::uint32_t), mpTriIndexListNo, fp)) return;
	//		fclose(fp);
	//	}
	//	else { // BVH has been built already and stored in a file, read the file
	//		if (1 != fread(&mpNumCFBVH, sizeof(std::uint32_t), 1, fp)) return;
	//		if (1 != fread(&mpTriIndexListNo, sizeof(std::uint32_t), 1, fp)) return;
	//		mpCFBVH = new CacheFriendlyBVHNode[mpNumCFBVH];
	//		mpTriIndexList = new std::uint32_t[mpTriIndexListNo];
	//		if (mpNumCFBVH != fread(mpCFBVH, sizeof(CacheFriendlyBVHNode), mpNumCFBVH, fp)) return;
	//		if (mpTriIndexListNo != fread(mpTriIndexList, sizeof(std::uint32_t), mpTriIndexListNo, fp)) return;
	//		fclose(fp);
	//	}
	//}
}

CacheFriendlyBVH::BVHNode* CacheFriendlyBVH::CreateBVH() {
	/* Summary:
	1. Create work BBox
	2. Create BBox for every triangle and compute bounds
	3. Expand bounds work BBox to fit all triangle bboxes
	4. Compute triangle bbox centre and add triangle to working list
	5. Build BVH tree with Recurse()
	6. Return root node
	*/
	std::vector<BBoxTmp> work;
	Vector3Df bottom(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector3Df top(-FLT_MAX, -FLT_MAX, -FLT_MAX);

#ifdef _DEBUG
	std::cout << "Gathering bounding box info from all triangles..." << std::endl;
#endif 
	// for each triangle
	for (std::uint32_t j = 0; j < mNumTriIndexList; j++) {
		const Triangle& triangle = mpTriangles[j];

		// create a new temporary bbox per triangle 
		BBoxTmp b;
		b.pTriangles = &triangle;

		// loop over triangle vertices and pick smallest vertex for bottom of triangle bbox
		b.Bottom = min3(b.Bottom, mpVertices[triangle.Index1]);  // index of vertex
		b.Bottom = min3(b.Bottom, mpVertices[triangle.Index2]);
		b.Bottom = min3(b.Bottom, mpVertices[triangle.Index3]);

		// loop over triangle vertices and pick largest vertex for top of triangle bbox
		b.Top = max3(b.Top, mpVertices[triangle.Index1]);
		b.Top = max3(b.Top, mpVertices[triangle.Index2]);
		b.Top = max3(b.Top, mpVertices[triangle.Index3]);

		// expand working list bbox by largest and smallest triangle bbox bounds
		bottom = min3(bottom, b.Bottom);
		top = max3(top, b.Top);

		// compute triangle bbox center: (bbox top + bbox bottom) * 0.5
		b.Center = (b.Top + b.Bottom) * 0.5f;

		// add triangle bbox to working list
		work.push_back(b);
	}

	// ...and pass it to the recursive function that creates the SAH AABB BVH
	// (Surface Area Heuristic, Axis-Aligned Bounding Boxes, Bounding Volume Hierarchy)
	BVHNode* root = Recurse(work); // builds BVH and returns root node
	root->Bottom = bottom; // bottom is bottom of bbox bounding all triangles in the scene
	root->Top = top;

	return root;
}

// recursive building of BVH nodes
// work is the working list (std::vector<>) of triangle bounding boxes 
CacheFriendlyBVH::BVHNode* CacheFriendlyBVH::Recurse(
		BBoxEntries& work, std::uint32_t depth) {
	// terminate recursion case: 
	// if work set has less then 4 elements (triangle bounding boxes), create a leaf node 
	// and create a list of the triangles contained in the node
	if (work.size() < 4) {
		BVHLeaf* leaf = new BVHLeaf;
		for (BBoxEntries::iterator it = work.begin(); it != work.end(); it++)
			leaf->Triangles.push_back(it->pTriangles);
		return leaf;
	}

	// else, work size > 4, divide  node further into smaller nodes
	// start by finding the working list's bounding box (top and bottom)
	Vector3Df bottom(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector3Df top(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// loop over all bboxes in current working list, expanding/growing the working list bbox
	for (std::uint32_t i = 0; i < work.size(); i++) {  // meer dan 4 bboxen in work
		BBoxTmp& v = work[i];
		bottom = min3(bottom, v.Bottom);
		top = max3(top, v.Top);
	}

	// SAH, surface area heuristic calculation
	// find surface area of bounding box by multiplying the dimensions of the working list's bounding box
	float side1 = top.x - bottom.x;  // length bbox along X-axis
	float side2 = top.y - bottom.y;  // length bbox along Y-axis
	float side3 = top.z - bottom.z;  // length bbox along Z-axis

	// the current bbox has a cost of (number of triangles) * surfaceArea of C = N * SA
	float minCost = work.size() * (side1 * side2 + side2 * side3 + side3 * side1);

	float bestSplit = FLT_MAX; // best split along axis, will indicate no split with better cost found (below)

	std::uint32_t bestAxis = InvalidAxis;

	// Try all 3 axises X, Y, Z
	for (std::uint32_t j = 0; j < 3; j++) {  // 0 = X, 1 = Y, 2 = Z axis
		std::uint32_t axis = j;

		// we will try dividing the triangles based on the current axis,
		// and we will try split values from "start" to "stop", one "step" at a time.
		float start, stop, step;

		// X-axis
		if (axis == 0) {
			start = bottom.x;
			stop = top.x;
		}
		// Y-axis
		else if (axis == 1) {
			start = bottom.y;
			stop = top.y;
		}
		// Z-axis
		else {
			start = bottom.z;
			stop = top.z;
		}

		// In that axis, do the bounding boxes in the work queue "span" across, (meaning distributed over a reasonable distance)?
		// Or are they all already "packed" on the axis? Meaning that they are too close to each other
		if (fabsf(stop - start) < 1e-4)
			// BBox side along this axis too short, we must move to a different axis!
			continue; // go to next axis

		// Binning: Try splitting at a uniform sampling (at equidistantly spaced planes) that gets smaller the deeper we go:
		// size of "sampling grid": 1024 (depth 0), 512 (depth 1), etc
		// each bin has size "step"
		step = (stop - start) / (1024.f / (depth + 1.f));

		// for each bin (equally spaced bins of size "step"):
		for (float testSplit = start + step; testSplit < stop - step; testSplit += step) {
			// Create left and right bounding box
			Vector3Df lbottom(FLT_MAX, FLT_MAX, FLT_MAX);
			Vector3Df ltop(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			Vector3Df rbottom(FLT_MAX, FLT_MAX, FLT_MAX);
			Vector3Df rtop(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			// The number of triangles in the left and right bboxes (needed to calculate SAH cost function)
			std::uint32_t countLeft = 0, countRight = 0;

			// For each test split (or bin), allocate triangles in remaining work list based on their bbox centers
			// this is a fast O(N) pass, no triangle sorting needed (yet)
			for (std::uint32_t i = 0; i < work.size(); i++) {
				BBoxTmp& v = work[i];

				// compute bbox center
				float value;
				if (axis == 0) value = v.Center.x;       // X-axis
				else if (axis == 1) value = v.Center.y;  // Y-axis
				else value = v.Center.z;			   // Z-axis

				if (value < testSplit) {
					// if center is smaller then testSplit value, put triangle in Left bbox
					lbottom = min3(lbottom, v.Bottom);
					ltop = max3(ltop, v.Top);
					++countLeft;
				}
				else {
					// else put triangle in right bbox
					rbottom = min3(rbottom, v.Bottom);
					rtop = max3(rtop, v.Top);
					++countRight;
				}
			}

			// Now use the Surface Area Heuristic to see if this split has a better "cost"

			// First, check for stupid partitionings, ie bins with 0 or 1 triangles make no sense
			if (countLeft <= 1 || countRight <= 1) continue;

			// It's a real partitioning, calculate the surface areas
			float lside1 = ltop.x - lbottom.x;
			float lside2 = ltop.y - lbottom.y;
			float lside3 = ltop.z - lbottom.z;

			float rside1 = rtop.x - rbottom.x;
			float rside2 = rtop.y - rbottom.y;
			float rside3 = rtop.z - rbottom.z;

			// calculate SurfaceArea of Left and Right BBox
			float surfaceLeft = lside1 * lside2 + lside2 * lside3 + lside3 * lside1;
			float surfaceRight = rside1 * rside2 + rside2 * rside3 + rside3 * rside1;

			// calculate total cost by multiplying left and right bbox by number of triangles in each
			float totalCost = surfaceLeft * countLeft + surfaceRight * countRight;

			// keep track of cheapest split found so far
			if (totalCost < minCost) {
				minCost = totalCost;
				bestSplit = testSplit;
				bestAxis = axis;
			}
		} // end of loop over all bins
	} // end of loop over all axises

	// at the end of this loop (which runs for every "bin" or "sample location"), 
	// we should have the best splitting plane, best splitting axis and bboxes with minimal traversal cost

	// If we found no split to improve the cost, create a BVH leaf
	if (bestAxis == InvalidAxis) {
		BVHLeaf* leaf = new BVHLeaf;
		for (BBoxEntries::iterator it = work.begin(); it != work.end(); it++)
			leaf->Triangles.push_back(it->pTriangles); // put triangles of working list in leaf's triangle list
		return leaf;
	}

	// Otherwise, create BVH inner node with L and R child nodes, split with the optimal value we found above
	BBoxEntries left;
	BBoxEntries right;  // BBoxEntries is a vector/list of BBoxTmp 
	Vector3Df lbottom(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector3Df ltop(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	Vector3Df rbottom(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector3Df rtop(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// distribute the triangles in the left or right child nodes
	// for each triangle in the work set
	for (std::uint32_t i = 0, end = static_cast<std::uint32_t>(work.size()); i < end; i++) {
		// create temporary bbox for triangle
		BBoxTmp& v = work[i];

		// compute bbox center 
		float value;
		if (bestAxis == 0) value = v.Center.x;
		else if (bestAxis == 1) value = v.Center.y;
		else value = v.Center.z;

		if (value < bestSplit) { // add temporary bbox v from work list to left BBoxentries list, 
			// becomes new working list of triangles in next step
			left.push_back(v);
			lbottom = min3(lbottom, v.Bottom);
			ltop = max3(ltop, v.Top);
		}
		else {

			// Add triangle bbox v from working list to right BBoxentries, 
			// becomes new working list of triangles in next step  
			right.push_back(v);
			rbottom = min3(rbottom, v.Bottom);
			rtop = max3(rtop, v.Top);
		}
	} // end loop for each triangle in working set

	// create inner node
	BVHInner* inner = new BVHInner;

	// recursively build the left child
	inner->Left = Recurse(left, depth + 1);
	inner->Left->Bottom = lbottom;
	inner->Left->Top = ltop;

	// recursively build the right child
	inner->Right = Recurse(right, depth + 1);
	inner->Right->Bottom = rbottom;
	inner->Right->Top = rtop;

	return inner;
}  // end of Recurse() function, returns the rootnode (when all recursion calls have finished)

// the following functions are required to create the cache-friendly BVH

void CacheFriendlyBVH::CreateCFBVH() {
	if (!mpSceneBVH) exit(1);

	std::uint32_t idxTriList = 0;
	std::uint32_t idxBoxes = 0;

	mNumTriIndexList = CountTriangles(mpSceneBVH);
	mpTriIndexList = new std::uint32_t[mNumTriIndexList];

	mpNumCFBVH = CountBoxes(mpSceneBVH);
	mpCFBVH = new CacheFriendlyBVHNode[mpNumCFBVH]; // array

	PopulateCacheFriendlyBVH(&mpTriangles[0], mpSceneBVH, idxBoxes, idxTriList);

	if ((idxBoxes != mpNumCFBVH - 1) || (idxTriList != mNumTriIndexList)) exit(1);

	std::uint32_t maxDepth = 0;
	CountDepth(mpSceneBVH, 0, maxDepth);
	if (maxDepth >= BVH_STACK_SIZE) exit(1);
}


void CacheFriendlyBVH::PopulateCacheFriendlyBVH(
		const Triangle* pFirstTriangle,
		BVHNode* root,
		std::uint32_t& idxBoxes,
		std::uint32_t& idxTriList) {
	std::uint32_t currIdxBoxes = idxBoxes;
	mpCFBVH[currIdxBoxes].Bottom = root->Bottom;
	mpCFBVH[currIdxBoxes].Top = root->Top;

	//DEPTH FIRST APPROACH (left first until complete)
	if (!root->IsLeaf()) { // inner node
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		// recursively populate left and right
		std::uint32_t idxLeft = ++idxBoxes;
		PopulateCacheFriendlyBVH(pFirstTriangle, p->Left, idxBoxes, idxTriList);
		std::uint32_t idxRight = ++idxBoxes;
		PopulateCacheFriendlyBVH(pFirstTriangle, p->Right, idxBoxes, idxTriList);
		mpCFBVH[currIdxBoxes].u.Inner.IdxLeft = idxLeft;
		mpCFBVH[currIdxBoxes].u.Inner.IdxRight = idxRight;
	}
	else { // leaf node
		BVHLeaf* p = dynamic_cast<BVHLeaf*>(root);
		std::uint32_t count = (std::uint32_t)p->Triangles.size();
		mpCFBVH[currIdxBoxes].u.Leaf.Count = 0x80000000 | count;  // highest bit set indicates a leaf node (inner node if highest bit is 0)
		mpCFBVH[currIdxBoxes].u.Leaf.StartIndexInTriIndexList = idxTriList;

		for (std::list<const Triangle*>::iterator it = p->Triangles.begin(); it != p->Triangles.end(); it++)
			mpTriIndexList[idxTriList++] = static_cast<std::uint32_t>(*it - pFirstTriangle);
	}
}

std::uint32_t CacheFriendlyBVH::CountBoxes(BVHNode* root) {
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		return 1 + CountBoxes(p->Left) + CountBoxes(p->Right);
	}

	return 1;
}

std::uint32_t CacheFriendlyBVH::CountTriangles(BVHNode* root) {
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		return CountTriangles(p->Left) + CountTriangles(p->Right);
	}

	BVHLeaf* p = dynamic_cast<BVHLeaf*>(root);
	return (std::uint32_t)p->Triangles.size();
}

void CacheFriendlyBVH::CountDepth(BVHNode* root, std::uint32_t depth, std::uint32_t& maxDepth) {
	if (maxDepth < depth) maxDepth = depth;
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		CountDepth(p->Left, depth + 1, maxDepth);
		CountDepth(p->Right, depth + 1, maxDepth);
	}
}