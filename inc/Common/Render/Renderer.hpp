#pragma once

#include "Common/Util/HashUtil.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#ifdef _DLLEXPORT
	#ifndef RendererAPI
		#define RendererAPI __declspec(dllexport)
	#endif
#else
	#ifndef RendererAPI
		#define RendererAPI __declspec(dllimport)
	#endif
#endif

namespace Common {
	namespace Debug {
		struct LogFile;
	}

	namespace Foundation {
		namespace Core {
			class WindowsManager;
		}

		namespace Camera {
			class GameCamera;
		}

		namespace Mesh 
		{
			struct Transform;

			class Mesh;
		}
	}

	namespace Render {
		namespace ShadingArgument {
			struct ShadingArgumentSet;
		}

		class Renderer {
		public: // Functions that is called only once
			RendererAPI virtual BOOL Initialize(
				Common::Debug::LogFile* const pLogFile,
				Common::Foundation::Core::WindowsManager* const pWndManager,
				ShadingArgument::ShadingArgumentSet* const pArgSet,
				UINT width, UINT height) = 0;
			RendererAPI virtual void CleanUp() = 0;

		public: // Functions that is called whenever a message is called
			RendererAPI virtual BOOL OnResize(UINT width, UINT height) = 0;

		public: // Functions that is called in every frame
			RendererAPI virtual BOOL Update(FLOAT deltaTime) = 0;
			RendererAPI virtual BOOL Draw() = 0;

		public:
			RendererAPI virtual BOOL AddMesh(Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Hash& hash) = 0;
			RendererAPI virtual BOOL UpdateMeshTransform(Common::Foundation::Hash hash, Common::Foundation::Mesh::Transform* const pTransform) = 0;
			RendererAPI virtual void RemoveMesh(Common::Foundation::Hash hash) = 0;

		public:
			__forceinline void SetCamera(Common::Foundation::Camera::GameCamera* const pCamera);

		public: // Shading control functions
			void EnableShadow(BOOL state);
			__forceinline constexpr BOOL IsShadowEnabled() const;

		protected:
			Common::Debug::LogFile* mpLogFile = nullptr;
			Common::Foundation::Core::WindowsManager* mpWindowsManager = nullptr;
			ShadingArgument::ShadingArgumentSet* mpArgumentSet;

			Common::Foundation::Camera::GameCamera* mpCamera = nullptr;

			BOOL mbRaytracingSupported = FALSE;
			BOOL mbMeshShaderSupported = FALSE;

			BOOL mbShadowEnabled = TRUE;
		};
	}
}

#include "Renderer.inl"