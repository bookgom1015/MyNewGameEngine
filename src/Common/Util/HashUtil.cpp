#include "Common/Util/HashUtil.hpp"

using namespace Common::Util;

size_t HashUtil::HashCombine(size_t seed, size_t value) {
	return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}