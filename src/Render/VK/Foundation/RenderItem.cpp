#include "Render/VK/Foundation/Core/pch_vk.h"
#include "Render/VK/Foundation/RenderItem.hpp"

using namespace Render::VK::Foundation;

RenderItem::RenderItem(std::uint32_t numSwapChainImages) {
	DescriptorSets.resize(numSwapChainImages);
	UniformBuffers.resize(numSwapChainImages);
	UniformBufferMemories.resize(numSwapChainImages);

	NumFramesDirty = numSwapChainImages;
}