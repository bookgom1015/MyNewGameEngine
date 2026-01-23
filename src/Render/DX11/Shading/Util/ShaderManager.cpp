#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/StringUtil.hpp"
#include "Common/Util/TaskQueue.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"

using namespace Render::DX11::Shading::Util;
using namespace Microsoft::WRL;

ShaderManager::ShaderManager() {}

ShaderManager::~ShaderManager() { CleanUp(); }

BOOL ShaderManager::Initialize(Common::Debug::LogFile* const pLogFile, LPCWSTR baseDir) {
	mpLogFile = pLogFile;
	mpBaseDir = baseDir;

	return TRUE;
}

void ShaderManager::CleanUp() {
	if (mbCleanedUp) return;

	for (auto& shader : mShaders) 
		shader.second.Reset();
	mShaders.clear();

	mbCleanedUp = TRUE;
}

BOOL ShaderManager::CompileVertexShader(
		Foundation::Core::Device* const pDevice,
		const D3D11ShaderInfo& shaderInfo,
		Common::Foundation::Hash& hash,
		ID3D11VertexShader** ppShader) {
	std::hash<D3D11ShaderInfo> hasher{};
	hash = hasher(shaderInfo);

	auto& blob = mShaders[hash];

	CheckReturn(mpLogFile, CompileShader(shaderInfo, &blob));

	CheckReturn(mpLogFile, pDevice->CreateVertexShader(
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		shaderInfo.ClassLinkage,
		ppShader));

	return TRUE;
}

BOOL ShaderManager::CompileGeometryShader(
		Foundation::Core::Device* const pDevice,
		const D3D11ShaderInfo& shaderInfo,
		Common::Foundation::Hash& hash,
		ID3D11GeometryShader** ppShader) {
	std::hash<D3D11ShaderInfo> hasher{};
	hash = hasher(shaderInfo);

	auto& blob = mShaders[hash];

	CheckReturn(mpLogFile, CompileShader(shaderInfo, &blob));

	CheckReturn(mpLogFile, pDevice->CreateGeometryShader(
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		shaderInfo.ClassLinkage,
		ppShader));

	return TRUE;
}
BOOL ShaderManager::CompilePixelShader(
		Foundation::Core::Device* const pDevice,
		const D3D11ShaderInfo& shaderInfo,
		Common::Foundation::Hash& hash,
		ID3D11PixelShader** ppShader) {
	std::hash<D3D11ShaderInfo> hasher{};
	hash = hasher(shaderInfo);

	auto& blob = mShaders[hash];

	CheckReturn(mpLogFile, CompileShader(shaderInfo, &blob));

	CheckReturn(mpLogFile, pDevice->CreatePixelShader(
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		shaderInfo.ClassLinkage,
		ppShader));

	return TRUE;
}
BOOL ShaderManager::CompileComputeShader(
		Foundation::Core::Device* const pDevice,
		const D3D11ShaderInfo& shaderInfo,
		Common::Foundation::Hash& hash,
		ID3D11ComputeShader** ppShader) {
	std::hash<D3D11ShaderInfo> hasher{};
	hash = hasher(shaderInfo);

	auto& blob = mShaders[hash];

	CheckReturn(mpLogFile, CompileShader(shaderInfo, &blob));

	CheckReturn(mpLogFile, pDevice->CreateComputeShader(
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		shaderInfo.ClassLinkage,
		ppShader));

	return TRUE;
}

BOOL ShaderManager::CompileShader(
		const D3D11ShaderInfo& shaderInfo,
		ID3DBlob** ppShaderBlob) {
#if defined(_DEBUG)  
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	HRESULT hr = S_OK;

	std::wstring filePath(mpBaseDir);
	filePath.append(shaderInfo.FileName);

	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(
		filePath.c_str(),
		shaderInfo.Defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		shaderInfo.EntryPoint,
		shaderInfo.Target,
		compileFlags,
		0,
		ppShaderBlob,
		&errors);

	std::wstringstream wsstream;
	if (errors != nullptr)
		wsstream << reinterpret_cast<CHAR*>(errors->GetBufferPointer());

	if (FAILED(hr) && errors != nullptr)
		ReturnFalse(mpLogFile, wsstream.str().c_str());

	return TRUE;
}