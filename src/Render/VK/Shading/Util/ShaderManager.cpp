#include "Render/VK/Shading/Util/ShaderManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/StringUtil.hpp"
#include "Common/Util/TaskQueue.hpp"
#include "Render/VK/Foundation/Core/Device.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>

using namespace Render::VK::Shading::Util;

ShaderManager::VkShaderInfo::VkShaderInfo(LPCSTR fileName, LPCSTR entryPoint) 
	: FileName(fileName), EntryPoint(entryPoint) {}

BOOL ShaderManager::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		Foundation::Core::Device* const pDevice, 
		UINT numThreads) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;
	mThreadCount = numThreads;

	mCompileMutexes.resize(numThreads);
	mStagingShaders.resize(numThreads);

	for (UINT i = 0; i < numThreads; ++i) 
		mCompileMutexes[i] = std::make_unique<std::mutex>();

	return TRUE;
}

void ShaderManager::CleanUp() {
	for (auto& shader : mShaders) {
		vkDestroyShaderModule(mpDevice->LogicalDevice(), shader.second, nullptr);
	}
}

BOOL ShaderManager::AddShader(
		const VkShaderInfo& shaderInfo,
		Common::Foundation::Hash& hash) {
	std::hash<VkShaderInfo> hasher;
	hash = hasher(shaderInfo);

	if (mShaders.find(hash) != mShaders.end())
		ReturnFalse(mpLogFile, L"The shader is already existed or hash collision occured");

	mShaderInfos[hash] = shaderInfo;

	return TRUE;
}

BOOL ShaderManager::CompileShaders(LPCWSTR baseDir) {
	Common::Util::TaskQueue taskQueue;

	for (const auto& shaderInfo : mShaderInfos)
		taskQueue.AddTask([&]() -> BOOL { 
		return CompileShader(shaderInfo.first, baseDir); });

	CheckReturn(mpLogFile, taskQueue.ExecuteTasks(mpLogFile, mThreadCount));

	CheckReturn(mpLogFile, CommitShaders());

	return TRUE;
}

BOOL ShaderManager::ReadFile(
		Common::Foundation::Hash hash,
		LPCWSTR baseDir,
		std::vector<CHAR>& data) {
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

	data.resize(fileSize);

	fin.seekg(0);
	fin.read(data.data(), fileSize);
	fin.close();

	return TRUE;
}

BOOL ShaderManager::CompileShader(Common::Foundation::Hash hash, LPCWSTR baseDir) {
	std::vector<CHAR> data;
	CheckReturn(mpLogFile, ReadFile(hash, baseDir, data));

	{
		UINT tid = 0;
		std::unique_lock<std::mutex> compileLock;
		for (; tid < mThreadCount; ++tid) {
			compileLock = std::unique_lock<std::mutex>(*mCompileMutexes[tid], std::defer_lock);
			if (compileLock.try_lock()) break;
		}
		if (tid == mThreadCount) ReturnFalse(mpLogFile, L"Failed to acquire shader compile mutex");

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.size();
		createInfo.pCode = reinterpret_cast<const UINT*>(data.data());

		VkShaderModule module;
		if (vkCreateShaderModule(mpDevice->LogicalDevice(), &createInfo, nullptr, &module) != VK_SUCCESS)
			ReturnFalse(mpLogFile, L"Failed to create shader module");

		mStagingShaders[tid].emplace_back(hash, std::move(module));
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