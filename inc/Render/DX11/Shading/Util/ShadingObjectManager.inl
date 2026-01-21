#ifndef __SHADINGOBJECTMANGER_INL__
#define __SHADINGOBJECTMANGER_INL__

namespace Render::DX11::Shading::Util {
	template <typename T>
		requires std::is_base_of_v<Foundation::ShadingObject, T>
	void ShadingObjectManager::Add() {
		auto obj = std::make_unique<T>();
		mShadingObjectRefs[typeid(T)] = obj.get();
		mShadingObjects.emplace_back(std::move(obj));
	}

	template <typename T>
		requires std::is_base_of_v<Foundation::ShadingObject, T>
	T* ShadingObjectManager::Get() {
		return static_cast<T*>(mShadingObjectRefs[typeid(T)]);
	}
}

#endif // __SHADINGOBJECTMANGER_INL__