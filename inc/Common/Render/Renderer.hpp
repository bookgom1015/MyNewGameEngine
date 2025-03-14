#pragma once

#include <Windows.h>

#ifdef _DLLEXPORT
	#define RendererAPI __declspec(dllexport)
#else
	#define RendererAPI __declspec(dllimport)
#endif

#define InlineConstExpr __forceinline constexpr

namespace Common::Debug {
	struct LogFile;
}

namespace Render {
	class Renderer {
	public: // Functions that called only once
		RendererAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) = 0;
		RendererAPI virtual void CleanUp() = 0;

	public: // Functions that called whenever a message is called
		RendererAPI virtual BOOL OnResize(UINT width, UINT height) = 0;

	public: // Functions that called in every frame
		RendererAPI virtual BOOL Update(FLOAT deltaTime) = 0;
		RendererAPI virtual BOOL Draw() = 0;

	public: // Shading control functions
		void EnableShadow(BOOL state);
		InlineConstExpr BOOL IsShadowEnabled() const;

	protected:
		BOOL mbShadowEnabled = TRUE;
	};
}

#include "Renderer.inl"