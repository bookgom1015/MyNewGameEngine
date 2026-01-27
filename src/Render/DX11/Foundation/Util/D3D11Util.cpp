#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/Util/D3D11Util.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Vertex.h"
#include "Render/DX11/Foundation/Core/Device.hpp"

namespace {
	const D3D11_INPUT_ELEMENT_DESC gInputLayout[3] = {
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0,	offsetof(Common::Foundation::Mesh::Vertex, Position),	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0,	offsetof(Common::Foundation::Mesh::Vertex, Normal),		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0,	offsetof(Common::Foundation::Mesh::Vertex, TexCoord),	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
}

using namespace Render::DX11::Foundation::Util;
using namespace DirectX;

Common::Debug::LogFile* D3D11Util::msLogFile{};

BOOL D3D11Util::Initialize(Common::Debug::LogFile* const pLogFile) {
    msLogFile = pLogFile;

    return TRUE;
}

void D3D11Util::GetDefaultInputLayout(
		const D3D11_INPUT_ELEMENT_DESC*& outDesc, UINT& outCount) {
	outDesc = gInputLayout;
	outCount = 3;
}

BOOL D3D11Util::LoadDDStoTexture(
        LPCWSTR filePath,
        Core::Device* const pDevice,
        Microsoft::WRL::ComPtr<ID3D11Texture2D>& tex) {
    ScratchImage image{};
    TexMetadata metadata{};

    CheckHRESULT(msLogFile, LoadFromDDSFile(
        filePath,
        DDS_FLAGS_NONE,
        &metadata,
        image));

    CheckHRESULT(msLogFile, CreateTextureEx(
        pDevice->mDevice.Get(),
        image.GetImages(),
        image.GetImageCount(),
        metadata,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
        CREATETEX_DEFAULT,
        reinterpret_cast<ID3D11Resource**>(tex.GetAddressOf())));

	return TRUE;
}