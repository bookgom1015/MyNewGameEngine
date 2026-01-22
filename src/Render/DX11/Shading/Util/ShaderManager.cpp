#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/StringUtil.hpp"
#include "Common/Util/TaskQueue.hpp"

using namespace Render::DX11::Shading::Util;
using namespace Microsoft::WRL;

ShaderManager::ShaderManager() {}

ShaderManager::~ShaderManager() { CleanUp(); }

BOOL ShaderManager::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void ShaderManager::CleanUp() {
	mShaders.clear();
}

BOOL ShaderManager::AddShader(
		const D3D11ShaderInfo& shaderInfo, Common::Foundation::Hash& hash) {


	return TRUE;
}

BOOL ShaderManager::CompileShaders(LPCWSTR baseDir) {


	return TRUE;
}

BOOL ShaderManager::CompileShader(
		LPCWSTR filePath,
		D3D_SHADER_MACRO* defines,
		LPCSTR entryPoint,
		LPCSTR target,
		ID3DBlob** ppShaderByteCode) {
#if defined(_DEBUG)  
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filePath, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint, target, compileFlags, 0, ppShaderByteCode, &errors);

	std::wstringstream wsstream;
	if (errors != nullptr)
		wsstream << reinterpret_cast<CHAR*>(errors->GetBufferPointer());

	if (FAILED(hr) && errors != nullptr)
		ReturnFalse(mpLogFile, wsstream.str().c_str());

	return TRUE;
}