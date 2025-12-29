#ifndef __EYEADAPTION_INL__
#define __EYEADAPTION_INL__

Render::DX::Foundation::Resource::GpuResource*
Render::DX::Shading::EyeAdaption::EyeAdaptionClass::Luminance() const {
	return mSmoothedLuminance.get();
}

#endif // __EYEADAPTION_INL__