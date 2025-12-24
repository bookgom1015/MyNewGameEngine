#pragma once

#include "Render/VK/Foundation/ShadingObject.hpp"

namespace Render::VK::Shading {
	namespace GBuffer {
		class GBufferClass : public Foundation::ShadingObject {
		public:
			GBufferClass();
			virtual ~GBufferClass() = default;

		public:
			virtual BOOL Initialize(void* const pInitData) override;
			virtual void CleanUp() override;

			virtual BOOL CompileShaders() override;
		};
	}
}