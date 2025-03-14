#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"

using namespace Render::DX;

extern "C" RendererAPI Render::Renderer* Render::CreateRenderer() {
	return new DxRenderer();
}

extern "C" RendererAPI void Render::DestroyRenderer(Render::Renderer* renderer) {
	delete renderer;
}

DxRenderer::DxRenderer() {

}

DxRenderer::~DxRenderer() {

}

BOOL DxRenderer::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::Initialize(pLogFile, hWnd, width, height));

	return TRUE;
}

void DxRenderer::CleanUp() {
	DxLowRenderer::CleanUp();
}

BOOL DxRenderer::OnResize(UINT width, UINT height) {	
	CheckReturn(mpLogFile, DxLowRenderer::OnResize(width, height));

	return TRUE;
}

BOOL DxRenderer::Update(FLOAT deltaTime) {

	return TRUE;
}

BOOL DxRenderer::Draw() {

	return TRUE;
}
