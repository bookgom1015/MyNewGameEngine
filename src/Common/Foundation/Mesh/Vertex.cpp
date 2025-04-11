#include "Common/Foundation/Mesh/Vertex.h"
#include "Common/Util/MathUtil.hpp"

#include <vector>

using namespace Common::Foundation::Mesh;

namespace {
	const std::vector<D3D12_INPUT_ELEMENT_DESC> gInputLayout = {
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,			0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,			0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,			0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	const D3D12_INPUT_LAYOUT_DESC gInputLayoutDesc = { gInputLayout.data(), static_cast<UINT>(gInputLayout.size()) };
}

BOOL Vertex::operator==(const Vertex& other) const {
	return Common::Util::MathUtil::IsEqual(Position, other.Position) &&
		Common::Util::MathUtil::IsEqual(Normal, other.Normal) &&
		Common::Util::MathUtil::IsEqual(TexCoord, other.TexCoord);
}

D3D12_INPUT_LAYOUT_DESC Vertex::InputLayoutDesc() {
	return gInputLayoutDesc;
}