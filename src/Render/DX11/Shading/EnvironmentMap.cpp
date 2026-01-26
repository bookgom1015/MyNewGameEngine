#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/EnvironmentMap.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Vertex.h"
#include "Render/DX11/Foundation/RenderItem.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX11/Foundation/Resource/MaterialData.hpp"
#include "Render/DX11/Foundation/Util/D3D11Util.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"

using namespace Render::DX11::Shading;

EnvironmentMap::InitDataPtr EnvironmentMap::MakeInitData() {
	return std::unique_ptr<EnvironmentMapClass::InitData>(new EnvironmentMapClass::InitData());
}

EnvironmentMap::EnvironmentMapClass::EnvironmentMapClass() {}

EnvironmentMap::EnvironmentMapClass::~EnvironmentMapClass() { CleanUp(); }

BOOL EnvironmentMap::EnvironmentMapClass::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

void EnvironmentMap::EnvironmentMapClass::CleanUp() {
	if (mbCleanedUp) return;

	mInputLayout.Reset();
	mDrawSkySphereVS.Reset();
	mDrawSkySpherePS.Reset();

	mRasterizerState.Reset();
	mDepthStencilState.Reset();
	mBlendState.Reset();

	mbCleanedUp = TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CompileShaders() {
	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildPipelineStates() {
	D3D11_RASTERIZER_DESC rs{};
	rs.FillMode = D3D11_FILL_SOLID;
	rs.CullMode = D3D11_CULL_BACK;
	rs.FrontCounterClockwise = FALSE;
	rs.DepthClipEnable = TRUE;
	CheckReturn(mpLogFile, mInitData.Device->CreateRasterizerState(&rs, &mRasterizerState));

	D3D11_DEPTH_STENCIL_DESC ds{};
	ds.DepthEnable = TRUE;
	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	ds.DepthFunc = D3D11_COMPARISON_LESS;
	ds.StencilEnable = FALSE;
	CheckReturn(mpLogFile, mInitData.Device->CreateDepthStencilState(&ds, &mDepthStencilState));

	D3D11_BLEND_DESC bd{};
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;
	bd.RenderTarget[0].BlendEnable = FALSE;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	CheckReturn(mpLogFile, mInitData.Device->CreateBlendState(&bd, &mBlendState));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildResources() {
	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildDescriptors() {
	return TRUE;
}