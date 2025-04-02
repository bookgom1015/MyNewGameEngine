#ifndef __INPUTPROCESSOR_INL__
#define __INPUTPROCESSOR_INL__

DirectX::XMFLOAT2 Common::Input::MouseState::MousePosition() const { return mMousePos; }
DirectX::XMFLOAT2 Common::Input::MouseState::MouseDelta() const { return mMouseDelta; }

FLOAT Common::Input::MouseState::ScrollWheel() const { return mScrollWheel; }

BOOL Common::Input::MouseState::IsInputIgnored() const { return mbIsIgnored; }
BOOL Common::Input::MouseState::IsRelativeMouseMode() const { return mMouseMode == MouseModes::E_Relative; }

#endif // __INPUTPROCESSOR_INL__