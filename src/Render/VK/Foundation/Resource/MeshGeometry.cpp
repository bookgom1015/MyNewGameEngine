#include "Render/VK/Foundation/Resource/MeshGeometry.hpp"

using namespace Render::VK::Foundation::Resource;

Common::Foundation::Hash MeshGeometry::Hash(MeshGeometry* ptr) {
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
	return Common::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(addr));
}