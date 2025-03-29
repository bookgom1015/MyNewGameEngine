#pragma once


#include "Common/Input/InputProcessor.hpp"

namespace Input {
	extern "C" InputProcessorAPI Common::Input::InputProcessor* CreateInputProcessor();
	extern "C" InputProcessorAPI void DestroyInputProcessor(Common::Input::InputProcessor* const inputProcessor);

	class SimpleInputProcessor : public Common::Input::InputProcessor {
	public:
		SimpleInputProcessor();
		virtual ~SimpleInputProcessor();

	public:
		InputProcessorAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile) override;
		InputProcessorAPI virtual void CleanUp() override;

	public:
		InputProcessorAPI virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
		InputProcessorAPI virtual void RegisterOnResizeFunc(const OnResizeFunc& func) override;

	private:
		void OnKeyboardInput(UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		OnResizeFunc mOnResizeFunc;
		BOOL bRegisteredOnResizeFunc = FALSE;
	};
}