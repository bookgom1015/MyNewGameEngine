#include "Common/Foundation/Mesh/Vertex.hpp"
#include "Common/Foundation/Util/MathUtil.hpp"

BOOL Vertex::operator==(const Vertex& other) const {
	return Common::Foundation::Util::MathUtil::IsEqual(Position, other.Position) &&
		Common::Foundation::Util::MathUtil::IsEqual(Normal, other.Normal) &&
		Common::Foundation::Util::MathUtil::IsEqual(TexCoord, other.TexCoord);
}