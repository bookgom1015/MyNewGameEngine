#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX::Shading::Util;

namespace {
	const CD3DX12_STATIC_SAMPLER_DESC gPointWrap{
		0,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP		// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gPointClamp{
		1,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP	// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gLinearWrap{
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP		// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gLinearClamp{
		3,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP	// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gAnisotropicWrap{
		4,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,			// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressW
		0.f,								// mipLODBias
		8									// maxAnisotropy
	};

	const CD3DX12_STATIC_SAMPLER_DESC gAnisotropicClamp{
		5,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,			// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressW
		0.f,								// mipLODBias
		8									// maxAnisotropy
	};

	const CD3DX12_STATIC_SAMPLER_DESC gAnisotropicBorder{
		6,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,			// filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressW
		0.f,								// mipLODBias
		8,									// maxAnisotropy
		D3D12_COMPARISON_FUNC_ALWAYS,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK
	};

	const CD3DX12_STATIC_SAMPLER_DESC gDepthMap{
		7,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressW
		0.f,								// mipLODBias
		0,									// maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE
	};

	const CD3DX12_STATIC_SAMPLER_DESC gShadow{
		8,													// shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,					// addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,					// addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,					// addressW
		0.f,												// mipLODBias
		16,													// maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE
	};

	const CD3DX12_STATIC_SAMPLER_DESC gPointMirror{
		9,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR	// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gLinearMirror{
		10,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR	// addressW
	};
}

StaticSamplers SamplerUtil::msStaticSamplers = {
		gPointWrap, 
		gPointClamp,
		gLinearWrap, 
		gLinearClamp,
		gAnisotropicWrap, 
		gAnisotropicClamp, 
		gAnisotropicBorder,
		gDepthMap,
		gShadow,
		gPointMirror, 
		gLinearMirror };

const D3D12_STATIC_SAMPLER_DESC* SamplerUtil::GetStaticSamplers() noexcept {
	return msStaticSamplers.data();
}