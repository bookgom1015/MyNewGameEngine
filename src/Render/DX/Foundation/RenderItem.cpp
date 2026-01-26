#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Foundation/RenderItem.hpp"

using namespace Render::DX::Foundation;

RenderItem::RenderItem(INT numFrameResource) {
	NumFramesDirty = numFrameResource << 1;
}

Common::Foundation::Hash RenderItem::Hash(const RenderItem* ptr) {
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
	return Common::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(addr));
}