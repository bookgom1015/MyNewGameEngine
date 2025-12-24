#include "Render/VK/Shading/Util/ShaderManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/StringUtil.hpp"
#include "Common/Util/TaskQueue.hpp"
#include "Render/VK/Foundation/Core/Device.hpp"

#include <fstream>
#include <filesystem>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Standalone/DirStackFileIncluder.h>

using namespace Render::VK::Shading::Util;

namespace {
	EShLanguage ConverToEShLanguage(ShaderManager::ShaderStage stage) {
		switch (stage) {
		case ShaderManager::ShaderStage::E_VertexStage:
			return EShLangVertex;
		case ShaderManager::ShaderStage::E_FragmentStage:
			return EShLangFragment;
		default:
			return EShLangCount;
		}
	}

    TBuiltInResource DefaultResource() {
        TBuiltInResource Resources;

        Resources.maxLights = 32;
        Resources.maxClipPlanes = 6;
        Resources.maxTextureUnits = 32;
        Resources.maxTextureCoords = 32;
        Resources.maxVertexAttribs = 64;
        Resources.maxVertexUniformComponents = 4096;
        Resources.maxVaryingFloats = 64;
        Resources.maxVertexTextureImageUnits = 32;
        Resources.maxCombinedTextureImageUnits = 80;
        Resources.maxTextureImageUnits = 32;
        Resources.maxFragmentUniformComponents = 4096;
        Resources.maxDrawBuffers = 32;
        Resources.maxVertexUniformVectors = 128;
        Resources.maxVaryingVectors = 8;
        Resources.maxFragmentUniformVectors = 16;
        Resources.maxVertexOutputVectors = 16;
        Resources.maxFragmentInputVectors = 15;
        Resources.minProgramTexelOffset = -8;
        Resources.maxProgramTexelOffset = 7;
        Resources.maxClipDistances = 8;
        Resources.maxComputeWorkGroupCountX = 65535;
        Resources.maxComputeWorkGroupCountY = 65535;
        Resources.maxComputeWorkGroupCountZ = 65535;
        Resources.maxComputeWorkGroupSizeX = 1024;
        Resources.maxComputeWorkGroupSizeY = 1024;
        Resources.maxComputeWorkGroupSizeZ = 64;
        Resources.maxComputeUniformComponents = 1024;
        Resources.maxComputeTextureImageUnits = 16;
        Resources.maxComputeImageUniforms = 8;
        Resources.maxComputeAtomicCounters = 8;
        Resources.maxComputeAtomicCounterBuffers = 1;
        Resources.maxVaryingComponents = 60;
        Resources.maxVertexOutputComponents = 64;
        Resources.maxGeometryInputComponents = 64;
        Resources.maxGeometryOutputComponents = 128;
        Resources.maxFragmentInputComponents = 128;
        Resources.maxImageUnits = 8;
        Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
        Resources.maxCombinedShaderOutputResources = 8;
        Resources.maxImageSamples = 0;
        Resources.maxVertexImageUniforms = 0;
        Resources.maxTessControlImageUniforms = 0;
        Resources.maxTessEvaluationImageUniforms = 0;
        Resources.maxGeometryImageUniforms = 0;
        Resources.maxFragmentImageUniforms = 8;
        Resources.maxCombinedImageUniforms = 8;
        Resources.maxGeometryTextureImageUnits = 16;
        Resources.maxGeometryOutputVertices = 256;
        Resources.maxGeometryTotalOutputComponents = 1024;
        Resources.maxGeometryUniformComponents = 1024;
        Resources.maxGeometryVaryingComponents = 64;
        Resources.maxTessControlInputComponents = 128;
        Resources.maxTessControlOutputComponents = 128;
        Resources.maxTessControlTextureImageUnits = 16;
        Resources.maxTessControlUniformComponents = 1024;
        Resources.maxTessControlTotalOutputComponents = 4096;
        Resources.maxTessEvaluationInputComponents = 128;
        Resources.maxTessEvaluationOutputComponents = 128;
        Resources.maxTessEvaluationTextureImageUnits = 16;
        Resources.maxTessEvaluationUniformComponents = 1024;
        Resources.maxTessPatchComponents = 120;
        Resources.maxPatchVertices = 32;
        Resources.maxTessGenLevel = 64;
        Resources.maxViewports = 16;
        Resources.maxVertexAtomicCounters = 0;
        Resources.maxTessControlAtomicCounters = 0;
        Resources.maxTessEvaluationAtomicCounters = 0;
        Resources.maxGeometryAtomicCounters = 0;
        Resources.maxFragmentAtomicCounters = 8;
        Resources.maxCombinedAtomicCounters = 8;
        Resources.maxAtomicCounterBindings = 1;
        Resources.maxVertexAtomicCounterBuffers = 0;
        Resources.maxTessControlAtomicCounterBuffers = 0;
        Resources.maxTessEvaluationAtomicCounterBuffers = 0;
        Resources.maxGeometryAtomicCounterBuffers = 0;
        Resources.maxFragmentAtomicCounterBuffers = 1;
        Resources.maxCombinedAtomicCounterBuffers = 1;
        Resources.maxAtomicCounterBufferSize = 16384;
        Resources.maxTransformFeedbackBuffers = 4;
        Resources.maxTransformFeedbackInterleavedComponents = 64;
        Resources.maxCullDistances = 8;
        Resources.maxCombinedClipAndCullDistances = 8;
        Resources.maxSamples = 4;
        Resources.maxMeshOutputVerticesNV = 256;
        Resources.maxMeshOutputPrimitivesNV = 512;
        Resources.maxMeshWorkGroupSizeX_NV = 32;
        Resources.maxMeshWorkGroupSizeY_NV = 1;
        Resources.maxMeshWorkGroupSizeZ_NV = 1;
        Resources.maxTaskWorkGroupSizeX_NV = 32;
        Resources.maxTaskWorkGroupSizeY_NV = 1;
        Resources.maxTaskWorkGroupSizeZ_NV = 1;
        Resources.maxMeshViewCountNV = 4;

        Resources.limits.nonInductiveForLoops = 1;
        Resources.limits.whileLoops = 1;
        Resources.limits.doWhileLoops = 1;
        Resources.limits.generalUniformIndexing = 1;
        Resources.limits.generalAttributeMatrixVectorIndexing = 1;
        Resources.limits.generalVaryingIndexing = 1;
        Resources.limits.generalSamplerIndexing = 1;
        Resources.limits.generalVariableIndexing = 1;
        Resources.limits.generalConstantMatrixVectorIndexing = 1;

        return Resources;
    }
}

ShaderManager::~ShaderManager() {
	if (mbInitialized) CleanUp();
}

BOOL ShaderManager::Initialize(Common::Debug::LogFile* const pLogFile, UINT numThreads) {
	mpLogFile = pLogFile;
	mThreadCount = numThreads;

    glslang::InitializeProcess();

	mbInitialized = TRUE;

	return TRUE;
}

void ShaderManager::CleanUp() {
	glslang::FinalizeProcess();	
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

BOOL ShaderManager::ConvertGLSLtoSPIRV(
		const VkShaderInfo& shaderInfo, 
		LPCSTR pCodeData,
        const std::vector<LPCSTR>& includeDirs,
		std::vector<UINT>& spirv) {
	auto stage = ConverToEShLanguage(shaderInfo.Stage);
    
    glslang::TShader shader(stage);
    shader.setStrings(&pCodeData, 1);
    shader.setEntryPoint(shaderInfo.EntryPoint);
    shader.setSourceEntryPoint(shaderInfo.EntryPoint);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    const int DefaultVersion = 450;
    EProfile profile = ECoreProfile;

    // Message flag(Error/Warning/Spiral-Rule)
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    // Resource limit(using default)
    TBuiltInResource resources = DefaultResource();

    // Supporting #include
    DirStackFileIncluder includer;
    for (auto& dir : includeDirs) 
        includer.pushExternalLocalDirectory(dir);

    // Parsing
    // Set last argument as nullptr if app not use includer
    if (!shader.parse(
			&resources,
			DefaultVersion,
			profile,
			FALSE,   // forceDefaultVersionAndProfile
			FALSE,   // forwardCompatible
			messages,
            includer)) {
		std::wstringstream wsstream;
		wsstream << L"GLSL parse failed: " << shaderInfo.FileName << std::endl;
		wsstream << shader.getInfoLog() << std::endl;
		wsstream << shader.getInfoDebugLog();
		ReturnFalse(mpLogFile, wsstream.str().c_str());
    }

    // Linking
    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
		std::wstringstream wsstream;
		wsstream << L"GLSL link failed: " << shaderInfo.FileName << std::endl;
		wsstream << program.getInfoLog() << std::endl;
		wsstream << shader.getInfoDebugLog();
		ReturnFalse(mpLogFile, wsstream.str().c_str());
    }

    // Conver to SPIR-V
    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = TRUE;
    spvOptions.disableOptimizer = FALSE;
    spvOptions.optimizeSize = FALSE;

    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &spvOptions);

	return TRUE;
}

BOOL ShaderManager::CompileShaders(const Foundation::Core::Device& device, LPCWSTR baseDir) {
	Common::Util::TaskQueue taskQueue;

	for (const auto& shaderInfo : mShaderInfos)
		taskQueue.AddTask([&]() -> BOOL { 
		return CompileShader(device, shaderInfo.first, baseDir); });

	CheckReturn(mpLogFile, taskQueue.ExecuteTasks(mpLogFile, mThreadCount));

	CheckReturn(mpLogFile, CommitShaders());

	return TRUE;
}

BOOL ShaderManager::CompileShader(
		const Foundation::Core::Device& device, 
		Common::Foundation::Hash hash, 
		LPCWSTR baseDir) {
	std::vector<CHAR> data;
	CheckReturn(mpLogFile, ReadFile(hash, baseDir, data));

    std::vector<UINT> spirv;
    CheckReturn(mpLogFile, ConvertGLSLtoSPIRV(
        mShaderInfos[hash], 
        data.data(), 
        {}, 
        spirv));

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
		if (vkCreateShaderModule(device.LogicalDevice(), &createInfo, nullptr, &module) != VK_SUCCESS)
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