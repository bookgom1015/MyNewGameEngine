#ifndef __SHADERMANAGER_INL__
#define __SHADERMANAGER_INL__

IDxcBlob* Render::DX::Shading::Util::ShaderManager::GetShader(Common::Foundation::Hash hash) {
	return mShaders[hash].Get();
}

#endif // __SHADERMANAGER_INL__