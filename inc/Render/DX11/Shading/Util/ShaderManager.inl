#ifndef __SHADERMANAGER_INL__
#define __SHADERMANAGER_INL__

namespace std {
	template<>
	struct hash<Render::DX11::Shading::Util::ShaderManager::D3D11ShaderInfo> {
		Common::Foundation::Hash operator()(const Render::DX11::Shading::Util::ShaderManager::D3D11ShaderInfo& info) const {
			Common::Foundation::Hash hash = 0;
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.FileName));
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.EntryPoint));
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.TargetProfile));
			for (UINT i = 0, end = static_cast<UINT>(info.DefineCount); i < end; ++i) {
				hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.Defines[i].Name));
				hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCWSTR>()(info.Defines[i].Value));
			}
			hash = Common::Util::HashUtil::HashCombine(hash, static_cast<UINT>(info.DefineCount));
			return hash;
		}
	};
}

IDxcBlob* Render::DX11::Shading::Util::ShaderManager::GetShader(Common::Foundation::Hash hash) {
	return mShaders[hash].Get();
}

#endif // __SHADERMANAGER_INL__