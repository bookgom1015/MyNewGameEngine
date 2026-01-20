#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Foundation/Resource/MaterialData.hpp"

using namespace Render::DX::Foundation::Resource;

MaterialData::MaterialData(INT numFrameResource) {
	NumFramesDirty = numFrameResource;
}