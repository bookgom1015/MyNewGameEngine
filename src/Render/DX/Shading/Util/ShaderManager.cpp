#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/StringUtil.hpp"
#include "Common/Util/TaskQueue.hpp"

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

	mUtils.resize(numThreads);
	mCompilers.resize(numThreads);
	mCompileMutexes.resize(numThreads);
	mStagingShaders.resize(numThreads);

	for (UINT i = 0; i < numThreads; ++i) {
		CheckHRESULT(mpLogFile, DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&mUtils[i])));
		CheckHRESULT(mpLogFile, DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mCompilers[i])));
		mCompileMutexes[i] = std::make_unique<std::mutex>();
	}

	return TRUE;
}

BOOL ShaderManager::AddShader(const D3D12ShaderInfo& shaderInfo, Common::Foundation::Hash& hash) {
	std::hash<D3D12ShaderInfo> hasher;
	hash = hasher(shaderInfo);

	if (mShaders.find(hash) != mShaders.end())
		ReturnFalse(mpLogFile, L"The shader is already existed or hash collision occured");

	mShaderInfos[hash] = shaderInfo;

	return TRUE;
}

BOOL ShaderManager::CompileShaders(LPCWSTR baseDir) {
	Common::Util::TaskQueue taskQueue;

	for (const auto& shaderInfo : mShaderInfos) 
		taskQueue.AddTask([&]() -> BOOL { return CompileShader(shaderInfo.first, baseDir); });

	CheckReturn(mpLogFile, taskQueue.ExecuteTasks(mpLogFile, mThreadCount));

	CheckReturn(mpLogFile, CommitShaders());

	return TRUE;
}

BOOL ShaderManager::CompileShader(Common::Foundation::Hash hash, LPCWSTR baseDir) {
	const auto shaderInfo = mShaderInfos[hash];

	std::wstringstream wsstream;
	wsstream << baseDir << shaderInfo.FileName;
	std::wstring filePath = wsstream.str();

	std::ifstream fin(filePath.c_str(), std::ios::ate | std::ios::binary);
	if (!fin.is_open()) { 
		std::wstring msg(L"Failed to open shader file: ");
		msg.append(filePath.c_str());
		ReturnFalse(mpLogFile, msg);
	}
	
	size_t fileSize = static_cast<size_t>(fin.tellg());
	if (fileSize == 0) ReturnFalse(mpLogFile, "Shader file is empty");

	std::vector<CHAR> data(fileSize);
	
	fin.seekg(0);
	fin.read(data.data(), fileSize);
	fin.close();

	ComPtr<IDxcResult> result;
	{
		UINT tid = 0;
		std::unique_lock<std::mutex> compileLock;
		for (; tid < mThreadCount; ++tid) {
			compileLock = std::unique_lock<std::mutex>(*mCompileMutexes[tid], std::defer_lock);
			if (compileLock.try_lock()) break;
		}
		if (tid == mThreadCount) ReturnFalse(mpLogFile, L"Failed to acquire shader compile mutex");

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

#ifdef _DEBUG
		CheckReturn(mpLogFile, BuildPdb(result.Get(), filePath.c_str()));
#endif

		ComPtr<IDxcBlob> newShader;
		CheckHRESULT(mpLogFile, result->GetResult(&newShader));

		mStagingShaders[tid].emplace_back(hash, std::move(newShader));
	}

	return TRUE;
}

BOOL ShaderManager::CommitShaders() {
	for (UINT i = 0; i < mThreadCount; ++i) {
		auto& shaders = mStagingShaders[i];

		for (auto& shader : shaders) 
			mShaders[shader.first] = shader.second;
	}

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
	auto delimIdx = fileNameWithExtW.rfind(L'\\');

	{
		std::wstring filePathW = fileNameWithExtW.substr(0, delimIdx);
		filePathW.append(L"\\PDB");

		std::filesystem::path debugDir(filePathW);
		if (!std::filesystem::exists(debugDir)) std::filesystem::create_directory(debugDir);
	}

	fileNameWithExtW.insert(delimIdx, L"\\PDB");

	const auto& fileNameA = Common::Util::StringUtil::WStringToString(fileNameWithExtW);

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