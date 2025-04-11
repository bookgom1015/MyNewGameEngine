#pragma once

namespace Common::Foundation {
	typedef size_t Hash;
}

namespace Common::Util {
	class HashUtil {
	public:
		static Foundation::Hash HashCombine(Foundation::Hash seed, Foundation::Hash value);
	};
}