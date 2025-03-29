#pragma once

namespace Common::Foundation {
	typedef size_t Hash;

	namespace Util {
		class HashUtil {
		public:
			static Hash HashCombine(Hash seed, Hash value);
		};
	}
}