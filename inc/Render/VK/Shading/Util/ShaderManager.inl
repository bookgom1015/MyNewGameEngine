#ifndef __SHADERMANAGER_INL__
#define __SHADERMANAGER_INL__

namespace std {
	template<>
	struct hash<Render::VK::Shading::Util::ShaderManager::VkShaderInfo> {
		Common::Foundation::Hash operator()(const Render::VK::Shading::Util::ShaderManager::VkShaderInfo& info) const {
			Common::Foundation::Hash hash = 0;
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCSTR>()(info.FileName));
			hash = Common::Util::HashUtil::HashCombine(hash, std::hash<LPCSTR>()(info.EntryPoint));
			return hash;
		}
	};
}

#endif // __SHADERMANAGER_INL__