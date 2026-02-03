#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/Resource/MaterialData.hpp"

using namespace Render::DX11::Foundation::Resource;

MaterialData::MaterialData(UINT numFrameResources) {
	FrameDirty = numFrameResources << 1;
}