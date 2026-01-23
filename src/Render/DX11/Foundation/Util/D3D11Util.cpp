#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/Util/D3D11Util.hpp"
#include "Common/Foundation/Mesh/Vertex.h"

namespace {
	const D3D11_INPUT_ELEMENT_DESC gInputLayout[3] = {
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0,	offsetof(Common::Foundation::Mesh::Vertex, Position),	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0,	offsetof(Common::Foundation::Mesh::Vertex, Normal),		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0,	offsetof(Common::Foundation::Mesh::Vertex, TexCoord),	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
}

using namespace Render::DX11::Foundation::Util;

void D3D11Util::GetDefaultInputLayout(
		const D3D11_INPUT_ELEMENT_DESC*& outDesc, UINT& outCount) {
	outDesc = gInputLayout;
	outCount = 3;
}