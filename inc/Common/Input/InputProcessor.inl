#ifndef __INPUTPROCESSOR_INL__
#define __INPUTPROCESSOR_INL__

DirectX::XMFLOAT2 Common::Input::MouseState::MousePosition() const { return mMousePos; }
DirectX::XMFLOAT2 Common::Input::MouseState::MouseDelta() const { return mMouseDelta; }

FLOAT Common::Input::MouseState::ScrollWheel() const { return mScrollWheel; }

BOOL Common::Input::MouseState::IsInputIgnored() const { return mbIsIgnored; }
BOOL Common::Input::MouseState::IsRelativeMouseMode() const { return mMouseMode == MouseModes::E_Relative; }

constexpr BOOL Common::Input::InputProcessor::AppPaused() const { return bAppPaused; }
constexpr BOOL Common::Input::InputProcessor::Minimized() const { return bMinimized; }
constexpr BOOL Common::Input::InputProcessor::Maximized() const { return bMaximized; }
constexpr BOOL Common::Input::InputProcessor::Resizing() const { return bResizing; }
constexpr BOOL Common::Input::InputProcessor::Fullscreen() const { return bFullscreenState; }
constexpr BOOL Common::Input::InputProcessor::Destroying() const { return bDestroying; }

#endif // __INPUTPROCESSOR_INL__