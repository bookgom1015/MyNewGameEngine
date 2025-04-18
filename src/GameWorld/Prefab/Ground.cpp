#include "GameWorld/Prefab/Ground.hpp"
#include "Common/Debug/Logger.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"

using namespace GameWorld::Prefab;
using namespace DirectX;

Ground::Ground(
		Common::Debug::LogFile* const pLogFile,
		const std::string& name,
		XMFLOAT3 pos,
		XMFLOAT4 rot,
		XMFLOAT3 scale) : Actor(pLogFile, name, pos, rot, scale) {

}

Ground::Ground(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name, 
		const Transform& trans)
	: Actor(pLogFile, name, trans) {

}

BOOL Ground::OnInitialzing() {
	return TRUE;
}

BOOL Ground::ProcessActorInput(Common::Input::InputState* const pInputState) {
	return TRUE;
}

BOOL Ground::UpdateActor(FLOAT delta) {
	return TRUE;
}
