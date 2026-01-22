#ifndef __SHADERMANAGER_INL__
#define __SHADERMANAGER_INL__

namespace std {
	template<>
	struct hash<Render::DX11::Shading::Util::ShaderManager::D3D11ShaderInfo> {
		Common::Foundation::Hash operator()(const Render::DX11::Shading::Util::ShaderManager::D3D11ShaderInfo& info) const {
			Common::Foundation::Hash hash = 0;
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCSTR>()(info.FileName));
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCSTR>()(info.EntryPoint));
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCSTR>()(info.Target));
			return hash;
		}
	};
}

ID3DBlob* Render::DX11::Shading::Util::ShaderManager::GetShader(Common::Foundation::Hash hash) {
	return mShaders[hash].Get();
}

#endif // __SHADERMANAGER_INL__