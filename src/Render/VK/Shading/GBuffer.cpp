#include "Render/VK/Shading/GBuffer.hpp"

using namespace Render::VK::Shading;

GBuffer::GBufferClass::GBufferClass() {

}

BOOL GBuffer::GBufferClass::Initialize(void* const pInitData) {
	return TRUE;
}

void GBuffer::GBufferClass::CleanUp() {}

BOOL GBuffer::GBufferClass::CompileShaders() {
	return TRUE;
}