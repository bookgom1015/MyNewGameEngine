#pragma once

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

	namespace Render {
		class Renderer {
		public: // Functions that is called only once
			RendererAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) = 0;
			RendererAPI virtual void CleanUp() = 0;

		public: // Functions that is called whenever a message is called
			RendererAPI virtual BOOL OnResize(UINT width, UINT height) = 0;

		public: // Functions that is called in every frame
			RendererAPI virtual BOOL Update(FLOAT deltaTime) = 0;
			RendererAPI virtual BOOL Draw() = 0;

		public:
			RendererAPI virtual BOOL AddMesh() = 0;
			RendererAPI virtual BOOL RemoveMesh() = 0;

		public: // Shading control functions
			void EnableShadow(BOOL state);
			__forceinline constexpr BOOL IsShadowEnabled() const;

		protected:
			BOOL mbShadowEnabled = TRUE;
		};
	}
}

#include "Renderer.inl"