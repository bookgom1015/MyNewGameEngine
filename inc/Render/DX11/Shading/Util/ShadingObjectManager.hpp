#pragma once

#include "Render/DX11/Foundation/ShadingObject.hpp"	

namespace Render::DX11::Shading::Util {
	class ShaderManager;

	class ShadingObjectManager {
	public:
		ShadingObjectManager();
		virtual ~ShadingObjectManager();

	public:
		BOOL Initialize(Common::Debug::LogFile* const pLogFile);
		void CleanUp();

	public:
		BOOL CompileShaders(Shading::Util::ShaderManager* const pShaderManager, LPCWSTR baseDir);
		BOOL BuildPipelineStates();
		BOOL OnResize(UINT width, UINT height);
		BOOL Update();

	public:
		template <typename T>
			requires std::is_base_of_v<Foundation::ShadingObject, T>
		__forceinline void Add();

		template <typename T>
			requires std::is_base_of_v<Foundation::ShadingObject, T>
		__forceinline T* Get();

	private:
		Common::Debug::LogFile* mpLogFile{};

		std::vector<std::unique_ptr<Foundation::ShadingObject>> mShadingObjects{};
		std::unordered_map<std::type_index, Foundation::ShadingObject*> mShadingObjectRefs{};
	};
}

#include "ShadingObjectManager.inl"