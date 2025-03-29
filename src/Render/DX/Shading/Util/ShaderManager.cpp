#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Util/StringUtil.hpp"
#include "Common/Foundation/Util/TaskQueue.hpp"

#include <fstream>
#include <filesystem>

using namespace Render::DX::Shading::Util;
using namespace Microsoft::WRL;

ShaderManager::D3D12ShaderInfo::D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile) {
	FileName = fileName;
	EntryPoint = entryPoint;
	TargetProfile = profile;
}

ShaderManager::D3D12ShaderInfo::D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile, DxcDefine* defines, UINT32 defCount) {
	FileName = fileName;
	EntryPoint = entryPoint;
	TargetProfile = profile;
	Defines = defines;
	DefineCount = defCount;
}

BOOL ShaderManager::Initialize(Common::Debug::LogFile* const pLogFile, UINT numThreads) {
	mpLogFile = pLogFile;
	mThreadCount = numThreads;

	mUtils.resize(mThreadCount);
	mCompilers.resize(mThreadCount);
	mCompileMutexes.resize(mThreadCount);

	for (UINT i = 0; i < mThreadCount; ++i) {
		CheckHRESULT(mpLogFile, DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&mUtils[i])));
		CheckHRESULT(mpLogFile, DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mCompilers[i])));
		mCompileMutexes[i] = std::make_unique<std::mutex>();
	}

	return TRUE;
}

void ShaderManager::AddShader(const D3D12ShaderInfo& shaderInfo, Common::Foundation::Hash& hash) {
	std::hash<D3D12ShaderInfo> hasher;
	hash = hasher(shaderInfo);

	mShaderInfos[hash] = shaderInfo;
}

BOOL ShaderManager::CompileShaders() {
	Common::Foundation::Util::TaskQueue taskQueue;

	for (const auto& shaderInfo : mShaderInfos) 
		taskQueue.AddTask([&]() -> BOOL { return CompileShader(shaderInfo.first); });

	CheckReturn(mpLogFile, taskQueue.ExecuteTasks(mpLogFile, mThreadCount));

	return TRUE;
}

BOOL ShaderManager::CompileShader(Common::Foundation::Hash hash) {
	const auto shaderInfo = mShaderInfos[hash];

	std::ifstream fin(shaderInfo.FileName, std::ios::ate | std::ios::binary);
	if (!fin.is_open()) { 
		std::wstring msg(L"Failed to open shader file: ");
		msg.append(shaderInfo.FileName);
		ReturnFalse(mpLogFile, msg);
	}
	
	size_t fileSize = static_cast<size_t>(fin.tellg());
	std::vector<CHAR> data(fileSize);
	
	fin.seekg(0);
	fin.read(data.data(), fileSize);
	fin.close();

	IDxcResult* result;
	{
		std::unique_lock<std::mutex> compileLock;
		UINT tid = 0;
		for (; tid < mThreadCount; ++tid) {
			compileLock = std::unique_lock<std::mutex>(*mCompileMutexes[tid], std::defer_lock);
			if (compileLock.try_lock()) break;
		}

		const auto& utils = mUtils[tid];
		const auto& compiler = mCompilers[tid];

		ComPtr<IDxcBlobEncoding> shaderText;
		CheckHRESULT(mpLogFile, utils->CreateBlob(data.data(), static_cast<UINT32>(fileSize), 0, &shaderText));

		ComPtr<IDxcIncludeHandler> includeHandler;
		CheckHRESULT(mpLogFile, utils->CreateDefaultIncludeHandler(&includeHandler));

		DxcBuffer sourceBuffer;
		sourceBuffer.Ptr = shaderText->GetBufferPointer();
		sourceBuffer.Size = shaderText->GetBufferSize();
		sourceBuffer.Encoding = 0;

		std::vector<LPCWSTR> arguments;

#ifdef _DEBUG
		arguments.push_back(L"-Qembed_debug");
		arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); // -WX
		arguments.push_back(DXC_ARG_DEBUG); // -Zi
#endif

		ComPtr<IDxcCompilerArgs> compilerArgs;
		CheckHRESULT(mpLogFile, utils->BuildArguments(
			shaderInfo.FileName,
			shaderInfo.EntryPoint,
			shaderInfo.TargetProfile,
			arguments.data(),
			static_cast<UINT32>(arguments.size()),
			shaderInfo.Defines,
			shaderInfo.DefineCount,
			&compilerArgs));

		CheckHRESULT(mpLogFile, compiler->Compile(
			&sourceBuffer,
			compilerArgs->GetArguments(),
			static_cast<UINT32>(compilerArgs->GetCount()),
			includeHandler.Get(),
			IID_PPV_ARGS(&result)));

		HRESULT hr;
		CheckHRESULT(mpLogFile, result->GetStatus(&hr));
		if (FAILED(hr)) {
			IDxcBlobEncoding* error;
			CheckHRESULT(mpLogFile, result->GetErrorBuffer(&error));

			auto bufferSize = error->GetBufferSize();
			std::vector<CHAR> infoLog(bufferSize + 1);
			std::memcpy(infoLog.data(), error->GetBufferPointer(), bufferSize);
			infoLog[bufferSize] = 0;

			std::string errorMsg = "Shader Compiler Error:\n";
			errorMsg.append(infoLog.data());

			std::wstring errorMsgW;
			errorMsgW.assign(errorMsg.begin(), errorMsg.end());

			ReturnFalse(mpLogFile, errorMsgW);
		}
	}

#ifdef _DEBUG
	CheckReturn(mpLogFile, BuildPdb(result, shaderInfo.FileName));
#endif

	CheckHRESULT(mpLogFile, result->GetResult(&mShaders[hash]));

	return TRUE;
}

BOOL ShaderManager::BuildPdb(IDxcResult* const result, LPCWSTR fileName) {
	ComPtr<IDxcBlob> pdbBlob;
	ComPtr<IDxcBlobUtf16> debugDataPath;
	result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbBlob), &debugDataPath);

	std::wstring fileNameW = fileName;
	auto extIdx = fileNameW.rfind(L'.');
	std::wstring fileNameWithExtW = fileNameW.substr(0, extIdx);
	fileNameWithExtW.append(L".pdb");

	std::filesystem::path debugDir(fileNameWithExtW);
	if (!std::filesystem::exists(debugDir)) std::filesystem::create_directory(debugDir);

	const auto& fileNameA = Common::Foundation::Util::StringUtil::WStringToString(fileNameWithExtW);

	std::ofstream fout;
	fout.open(fileNameA, std::ios::beg | std::ios::binary | std::ios::trunc);
	if (!fout.is_open()) {
		std::wstring msg = L"Failed to open file for PDB: ";
		msg.append(fileNameWithExtW);
		ReturnFalse(mpLogFile, msg);
	}

	fout.write(reinterpret_cast<const CHAR*>(pdbBlob->GetBufferPointer()), pdbBlob->GetBufferSize());
	fout.close();

	return TRUE;
}