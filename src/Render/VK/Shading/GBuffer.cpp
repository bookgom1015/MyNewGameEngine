#include "Render/VK/Shading/GBuffer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/Util/VulkanUtil.hpp"
#include "Render/VK/Foundation/Core/Device.hpp"
#include "Render/VK/Foundation/Core/CommandObject.hpp"
#include "Render/VK/Shading/Util/ShaderManager.hpp"

using namespace Render::VK::Shading;

namespace {
	const CHAR* const GLSL_GBufferVS = "GBufferVS.spv";
	const CHAR* const GLSL_GBufferPS = "GBufferPS.spv";

	VkFormat ColorImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	VkFormat NormalImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkFormat PositionImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
}

GBuffer::InitDataPtr GBuffer::MakeInitData() {
	return std::unique_ptr<GBufferClass::InitData>(new GBufferClass::InitData());
}

GBuffer::GBufferClass::GBufferClass() {

}

BOOL GBuffer::GBufferClass::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

void GBuffer::GBufferClass::CleanUp() {
	DestroyResizableObjects();

	if (vkIsValidHandle(mGraphicsPipeline))
		vkDestroyPipeline(
			mInitData.Device->LogicalDevice(),
			mGraphicsPipeline,
			nullptr);
	if (vkIsValidHandle(mRenderPass))
		vkDestroyRenderPass(
			mInitData.Device->LogicalDevice(),
			mRenderPass,
			nullptr);
	if (vkIsValidHandle(mPipelineLayout))
		vkDestroyPipelineLayout(
			mInitData.Device->LogicalDevice(),
			mPipelineLayout,
			nullptr);
	if (vkIsValidHandle(mDescriptorSetLayout))
		vkDestroyDescriptorSetLayout(
			mInitData.Device->LogicalDevice(),
			mDescriptorSetLayout,
			nullptr);
}

BOOL GBuffer::GBufferClass::CompileShaders() {
	{
		const auto VS = Util::ShaderManager::VkShaderInfo(GLSL_GBufferVS, "main");
		mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_GBuffer]);
	}
	{
		const auto PS = Util::ShaderManager::VkShaderInfo(GLSL_GBufferPS, "main");
		mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_GBuffer]);
	}

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildDescriptorSets() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = DescriptorSet::UB_Pass;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding ubpLayoutBinding{};
	ubpLayoutBinding.binding = DescriptorSet::UB_Object;
	ubpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubpLayoutBinding.descriptorCount = 1;
	ubpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	ubpLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding ubmLayoutBinding{};
	ubmLayoutBinding.binding = DescriptorSet::UB_Material;
	ubmLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubmLayoutBinding.descriptorCount = 1;
	ubmLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	ubmLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding bindings[DescriptorSet::Count] = {
		uboLayoutBinding, ubpLayoutBinding, ubmLayoutBinding
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = _countof(bindings);
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(
			mInitData.Device->LogicalDevice(),
			&layoutInfo, 
			nullptr, 
			&mDescriptorSetLayout) != VK_SUCCESS) {
		ReturnFalse(mpLogFile, L"Failed to create descriptor set layout");
	}

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = ColorImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentDescription normalAttachment{};
	normalAttachment.format = NormalImageFormat;
	normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentDescription posAttachment{};
	posAttachment.format = PositionImageFormat;
	posAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	posAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	posAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	posAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	posAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	posAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	posAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentDescription attachments[RenderPass::Count] = {
		colorAttachment, normalAttachment, posAttachment
	};

	VkAttachmentReference colorRefs[RenderPass::Count] = {
		{ RenderPass::C_Color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ RenderPass::C_Normal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ RenderPass::C_Position, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
	};

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = RenderPass::Count;
	subpass.pColorAttachments = colorRefs;

	VkSubpassDependency subPassDependency{};
	subPassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subPassDependency.dstSubpass = 0;
	subPassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDependency.srcAccessMask = 0;
	subPassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = _countof(attachments);
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &subPassDependency;

	if (vkCreateRenderPass(
			mInitData.Device->LogicalDevice(), 
			&renderPassInfo, 
			nullptr, 
			&mRenderPass) != VK_SUCCESS) 
		ReturnFalse(mpLogFile, "Failed to create render pass");

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildPipelineLayouts() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; 
	pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(
			mInitData.Device->LogicalDevice(), 
			&pipelineLayoutInfo, 
			nullptr, 
			&mPipelineLayout) != VK_SUCCESS) {
		ReturnFalse(mpLogFile, L"Failed to create pipeline layout");
	}

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildImages() {
	CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::CreateImage(
		mpLogFile,
		mInitData.Device->PhysicalDevice(),
		mInitData.Device->LogicalDevice(),
		mInitData.ClientWidth,
		mInitData.ClientHeight,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		ColorImageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mColorImage,
		mColorImageMemory));

	CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::CreateImage(
		mpLogFile,
		mInitData.Device->PhysicalDevice(),
		mInitData.Device->LogicalDevice(),
		mInitData.ClientWidth,
		mInitData.ClientHeight,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		NormalImageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mNormalImage,
		mNormalImageMemory));

	CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::CreateImage(
		mpLogFile,
		mInitData.Device->PhysicalDevice(),
		mInitData.Device->LogicalDevice(),
		mInitData.ClientWidth,
		mInitData.ClientHeight,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		PositionImageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mPositionImage,
		mPositionImageMemory));

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildImageViews() {
	CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::CreateImageView(
		mpLogFile,
		mInitData.Device->LogicalDevice(),
		mColorImage,
		ColorImageFormat,
		1,
		VK_IMAGE_ASPECT_COLOR_BIT,
		mColorImageView));

	CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::CreateImageView(
		mpLogFile,
		mInitData.Device->LogicalDevice(),
		mNormalImage,
		NormalImageFormat,
		1,
		VK_IMAGE_ASPECT_COLOR_BIT,
		mNormalImageView));

	CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::CreateImageView(
		mpLogFile,
		mInitData.Device->LogicalDevice(),
		mPositionImage,
		PositionImageFormat,
		1,
		VK_IMAGE_ASPECT_COLOR_BIT,
		mPositionImageView));

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildPipelineStates() {
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = mPipelineLayout;
	pipelineInfo.renderPass = mRenderPass;

	// Shaders
	VkPipelineShaderStageCreateInfo vsStageInfo{};
	{
		const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_GBuffer]);
		vsStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vsStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vsStageInfo.module = VS;
		vsStageInfo.pName = "main";
	}

	VkPipelineShaderStageCreateInfo psStageInfo{};
	{
		const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::PS_GBuffer]);
		psStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		psStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		psStageInfo.module = PS;
		psStageInfo.pName = "main";
	}

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vsStageInfo, psStageInfo
	};

	pipelineInfo.stageCount = _countof(shaderStages);
	pipelineInfo.pStages = shaderStages;

	// Dynamic states
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = _countof(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;

	pipelineInfo.pDynamicState = &dynamicState;

	// Vertex input
	auto bindingDesc = Foundation::Util::VulkanUtil::GetVertexBindingDescription();
	auto vertexAttrDesc = Foundation::Util::VulkanUtil::GetVertexAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexAttrDesc.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc.data();

	pipelineInfo.pVertexInputState = &vertexInputInfo;

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	pipelineInfo.pInputAssemblyState = &inputAssembly;

	// Viewports and scissors
	VkViewport viewport{};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = static_cast<FLOAT>(mInitData.ClientWidth);
	viewport.height = static_cast<FLOAT>(mInitData.ClientHeight);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { mInitData.ClientWidth, mInitData.ClientHeight };

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	pipelineInfo.pViewportState = &viewportState;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.f;
	rasterizer.depthBiasClamp = 0.f;
	rasterizer.depthBiasSlopeFactor = 0.f;

	pipelineInfo.pRasterizationState = &rasterizer;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 0.2f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	pipelineInfo.pMultisampleState = &multisampling;

	// Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_SUBTRACT;

	VkPipelineColorBlendAttachmentState attachments[RenderPass::Count] = {
		colorBlendAttachment, colorBlendAttachment, colorBlendAttachment
	};

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = _countof(attachments);
	colorBlending.pAttachments = attachments;
	colorBlending.blendConstants[0] = 0.f;
	colorBlending.blendConstants[1] = 0.f;
	colorBlending.blendConstants[2] = 0.f;
	colorBlending.blendConstants[3] = 0.f;

	pipelineInfo.pColorBlendState = &colorBlending;

	// Depth and stencil testing
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.f;
	depthStencil.maxDepthBounds = 1.f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	pipelineInfo.pDepthStencilState = &depthStencil;

	if (vkCreateGraphicsPipelines(
			mInitData.Device->LogicalDevice(), 
			VK_NULL_HANDLE, 
			1, 
			&pipelineInfo,
			nullptr, 
			&mGraphicsPipeline) != VK_SUCCESS) 
		ReturnFalse(mpLogFile, L"Failed to create graphics pipeline");

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildFramebuffers() {
	VkImageView attachments[] = {
		mColorImageView, mNormalImageView, mPositionImageView
	};

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = mRenderPass;
	framebufferInfo.attachmentCount = _countof(attachments);
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.width = mInitData.ClientWidth;
	framebufferInfo.height = mInitData.ClientHeight;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(
			mInitData.Device->LogicalDevice(), 
			&framebufferInfo, 
			nullptr, 
			&mFramebuffer) != VK_SUCCESS) 
		ReturnFalse(mpLogFile, "Failed to create framebuffer");

	return TRUE;
}

BOOL GBuffer::GBufferClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	DestroyResizableObjects();
	CheckReturn(mpLogFile, BuildImages());
	CheckReturn(mpLogFile, BuildImageViews());
	CheckReturn(mpLogFile, BuildFramebuffers());

	return TRUE;
}

BOOL GBuffer::GBufferClass::DrawGBuffer(
		Foundation::Core::CommandObject* const pCmdObject,
		const VkViewport& viewport,
		const VkRect2D& scissorRect) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	decltype(auto) CmdBuffer = pCmdObject->CommandBuffer();

	if (vkBeginCommandBuffer(CmdBuffer, &beginInfo) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to begin recording command buffer");

	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mRenderPass;
		renderPassInfo.framebuffer = mFramebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { 
			mInitData.ClientWidth, mInitData.ClientHeight };

		const VkClearValue ClearColors[RenderPass::Count] = {
			{{0.0f, 0.0f, 0.0f, 1.0f}},
			{{0.0f, 0.0f, 0.0f, 0.0f}},
			{{0.0f, 0.0f, 0.0f, 1.0f}}
		};
		renderPassInfo.clearValueCount = _countof(ClearColors);
		renderPassInfo.pClearValues = ClearColors;
		vkCmdBeginRenderPass(CmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

		vkCmdSetViewport(CmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(CmdBuffer, 0, 1, &scissorRect);

		vkCmdEndRenderPass(CmdBuffer);
	}

	if (vkEndCommandBuffer(CmdBuffer) != VK_SUCCESS) 
		ReturnFalse(mpLogFile, L"Failed to record command buffer");

	return TRUE;
}

void GBuffer::GBufferClass::DestroyResizableObjects() {
	// Framebuffer
	if (vkIsValidHandle(mFramebuffer))
		vkDestroyFramebuffer(
			mInitData.Device->LogicalDevice(),
			mFramebuffer,
			nullptr);
	// Color
	if (vkIsValidHandle(mColorImageView))
		vkDestroyImageView(
			mInitData.Device->LogicalDevice(),
			mColorImageView,
			nullptr);
	if (vkIsValidHandle(mColorImage))
		vkDestroyImage(
			mInitData.Device->LogicalDevice(),
			mColorImage,
			nullptr);
	if (vkIsValidHandle(mColorImageMemory))
		vkFreeMemory(
			mInitData.Device->LogicalDevice(),
			mColorImageMemory,
			nullptr);
	// Normal
	if (vkIsValidHandle(mNormalImageView))
		vkDestroyImageView(
			mInitData.Device->LogicalDevice(),
			mNormalImageView,
			nullptr);
	if (vkIsValidHandle(mNormalImage))
		vkDestroyImage(
			mInitData.Device->LogicalDevice(),
			mNormalImage,
			nullptr);
	if (vkIsValidHandle(mNormalImageMemory))
		vkFreeMemory(
			mInitData.Device->LogicalDevice(),
			mNormalImageMemory,
			nullptr);
	// Position
	if (vkIsValidHandle(mPositionImageView))
		vkDestroyImageView(
			mInitData.Device->LogicalDevice(),
			mPositionImageView,
			nullptr);
	if (vkIsValidHandle(mPositionImage))
		vkDestroyImage(
			mInitData.Device->LogicalDevice(),
			mPositionImage,
			nullptr);
	if (vkIsValidHandle(mPositionImageMemory))
		vkFreeMemory(
			mInitData.Device->LogicalDevice(),
			mPositionImageMemory,
			nullptr);
}