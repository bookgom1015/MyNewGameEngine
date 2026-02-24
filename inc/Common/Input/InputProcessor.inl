#ifndef __INPUTPROCESSOR_INL__
#define __INPUTPROCESSOR_INL__

DirectX::SimpleMath::Vector2 Common::Input::MouseState::MousePosition() const { 
	return mMousePos; 
}

DirectX::SimpleMath::Vector2 Common::Input::MouseState::MouseDelta() const { 
	return mMouseDelta; 
}

float Common::Input::MouseState::ScrollWheel() const { return mScrollWheel; }

bool Common::Input::MouseState::IsInputIgnored() const { return mbIsIgnored; }

bool Common::Input::MouseState::IsRelativeMouseMode() const { 
	return mMouseMode == MouseModes::E_Relative; 
}

Common::Input::InputState Common::Input::InputProcessor::GetInputState() const { 
	return mInputState; 
}

#endif // __INPUTPROCESSOR_INL__