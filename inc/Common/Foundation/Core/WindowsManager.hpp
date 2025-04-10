#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <Windows.h>

namespace Common {
	namespace Debug {
		struct LogFile;
	}

	namespace Input {
		class InputProcessor;
	}

	namespace Foundation::Core {
		class WindowsManager {
		public:
			struct SelectDialogInitData {
				HINSTANCE InstanceHandle;
				std::vector<std::wstring> Items;
				UINT SelectedItemIndex;
			};

			using OnResizeFunc = std::function<void(UINT, UINT)>;
			using SelectDialogInitDataPtr = std::unique_ptr<SelectDialogInitData>;

		public:
			static SelectDialogInitDataPtr MakeSelectDialogInitData();

		public:
			WindowsManager();
			virtual ~WindowsManager() = default;

		public:
			__forceinline constexpr BOOL AppPaused() const;
			__forceinline constexpr BOOL Minimized() const;
			__forceinline constexpr BOOL Maximized() const;
			__forceinline constexpr BOOL Resizing() const;
			__forceinline constexpr BOOL Fullscreen() const;
			__forceinline constexpr BOOL Destroyed() const;

			__forceinline constexpr HINSTANCE InstanceHandle() const;
			__forceinline constexpr HWND MainWindowHandle() const;

			__forceinline FLOAT AspectRatio() const;

		public:
			static INT ToDLUsWidth(INT width);
			static INT ToDLUsHeight(INT height);

		public:
			LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		public:
			BOOL Initialize(Common::Debug::LogFile* const pLogFile, HINSTANCE hInstance, UINT wndWidth, UINT wndHeight);

			void RegisterOnResizeFunc(const OnResizeFunc& func);
			void DestroyWindow();

			void RegisterInputProcessor(Common::Input::InputProcessor* const pInputProc);

		public:
			BOOL SelectDialog(SelectDialogInitData* const pInitData);

		private:
			BOOL InitMainWnd(UINT wndWidth, UINT wndHeight);
			void OnResize(UINT width, UINT height);

		public:
			static WindowsManager* sWindowsManager;

		private:
			static INT DialogBaseUnits;

		private:
			Common::Debug::LogFile* mpLogFile = nullptr;

			Common::Input::InputProcessor* mpInputProcessor = nullptr;
			BOOL mbInputProcessorRegistered = FALSE;

			HINSTANCE mhInst = NULL;
			HWND mhMainWnd = NULL;

			BOOL mbAppPaused = FALSE;		
			BOOL mbMinimized = FALSE;		
			BOOL mbMaximized = FALSE;		
			BOOL mbResizing = FALSE;		
			BOOL mbFullscreenState = FALSE;	
			BOOL mbDestroyed = FALSE;

			OnResizeFunc mOnResizeFunc;
			BOOL mbRegisteredOnResizeFunc = FALSE;

			UINT mMainWndWidth = 0;
			UINT mMainWndHeight = 0;
		};
	}
}

#include "Common/Foundation/Core/WindowsManager.inl"