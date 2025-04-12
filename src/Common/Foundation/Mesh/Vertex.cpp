#include "Common/Foundation/Mesh/Vertex.h"
#include "Common/Util/MathUtil.hpp"

#include <vector>

using namespace Common::Foundation::Mesh;

BOOL Vertex::operator==(const Vertex& other) const {
	return Common::Util::MathUtil::IsEqual(Position, other.Position) &&
		Common::Util::MathUtil::IsEqual(Normal, other.Normal) &&
		Common::Util::MathUtil::IsEqual(TexCoord, other.TexCoord);
}