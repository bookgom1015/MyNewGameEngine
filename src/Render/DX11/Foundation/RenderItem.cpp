#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/RenderItem.hpp"
#include "Render/DX11/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX11/Foundation/Resource/MaterialData.hpp"

using namespace Render::DX11::Foundation;

Common::Foundation::Hash RenderItem::Hash(const RenderItem* ptr) {
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
	return Common::Util::HashUtil::HashCombine(
		0, static_cast<Common::Foundation::Hash>(addr));
}