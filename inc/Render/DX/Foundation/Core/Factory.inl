#ifndef __FACTORY_INL__
#define __FACTORY_INL__

IDXGIFactory4* Render::DX::Foundation::Core::Factory::DxgiFactory() const {
	return mDxgiFactory.Get();
}

BOOL Render::DX::Foundation::Core::Factory::AllowTearing() const {
	return mbAllowTearing;
}

#endif // __FACTORY_INL__