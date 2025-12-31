#pragma once

#include "Render/VK/Foundation/ShadingObject.hpp"

namespace Render::VK::Shading {
	namespace Shader {
		enum Type {
			VS_GBuffer = 0,
			PS_GBuffer,
			Count
		};
	}

	namespace DescriptorSet {
		enum {
			UB_Pass = 0,
			UB_Object,
			UB_Material,
			Count
		};
	}

	namespace RenderPass {
		enum {
			C_Color = 0,
			C_Normal,
			C_Position,
			Count
		};
	}

	namespace GBuffer {
		class GBufferClass : public Foundation::ShadingObject {
		public:
			struct InitData {
				Foundation::Core::Device* Device = nullptr;
				Util::ShaderManager* ShaderManager = nullptr;
				UINT ClientWidth = 0;
				UINT ClientHeight = 0;
			};

		public:
			GBufferClass();
			virtual ~GBufferClass() = default;

		public:
			virtual BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				void* const pData) override;
			virtual void CleanUp() override;

			virtual BOOL CompileShaders() override;
			virtual BOOL BuildDescriptorSets() override;
			virtual BOOL BuildPipelineLayouts() override;
			virtual BOOL BuildImages() override;
			virtual BOOL BuildImageViews() override;
			virtual BOOL BuildRenderPass() override;
			virtual BOOL BuildPipelineStates() override;
			virtual BOOL BuildFramebuffers() override;
			virtual BOOL OnResize(UINT width, UINT height) override;

		private:
			void DestroyResizableObjects();

		private:
			InitData mInitData;

			std::array<Common::Foundation::Hash, Shader::Count> mShaderHashes;

			VkDescriptorSetLayout mDescriptorSetLayout;
			VkPipelineLayout mPipelineLayout;
			VkRenderPass mRenderPass;
			VkPipeline mGraphicsPipeline;
			VkFramebuffer mFramebuffer;

			VkImage mColorImage;
			VkImageView mColorImageView;
			VkDeviceMemory mColorImageMemory;

			VkImage mNormalImage;
			VkImageView mNormalImageView;
			VkDeviceMemory mNormalImageMemory;

			VkImage mPositionImage;
			VkImageView mPositionImageView;
			VkDeviceMemory mPositionImageMemory;
		};

		using InitDataPtr = std::unique_ptr<GBufferClass::InitData>;

		InitDataPtr MakeInitData();
	}
}