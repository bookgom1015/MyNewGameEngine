#ifndef __RENDERER_INL__
#define __RENDERER_INL__

void Common::Render::Renderer::SetCamera(Common::Foundation::Camera::GameCamera* const pCamera) {
	mpCamera = pCamera;
}

constexpr BOOL Common::Render::Renderer::IsShadowEnabled() const {
	return mbShadowEnabled;
}

#endif // __RENDERER_INL__