#pragma once

namespace Common::Foundation::Util {
	class HashUtil {
	public:
		static size_t HashCombine(size_t seed, size_t value);
	};
}